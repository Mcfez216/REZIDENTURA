// sim_spy.cpp - officers of foreign services as recruits, what they tell you about your own affairs,
// getting an agent out of the country, and listening devices.
#include "sim.h"

static bool has(const std::vector<int>& v, int x) { return std::find(v.begin(), v.end(), x) != v.end(); }

// ============================================================================ officers of a foreign service
/**
 * A professional: trained, well placed, hard to recruit, and very often a provocation.
 * If he is genuine he is the most valuable agent you can have, because he can tell you what the other side knows.
 */
Person Sim::makeSpy(float dangleChance) {
    const Theatre& t = T();
    Person p = makePerson(SECURITY, 0);
    const SpyT& j = t.spies[r().n((int)t.spies.size())];
    p.job = j.job; p.employer = j.employer; p.service = j.service;
    p.tru[A_ACCESS] = rnd((float)j.lo, (float)j.hi);
    p.empSec = rnd(84, 96);                           // nobody watches its own people as a service does
    p.craft = rnd(50, 78);
    p.tru[A_DISC] = rnd(55, 95);
    p.tru[A_NERVE] = rnd(45, 92);
    p.tru[A_IDEO] = std::pow(r().f(), 2.2f) * 60 + 2;
    p.tru[A_INFL] = rnd(0, 12);
    p.fabricator = false;
    p.dangle = false;
    for (int i = 0; i < N_ATTR; i++) p.app[i] = p.tru[i];
    if (r().f() < dangleChance) makeDangle(p);
    p.estV[A_ACCESS] = (j.lo + j.hi) / 2.f; p.estC[A_ACCESS] = 25;
    initSkills(p, true);
    return p;
}

std::string Sim::serviceNote(const Person& p) {
    if (p.service == SVC_NONE) return "";
    return std::string("OFFICER OF A FOREIGN SERVICE (") + SERVICE_LABEL[p.service] + ")";
}

/** Line KR points somebody out. */
void Sim::krTip() {
    if (T().spies.empty()) return;
    Person np = makeSpy(0.25f + T().dangle * hard());
    static const char* const WHY[] = {"said to be in debt", "passed over for promotion and bitter about it", "drinking heavily since his divorce",
                                      "known to speak slightingly of his superiors", "living beyond his salary"};
    std::string why = pick(WHY);
    g.persons.push_back(np);
    Person& p = g.persons.back();
    g.totSpotted++; g.mSpotted++;
    learn(p, A_MONEY, 30); learn(p, A_EGO, 30);
    file(p, "Pointed out by Line KR: an officer of " + p.employer + ", " + why + ".");
    Person* keep = subj; subj = &p;
    cable("Line KR", "A person of interest",
          "Line KR draws your attention to " + p.name + ", " + p.job + ", " + p.employer + ". He is " + why + " and is to be seen at diplomatic receptions.\n\n"
          "An officer of the adversary who can be had is worth a dozen clerks: he can tell us what they know about us. For the same reason they put such men in our way on purpose. "
          "If you take him up, report every contact, and believe nothing he says about our own affairs until something else confirms it.\n\nLINE KR");
    subj = keep;
}

// ---------------------------------------------------------------------------- what a penetration tells you
struct CiItem { int key; float weight; std::string text; int about; std::string note; int officer; };
static int K(int type, int id) { return type * 100000 + id; }

/**
 * At a long meeting an agent inside a foreign service reports what his service knows of YOUR affairs:
 * which of your agents are under investigation, which are run against you, which places are watched,
 * how you yourself are regarded, and whether there is a traitor in your station.
 * A genuine source tells the truth as far as his desk reaches. A controlled one lies, and lies usefully.
 */
