// sim_events.cpp - the things that happen to a station whether or not it has planned for them:
// the deck of events, the city changing month by month, and the skills an agent learns from you.
#include "sim.h"
#include <algorithm>
#include <cmath>

static bool has(const std::vector<int>& v, int x) { return std::find(v.begin(), v.end(), x) != v.end(); }

namespace {
enum { EV_BURGLARY, EV_AUDIT, EV_PRESS, EV_DELEGATION, EV_DEFECTOR, EV_CRASH, EV_CONCIERGE, EV_WIFE, EV_FLASH,
       EV_ILL, EV_DEMO, EV_COLLEAGUE, EV_WEEKEND, EV_RAISE, EV_STRIKE,
       EV_GRU_POACH, EV_GRU_LEAD, EV_GRU_ARREST, EV_ENVELOPE, EV_STRANGER, EV_RIVAL, N_EV };
// flags kept in GameState::evLast beside the events' own dates
enum { EVF_ANY = 1000, EVF_STRIKE = 1001, EVF_MEETS = 1002 };
const int COOLDOWN[N_EV] = {600, 480, 300, 300, 1400, 120, 150, 120, 110, 365, 160, 365, 365, 90, 360, 240, 300, 500, 400, 450, 300};
const int EVK_WIFE = 10000000, EVK_RAISE = 20000000;     // + agent id: each agent's wife finds out once, and he asks for more once a year at most
const char* const DELEGATIONS[] = {"the Ministry of Foreign Trade", "the Ministry of the Fishing Industry", "the Academy of Sciences", "a Komsomol youth delegation",
                                   "the Ministry of Civil Aviation", "the State Committee for Science and Technology", "the Union of Soviet Writers", "the Ministry of the Chemical Industry"};
const char* const FLASH_POL[] = {"the government's real position before this week's summit", "what the cabinet decided at yesterday's emergency meeting", "the instructions given to the delegation at the disarmament talks"};
const char* const FLASH_MIL[] = {"the alert state of the forces in the theatre this week", "the timetable of the coming exercise", "the deployment of the new missile units"};
const char* const FLASH_SCI[] = {"the specifications of the computer offered to the Chinese", "the results of last month's trials", "the export licence decision on the machine tools"};
const char* const FLASH_SEC[] = {"what the security service knows of the defector's debriefing", "the names of the officers on the new counter-intelligence task force", "the adversary's plans for the coming visit"};
}