void Sim::ciReport(Person& src) {
    if (src.service == SVC_NONE || src.stage != AGENT) return;
    bool ctl = controlled(src);
    std::vector<CiItem> items;
    auto add = [&](int key, float w, const std::string& text, int about = -1, const std::string& note = "", int officerId = -1) {
        if (!has(src.told, key)) items.push_back({key, w + r().f() * 0.5f, text, about, note, officerId});
    };
    const std::string svc = src.employer;
    int season = g.day / 120;
    float heatNow = g.you.heat;
    if (ctl) {
        // disinformation: blacken your good agents, vouch for their own, and tell you that you are safe
        for (auto& q : g.persons) {
            if (&q == &src || q.stage != AGENT) continue;
            if (!controlled(q)) add(K(1, q.id), 3, gx(&q, q.label() + " is under our control. He reported your approach and has been run against you ever since."), q.id, "is CONTROLLED by the adversary");
            else add(K(14, q.id), 2, gx(&q, "There is no file on " + q.label() + ". So far as " + svc + " knows, he does not exist."), q.id, "is NOT KNOWN to the adversary");
        }
        add(K(6, season), 2, "You personally are taken for a diplomat. No team is assigned to you.");
        add(K(10, 0), 1, "We have no source inside your rezidentura. I would know.");
    } else {
        bool sec = src.service == SVC_SECURITY, adv = src.service == SVC_ADVERSARY;
        float acc = src.tru[A_ACCESS];
        for (auto& q : g.persons) {
            if (&q == &src || !q.live()) continue;
            bool knows = sec || (adv ? pct(50) : pct(20));
            if (controlled(q) && knows && acc > 55)
                add(K(1, q.id), 5, gx(&q, q.label() + " is a controlled case. Everything he gives you is passed by " + (sec ? "my" : "their") + " people first."), q.id, "is CONTROLLED by the adversary");
            if (sec && q.stage == AGENT && !controlled(q)) {
                if (q.suspicion >= 75) add(K(3, q.id), 6, gx(&q, "An arrest of " + q.label() + " is being prepared. They want him with the material in his hands, and you beside him."), q.id, "is about to be ARRESTED");
                else if (q.suspicion >= 40) add(K(2, q.id), 4, gx(&q, q.label() + " is under investigation by his own security people. Nothing is proved yet."), q.id, "is UNDER INVESTIGATION");
            }
        }
        if (sec) {
            for (auto& s : g.sites) if (!s.retired && s.burned) {
                if (s.staked) add(K(5, s.id), 5, "There are men waiting at this moment at " + s.name + ".");
                else add(K(4, s.id), 3, "The place " + s.name + " is on the surveillance section's list.");
            }
            add(K(6, season), 3, heatNow < 22 ? "You are not carded. They take you for a diplomat."
                                : heatNow < 45 ? "You are carded as a suspected intelligence officer and covered from time to time."
                                : heatNow < 68 ? "You are identified as an officer of the service. A team has you several days in the week."
                                : "You are a priority target. They are waiting for the moment to take you in the act.");
            for (auto& o : g.officers) if (o.role == ROLE_SUB && !o.gone && o.a.heat >= 45) add(K(7, o.id * 16 + season % 16), 2, "Your officer " + o.title() + " is identified and regularly covered.");
            if (g.theatre == TH_LONDON && g.day > dayOf(1971, 5, 1) && g.day < dayOf(1971, 9, 24))
                add(K(12, 0), 7, std::string("A list of more than a hundred of your people is being drawn up for expulsion in the autumn. Everybody identified as an officer will be on it. ")
                                     + (heatNow >= 45 ? "Your name is on the draft." : "I have not seen your name on it."));
        }
        // the traitor in your own station, and the man who means to run
        Officer* mole = nullptr; Officer* runner = nullptr;
        for (auto& o : g.officers) {
            if (o.gone) continue;
            if (o.mole && o.moleDay <= g.day) mole = &o;
            if (o.defectDay > g.day && o.defectDay - g.day < 150) runner = &o;
        }
        if (adv || (sec && acc > 82)) {
            if (mole) {
                if (adv && acc > 80) add(K(8, mole->id), 8, "The source inside your rezidentura is " + mole->title() + ". I have seen his reports.", -1, "", mole->id);
                else {
                    std::string hint = std::string("There is a source inside your rezidentura. I do not have the name. He holds the rank of ") + RANK[mole->rank] + ".";
                    for (auto& q : g.persons) if (!q.codename.empty() && (q.handler == mole->id || has(q.witting, mole->id))) { hint += " He has reported on your meetings with " + q.codename + "."; break; }
                    add(K(9, mole->id), 5, hint);
                }
            } else if (adv && acc > 70) add(K(10, season), 2, "So far as my desk knows, we have no source inside your rezidentura.");
            if (runner && adv) add(K(11, runner->id), 7, "One of your officers is talking to us and means to come over within months: " + (acc > 80 ? runner->title() : std::string("a ") + RANK[runner->rank] + ", I do not have the name") + ".", -1, "", acc > 80 ? runner->id : -1);
        }
    }
    Person* keep = subj; subj = nullptr;       // these lines name third parties: no pronoun conversion
    if (items.empty()) {
        say("    Asked what " + svc + " knows of our affairs: nothing new.");
        subj = keep;
        return;
    }
    std::sort(items.begin(), items.end(), [](const CiItem& a, const CiItem& b) { return a.weight > b.weight; });
    int n = ctl ? 1 : (src.tru[A_ACCESS] > 80 ? 2 : 1);
    for (int i = 0; i < n && i < (int)items.size(); i++) {
        const CiItem& it = items[i];
        src.told.push_back(it.key);
        alert(src.codename + " ON OUR AFFAIRS: \"" + it.text + "\"");
        src.file.push_back(pad(fmtShort(g.day), 11) + "REPORTED: " + it.text);
        if (Person* q = person(it.about)) {
            q->file.push_back(pad(fmtShort(g.day), 11) + gx(q, "SOURCE " + src.codename + " (" + svc + ") reports that he " + it.note + "."));
            q->ciBy = src.id; q->ciDay = g.day;
        }
        if (Officer* o = officer(it.officer)) o->file.push_back(pad(fmtShort(g.day), 11) + "NAMED by source " + src.codename + " (" + svc + ") as working for the adversary.");
        paper(1);
    }
    if (!ctl) credit(SECURITY, 6.f * n, -1);
    subj = keep;
}