/** A new piece of the world each fortnight or so: some need a decision from you, some only need to be known. */
void Sim::events() {
    if (training() || g.phase != PH_PLAY || !g.decisions.empty() || g.day <= g.busyUntil) return;
    int last = g.evLast.count(EVF_ANY) ? g.evLast[EVF_ANY] : g.postStart;
    if (g.day - last < 9 || g.day - g.postStart < 14 || g.postEnd - g.day < 10) return;
    if (!pct(9)) return;
    auto ready = [&](int e) { return !g.evLast.count(e) || g.day - g.evLast[e] >= COOLDOWN[e]; };
    // candidates: event, its weight, and the person or site it concerns
    struct C { int ev; float w; int ref; };
    std::vector<C> cs;
    auto add = [&](int e, float w, int ref = 0) { if (ready(e) && w > 0) cs.push_back({e, w, ref}); };
    add(EV_BURGLARY, g.you.heat > 25 ? 1.0f : 0.4f);
    add(EV_AUDIT, g.day - g.postStart > 120 ? 0.8f : 0);
    add(EV_PRESS, T().slot > 0 && g.you.heat > 28 ? 0.8f : 0);
    add(EV_DELEGATION, 1.2f);
    add(EV_DEFECTOR, T().slot > 0 && g.day - g.postStart > 200 ? 0.12f : 0);
    add(EV_CRASH, g.lastCarDay >= g.day - 2 ? 2.5f : 0);
    for (auto& s : g.sites) if (s.flat && !s.retired && s.neighbours > 28) { add(EV_CONCIERGE, 1.0f, s.id); break; }
    {
        std::vector<int> c;
        for (auto& p : g.persons) if (p.stage == AGENT && p.handler == 0 && p.paidTotal > P(80) && !g.evLast.count(EVK_WIFE + p.id)) c.push_back(p.id);
        if (!c.empty()) add(EV_WIFE, 0.6f, c[r().n((int)c.size())]);
        c.clear();
        for (auto& p : g.persons) if (p.stage == AGENT && p.salary > 0 && p.app[A_MONEY] > 40 && (!g.evLast.count(EVK_RAISE + p.id) || g.day - g.evLast[EVK_RAISE + p.id] > 365)) c.push_back(p.id);
        if (!c.empty()) add(EV_RAISE, 0.4f, c[r().n((int)c.size())]);
    }
    add(EV_FLASH, g.day - g.postStart > 60 ? 1.0f : 0);
    add(EV_ILL, g.fatigue > 55 ? 0.8f : 0.25f);
    add(EV_DEMO, T().slot > 0 ? 0.7f : 0);
    {
        std::vector<int> c;
        for (auto& s : g.sites) if (!s.retired && !s.flat && !s.burned && (s.type == ST_MEET || s.type == ST_DROP) && s.casing >= 2) c.push_back(s.id);
        bool colleague = false;
        for (auto& o : g.officers) if (!o.gone && o.role == ROLE_COLLEAGUE) colleague = true;
        if (!c.empty() && colleague) add(EV_COLLEAGUE, 0.5f, c[r().n((int)c.size())]);
    }
    add(EV_WEEKEND, 0.6f);
    {
        std::vector<int> c;
        for (auto& p : g.persons) if ((p.stage == DEVELOPING || p.stage == SPOTTED) && (p.sector == MILITARY || p.sector == SCITECH) && p.service == SVC_NONE && p.meetings >= 1) c.push_back(p.id);
        if (!c.empty()) add(EV_GRU_POACH, g.gru < 40 ? 1.0f : 0.6f, c[r().n((int)c.size())]);
        c.clear();
        for (auto& s : g.sites) if (!s.retired && !s.flat && !s.burned && s.type == ST_DROP && s.casing >= 2) c.push_back(s.id);
        if (!c.empty() && g.gru >= 40) add(EV_GRU_LEAD, 0.5f, c[r().n((int)c.size())]);
        bool rival = false;
        for (auto& o : g.officers) if (!o.gone && o.role == ROLE_COLLEAGUE) rival = true;
        if (rival && T().role < 3 && g.mValue + g.totValue > 60) add(EV_RIVAL, 0.5f);
    }
    add(EV_GRU_ARREST, g.day - g.postStart > 150 ? 0.2f : 0);
    add(EV_ENVELOPE, g.day - g.postStart > 90 ? 0.5f : 0);
    add(EV_STRANGER, g.day - g.postStart > 120 ? (g.wifeHere && g.home >= 40 ? 0.3f : 0.7f) : 0);
    add(EV_STRIKE, g.theatre == TH_LONDON || g.theatre == TH_DELHI || g.theatre == TH_BONN ? 0.8f : 0.3f);
    if (cs.empty()) return;
    float tot = 0;
    for (auto& c : cs) tot += c.w;
    float x = r().f() * tot;
    C ch = cs.back();
    for (auto& c : cs) { if (x < c.w) { ch = c; break; } x -= c.w; }
    g.evLast[ch.ev] = g.day;
    g.evLast[EVF_ANY] = g.day;
    if (ch.ev == EV_WIFE) g.evLast[EVK_WIFE + ch.ref] = g.day;
    if (ch.ev == EV_RAISE) g.evLast[EVK_RAISE + ch.ref] = g.day;
    int ref = ch.ref * 64 + ch.ev;
    Person* p = person(ch.ref);
    Site* s = site(ch.ref);
    switch (ch.ev) {
        case EV_BURGLARY:
            addDecision(DK_EVENT, ref, "Somebody has been in your flat",
                        "When you came home last night the door was locked as you had left it. But the papers on your desk had been squared up, and the drawer you always leave an inch open was shut. "
                        "Nothing is missing.\n\nIt could be the cleaner. It is not the cleaner.",
                        {"Report it to the Embassy's security officer (a day of statements while the flat is swept)",
                         "Say nothing, and keep nothing at home from now on"});
            break;
        case EV_AUDIT:
            addDecision(DK_EVENT, ref, "An audit",
                        "A team from Centre's finance directorate arrives on Monday to audit the rezidentura's operational accounts: every receipt, every advance, every report of a meeting at which money changed hands.",
                        {"Spend two days putting your accounts and reports in order", "Carry on: your accounts are what they are"});
            break;
        case EV_PRESS:
            addDecision(DK_EVENT, ref, "A journalist",
                        std::string("A reporter telephoned the Embassy's press office and asked for you by name. He is writing about 'Soviet diplomats who are not what they seem', he says, and would like your side of it. ")
                            + "The press attaché wants to know what to tell him.",
                        {"Have the press attaché decline on your behalf", "Meet him for lunch, and charm him", "Ask the Ambassador to protest to the Foreign Ministry about press harassment"});
            break;
        case EV_DELEGATION: {
            const char* who = DELEGATIONS[r().n(8)];
            g.busyWhy = std::string("escorting the delegation of ") + who;
            addDecision(DK_EVENT, ref, "A delegation from Moscow",
                        std::string("A delegation of ") + who + " arrives tomorrow for four days. The Ambassador's office wants you as escort and interpreter: "
                            "the airport, the hotel, the ministry, the factory visit, the reception and the shopping. Four days in which you will have no time of your own.",
                        {"Escort them (four days; good for your cover, and there are always receptions)", "Ask the Rezident to excuse you on operational grounds"});
            break;
        }
        case EV_DEFECTOR: {
            static const char* const WHO[] = {"a driver of the trade mission", "a typist of the consular section", "an engineer of the trade delegation", "the Embassy's assistant accountant"};
            std::string who = WHO[r().n(4)];
            for (auto& o : g.officers) if (!o.gone) o.a.heat = clampf(o.a.heat + 3, 0, 100);
            heat(pct(50) ? 5 : 2);
            g.a4 = clampf(g.a4 + 0.3f, 0.3f, 2.4f);
            addDecision(DK_EVENT, ref, "A defection",
                        "Last night " + who + " failed to come back from the shops. His wife has been taken to the residential compound and is not allowed visitors. "
                        "By now he is talking to " + T().adversary + ", and he knew faces: who goes out in the evenings, who has a car, who never seems to be at his desk.\n\n"
                        "Line KR orders all officers to stand down from operational acts for ten days.",
                        {"Stand down as ordered (your agents will miss their meetings)", "Stand down, but keep the agents' agreed meetings - only those"});
            break;
        }
        case EV_CRASH:
            addDecision(DK_EVENT, ref, "A collision",
                        "Yesterday, turning out of a side street, the Embassy car you were driving touched the wing of a parked van. A small dent. The owner was there at once, and so, in two minutes, was a policeman, "
                        "who wrote down the number on the diplomatic plates and looked at you for some time.",
                        {"Claim immunity and drive off", "Exchange particulars like any motorist and pay for the damage"});
            break;
        case EV_CONCIERGE:
            if (!s) break;
            addDecision(DK_EVENT, ref, "The concierge",
                        "The cut-out who holds the lease of " + s->name + " says the concierge stopped him on the stairs and asked, very pleasantly, who the two gentlemen are who come in the evenings and never at the same time.",
                        {fstr("Pay the concierge to see nothing (%d %s)", P(30), T().currency), "Give the flat up", "Ignore it"});
            break;
        case EV_WIFE:
            if (!p) break;
            subj = p;
            addDecision(DK_EVENT, ref, p->codename + "'s wife",
                        p->codename + " put the emergency mark on his signal site, and at the meeting he was white. His wife has found the money he keeps behind the boiler"
                            + (p->camera ? ", and the little camera with it" : std::string("")) + ". She thinks it is another woman, or gambling. He has told her nothing yet, and she will not let it rest.",
                        {"Let him tell her something near the truth, and pay her a share too", "Give him a story: a bonus, a win on the pools", "Put him on ice for two months until it settles"});
            subj = nullptr;
            break;
        case EV_RAISE:
            if (!p) break;
            addDecision(DK_EVENT, ref, p->codename + " asks for more",
                        p->codename + " left a note with his last delivery: his son's school fees have gone up, the house needs a roof, and he thinks that what he brings is worth more than he is paid. "
                            + fstr("He is paid %d %s a month.", p->salary, T().currency),
                        {"Raise his pay by half", fstr("A single payment this once (%d %s)", P(40), T().currency), "Refuse: he is paid what was agreed"});
            break;
        case EV_FLASH: {
            int sec = r().n(4);
            std::vector<int> mine;
            for (auto& a : g.persons) if (a.stage == AGENT) mine.push_back(a.sector);
            if (!mine.empty() && pct(70)) sec = mine[r().n((int)mine.size())];
            Requirement q;
            q.id = g.nextReqNo++; q.kind = RK_COLLECT; q.sector = sec; q.issued = g.day; q.due = g.day + 12 + r().n(6); q.priority = 1;
            q.need = 35 + 12.f * T().role;
            const char* topic = sec == POLITICAL ? FLASH_POL[r().n(3)] : sec == MILITARY ? FLASH_MIL[r().n(3)] : sec == SCITECH ? FLASH_SCI[r().n(3)] : FLASH_SEC[r().n(3)];
            q.title = std::string("FLASH: ") + topic;
            q.text = std::string("FLASH. Centre requires, as a matter of urgency, any information on ") + topic + ". Every source with access is to be tasked at once. Cable summaries are authorised.";
            g.reqs.push_back(q);
            cable("Centre", fstr("FLASH requirement No. %d", q.id), q.text + "\n\nRequired by " + fmtDay(q.due) + ".\n\nCENTRE");
            break;
        }
        case EV_ILL:
            g.busyWhy = "in bed with influenza";
            addDecision(DK_EVENT, ref, "Influenza",
                        "You woke at four with a fever, aching in every joint. The Embassy doctor says it is the influenza that has half the city in bed, and that you should be in bed too.",
                        {"Stay in bed for three days", "Work through it"});
            break;
        case EV_DEMO: {
            int y = dateOf(g.day).y;
            std::string why = y >= 1986 && y <= 1987 ? "Chernobyl" : y >= 1983 && y <= 1984 ? "the Korean airliner shot down over Sakhalin"
                              : y >= 1980 && y <= 1982 ? (pct(50) ? "Afghanistan" : "martial law in Poland") : y <= 1970 ? "Czechoslovakia" : "the refuseniks and Soviet Jewry";
            g.a4 = clampf(g.a4 - 0.3f, 0.3f, 2.4f);
            heat(0.5f);
            cable("Embassy bulletin", "Demonstration", "Some thousands are expected outside the Embassy today and tomorrow over " + why + ". The police have closed the street. "
                  "Staff are to leave by the rear entrance and to expect to be photographed. Every policeman in the district is on duty at the gate, and that includes the watchers' reserve.");
            break;
        }
        case EV_COLLEAGUE: {
            if (!s) break;
            Officer* c = nullptr;
            for (auto& o : g.officers) if (!o.gone && o.role == ROLE_COLLEAGUE) { c = &o; if (pct(50)) break; }
            if (!c) break;
            addDecision(DK_EVENT, c->id * 64 + EV_COLLEAGUE,
                        "A colleague asks a favour",
                        c->title() + " of another line asks to borrow " + s->name + " for one of his own agents for a month or two. 'You cased it properly, and I have nothing in that district.' "
                            "He will know the place. So will whoever he tells.",
                        {"Lend it to him", "Refuse"});
            g.evLast[2000] = s->id;     // which site he asked for
            break;
        }
        case EV_WEEKEND:
            addDecision(DK_EVENT, ref, "An invitation for the weekend",
                        "A man from the Chamber of Commerce whom you met at a function has invited you to a weekend at a country house: a shoot, dinner, a dozen guests, "
                        "\"people you ought to know\". It is two days, and the watchers will come too.",
                        {"Go (two days; there will be people worth knowing)", "Decline politely"});
            break;
        case EV_GRU_POACH:
            if (!p) break;
            addDecision(DK_EVENT, ref, "The neighbours want " + p->name,
                        "The deputy of the military attache - which is to say the neighbours' Rezident - came to see " + std::string(T().role == 3 ? "you" : "your Rezident") + ". " + p->name + ", " + p->job + ", " + p->employer
                            + ", is theirs, he says: one of his officers has been working on the man since the spring, and here you are buying him lunch. He wants you to stand aside.\n\n"
                              "He may even be telling the truth.",
                        {"Give the contact up to the neighbours", "Fight it: the man is yours, and the file shows it", "Propose to work him jointly, product shared"});
            break;
        case EV_GRU_LEAD:
            if (!s) break;
            addDecision(DK_EVENT, ref, "The neighbours offer a trade",
                        "A colonel of the neighbours stopped you in the Embassy corridor. They have come across a man they cannot use - no military access at all, but a great deal of the other kind - and he would rather see him "
                        "with you than with nobody. In return he asks for the use of " + s->name + " for one of his own people: 'You have more holes in the ground than you need.'",
                        {"Agree: his man for your drop", "Thank him, and decline"});
            break;
        case EV_GRU_ARREST:
            g.a4 = clampf(g.a4 + 0.3f, 0.3f, 2.4f);
            heat(1.5f);
            for (auto& o : g.officers) if (!o.gone) o.a.heat = clampf(o.a.heat + 1.5f, 0, 100);
            cable("Line KR", "Arrest of an officer of the neighbours", std::string("An officer of the military attache's staff was detained last night by ") + T().adversary + " while receiving documents from an agent, and will be expelled. "
                  "The newspapers have it. For some weeks every Soviet official in " + T().city + " will be looked at more closely, and the watchers will be pleased with themselves.\n\nLINE KR");
            break;
        case EV_ENVELOPE:
            addDecision(DK_EVENT, ref, "A man with an envelope",
                        "Outside the Embassy gate a man you have never seen fell into step beside you, said in bad Russian that he had 'papers your people will want', pushed a thick envelope against your chest and looked at you, waiting.\n\n"
                        "The best agents the service ever had began exactly like this. So did some of its worst afternoons.",
                        {"Take it", "Keep your hands in your pockets and walk on", "Do not touch it: tell him to write to the Consulate, and tell Line KR"});
            break;
        case EV_STRANGER:
            addDecision(DK_EVENT, ref, "An acquaintance",
                        std::string("At a reception a woman from one of the ministries - a secretary, she said, though she knew a good deal for a secretary - stayed talking to you for most of the evening, and at the end of it wrote a telephone number on a matchbook. ")
                            + (g.wifeHere ? "Your wife was at the other end of the room." : "You went home to an empty flat.")
                            + "\n\nShe has access, if she is what she says. If she is not what she says, somebody chose her with care.",
                        {"Report the acquaintance to Line KR, as the rules require, and leave it there", "Throw the matchbook away", "Telephone her"});
            break;
        case EV_RIVAL: {
            Officer* c = nullptr;
            for (auto& o : g.officers) if (!o.gone && o.role == ROLE_COLLEAGUE) { c = &o; if (pct(50)) break; }
            if (!c) break;
            addDecision(DK_EVENT, c->id * 64 + EV_RIVAL, "Somebody else's credit",
                        "At the weekly meeting " + c->title() + " reported to the Rezident, as the fruit of his own conversations, an item that came out of your material last month - you recognised your own phrases. "
                        "The Rezident wrote it down and nodded.",
                        {"Let it pass", "Raise it with the Rezident privately", "Say nothing, and see that something of his goes missing in its turn"});
            break;
        }
        case EV_STRIKE: {
            int n = 2 + r().n(4);
            g.evLast[EVF_STRIKE] = g.day + n - 1;
            cable("Embassy bulletin", "Strike", fstr("The transport unions are on strike from today for %d days. There will be no buses or trains. ", n)
                  + "Taxis will be scarce and the roads full. Allow for walking.");
            break;
        }
        default: break;
    }
}