// ============================================================================ exfiltration
std::string Sim::requestExfil(Person& p) {
    if (p.stage != AGENT) return "Exfiltration is for recruited agents.";
    if (p.exfil == 1) return "The request is already with Centre.";
    if (p.exfil == 2) return gx(&p, "The documents for " + p.label() + " are in your safe. Hand them over at a meeting: tick 'travel documents' on the order. He leaves the same night.");
    p.exfil = 1;
    queue(C_EXFIL, p.id, 150, "Exfiltration of " + p.codename);
    g.paperwork += 2;
    file(p, "Exfiltration requested of Centre.");
    return "Request drafted (150 groups, 2 hours). Centre weighs the worth of the source against the cost. If it agrees, a passport and a route arrive in one to two weeks.";
}

/** Handing over the documents at a meeting. He goes tonight. */
void Sim::exfiltrate(Person& p) {
    if (p.exfil != 2) { say("    You had no travel documents for him."); return; }
    p.exfil = 0;
    say("    You gave him the passport, the money and the route, and made him repeat it. He leaves tonight and takes nothing with him.");
    clearAppointments(p);
    p.nextMeet = -1; p.nextDrop = -1; p.tempo = 0;
    if (controlled(p)) {
        // he was theirs: they now have our documentation, and a press conference
        p.stage = TERMINATED;
        heat(10); g.standCentre = clampf(g.standCentre - 6, 0, 100);
        file(p, "Given exfiltration documents. He did not travel.");
        pend(g.day + 4 + r().n(8), P_SECRET, p.id, 4);
        return;
    }
    float ok = 94 - std::max(0.f, p.suspicion - 45) * 1.0f - (p.tru[A_NERVE] < 40 ? 8 : 0);
    if (pct(ok)) {
        p.stage = EXFILTRATED;
        g.totExfil++;
        for (int id : p.held) if (Roll* x = roll(id)) x->state = R_LOST;
        p.held.clear();
        g.standCentre = clampf(g.standCentre + (p.service != SVC_NONE ? 8.f : 4.f), 0, 100); g.standRez = clampf(g.standRez + 3, 0, 100);
        file(p, "EXFILTRATED. Crossed the frontier on our documents; now in the Soviet Union.");
        alert(p.label() + " IS OUT. He crossed the frontier during the night on our documents. Centre will look after him.");
    } else arrest(p, "ARRESTED AT THE FRONTIER with our documents on him.");
}