void Sim::eventDecide(int ref, int option) {
    int ev = ref % 64, id = ref / 64;
    Person* p = person(id);
    Site* s = site(id);
    switch (ev) {
        case EV_BURGLARY:
            if (option == 0) {
                g.busyUntil = std::max(g.busyUntil, g.day); g.busyWhy = "statements to the security officer while your flat was swept";
                g.paperwork += 2; g.standRez = clampf(g.standRez + 1, 0, 100);
                if (pct(50)) cable("Technical officer", "Your flat", "A device was found in the base of your telephone and has been removed. Assume that what was said in the flat over the last weeks is known. "
                                   "A second sweep will be made next month.\n\nTECHNICAL OFFICER");
                else cable("Technical officer", "Your flat", "Your flat was swept and nothing was found. That does not mean nothing was there.\n\nTECHNICAL OFFICER");
            } else if (pct(50)) heat(4);         // the device stays where it is, and listens
            break;
        case EV_AUDIT:
            if (g.skimmed > 0 && pct(std::min(80.f, g.skimmed / (float)P(4)) * (option == 0 ? 0.4f : 1.f))) {
                skimExposed("The auditors compared the agents' receipts with what the agents themselves say they were given.");
                if (g.phase != PH_PLAY) break;
            }
            if (option == 0) {
                g.busyUntil = std::max(g.busyUntil, g.day + 1); g.busyWhy = "the auditors from Centre";
                g.paperwork = std::max(0.f, g.paperwork - 6);
                g.standCentre = clampf(g.standCentre + 2, 0, 100);
                cable("Centre", "Audit of the rezidentura", "The audit found the operational accounts in order and the reports complete.\n\nCENTRE");
            } else {
                Person* bad = nullptr;
                for (auto& a : g.persons) {
                    if (a.stage != AGENT || a.paidTotal < P(200)) continue;
                    bool graded = false;
                    for (auto& x : g.rolls) if (x.agentId == a.id && x.state == R_GRADED && x.value >= 25) graded = true;
                    if (!graded) bad = &a;
                }
                if (g.paperwork > 6) {
                    g.standCentre = clampf(g.standCentre - 5, 0, 100); g.standRez = clampf(g.standRez - 3, 0, 100);
                    cable("Centre", "Audit of the rezidentura", fstr("The audit found %d hours of contact reports outstanding and receipts that cannot be matched to them. ", (int)g.paperwork)
                          + "This has been entered in your personal file.\n\nCENTRE");
                } else if (bad) {
                    g.standCentre = clampf(g.standCentre - 3, 0, 100);
                    cable("Centre", "Audit of the rezidentura", fstr("The auditors ask why agent %s has been paid %d %s for material Centre has not found of interest. You will explain.\n\nCENTRE", bad->codename.c_str(), bad->paidTotal, T().currency));
                } else {
                    g.standCentre = clampf(g.standCentre + 1, 0, 100);
                    cable("Centre", "Audit of the rezidentura", "The audit found nothing to remark on.\n\nCENTRE");
                }
            }
            break;
        case EV_PRESS: {
            float runs = option == 0 ? 35 : option == 1 ? 20 : 10;
            if (option == 1) {
                g.fatigue += 4;
                money(-std::min(g.funds, P(4)), "Lunch with a journalist");
                heat(1.5f);
                if (pct(45)) spot(POLITICAL, "through the journalist who wanted to write about you", 0);
            }
            if (option == 2) { g.standRez = clampf(g.standRez - 2, 0, 100); heat(1); }
            if (pct(runs)) {
                heat(2); g.cover = std::max(0.f, g.cover - 6);
                g.standRez = clampf(g.standRez - 2, 0, 100);
                cable("Embassy bulletin", "Press", std::string("The article appeared this morning: half a page, with a photograph of you leaving the Embassy and the words 'diplomat' in inverted commas. ")
                      + (option == 1 ? "He was kinder than he might have been; he says you bought him a good lunch." : "The Ambassador read it at breakfast."));
            }
            break;
        }
        case EV_DELEGATION:
            if (option == 1 && pct(50)) { g.standRez = clampf(g.standRez - 1, 0, 100); cable("Rezident", "The delegation", "Very well. Somebody else will carry their suitcases.\n\nREZIDENT"); break; }
            if (option == 1) { g.standRez = clampf(g.standRez - 1, 0, 100); cable("Rezident", "The delegation", "Your operational commitments can wait four days. The Ambassador's cannot.\n\nREZIDENT"); }
            g.busyUntil = std::max(g.busyUntil, g.day + 3);
            g.cover = std::min(100.f, g.cover + 8); g.standRez = clampf(g.standRez + 2, 0, 100);
            lang() = std::min(95.f, lang() + 0.5f);
            if (pct(35)) spot(r().n(3), "at a reception for the delegation", 0);
            break;
        case EV_DEFECTOR:
            g.standDownUntil = g.day + 9;
            if (option == 1) { g.evLast[EVF_MEETS] = g.day + 9; g.standRez = clampf(g.standRez - 2, 0, 100); }
            break;
        case EV_CRASH:
            if (option == 0) { heat(2); g.standRez = clampf(g.standRez - 2, 0, 100); g.cover = std::max(0.f, g.cover - 3);
                               cable("Ambassador's office", "The incident", "The Foreign Ministry has asked about an incident involving an Embassy vehicle. The Ambassador does not like incidents."); }
            else { money(-std::min(g.funds, P(25)), "Damage to a van (Embassy car)"); paper(1.5f); heat(0.5f); }
            break;
        case EV_CONCIERGE:
            if (!s) break;
            if (option == 0) {
                money(-std::min(g.funds, P(30)), "A consideration for the concierge (through the cut-out)");
                s->neighbours = std::max(0.f, s->neighbours - 20);
                if (pct(20)) s->burned = true;                 // and he told the police anyway
            } else if (option == 1) {
                s->retired = true;
                for (auto& a : g.persons) if (a.meetSites.size() > 1) a.meetSites.erase(std::remove(a.meetSites.begin(), a.meetSites.end(), s->id), a.meetSites.end());
                cable("Support officer", "The flat", "The lease of " + s->name + " has been given up and the keys returned. The cut-out has been thanked.");
            } else s->neighbours += 8;
            break;
        case EV_WIFE:
            if (!p) break;
            if (option == 0) {
                p->salary += P(8); p->stress = clampf(p->stress - 10, 0, 100);
                file(*p, "His wife was told something near the truth and is paid a share.");
                if (!controlled(*p) && pct(15)) { p->suspicion += 25; p->warn.push_back("says his wife has left him and taken the children to her mother's. He does not know what she has said, or to whom."); }
            } else if (option == 1) {
                p->stress = clampf(p->stress + 8, 0, 100);
                file(*p, "Given a story for his wife.");
                if (!controlled(*p) && pct(30)) { p->suspicion += 10; p->warn.push_back("says his wife has told her sister about the 'win', and the sister told everybody."); }
            } else {
                p->tempo = 0; p->stress = clampf(p->stress - 15, 0, 100); p->rapport = clampf(p->rapport - 3, 0, 100);
                file(*p, "Put on ice while his wife's suspicions settle.");
            }
            break;
        case EV_RAISE:
            if (!p) break;
            if (option == 0) {
                p->salary = (int)std::lround(p->salary * 1.5f); p->rapport = clampf(p->rapport + 8, 0, 100); p->commitment = clampf(p->commitment + 4, 0, 100);
                file(*p, fstr("Pay raised to %d a month.", p->salary));
            } else if (option == 1) {
                p->owed += P(40); p->rapport = clampf(p->rapport + 4, 0, 100);
                file(*p, fstr("A single payment of %d agreed.", P(40)));
            } else {
                p->rapport = clampf(p->rapport - 8, 0, 100);
                if (p->tru[A_MONEY] > 65 && !controlled(*p)) { p->commitment = clampf(p->commitment - 10, 0, 100); p->stress = clampf(p->stress + 5, 0, 100); }
                file(*p, "His request for more money was refused.");
            }
            break;
        case EV_ILL:
            if (option == 0) { g.busyUntil = std::max(g.busyUntil, g.day + 2); g.busyWhy = "in bed with influenza"; g.fatigue = std::max(0.f, g.fatigue - 35); }
            else g.fatigue = std::min(100.f, g.fatigue + 30);
            break;
        case EV_COLLEAGUE: {
            Site* st = g.evLast.count(2000) ? site(g.evLast[2000]) : nullptr;
            if (!st) break;
            if (option == 0) {
                witness(st->witting, id);
                st->exposure += 15;
                g.standRez = clampf(g.standRez + 2, 0, 100);
            } else g.standRez = clampf(g.standRez - 1, 0, 100);
            break;
        }
        case EV_GRU_POACH:
            if (!p || !p->live()) break;
            if (option == 0) {
                clearAppointments(*p);
                p->stage = TERMINATED;
                file(*p, "Passed to the neighbours at their request. File closed.");
                g.gru = clampf(g.gru + 14, 0, 100);
            } else if (option == 1) {
                if (pct(45 + g.standRez / 3)) {
                    g.gru = clampf(g.gru - 16, 0, 100);
                    file(*p, "The neighbours claimed him. The claim was rejected.");
                    cable(T().chief, p->name, "The neighbours have been told that the contact is ours and that the file shows it. They did not like it.\n\n" + std::string(T().role == 3 ? "CENTRE" : "REZIDENT"));
                } else {
                    clearAppointments(*p);
                    p->stage = TERMINATED;
                    g.gru = clampf(g.gru - 8, 0, 100); g.standRez = clampf(g.standRez - 2, 0, 100);
                    file(*p, "Passed to the neighbours on Centre's ruling. File closed.");
                    cable("Centre", p->name, "The question was referred to Centre, which has consulted the General Staff. The contact goes to the neighbours. Do not let it happen again.\n\nCENTRE");
                }
            } else {
                g.gru = clampf(g.gru + 6, 0, 100);
                p->rapport = clampf(p->rapport - 5, 0, 100);
                if (pct(30)) p->knownAdv = true;          // two services round one man are twice as easy to see
                file(*p, "Worked jointly with the neighbours by agreement: product to be shared.");
            }
            break;
        case EV_GRU_LEAD:
            if (!s) break;
            if (option == 0) {
                s->exposure += 20;
                if (pct(25)) s->burned = true;
                g.gru = clampf(g.gru + 8, 0, 100);
                int sec = r().n(3) == 0 ? SCITECH : POLITICAL;
                Person np = makePerson(sec, T().dangle * hard());
                np.tru[A_ACCESS] = clampf(np.tru[A_ACCESS] + rnd(10, 25), 5, 95);
                if (!np.dangle) np.app[A_ACCESS] = np.tru[A_ACCESS];
                np.rapport = 10;
                file(np, "Passed to us by the neighbours, who had no use for him: " + np.job + ", " + np.employer + ".");
                std::string line = "From the neighbours: " + np.name + ", " + np.job + ", " + np.employer + ". The file is in your card index. " + s->name + " is now known to them.";
                g.totSpotted++; g.mSpotted++;
                g.persons.push_back(np);
                alert(line);
            } else g.gru = clampf(g.gru - 3, 0, 100);
            break;
        case EV_ENVELOPE: {
            bool trap = pct(clampf(25 + g.you.heat * 0.6f, 25, 80));
            if (option == 0) {
                if (trap) {
                    heat(14); g.standRez = clampf(g.standRez - 5, 0, 100); g.cover = std::max(0.f, g.cover - 10);
                    cable("Line KR", "The envelope", "The envelope contained forty blank sheets and a page from a telephone directory. You were photographed taking it, from two sides; the pictures were in the evening papers "
                          "under the words 'Soviet diplomat accepts secret documents'. The Foreign Ministry has protested, and so has the Ambassador - to the Rezident.\n\nLINE KR");
                } else {
                    cable("Line KR", "The envelope", "The envelope contained eleven pages that appear to be genuine, and a note saying where the writer can be found. He has been told to wait.\n\nLINE KR");
                    pend(g.day + 2 + r().n(4), P_WALKIN, 0, 0);
                }
            } else if (option == 2) {
                g.standRez = clampf(g.standRez + 1, 0, 100);
                if (!trap && pct(50)) pend(g.day + 10 + r().n(15), P_WALKIN, 0, 0);
            }
            break;
        }
        case EV_STRANGER:
            if (option == 0) { g.standRez = clampf(g.standRez + 1, 0, 100); g.paperwork += 1; }
            else if (option == 2) {
                if (g.wifeHere) g.home = clampf(g.home - 15, 0, 100);
                if (pct(clampf(45 + g.you.heat * 0.5f, 45, 85))) g.dirt = std::min(100.f, g.dirt + 45);      // somebody chose her with care
                else {
                    Person np = makePerson(POLITICAL, 0);
                    for (int k = 0; k < 30 && !np.female; k++) np = makePerson(POLITICAL, 0);
                    np.rapport = 38; np.commitment = 20; np.stage = DEVELOPING;
                    file(np, "Met at a reception; the acquaintance was continued privately. " + np.job + ", " + np.employer + ".");
                    std::string line = np.name + ", " + np.job + ", " + np.employer + ", is in your card index. Nobody at the Embassy knows how you came by her.";
                    g.totSpotted++; g.mSpotted++;
                    g.persons.push_back(np);
                    alert(line);
                }
            }
            break;
        case EV_RIVAL:
            if (option == 0) g.standRez = clampf(g.standRez - 2, 0, 100);
            else if (option == 1) {
                if (pct(50 + g.standRez / 3)) { g.standRez = clampf(g.standRez + 2, 0, 100); cable("Rezident", "A word", "I know whose material it was. I read the files, including his. Leave it with me.\n\nREZIDENT"); }
                else { g.standRez = clampf(g.standRez - 3, 0, 100); cable("Rezident", "A word", "I have a station to run, and no time for officers who count each other's credits. Work.\n\nREZIDENT"); }
            } else {
                g.standRez = clampf(g.standRez + 1, 0, 100);
                if (pct(20)) { g.standRez = clampf(g.standRez - 5, 0, 100); alert("Your small revenge on a colleague was noticed. The Rezident's secretary notices everything."); }
            }
            break;
        case EV_WEEKEND:
            if (option == 0) {
                g.busyUntil = std::max(g.busyUntil, g.day + 1); g.busyWhy = "a weekend in the country";
                g.cover = std::min(100.f, g.cover + 3); heat(1);
                if (pct(60)) spot(pct(60) ? POLITICAL : r().n(4), "at a country-house weekend", 0);
                if (pct(25)) spot(r().n(4), "at the same weekend", 0);
            }
            break;
        default: break;
    }
}