/** The agent is taken, publicly. (caught() decides first whether they would rather turn him.) */
void Sim::arrest(Person& p, const std::string& how) {
    Actor& ha = p.handler && officer(p.handler) ? officer(p.handler)->a : g.you;
    p.stage = ARRESTED;
    clearAppointments(p);
    p.nextMeet = -1; p.nextDrop = -1;
    for (int id : p.held) if (Roll* x = roll(id)) x->state = R_LOST;
    p.held.clear();
    for (int id : p.meetSites) if (Site* s = site(id)) s->burned = true;
    if (Site* s = site(p.dropSite)) stake(*s);
    if (Site* s = site(p.signalSite)) s->burned = true;
    if (p.bug == 2 || p.bug == 1) p.bug = 3;
    ha.heat = clampf(ha.heat + 10 * hard(), 0, 100);
    // a chief with twenty agents loses more of them than an officer with two, and is judged accordingly
    g.standRez -= 10 / (1 + 0.5f * T().role); g.standCentre -= (p.service != SVC_NONE ? 9 : 6) / (1 + 0.5f * T().role); g.totArrested++;
    file(p, how);
    Person* keep = subj; subj = &p;
    cable(T().chief, "Arrest of " + p.codename,
          "The newspapers report that " + p.name + fstr(", %d, ", p.age) + p.job + ", has been arrested and charged with espionage.\n\n"
              "Assume he is talking. Every site, signal and routine known to " + p.codename + " is finished as of today, and his case officer may be named in court. "
              "A full damage report is required.");
    subj = keep;
}

// ============================================================================ listening devices
std::string Sim::requestBug() {
    for (auto& x : g.pending) if (x.kind == P_BUG) return "A device is already being prepared.";
    if (g.bugs >= 2) return "You hold two devices already. The technical officer will not build a third until they are used.";
    pend(g.day + 12 + r().n(10), P_BUG, 0, 1);
    g.paperwork += 1;
    return "The technical officer will prepare a listening device with its transmitter and arrange a listening post. Allow two to three weeks. "
           "It must then be placed by an agent inside his own office: give it to him at a long meeting.";
}

/** Nightly: the agent places the device; a live device makes a tape every week or so until it is found or the battery dies. */
void Sim::bugNight(Person& p, bool work) {
    bool ctl = controlled(p);
    if (p.bug == 1 && g.day >= p.bugDay && work) {
        if (!ctl && pct((p.empSec - p.craft * 0.5f) * 0.22f)) {            // nearly seen with it in his hands
            p.bug = 3; p.suspicion += 25; p.stress += 15;
            p.warn.push_back("could not place the device: he was nearly seen with it and has thrown it in the river.");
        } else {
            p.bug = 2; p.bugUntil = g.day + 60 + r().n(70); p.bugNext = g.day + 6 + r().n(6);
            p.warn.push_back("reports that the device is in place.");
        }
    }
    if (p.bug != 2) return;
    if (g.day >= p.bugNext) {
        Requirement* q = req(p.reqId);
        float match = q && !q->closed && q->kind == RK_COLLECT && q->sector == p.sector ? 1.f : 0.6f;
        float v = ctl ? rnd(20, 45) : clampf(p.tru[A_ACCESS] * rnd(0.45f, 0.85f) * match, 3, 100);
        Roll& x = newRoll(p);
        x.tech = true; x.frames = 0; x.value = v;
        x.state = R_LAB;                                                  // the listening post's tape goes straight to the technical officer
        p.bugNext = g.day + 7 + r().n(6);
    }
    if (g.day > p.bugUntil) p.bug = 3;                                    // the battery
    else if (!ctl && pct(0.12f + p.empSec * 0.004f)) { p.bug = 3; p.suspicion += 26; }   // found in a sweep. Nobody tells you: the tapes stop.
}