/** Is today one of the days of a transport strike? */
bool strikeToday(const GameState& g) {
    auto it = g.evLast.find(EVF_STRIKE);
    return it != g.evLast.end() && g.day <= it->second;
}
/** During a stand-down, are the agents' agreed meetings still kept? */
bool meetsAllowed(const GameState& g) {
    auto it = g.evLast.find(EVF_MEETS);
    return it != g.evLast.end() && g.day <= it->second;
}

// ============================================================================ the city changes
void Sim::cityChanges() {
    if (training()) return;
    // the watchers move their effort about
    for (int i = 0; i < 8; i++) g.distMod[i] = clampf(g.distMod[i] + r().gauss() * 0.06f + (1 - g.distMod[i]) * 0.1f, 0.55f, 1.7f);
    if (pct(8)) {
        int d = r().n(8);
        if (!T().east[d]) {
            g.distMod[d] = clampf(g.distMod[d] + 0.35f, 0.55f, 1.7f);
            if (pct(60)) cable("Line KR", "A new observation post", std::string("There are signs that the ") + T().watchers + " have taken premises overlooking the station in " + T().district[d]
                               + ". Expect more of them there for some months.\n\nLINE KR");
        }
    }
    // a venue closes
    if (pct(12)) {
        std::vector<Site*> c;
        for (auto& s : g.sites) {
            if (s.retired || s.flat || s.type != ST_MEET) continue;
            bool used = false;
            for (auto& p : g.persons) if (p.live() && has(p.meetSites, s.id)) used = true;
            for (auto& d : g.plan) for (auto& o : d.second) if (o.set && o.site == s.id) used = true;
            if (!used) c.push_back(&s);
        }
        if (!c.empty()) {
            Site* s = c[r().n((int)c.size())];
            s->retired = true;
            alert(s->name + " has closed down (the shutters are up and a sign says 'premises to let'). It is struck off your list of sites.");
        }
    }
    // somewhere new opens
    if (pct(15)) {
        int d = r().n(8);
        Site s = makeSite(ST_MEET, d);
        s.casing = 0;
        casingEstimate(s);
        g.sites.push_back(s);
        alert("A new place has opened that might serve for meetings: " + s.name + ". It has not been cased.");
    }
    // building works by a drop or a signal site
    if (pct(12)) {
        std::vector<Site*> c;
        for (auto& s : g.sites) if (!s.retired && (s.type == ST_DROP || s.type == ST_SIGNAL)) c.push_back(&s);
        if (!c.empty()) {
            Site* s = c[r().n((int)c.size())];
            s->cover = std::max(5.f, s->cover - rnd(18, 32));
            if (pct(50)) alert("Building works have begun beside " + s->name + ": scaffolding, a site hut, and a night-watchman. Case it again before you use it.");
        }
    }
    // a route changes
    if (pct(10)) {
        std::vector<Site*> c;
        for (auto& s : g.sites) if (!s.retired && s.type == ST_ROUTE) c.push_back(&s);
        if (!c.empty()) {
            Site* s = c[r().n((int)c.size())];
            float dv = rnd(-18, 12);
            s->sight = clampf(s->sight + dv, 5, 95);
            alert(std::string("The traffic scheme along ") + shortSite(s) + (dv < 0 ? " has been changed: a one-way system and new bus lanes. Some of your checkpoints no longer work." : " has been changed, and it is quieter. Walk it again."));
        }
    }
}

// ============================================================================ agents' skills
void Sim::initSkills(Person& p, bool officer) {
    if (officer) {
        p.skPhoto = rnd(40, 70); p.skElicit = rnd(40, 75); p.skSpot = rnd(30, 60); p.skComms = rnd(50, 80);
        return;
    }
    p.skPhoto = rnd(0, 22);
    p.skElicit = clampf(rnd(5, 25) + p.tru[A_INFL] * 0.2f + p.tru[A_EGO] * 0.05f, 0, 60);
    p.skSpot = clampf(rnd(0, 18) + p.tru[A_INFL] * 0.15f, 0, 50);
    p.skComms = rnd(0, 20);
}

void Sim::trainAgent(Person& p, int what, bool flat) {
    float m = (5 + A->tradecraft * 0.05f) * (flat ? 1.4f : 1.f);
    switch (what) {
        case 1:
            p.skPhoto = std::min(90.f, p.skPhoto + m * 1.3f);
            say("    Instruction in photographing documents: the light, the distance, the hands steady, the whole page in the frame, and never the same desk twice.");
            break;
        case 2:
            p.skElicit = std::min(90.f, p.skElicit + m * 1.2f);
            say("    Instruction in elicitation: how to bring a colleague round to a subject without asking a question he will remember.");
            break;
        case 3:
            p.skSpot = std::min(90.f, p.skSpot + m * 1.2f);
            say("    Instruction in talent-spotting: whom to notice, what to report about them, and never to try to recruit anybody himself.");
            break;
        case 4:
            p.skComms = std::min(90.f, p.skComms + m * 1.3f);
            p.diligence = std::min(97.f, p.diligence + 2 * (flat ? 1.4f : 1.f));
            say("    Rehearsed the drops, the signals and the radio schedule until he could do them half asleep.");
            break;
        default:
            p.craft = std::min(80.f, p.craft + m);
            say("    Security instruction given: routes to meetings, handling of the camera at work, what to say if questioned.");
            break;
    }
    if (flat) say("    (At the flat there was time to practise, not only to talk.)");
}

/** A talent-spotter among the agents puts a colleague in your way. If he is theirs, so is the colleague. */
void Sim::referral(Person& p) {
    if (p.stage != AGENT || p.referrals >= 4 || p.skSpot < 12) return;
    if (!pct(p.skSpot * 0.12f)) return;
    int open = 0;
    for (auto& q : g.persons) if (q.stage == SPOTTED || q.stage == DEVELOPING) open++;
    if (open >= 14) return;
    p.referrals++;
    say("    He has a name for you: a colleague who is short of money and fed up. He will write the particulars down and leave them in the usual way.");
    pend(g.day + 1, P_REFERRAL, p.id, 0);       // the new file is opened in the morning (the list of people must not move under a meeting)
}

void Sim::referralArrives(int agentId) {
    Person* pp = person(agentId);
    if (!pp) return;
    Person& p = *pp;
    float dangle = controlled(p) ? 1.f : T().dangle * hard() * 0.6f;
    Person np = makePerson(p.sector, dangle);
    if (pct(60)) {
        np.employer = p.employer;
        np.empSec = clampf(p.empSec + rnd(-5, 5), 2, 95);
        np.tru[A_ACCESS] = clampf(p.tru[A_ACCESS] * rnd(0.6f, 1.1f), 5, 95);
        if (!np.dangle) np.app[A_ACCESS] = np.tru[A_ACCESS];
    }
    np.rapport = 12;
    np.handler = p.handler;
    np.witting = p.witting;
    np.estV[A_VULN] = 55; np.estC[A_VULN] = 15;           // he says what the man needs
    std::string code = p.codename;
    file(np, "Put forward by agent " + code + ": " + np.job + ", " + np.employer + ". Described as 'short of money and fed up'.");
    file(p, "Talent-spotted " + np.name + ", " + np.job + ", " + np.employer + ".");
    std::string line = code + " has talent-spotted a colleague: " + np.name + ", " + np.job + ", " + np.employer + ". The file is in your contacts.";
    g.mSpotted++; g.totSpotted++;
    g.persons.push_back(np);                              // p is not used after this
    Person* keep = subj; subj = nullptr;
    alert(line);
    subj = keep;
}
