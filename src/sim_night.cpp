// sim_night.cpp - end of day: your officers' work, the night, the next dawn, Centre's answers, history.
#include "sim.h"

static std::string upper(std::string s) { for (auto& c : s) c = (char)toupper((unsigned char)c); return s; }
static bool has(const std::vector<int>& v, int x) { return std::find(v.begin(), v.end(), x) != v.end(); }

// ============================================================================ END OF DAY
/**
 * The day, with a pause in it. If a meeting or a drop of yours meets surveillance and you have asked to decide such
 * things yourself, the day is put back exactly as it was this morning and the question is put to you (DK_STREET).
 * Your answer is kept in g.streetForce and the day is played again from the start: the random numbers are part of the
 * state that was put back, so everything up to that moment happens exactly as before, and from then on as you chose.
 */
void Sim::endDay() {
    if (g.phase != PH_PLAY || !g.decisions.empty()) return;
    deputyPlan();
    bool mayAsk = (askInStreet || askScenes) && !training();
    if (mayAsk) {
        mayAsk = false;
        for (int b = 0; b < 3; b++) {
            int t = order(g.day, b).type;
            if ((askInStreet && (t == O_MEET_CLAN || t == O_LOAD_DROP || t == O_UNLOAD_DROP || t == O_LINE_N)) || (askScenes && t == O_MEET_CLAN)) mayAsk = true;
        }
    }
    if (!mayAsk) { endDayInner(); return; }
    GameState morning = g;
    try {
        endDayInner();
    } catch (const StreetHalt& h) {
        g = std::move(morning);
        rep = nullptr; subj = nullptr; actAs(nullptr);
        addDecision(h.kind, h.block, h.title, h.text, h.opts);
        attention = true;
    }
}

void Sim::endDayInner() {
    rep = &g.reports[g.day];
    rep->clear();
    attempted.clear();
    say("OPERATIONS REPORT - " + upper(fmtDay(g.day)) + " - " + upper(T().city));
    say("");
    if (g.deputy && !deputyLog.empty()) { say(deputyLog); say(""); }
    for (int b = 0; b < 3; b++) { sdrDone[b] = false; sdrRoute[b] = -1; }
    if (g.fatigue >= 92) {
        alert("You are ill with exhaustion and stayed in your flat all day. Today's orders were not carried out.");
        g.fatigue -= 30;
    } else {
        bool busy = g.day <= g.busyUntil, down = g.day <= g.standDownUntil;
        if (busy) {
            bool bed = g.busyWhy.find("bed") != std::string::npos;
            say("Your day belonged to somebody else: " + g.busyWhy + ".");
            if (bed) g.fatigue = std::max(0.f, g.fatigue - 14);
            else if (!onLeave(g.day)) { g.fatigue = std::min(100.f, g.fatigue + 7); g.cover = std::min(100.f, g.cover + 1.5f); }
        }
        for (int b = 0; b < 3 && g.phase == PH_PLAY; b++) {
            Order o = order(g.day, b);
            if (busy) {
                if (o.set && o.type != O_REST && o.type != O_COVER) say(upper(BLOCK_LABEL[b]) + " - not carried out: " + OTYPE_LABEL[o.type]);
                continue;
            }
            if (down && isOp(o.type)) {
                Person* ap = person(o.person);
                bool kept = meetsAllowed(g) && o.type == O_MEET_CLAN && ap && ap->stage == AGENT && ap->nextMeet == g.day;
                if (!kept) {
                    say(upper(BLOCK_LABEL[b]) + " - " + OTYPE_LABEL[o.type] + ": not carried out. Line KR has stood the station down.");
                    continue;
                }
            }
            say(upper(BLOCK_LABEL[b]) + " - " + OTYPE_LABEL[o.type]);
            resolve(o, b);
            if (b == 2 && g.phase == PH_PLAY) embassyReception(b);
        }
    }
    if (g.phase == PH_PLAY) officersDay();
    if (g.phase == PH_PLAY) night();
    if (g.phase == PH_PLAY && training()) tutorialNight();
    g.plan.erase(g.day);
    for (int& f : g.streetForce) f = -1;
    for (int& f : g.sceneForce) f = -1;
    if (g.phase == PH_PLAY) { g.day++; dawn(); }
    if (g.phase == PH_PLAY && g.deputy) {          // he plans the new day before you see it, so that you can change what he did
        deputyPlan();
        if (!deputyLog.empty()) g.brief += "\nDEPUTY:\n  " + deputyLog.substr(deputyLog.find(": ") + 2) + "\n";
    }
    // keep the save file from growing without limit
    while (!g.reports.empty() && g.reports.begin()->first < g.day - 500) g.reports.erase(g.reports.begin());
    while (!g.zenit.empty() && g.zenit.begin()->first < g.day - 500) g.zenit.erase(g.zenit.begin());
    while (!g.csDays.empty() && g.csDays.begin()->first < g.day - 30) g.csDays.erase(g.csDays.begin());
    rep = nullptr;
    actAs(nullptr);
    subj = nullptr;
}

// ============================================================================ the officers under you
/** Each officer under your command does at most one thing a day, on his own judgement, within the assignment you gave him. */
void Sim::officersDay() {
    bool any = false;
    for (auto& o : g.officers) {
        if (o.gone || o.role != ROLE_SUB || o.assign == AS_REST) continue;
        actAs(&o);
        auto header = [&]() {
            if (!any) { say(""); say("YOUR OFFICERS"); any = true; }
            say("  " + o.title() + ":");
        };
        bool acted = false;
        // 1. an agreed meeting with one of his agents
        for (auto& p : g.persons) {
            if (p.stage != AGENT || p.handler != o.id || p.nextMeet != g.day) continue;
            Site* s = agreedSite(p);
            if (!s) break;
            header();
            Order m;
            m.type = O_MEET_CLAN; m.person = p.id; m.site = s->id; m.appointment = true; m.policy = o.policy;
            m.longMeet = true; m.train = p.craft < 60;
            m.pay = std::max(0, std::min(p.owed, g.funds));
            m.film = (p.camera && p.film < 2) ? std::min(2, g.film) : 0;
            m.camera = !p.camera && g.cameras > 0;
            subj = &p;
            // your instruction first; failing that, his own judgement
            bool alarm = false;
            for (auto& w : p.warn) if (w.find("security officer") != std::string::npos || w.find("clearance") != std::string::npos || w.find("searched") != std::string::npos || w.find("telephone clicks") != std::string::npos || w.find("followed from") != std::string::npos || w.find("further conversation") != std::string::npos) alarm = true;
            if (alarm && o.policy < 2) { m.collect = false; m.longMeet = false; }       // talk only: nothing incriminating changes hands
            if (p.instrTempo >= 0) { m.tempo = p.instrTempo; p.instrTempo = -1; }
            else if (alarm && p.tempo > 0 && o.policy < 2) { m.tempo = 0; say("    (On his own judgement he is putting " + p.label() + " on ice. Instruct him otherwise if you disagree.)"); }
            if (g.day <= o.waryUntil && o.policy < 2 && !alarm) {
                m.brush = true; m.longMeet = false; m.train = false;
                say("    (He is lying low after seeing surveillance: a brush pass only, nothing said.)");
            }
            // a worn meeting place is changed at this meeting, for one he has not used too often
            if (!m.brush && s->exposure > 50) {
                Site* fresh = nullptr;
                for (auto& x : g.sites)
                    if (x.type == ST_MEET && !x.retired && !x.flat && x.casing >= 1 && x.exposure < 30 && std::find(p.meetSites.begin(), p.meetSites.end(), x.id) == p.meetSites.end()
                        && (!fresh || x.exposure < fresh->exposure)) fresh = &x;
                if (fresh) {
                    m.setMeetSites = true;
                    m.meetSites = p.meetSites;
                    std::replace(m.meetSites.begin(), m.meetSites.end(), s->id, fresh->id);
                    witness(fresh->witting, o.id);
                    say("    (" + shortSite(s) + " is worn: he is moving " + p.label() + " to " + shortSite(fresh) + ".)");
                }
            }
            if (p.instrReq > -2) { m.reqId = p.instrReq; p.instrReq = -2; }
            else { Requirement* cur = req(p.reqId); if (!cur || cur->closed) for (auto& q : g.reqs) if (!q.closed && q.kind == RK_COLLECT && q.sector == p.sector) { m.reqId = q.id; break; } }
            meetClan(m, 2);
            subj = nullptr;
            acted = true;
            break;
        }
        if (g.phase != PH_PLAY) break;
        // 2. empty a drop one of his agents has filled
        if (!acted && !o.gone) for (auto& p : g.persons) {
            if (p.stage != AGENT || p.handler != o.id || p.fetchDay < 0 || p.fetchDay > g.day || !site(p.dropSite)) continue;
            header();
            if (g.day <= o.waryUntil && o.policy < 2 && pct(70)) {
                p.fetchDay = g.day + 3;
                say("    He is lying low, and has put off emptying " + p.label() + "'s drop for a few days.");
                acted = true;
                break;
            }
            p.fetchDay = -1;
            Site* sg = site(p.signalSite);
            if (sg && sg->mark == 2) {
                subj = &p;
                alert("He found " + p.label() + "'s DANGER signal up and did not go to the drop.");
                subj = nullptr;
            } else {
                Order u;
                u.type = O_UNLOAD_DROP; u.site = p.dropSite; u.policy = o.policy;
                unloadDrop(u, 1);
            }
            acted = true;
            break;
        }
        if (g.phase != PH_PLAY) break;
        // 3. work on one of his developmental contacts (not while he is lying low)
        if (!acted && !o.gone && g.day - o.lastAct >= 2 && g.day > o.waryUntil) {
            Person* best = nullptr;
            for (auto& p : g.persons) {
                if (!p.live() || p.stage == AGENT || p.handler != o.id) continue;
                if (g.day - std::max(p.lastContact, p.firstDay) < 8) continue;
                if (!best || p.rapport > best->rapport) best = &p;
            }
            if (best) {
                Person& p = *best;
                header();
                subj = &p;
                Site* ms = nullptr;
                for (auto& s : g.sites) if (s.type == ST_MEET && !s.retired && s.casing >= 1 && (!ms || s.exposure < ms->exposure)) ms = &s;
                if (p.pitchAuth && p.sanction == 2 && p.rapport > 45 && g.day - p.lastPitchDay > 60 && ms) {
                    Order m;
                    m.type = O_MEET_CLAN; m.person = p.id; m.site = ms->id; m.policy = o.policy;
                    int bi = 0;
                    static const int at[] = {A_MONEY, A_IDEO, A_EGO};
                    for (int i = 1; i < 3; i++) if (p.estV[at[i]] > p.estV[at[bi]]) bi = i;
                    m.pitch = bi; m.pay = std::min(P(20), g.funds); m.camera = true; m.film = 2; m.tempo = 1;
                    meetClan(m, 2);
                } else {
                    Order m;
                    m.type = O_MEET_OVERT; m.person = p.id;
                    m.favour = p.rapport > 35 && p.commitment < 60;
                    meetOvert(m, 1);
                    if (p.live() && !p.recommended && p.rapport > 50 && p.commitment > 30) {
                        p.recommended = true;
                        alert(o.title() + " considers " + p.name + " ripe for an approach. He asks you to obtain a trace and Centre's sanction, and then either to authorise him to make it or to take the case yourself.");
                    }
                }
                subj = nullptr;
                acted = true;
            }
        }
        if (g.phase != PH_PLAY) break;
        // 4. look for new people
        if (!acted && !o.gone && g.day > o.waryUntil && o.assign == AS_SPOT && (g.day % 7) == (o.id % 7) && pct(24 + o.nose * 0.2f)) {
            header();
            spot(o.sector, "by " + o.title() + " at a function", o.id);
            money(-std::min(g.funds, P(3)), "Function expenses, " + o.title());
            acted = true;
        }
        if (acted) o.lastAct = g.day;
    }
    actAs(nullptr);
    subj = nullptr;
}

// ============================================================================ NIGHT
void Sim::night() {
    actAs(nullptr);
    subj = nullptr;
    // --- material goes into the lab queue; the lab works; Thursday is pouch day
    for (auto& x : g.rolls) if (x.state == R_YOU) { x.state = x.oral ? R_READY : R_LAB; x.stateDay = g.day; }
    int cap = labPerDay();
    for (auto& x : g.rolls) if (x.state == R_LAB && x.stateDay < g.day && cap > 0) { x.state = R_READY; x.stateDay = g.day; cap--; }
    if (dateOf(g.day).dow == 3) {
        int n = 0;
        for (auto& x : g.rolls) if (x.state == R_READY) {
            x.state = R_POUCH; x.stateDay = g.day; n++;
            pend(g.day + 4, P_POUCH_ARRIVE, x.id, 0);
        }
        if (n > 0) say(fstr("POUCH: %d item%s of material left by diplomatic courier tonight.", n, n == 1 ? "" : "s"));
    }
    // --- cipher room
    int used = 0;
    while (!g.outQ.empty()) {
        OutCable c = g.outQ.front();
        if (used + c.groups > cipherQuota()) break;
        used += c.groups;
        g.outQ.erase(g.outQ.begin());
        sendCable(c);
    }
    g.groupsToday = used;
    if (used > 0) say(fstr("CIPHER: %d of your %d groups used today.", used, cipherQuota()) + (g.outQ.empty() ? std::string("") : fstr(" %d cable(s) held over to tomorrow.", (int)g.outQ.size())));

    // --- agents and contacts live their lives
    for (size_t i = 0; i < g.persons.size() && g.phase == PH_PLAY; i++) {
        Person& p = g.persons[i];
        subj = &p;
        if (p.stage == AGENT) agentNight(p);
        else if (p.live()) contactNight(p);
    }
    subj = nullptr;
    if (g.phase != PH_PLAY) return;
    moles();
    // --- sites
    for (auto& s : g.sites) {
        s.exposure = std::max(0.f, s.exposure - 0.4f);
        if (s.flat) s.neighbours = std::max(0.f, s.neighbours - 0.22f);
        if (s.staked && g.day - s.stakedDay > 12) s.staked = false;   // the observation post is withdrawn; the site stays known
        bool load = !s.rolls.empty() || s.cash > 0 || s.film > 0 || s.camera;
        if (s.type == ST_DROP && load && !s.staked) {
            float risk = (100 - s.cover) / 100.f * 2.5f * (s.container ? 0.35f : 1.f);
            if (pct(risk)) {       // found by a gardener, a child, a dog. You are not told.
                for (int id : s.rolls) if (Roll* x = roll(id)) { x->state = R_LOST; if (Person* ow = person(x->agentId)) ow->suspicion += 15; }
                s.rolls.clear(); s.cash = 0; s.film = 0; s.camera = false; s.forAgent = -1;
                if (pct(50)) s.burned = true;
            }
        }
    }
    // --- the adversary's picture of your habits fades if it is not refreshed (about two months to half), and so does its alarm
    for (auto& row : g.habit) for (float& x : row) x *= 0.985f;
    for (float& x : g.habitD) x *= 0.985f;
    g.advAlert *= 0.97f;
    // --- you
    homeNight();
    g.fatigue = std::max(0.f, g.fatigue - (training() ? 10 : g.wifeHere ? 10 + (g.home - 50) / 12.5f : 9));     // a man sleeps better in a quiet house
    if (!dayOff(g.day) && !onLeave(g.day)) g.cover = std::max(0.f, g.cover - 1.6f);
    for (int k = -1; k < (int)g.officers.size(); k++) {
        Actor& a = k < 0 ? g.you : g.officers[k].a;
        float floor = std::max(8.f, a.peakHeat * 0.3f);     // once carded you are never quite forgotten
        if (a.heat > floor) a.heat = std::max(floor, a.heat - 0.15f - 0.012f * (a.heat - floor));   // interest fades fastest when it is highest
    }
    if (g.cover < 20) g.you.heat += 0.6f; else if (g.cover < 40) g.you.heat += 0.3f;     // a "diplomat" who is never at his desk
    if (onLeave(g.day)) {}                                    // nobody expects reports from a man on the Black Sea
    else if (g.paperwork > 8) g.overdueDays++; else g.overdueDays = 0;
    if (g.overdueDays > 0 && g.overdueDays % 5 == 0) {
        g.standRez -= 3;
        alert(fstr("Your contact reports are %d hours in arrears, and it has been noticed.", (int)std::lround(g.paperwork)));
    }
    if (g.fatigue > 75) say("You are badly over-tired. Fatigue dulls your eye for surveillance.");
    g.standRez = clampf(g.standRez, 0, 100); g.standCentre = clampf(g.standCentre, 0, 100);
    heat(0);
    if (g.standRez > 35) g.warnedLow = false;
    if (g.standRez < 20 && !g.warnedLow) {
        g.warnedLow = true;
        cable(T().chief, "A warning", T().role == 3
            ? "The department is dissatisfied with the work of your rezidentura and with your direction of it. Unless the position improves in the coming months the question of your replacement will be raised.\n\nHEAD OF THE FOURTH DEPARTMENT"
            : "I am dissatisfied with your work, and I have said so to Centre. Results, reports on time, and no more incidents - or I shall ask for you to be replaced. This is the only warning you will have.\n\nREZIDENT");
    }
    if (g.standRez < 6 && g.patron >= 60 && !g.patronSaved && !training()) {
        g.patronSaved = true;
        g.patron = clampf(g.patron - 35, 0, 100);
        g.standRez = 24;
        cable("Centre", "The question of your recall", "The question of your recall was raised by the station and has been closed by the leadership of the directorate. "
              "General " + g.patronName + " has said what he had to say on your behalf. He will not say it a second time.\n\nCENTRE");
    }
    if (g.standRez < 6)
        endPosting(END_RECALLED, "RECALLED TO MOSCOW", T().role == 3
            ? "The department has lost confidence in your direction of the rezidentura. You are recalled 'for consultations' and will not be returning."
            : "The Rezident has reported to Centre that you are unfit for operational work at this station. You are recalled 'for consultations' and will not be returning.");
}

void Sim::sendCable(const OutCable& c) {
    switch (c.kind) {
        case C_TRACE: pend(g.day + (T().friends ? 2 + r().n(3) : 6 + r().n(9)), P_TRACE, c.ref, 0); break;   // the friends' card index answers in days
        case C_EXFIL: pend(g.day + 7 + r().n(7), P_EXFIL, c.ref, 0); break;
        case C_SANCTION: pend(g.day + 5 + r().n(8), P_SANCTION, c.ref, 0); break;
        case C_FUNDS: pend(g.day + 4 + r().n(6), P_FUNDS, 0, P(100)); break;
        case C_KR: pend(g.day + 14 + r().n(16), P_KR, c.ref, 0); break;
        case C_RADIO: pend(g.day + 2 + r().n(3), P_RADIO, c.ref, c.p1, c.p2, c.p3); break;
        case C_VERIFY: pend(g.day + 10 + r().n(11), P_VERIFY, c.ref, 0); break;
        case C_KOMP: pend(g.day + 7 + r().n(8), P_KOMP, c.ref, 0); break;
        case C_PLAY: pend(g.day + 8 + r().n(8), P_PLAY, c.ref, 0); break;
        case C_CONFID: pend(g.day + 5 + r().n(6), P_CONFID, c.ref, 0); break;
        case C_GAME: pend(g.day + 8 + r().n(8), P_GAME, 0, 0); break;
        case C_SUMMARY: {
            Roll* x = roll(c.ref);
            if (x && x->state == R_READY) {
                x->state = R_CENTRE; x->stateDay = g.day;
                x->value *= 0.85f;
                pend(g.day + 3 + r().n(3), P_GRADE, x->id, 0);
            } else if (x) x->cabled = false;
            break;
        }
        default: break;
    }
}

static const char* const WARN25[] = {"says a new signing-out register has been introduced for classified papers in his section.",
                                     "mentions that a two-man rule now applies to the registry after hours."};
static const char* const WARN40[] = {"says the departmental security officer asked 'routine' questions about his working late.",
                                     "has been told his clearance is to be reviewed earlier than usual."};
static const char* const WARN60[] = {"is sure his desk has been searched, and a man nobody knows has joined the office.",
                                     "says his telephone clicks and a car was parked in his road three nights running."};
static const char* const WARN75[] = {"is certain he is followed from his office every evening. He asks what will happen to his family.",
                                     "has been told not to leave the country and to hold himself ready for 'a further conversation'. He is very frightened."};
static const char* const WARNFALSE[] = {"thinks he was followed home last week, but is not sure.",
                                        "asks whether you are certain the meetings are safe. He has slept badly."};

void Sim::agentNight(Person& p) {
    bool ctl = controlled(p);
    bool work = !dayOff(g.day);
    float before = p.suspicion;
    // --- an inherited source whose name is in somebody else's files
    if (p.exposeDay >= 0 && g.day >= p.exposeDay && !ctl) {
        p.exposeDay = -1;
        p.suspicion = 100;
        file(p, "Named in records that have reached the adversary.");
    }
    // --- collection
    if (p.tempo > 0 && work && g.day >= p.sickUntil) {
        static const float mult[] = {0, 4, 10, 22};
        float access = ctl ? p.app[A_ACCESS] : p.tru[A_ACCESS];
        float pages = access / 100.f * mult[p.tempo] * rnd(0.5f, 1.5f) * (0.85f + p.skPhoto / 300.f);   // a practised hand photographs more in the same ten minutes
        if (p.camera && p.film > 0) {
            Requirement* q = req(p.reqId);
            float match = q && !q->closed && q->kind == RK_COLLECT && q->sector == p.sector ? 1.f : 0.55f;
            float v = ctl ? (p.fed < 3 ? rnd(22, 42) : rnd(30, 62)) : (p.fabricator && pct(45) ? -1 : p.tru[A_ACCESS] * rnd(0.6f, 1.05f) * match * (0.82f + p.skPhoto / 330.f));   // and fewer of the frames are unreadable
            p.frameVal += pages * v;
            p.frames += (int)std::lround(pages);
            if (p.frames >= FRAMES) {
                float avg = p.frameVal / p.frames;
                Roll& x = newRoll(p);
                x.frames = FRAMES;
                if (avg < 0 || (p.fabricator && avg < p.tru[A_ACCESS] * 0.35f)) { x.genuine = false; avg = rnd(25, 55); }
                if (ctl) x.genuine = p.fed++ < 3;            // the first deliveries are true, and cost their owners nothing: chicken feed
                x.value = clampf(avg, 3, 100);
                x.state = R_AGENT;
                p.held.push_back(x.id);
                p.film--; p.frames = 0; p.frameVal = 0;
            }
        }
        if (!ctl) {
            float t = (float)p.tempo;
            p.suspicion += (t * t * (p.empSec / 100.f) * (1 - p.craft / 150.f) * 0.11f + (100 - p.tru[A_DISC]) / 100.f * 0.03f * t) * hard();
            p.stress += t * 0.14f * (1.5f - p.tru[A_NERVE] / 100.f);
        }
    } else if (!ctl) {
        p.stress -= 0.3f;
        p.suspicion -= p.suspicion < 40 ? 0.15f : 0.4f;
    }
    bugNight(p, work);
    if (ctl) { p.stress = 15; p.suspicion = 0; }
    // --- money
    static const std::string unpaid = "complains that he has not been paid what was promised.";
    if (p.owed > p.salary * 2 && p.salary > 0 && pct(8)) {
        p.rapport -= 3; p.stress += 3;
        if (std::find(p.warn.begin(), p.warn.end(), unpaid) == p.warn.end()) p.warn.push_back(unpaid);
    }
    if (!ctl && p.paid90 > P(150) && p.tru[A_DISC] < 45 && pct(1.2f)) {
        p.suspicion += 8;
        p.warn.push_back("is wearing a new watch and talks of buying a motor-car. He was warned against spending conspicuously.");
    }
    // --- his own life
    lifeEvent(p);
    if (p.stage != AGENT) return;
    // --- investigation
    if (!ctl) {
        if (p.suspicion >= 40 && p.tempo > 0) p.suspicion += 0.35f;
        p.suspicion = std::max(0.f, p.suspicion);
        float tell = p.tru[A_HONEST] * 0.6f;
        if (before < 25 && p.suspicion >= 25 && pct(tell)) p.warn.push_back(pick(WARN25));
        if (before < 40 && p.suspicion >= 40 && pct(tell)) p.warn.push_back(pick(WARN40));
        if (before < 60 && p.suspicion >= 60) {
            if (pct(tell)) p.warn.push_back(pick(WARN60));
            if (p.signalSite >= 0 && p.craft > 25 && pct(40)) if (Site* sg = site(p.signalSite)) sg->mark = 2;
        }
        if (before < 75 && p.suspicion >= 75 && pct(p.tru[A_HONEST] * 0.9f)) p.warn.push_back(pick(WARN75));
        if (before < 75 && p.suspicion >= 75 && p.escape == 2) escapeSignal(p);
        if (p.escape == 3) p.suspicion += 5;          // he is waiting at the corner every evening, and they are getting closer
        else if (p.escape == 4) p.suspicion += 1.2f;
        if (p.suspicion >= 60) p.stress += 0.6f;         // a man who knows he is watched does not hide it well
        if (p.tru[A_NERVE] < 45 && pct(0.5f)) p.warn.push_back(pick(WARNFALSE));
        if (p.suspicion >= 100) {
            if (p.escape >= 3) { p.escape = 0; if (Site* sg = site(p.signalSite)) if (sg->mark == 4) sg->mark = 0; }      // too late: nobody came past in time
            caught(p);
            return;
        }
        p.stress = clampf(p.stress, 0, 100);
        if (p.stress > 85 && pct(3)) { breakdown(p); return; }
    }
    // --- dead drop cycle
    if (p.escape < 3 && p.dropEvery > 0 && p.dropSite >= 0 && p.nextDrop >= 0 && g.day >= p.nextDrop) serviceDrop(p);
    // --- meetings
    if (p.nextMeet == g.day && !attempted.count(p.id)) {
        Site* s = agreedSite(p);
        say("MISSED MEETING: " + p.label() + " waited at " + (s ? s->name : std::string("the agreed place")) + " and nobody came.");
        p.stress += 8; p.rapport = clampf(p.rapport - 4, 0, 100);
        attention = true;
        fallback(p);
    }
    if (p.nextMeet < 0 && p.lastContact >= 0 && g.day - p.lastContact > 150 && p.dropEvery <= 0) {
        p.stage = TERMINATED;
        file(p, "No contact for five months. Agent considered lost; file to archive.");
        alert("Contact with " + p.label() + " has lapsed. File closed.");
    }
}

void Sim::serviceDrop(Person& p) {
    Site* sp = site(p.dropSite);
    bool ctl = controlled(p);
    if (!sp || sp->retired) { p.nextDrop = -1; return; }
    Site& s = *sp;
    Site* sg = site(p.signalSite);
    if (!ctl && pct((100 - p.diligence) * 0.5f * (1 - p.skComms / 160.f) + (p.stress > 80 ? 20 : 0))) { p.nextDrop = g.day + 2; return; }   // he put it off
    if (!ctl && p.suspicion >= 60 && sg && sg->mark == 2) { p.nextDrop = g.day + p.dropEvery; return; }
    if (!ctl) {
        if (s.staked) p.suspicion = std::max(p.suspicion + 30, 75.f);     // walked into a watched drop
        else if (p.suspicion >= 50 && pct((p.suspicion - 35) * (1 - p.skComms / 220.f))) stake(s);     // led the watchers to it
    } else {
        s.burned = true;
        if (p.trapDay >= 0 && g.day >= p.trapDay) stake(s);
    }
    if (s.forAgent == p.id) {
        p.owed -= s.cash; p.paidTotal += s.cash; p.paid90 += s.cash;
        p.film += s.film;
        if (s.camera) p.camera = true;
        if (s.tempoNote >= 0) p.tempo = s.tempoNote;
        if (s.reqNote > -2) p.reqId = s.reqNote;
        s.cash = 0; s.film = 0; s.camera = false; s.tempoNote = -1; s.reqNote = -2; s.forAgent = -1;
    }
    if (!p.held.empty()) {
        for (int id : p.held) if (Roll* x = roll(id)) { x->state = R_DROP; x->stateDay = g.day; s.rolls.push_back(id); }
        p.held.clear();
        s.loadedDay = g.day;
        if (sg && sg->mark != 2) sg->mark = 1;
        if (p.handler != 0) p.fetchDay = g.day + 2;
    }
    s.exposure += 3 * (1 - p.skComms / 200.f) + (g.wx == WX_SNOW ? 4 : 0);
    p.nextDrop = g.day + p.dropEvery;
}

void Sim::caught(Person& p) {
    float turn = 25 + (100 - p.tru[A_NERVE]) / 2.5f;
    Actor& ha = p.handler && officer(p.handler) ? officer(p.handler)->a : g.you;
    if (pct(turn) && !p.orphan) {           // confessed, and agreed to go on meeting. You are told nothing.
        p.doubled = true; p.suspicion = 0; p.stress = 15;
        p.trapDay = g.day + 60 + r().n(200);
        for (int id : p.meetSites) if (Site* s = site(id)) s->burned = true;
        if (Site* s = site(p.dropSite)) s->burned = true;
        if (ha.heat < 45) ha.heat = 45;
        return;
    }
    arrest(p, "ARRESTED. Charged with espionage.");
}

void Sim::breakdown(Person& p) {
    if (pct(40)) { p.suspicion = 100; caught(p); return; }     // goes to the authorities himself
    p.stage = TERMINATED;
    clearAppointments(p);
    p.nextMeet = -1; p.nextDrop = -1;
    for (int id : p.held) if (Roll* x = roll(id)) x->state = R_LOST;
    p.held.clear();
    if (Site* sg = site(p.signalSite)) sg->mark = 2;
    file(p, "Agent's nerve has gone. He has destroyed his equipment and refuses all further contact.");
    alert(p.label() + " has broken down under the strain and refuses all further contact. He says he has thrown the camera in the river.");
}

void Sim::contactNight(Person& p) {
    int idle = g.day - std::max(p.lastContact, p.firstDay);
    if (idle > 21) p.rapport -= 0.15f;
    if (p.rapport < 3 && p.meetings > 0 && idle > 60) { p.stage = BROKEN; file(p, "Acquaintance has lapsed through neglect."); }
    if (p.stage == SPOTTED && idle > 120) { p.stage = BROKEN; file(p, "Never followed up. File closed."); }
}

// ============================================================================ treachery
/** A colleague who works for the other side passes on whatever he has seen. Slowly: they protect their source. */
void Sim::moles() {
    for (auto& o : g.officers) {
        if (o.gone || !o.mole || o.moleDay > g.day) continue;
        float rate = o.cautious ? 0.5f : 1.4f;
        for (auto& p : g.persons) {
            if (!p.live() || controlled(p)) continue;
            if (p.handler != o.id && !has(p.witting, o.id)) continue;
            if (!pct(rate)) continue;
            if (p.stage == AGENT) p.suspicion = std::max(p.suspicion, 50.f) + 22;   // "information received"
            else makeDangle(p);                                                      // intercepted, and played back
        }
        for (auto& s : g.sites) if (!s.burned && has(s.witting, o.id) && pct(rate * 2)) s.burned = true;
        if (g.you.heat < 40) g.you.heat += 0.08f;
    }
}

/** Everything this officer knew is in the adversary's hands as of today. */
void Sim::blowTo(Officer& o, bool everything) {
    for (auto& p : g.persons) {
        if (!p.live() || controlled(p)) continue;
        if (p.handler != o.id && !has(p.witting, o.id)) continue;
        if (!everything && !pct(60)) continue;
        if (p.stage == AGENT) { p.suspicion = std::max(p.suspicion, 88.f); p.stress += 20; }
        else if (pct(50)) { p.stage = BROKEN; p.file.push_back(pad(fmtShort(g.day), 11) + "Warned off by his security people. Contact broken."); }
        else makeDangle(p);
    }
    for (auto& s : g.sites) if (has(s.witting, o.id)) { s.burned = true; if (s.type == ST_DROP) stake(s); }
    for (auto& p : g.persons) if (p.live() && p.handler == o.id) { p.handler = 0; p.pitchAuth = false; if (p.stage == AGENT) schedule(p); }
}

void Sim::defection(Officer& o) {
    o.gone = true;
    o.file.push_back(pad(fmtShort(g.day), 11) + "DEFECTED to the adversary.");
    blowTo(o, true);
}

void Sim::addDecision(int kind, int ref, const std::string& title, const std::string& text, std::vector<std::string> opts) {
    Decision d;
    d.id = g.nextId++; d.kind = kind; d.ref = ref; d.day = g.day; d.title = title; d.text = text; d.opts = opts;
    g.decisions.push_back(d);
    attention = true;
}

// ============================================================================ credit and grading
void Sim::credit(int sector, float pts, int reqId) {
    Requirement* q = req(reqId);
    if (!q || q->closed || q->sector != sector || q->kind != RK_COLLECT) {
        q = nullptr;
        for (auto& x : g.reqs) if (!x.closed && x.kind == RK_COLLECT && x.sector == sector) { q = &x; break; }
    }
    if (!q) return;
    q->progress += pts;
    if (q->progress >= q->need) {
        q->closed = true;
        g.standCentre = clampf(g.standCentre + 8, 0, 100); g.standRez = clampf(g.standRez + 3, 0, 100);
        g.totReqDone++;
        Person* keep = subj; subj = nullptr;
        cable("Centre", fstr("Requirement No. %d satisfied", q->id),
              "The material received on '" + q->title + "' meets the requirement. The department concerned has expressed its thanks. "
              "Convey Centre's appreciation to the source in an appropriate form.\n\nCENTRE");
        subj = keep;
    }
}

void Sim::grade(Roll& x) {
    Person* p = person(x.agentId);
    std::string src = p ? p->label() : "?";
    float v = x.value, pts;
    std::string gr;
    if (v >= 72) { gr = "OF CONSIDERABLE VALUE"; pts = v * 1.5f; }
    else if (v >= 45) { gr = "VALUABLE"; pts = v; }
    else if (v >= 25) { gr = "of interest"; pts = v * 0.6f; }
    else { gr = "of little interest / already known"; pts = v * 0.2f; }
    if (x.oral) pts *= 0.5f;
    if (x.tech) pts *= 0.8f;                 // a tape is talk, not documents
    pts *= T().gradeMult;
    std::string note;
    if (!x.genuine && pct(p && p->fabricator ? 32 : 36))
        note = " NOTE: parts contradict information from reliable sources. The source requires verification.";
    std::string hint;
    if (p && !x.oral && !x.tech && ((x.genuine && controlled(*p) && pct(32)) || (x.genuine && !controlled(*p) && v < 34 && pct(6))))
        hint = " The material is accurate, but there is nothing in it that its owners would much mind losing.";
    if (p && p->play == 2) {            // nothing a played-back source brings is credited
        x.grade = "disregarded (playback)";
        x.state = R_GRADED; x.stateDay = g.day;
        g.totRolls++; g.mRolls++;
        gradeLines.push_back(fstr("No. %d (", x.id) + src + "): read for what it shows of the adversary's intentions, and otherwise disregarded.");
        return;
    }
    x.grade = gr + (note.empty() ? "" : " (queried)");
    if (p && p->verify == 2) verifyRoll(*p, x);
    x.state = R_GRADED; x.stateDay = g.day;
    g.totRolls++; g.mRolls++; g.totValue += pts; g.mValue += pts;
    g.standCentre = clampf(g.standCentre + pts / (80.f * (1 + 0.5f * T().role)), 0, 100);
    credit(x.sector, pts, x.reqId);
    if (x.sector == SECURITY && x.genuine && v >= 40) g.secBonusUntil = g.day + 120;
    gradeLines.push_back(fstr("No. %d (", x.id) + src + (x.oral ? ", oral report" : x.tech ? ", tape" : ", film") + (x.cabled ? ", by cable summary" : "") + "): " + gr + "." + note + hint);
}

// ============================================================================ DAWN
/** Is this officer followed today? Depends on how interested the adversary is in him and how stretched they are. */
/**
 * How much more likely the adversary is to have a team out for you in this block today, from what it has learned
 * of your habits: nothing at first, up to +60% for a weekday and hour you use for most of your acts.
 */
float Sim::habitLift(int b) const {
    float total = 0;
    for (auto& row : g.habit) for (float x : row) total += x;
    if (total < 2) return 0;
    float share = g.habit[dateOf(g.day).dow][b] / total;
    float conf = std::min(1.f, total / 6.f);
    return clampf((share * 21 - 1) * 0.25f, 0, 0.6f) * conf;
}

void Sim::coverage(Actor& a, bool isYou) {
    bool prev = false;
    for (int b = 0; b < 3; b++) {
        float p = clampf(0.04f + a.heat / 100.f * 0.72f, 0, 0.85f) * g.a4 * (b == 0 ? 0.7f : 1.f) * (dayOff(g.day) ? 0.8f : 1.f);
        if (isYou) p = std::min(0.92f, p * (1 + habitLift(b)));        // they put the team out when they expect you to move
        bool c = (b > 0 && r().f() < 0.55f) ? prev : r().f() < p;
        a.covered[b] = c; prev = c;
        a.teamQ[b] = (int)clampf(40 + a.heat * 0.4f + rnd(0, 15) + (isYou ? std::min(20.f, g.advAlert * 4) : 0), 30, 95);
    }
}

void Sim::dawn() {
    actAs(nullptr);
    subj = nullptr;
    Date d = dateOf(g.day);
    if (g.day > g.postEnd && training()) { endTraining(false); return; }
    if (g.day > g.postEnd) {
        endPosting(END_TOUR, "END OF POSTING", "Your tour in " + std::string(T().city) + " is complete. You hand over your agents, your sites and your safe to your successor and fly home by Aeroflot.");
        return;
    }
    replate();
    weather(false);
    // --- hidden surveillance picture for today
    g.a4 = clampf(g.a4 + r().gauss() * 0.12f + (g.a4Base - g.a4) * 0.2f, 0.3f, 2.4f);
    float dTotal = 0;
    for (float x : g.habitD) dTotal += x;
    for (int i = 0; i < 8; i++) {
        float lift = dTotal >= 3 ? clampf((g.habitD[i] / dTotal * 8 - 1) * 0.08f, 0, 0.3f) * std::min(1.f, dTotal / 8) : 0;   // a team waiting where you usually go
        g.hot[i] = r().f() < (T().east[i] && g.wallDown ? 0.12f : T().density[i] * g.distMod[i] * g.a4 + lift);
    }
    coverage(g.you, true);
    for (auto& o : g.officers) coverage(o.a, false);
    // --- things that arrive this morning
    gradeLines.clear();
    std::vector<Pending> due, rest;
    for (auto& x : g.pending) (x.day <= g.day ? due : rest).push_back(x);
    g.pending = rest;
    for (auto& x : due) { if (g.phase != PH_PLAY) break; arrive(x); }
    subj = nullptr;
    if (g.phase != PH_PLAY) return;
    if (!gradeLines.empty()) {
        std::string sb = "Evaluation of material received from your rezidentura:\n\n";
        for (auto& s : gradeLines) sb += "  " + s + "\n";
        cable("Centre", fstr("Evaluation of material (%d)", (int)gradeLines.size()), sb + "\nCENTRE");
    }
    if (training()) { tutorialDawn(); return; }
    if (d.d == 1 && g.day > g.postStart) monthly();
    history(d);
    if (g.phase != PH_PLAY) return;
    officeHistory(d);
    leaveDawn();
    planDawn(d);
    if (g.phase != PH_PLAY) return;
    requirements();
    opsDawn();
    if (g.phase != PH_PLAY) return;
    if (pct(T().walkIn) && g.day - g.postStart > 20) walkIn();
    if (g.day - g.postStart > 60 && pct(0.10f)) krTip();
    // --- somebody takes an interest in you personally
    if (g.dirt >= 30 && g.day - g.lastApproach > 200 && g.day > g.busyUntil && pct(1.2f)) {
        g.lastApproach = g.day;
        addDecision(DK_APPROACH, 1, "An approach, with photographs",
            std::string("The man was waiting by your car. He was polite and did not waste your time. He had an envelope: ")
            + (g.skimmed > 0 || g.skim > 0 ? "copies of receipts in your agents' hands, and beside each a figure in another hand - what the agent says he was really given. " : "photographs. You are in all of them. ")
            + "\n\n'Nobody at your Embassy need ever see these. We ask very little: now and then, a conversation.' He lunches on Thursdays at a place he named.\n\n"
              "He is right about one thing: what is in the envelope would finish you at home.",
            {"Go to Line KR this morning and tell them everything, including what is in the envelope",
             "Say nothing and do nothing. Perhaps they are bluffing",
             "Give him something small, once, to keep the envelope where it is"});
    } else if (T().slot > 0 && g.day - g.lastApproach > 240 && g.day - g.postStart > 90 && g.day > g.busyUntil && pct((0.10f + g.you.heat * 0.004f) * (g.wifeHere ? 1.f : 1.5f))) {
        g.lastApproach = g.day;
        addDecision(DK_APPROACH, 0, "An approach",
            std::string("At a reception last night a man you know slightly - a commercial counsellor, he says, though nobody has seen him do any commerce - took you aside. ")
            + "He knows your name and patronymic, your daughter's school, and the date your tour ends. He said that men of talent are not always valued at home, "
              "that he has friends who value them, and that he lunches on Thursdays at a place he named. Then he talked about fishing.\n\n"
              "It is a pitch, and a competent one. The rules say you report it to Line KR within twenty-four hours.",
            {"Report it to Line KR this morning (some hours of statements; you will be watched by your own side for a while)",
             "Say nothing. Nothing happened, and a report goes on your file for ever",
             "Report it, and propose to Line KR that you play along with him (an operational game)"});
    }
    events();
    if (g.game == 2) gameRound();
    if (g.rank < std::min(3, T().slot + 1) && g.standCentre >= 72 && g.day - g.postStart > 365) {
        g.rank++;
        cable("Centre", "Promotion", std::string("By order of the Chairman of the Committee you are promoted to the rank of ") + RANK[g.rank] + ".\n\nCENTRE");
    }
    g.standRez = clampf(g.standRez, 0, 100); g.standCentre = clampf(g.standCentre, 0, 100);
    // --- the morning brief
    std::string sb = "MORNING BRIEF - " + fmtDay(g.day) + "\n\n";
    sb += std::string("ZENIT (intercept of the ") + T().watchers + " radio net):\n";
    if (pct(5)) { sb += "  No summary today - receiver fault.\n"; g.zenit[g.day] = "no summary"; }
    else {
        float seen = g.a4 + r().gauss() * 0.12f;
        std::string traffic = seen < 0.68f ? "LIGHT" : (seen < 1.0f ? "NORMAL" : (seen < 1.4f ? "HEAVY" : "VERY HEAVY"));
        sb += "  Traffic: " + traffic + ".\n";
        std::string h;
        for (int i = 0; i < 8; i++) if ((g.hot[i] && pct(70)) || (!g.hot[i] && pct(7))) h += std::string(h.empty() ? "" : ", ") + T().district[i];
        sb += "  Teams working: " + (h.empty() ? std::string("no district identified") : h) + ".\n";
        g.zenit[g.day] = traffic + (h.empty() ? "" : "; " + h);
    }
    if (d.dow == 0) {
        float est = g.you.heat + r().gauss() * 9;
        g.krEst = est;
        std::string kr = est < 22 ? "No sign that the adversary takes a special interest in you."
                         : est < 45 ? "Periodic coverage. You are probably carded as a suspected intelligence officer."
                         : est < 68 ? "Regular coverage. Assume you are identified as an officer of the service."
                         : "You are a priority target. KR advises that you stand down from all operational acts.";
        sb += "\nLINE KR weekly assessment of your position:\n  " + kr + "\n";
        g.krLog.push_back(pad(fmtShort(g.day), 11) + kr);
        // what Line KR can see as well as you can: your own log. A routine the station can read, the other side can read too.
        Pattern pt = myPattern(60);
        if (pt.n >= 4 && pt.slotN * 100 >= pt.n * 40) {
            std::string blk = BLOCK_LABEL[pt.slotB];
            for (auto& ch : blk) ch = (char)tolower((unsigned char)ch);
            std::string t = fstr("%d of your last %d operational acts were on ", pt.slotN, pt.n) + DOW[pt.slotDow] + " " + blk + "s. A routine the adversary can learn is one it will use.";
            sb += "  " + t + "\n";
            g.krLog.push_back(pad("", 11) + t);
        }
        if (pt.n >= 5 && pt.topDistN * 100 >= pt.n * 50) {
            std::string t = fstr("%d of them in ", pt.topDistN) + std::string(T().district[pt.topDist]) + ". Teams wait where officers go.";
            sb += "  " + t + "\n";
        }
        if (g.advAlert > 2.5f && pct(65)) sb += "  Zenit suggests new call-signs on the net: the adversary may have put fresh teams on the station.\n";
        // and, if you have officers, a line on each of them
        for (auto& o : g.officers) if (o.role == ROLE_SUB && !o.gone) {
            float e = o.a.heat + r().gauss() * 11;
            sb += "  " + o.title() + ": " + (e < 22 ? "not noticed." : e < 45 ? "periodic coverage." : e < 68 ? "regular coverage." : "priority target - stand him down.") + "\n";
            if (e >= 68) o.waryUntil = std::max(o.waryUntil, g.day + 14);        // he reads the same assessment, and lies low
        }
    }
    sb += "\nWEATHER: " + weatherLine() + "\n  Forecast for tomorrow: " + g.forecast + ".\n";
    sb += "\nTODAY:\n";
    bool any = false;
    std::string hol = holiday(g.day);
    if (!hol.empty()) { sb += "  " + hol + ": a public holiday. Offices are closed and your agents are at home.\n"; any = true; }
    if (d.m == 11 && d.d == 7) { sb += "  The Embassy's reception for the anniversary of the Revolution is this evening. Every officer attends.\n"; any = true; }
    if (g.day <= g.busyUntil) { sb += "  You are taken up today: " + g.busyWhy + ".\n"; any = true; }
    if (g.day <= g.standDownUntil) { sb += fstr("  Line KR's stand-down is in force until %s", fmtShort(g.standDownUntil).c_str()) + (meetsAllowed(g) ? " (agents' agreed meetings are kept)" : "") + ".\n"; any = true; }
    if (strikeToday(g)) { sb += "  Transport strike: no buses or trains.\n"; any = true; }
    if (d.dow == 3) { sb += "  Diplomatic pouch closes tonight.\n"; any = true; }
    auto cs = g.csDays.find(g.day);
    if (cs != g.csDays.end() && cs->second.state == 2) {
        Officer* a = officer(cs->second.o1); Officer* b = officer(cs->second.o2);
        sb += "  Countersurveillance team at your disposal: " + (a ? a->title() : std::string("?")) + ", " + (b ? b->title() : std::string("?")) + ".\n"; any = true;
    }
    for (auto& p : g.persons) if (p.stage == AGENT && p.nextMeet == g.day) {
        Site* s = agreedSite(p);
        sb += "  Agreed meeting with " + p.codename + " this evening at " + (s ? s->name : std::string("?")) + (p.handler ? " (" + handlerName(p) + ")" : std::string("")) + ".\n"; any = true;
    }
    for (auto& q : g.reqs) if (!q.closed && q.due >= 0 && q.due - g.day <= 14 && q.due >= g.day) { sb += fstr("  Requirement No. %d falls due in %d days.\n", q.id, q.due - g.day); any = true; }
    if (g.paperwork > 8) { sb += fstr("  Reports in arrears: %d hours.\n", (int)std::lround(g.paperwork)); any = true; }
    if (g.fatigue > 70) { sb += "  You are over-tired.\n"; any = true; }
    if (!any) sb += "  Nothing in the diary.\n";
    g.brief = sb;
}

void Sim::arrive(const Pending& x) {
    Person* p = person(x.ref);
    subj = p;
    switch (x.kind) {
        case P_TRACE: {
            if (!p) break;
            p->traceState = 2;
            bool flag = controlled(*p) ? pct(T().friends ? 55 : 35) : pct(4);
            learn(*p, A_ACCESS, 35);
            p->traceBad = flag;
            file(*p, flag ? "TRACE: CAUTION - name connected with the adversary's special services." : "TRACE: nothing adverse. Position confirmed.");
            cable("Centre", "Trace: " + p->name,
                  "Reference your request for traces on " + p->name + ", " + p->job + ", " + p->employer + ".\n\n"
                      + (flag ? "CAUTION. The name appears in the records in connection with the adversary's special services (source and reliability not stated). Proceed only with the greatest care and report every contact."
                              : "No compromising information in the records of Centre or of the friends. His position is confirmed as described.") + "\n\nCENTRE");
            break;
        }
        case P_SANCTION: {
            if (!p || !p->live() || p->stage == AGENT) break;
            float conf = 0;
            for (int i = 0; i < A_INFL; i++) conf += p->estC[i];
            conf /= A_INFL;
            float score = conf + (p->traceState == 2 ? 15 : 0) + g.standCentre * 0.2f + rnd(-10, 10);
            if (p->traceBad) {
                p->sanction = 3;
                file(*p, "Sanction REFUSED in view of the trace.");
                cable("Centre", "Recruitment proposal: " + p->name, "In view of the results of the trace, sanction for the recruitment of " + p->name + " is REFUSED. Reduce contact.\n\nCENTRE");
            } else if (score >= 52 && p->estV[A_ACCESS] >= 30) {
                p->sanction = 2;
                file(*p, "Centre SANCTIONS recruitment.");
                cable("Centre", "Recruitment proposal: " + p->name, "Centre SANCTIONS the recruitment of " + p->name + ". Choose the basis of the approach according to your assessment; report the result by cable.\n\nCENTRE");
            } else {
                p->sanction = 3;
                file(*p, "Sanction refused: insufficiently studied.");
                cable("Centre", "Recruitment proposal: " + p->name, std::string("The candidate is INSUFFICIENTLY STUDIED") + (p->traceState != 2 ? " and has not been traced" : "")
                      + ". Sanction is withheld. Continue development, complete the assessment of motives and reliability, and resubmit.\n\nCENTRE");
            }
            break;
        }
        case P_FUNDS: {
            bool ok = pct(30 + g.standCentre * 0.65f - (g.day - g.lastFundsReq < 60 ? 30 : 0));
            g.lastFundsReq = g.day;
            if (ok) { money(x.amt, "Supplementary advance from Centre"); cable("Centre", "Supplementary funds", fstr("A supplementary advance of %d %s is approved. Account for it.\n\nCENTRE", x.amt, T().currency)); }
            else { g.standCentre -= 2; cable("Centre", "Supplementary funds", "Your request is not approved. Operational expenditure must be justified by results.\n\nCENTRE"); }
            break;
        }
        case P_FILM: g.film += x.amt; cable("Technical officer", "Stores issued", fstr("%d Minox cassettes are in your safe.", x.amt)); break;
        case P_CAMERA: g.cameras += x.amt; cable("Technical officer", "Stores issued", "One Minox camera, tested, is in your safe."); break;
        case P_CONTAINER: g.containers += x.amt; cable("Technical officer", "Stores issued", fstr("%d concealment containers (hollow bolts and a magnetic box) are in your safe.", x.amt)); break;
        case P_BUG:
            subj = nullptr;
            g.bugs += x.amt;
            cable("Technical officer", "Stores issued", "One listening device with transmitter is in your safe, tested. The listening post is arranged. The device must be placed inside the target office by somebody who works there; "
                  "the battery will last two to four months.");
            break;
        case P_EXFIL: {
            if (!p || p->stage != AGENT) break;
            int graded = 0;
            for (auto& rl : g.rolls) if (rl.agentId == p->id && rl.state == R_GRADED) graded++;
            bool ok = p->service != SVC_NONE || graded >= 4 || pct(g.standCentre);
            if (ok) {
                p->exfil = 2;
                file(*p, "Centre APPROVES exfiltration. Passport and route in the safe.");
                cable("Centre", "Exfiltration of " + p->codename, "Centre APPROVES. A passport in another name, money and the route are in your safe. Hand them to him at a personal meeting (tick 'travel documents' on the order) and make him repeat the route. "
                      "He is to leave the same night and take nothing with him. If he is already under close observation he may not reach the frontier.\n\nCENTRE");
            } else {
                p->exfil = 0;
                file(*p, "Centre REFUSES exfiltration.");
                cable("Centre", "Exfiltration of " + p->codename, "The source is not of a value to justify the expense and the risk to the route. The request is REFUSED. Break contact in good order.\n\nCENTRE");
            }
            break;
        }
        case P_PADS: g.pads += x.amt; cable("Cipher clerk", "Stores issued", fstr("%d one-time pad sheets with broadcast schedules are in your safe.", x.amt)); break;
        case P_RESUPPLY:
            g.stationFilm += 40; g.stationCameras += 1; g.stationPads += 6; g.resupplyOrdered = false;
            cable("Technical officer", "Station stores", "Film, a camera and cipher pads have arrived from Centre by pouch. Requisitions can be met again.");
            break;
        case P_POUCH_ARRIVE: {
            Roll* rl = roll(x.ref);
            if (rl && rl->state == R_POUCH) { rl->state = R_CENTRE; rl->stateDay = g.day; pend(g.day + 6 + r().n(9), P_GRADE, rl->id, 0); }
            break;
        }
        case P_GRADE: { Roll* rl = roll(x.ref); if (rl && rl->state != R_GRADED) grade(*rl); break; }
        case P_RETURN:
            if (p && p->live()) {
                p->rapport = std::max(p->rapport, 40.f);
                file(*p, "Telephoned the Embassy from a call-box: he has 'thought it over' and would like to meet again.");
                alert(p->name + " has telephoned. He has thought over the proposal and would like to meet again.");
            }
            break;
        case P_RADIO: {
            if (!p || p->stage != AGENT || !p->radio) break;
            bool heard = p->pads > 0 && (controlled(*p) || pct(p->diligence));
            if (heard) {
                p->pads--;
                if (x.amt >= 0) p->tempo = x.amt;
                if (x.p2 > -2) p->reqId = x.p2;
            }
            if (x.p3 && !p->meetSites.empty()) {
                // you will go to the meeting place either way; whether he comes depends on whether he was listening
                p->misses = 0; p->nextMeet = g.day + 3; p->callout = true;
                schedule(*p);
            }
            Site* ag = agreedSite(*p);
            cable("Centre", "Broadcast made: " + p->codename, "The groups for " + p->codename + " were transmitted on his schedule last night and repeated."
                  + (x.p3 ? " They summon him to " + (ag ? ag->name : std::string("his meeting site")) + " on the evening of " + fmtDay(p->nextMeet) + "." : std::string(""))
                  + " Centre has no means of knowing whether he was listening.\n\nCENTRE");
            break;
        }
        case P_KR: {
            g.krPending = false;
            Officer* o = officer(x.ref);
            subj = nullptr;
            if (!o || o->gone) { cable("Directorate K", "Your report", "The officer concerned is no longer at the station. The matter is closed.\n\nDIRECTORATE K"); break; }
            bool guilty = (o->mole && o->moleDay <= g.day) || (o->defectDay >= 0 && o->defectDay - g.day < 200);
            if (guilty && pct(75)) {
                std::string known;
                for (auto& q : g.persons) if (q.handler == o->id || has(q.witting, o->id)) {
                    known += "  " + q.full() + "\n";
                    q.file.push_back(pad(fmtShort(g.day), 11) + "KNOWN TO THE ADVERSARY through the traitor " + o->name + ". Everything since he learned of this case is suspect.");
                }
                o->gone = true;
                o->file.push_back(pad(fmtShort(g.day), 11) + "RECALLED AND ARRESTED. Directorate K confirms he worked for the adversary.");
                for (auto& q : g.persons) if (q.live() && q.handler == o->id) { q.handler = 0; q.pitchAuth = false; if (q.stage == AGENT) schedule(q); }
                g.standCentre = clampf(g.standCentre + 14, 0, 100); g.standRez = clampf(g.standRez + 10, 0, 100);
                g.you.heat = std::max(8.f, g.you.heat - 8);
                g.awards.push_back(fstr("%d  Commended by the Chairman for the exposure of a traitor (", dateOf(g.day).y) + T().city + ")");
                cable("Directorate K", "Your report on " + o->title(),
                      "Your report is CONFIRMED. " + o->title() + " was recalled on a pretext and has admitted working for the adversary.\n\n"
                      "He names the following as known to him and passed on:\n" + (known.empty() ? "  (nothing of yours)\n" : known)
                      + "\nTreat every one of them as controlled or compromised until proved otherwise. The Chairman has been informed of your part in this.\n\nDIRECTORATE K");
            } else if (guilty) {
                o->cautious = true;
                g.standRez = clampf(g.standRez - 4, 0, 100);
                cable("Directorate K", "Your report on " + o->title(), "Inquiries have produced no grounds for action against " + o->title() + ". The file remains open. You are reminded that unfounded suspicion damages a collective.\n\nDIRECTORATE K");
            } else if (pct(80)) {
                g.standRez = clampf(g.standRez - 9, 0, 100); g.standCentre = clampf(g.standCentre - 3, 0, 100);
                o->diligence = std::max(20.f, o->diligence - 15);
                o->file.push_back(pad(fmtShort(g.day), 11) + "Cleared by Directorate K. He knows who reported him.");
                cable("Directorate K", "Your report on " + o->title(), "Inquiries show NO GROUNDS for suspicion of " + o->title() + ". His record is clean. He has learned of the inquiry, as these things are learned.\n\nDIRECTORATE K");
            } else {
                o->gone = true;
                o->file.push_back(pad(fmtShort(g.day), 11) + "Recalled as a precaution. Nothing proved.");
                for (auto& q : g.persons) if (q.live() && q.handler == o->id) { q.handler = 0; q.pitchAuth = false; if (q.stage == AGENT) schedule(q); }
                g.standRez = clampf(g.standRez - 4, 0, 100);
                cable("Directorate K", "Your report on " + o->title(), "Nothing has been proved against " + o->title() + ", but in view of certain irregularities he is recalled as a precaution. You will manage without him.\n\nDIRECTORATE K");
            }
            break;
        }
        case P_SECRET: {
            subj = nullptr;
            if (x.amt == 1 || x.amt == 2) {             // the fate of a Line N package
                Requirement* q = req(x.ref);
                if (!q) break;
                if (x.amt == 1) {
                    g.totReqDone++;
                    g.standCentre = clampf(g.standCentre + 6, 0, 100); g.standRez = clampf(g.standRez + 3, 0, 100);
                    cable("Centre", fstr("Line N task No. %d", q->id), "The package was collected. Directorate S thanks the rezidentura.\n\nCENTRE");
                } else {
                    q->failed = true;
                    g.standCentre = clampf(g.standCentre - 15, 0, 100); g.standRez = clampf(g.standRez - 8, 0, 100);
                    cable("Centre", fstr("Line N task No. %d", q->id), "The comrade for whom the package was intended reported hostile observation at the place and has broken off. "
                          "An officer of Directorate S has been lost to the service for some years as a result. A written explanation is required: how the drop was chosen, who knew of it, and what you saw on the way there.\n\nCENTRE");
                }
            } else if (x.amt == 4) {                    // travel documents given to a man who was theirs
                Person* q = person(x.ref);
                g.standCentre = clampf(g.standCentre - 4, 0, 100);
                cable("Centre", "The documents issued for " + (q ? q->codename : std::string("your agent")),
                      "The passport and route prepared for " + (q ? q->codename : std::string("your agent")) + " were displayed yesterday at a press conference by the adversary's security service, with your photograph. "
                      "The route is lost to the service. The man was theirs. You will explain when you first had grounds to doubt him.\n\nCENTRE");
                if (q) q->file.push_back(pad(fmtShort(g.day), 11) + "The exfiltration documents were shown to the press by the adversary. HE WAS UNDER THEIR CONTROL.");
            } else if (x.amt == 5) {                    // what you gave the other side to keep them quiet: a source of ours inside their service has reported it
                summons();
                break;
            } else if (x.amt == 6) {
                finishCareer("LEFORTOVO",
                             "A source of Directorate K inside the adversary's service reported that an officer of the " + std::string(T().city) + " rezidentura had been passing them information under pressure. "
                             "There were not many officers it could be. You were asked to come to Moscow for consultations, and were met at the foot of the aircraft steps by two men who took your arms.\n\n"
                             "The investigation took four months and the trial one day, in closed session. Your name was removed from the lists of the service, and your photograph from the corridor of the department.");
                return;
            } else if (x.amt == 3) {                    // an approach you did not report
                g.standRez = clampf(g.standRez - 15, 0, 100); g.standCentre = clampf(g.standCentre - 8, 0, 100);
                g.paperwork += 6;
                cable("Line KR", "A matter you omitted to report", "It has come to the knowledge of this Line, by another channel, that an officer of the adversary made an approach to you some time ago. "
                      "There is no record of a report from you. You will write one now, in full, and explain the delay.\n\nLINE KR");
            }
            break;
        }
        case P_FLAT: {
            subj = nullptr;
            int d = x.ref;
            Site f = makeSite(ST_MEET, d);
            f.name = std::string("Safe flat, ") + T().street[d][r().n(3)] + " [" + T().district[d] + "]";
            f.flat = true; f.rent = x.amt;
            f.cover = rnd(85, 95); f.sight = rnd(35, 60);
            f.casing = 3;
            casingEstimate(f);
            g.sites.push_back(f);
            cable("Support officer", "The flat", f.name + " is ready. The lease is in the name of the cut-out, who believes it is for a married man's afternoons, and does not mind. "
                  + fstr("Rent: %d %s a month, paid through him. ", x.amt, T().currency)
                  + "Keys in your safe. Meet there only those who already know what you are; every visitor is seen by somebody on the stairs.");
            break;
        }
        case P_VERIFY:
            if (!p || p->stage != AGENT || p->verify != 1) break;
            p->verify = 2; p->verifyLeft = 2;
            file(*p, "Centre's control questions received. His next two deliveries will be checked against them.");
            cable("Centre", "Verification of " + p->codename, "Control questions for " + p->codename + " are in your safe. Task him against them at his next meeting, as if they were ordinary requirements. "
                  "Centre will check his next two deliveries and report.\n\nCENTRE");
            break;
        case P_GAME: subj = nullptr; gameSanction(); break;
        case P_KOMP: if (p) kompSanction(*p); break;
        case P_SKIM:
            subj = nullptr;
            skimExposed(x.amt == 1 ? "An agent told his new case officer what he had really been paid, and the officer did his duty and reported it."
                                   : "The auditors compared the agents' receipts with what the agents themselves say they received.");
            break;
        case P_WALKIN: subj = nullptr; walkIn(); break;
        case P_PLAY: if (p) playSanction(*p); break;
        case P_CONFID: if (p) confidRegistered(*p); break;
        case P_LETTER: subj = nullptr; letterAnswer(x.ref); break;
        case P_ESCAPE: if (p) escapePickup(*p); break;
        case P_KOBA:
            if (p && p->live() && !controlled(*p)) {
                arrest(*p, "ARRESTED. Charged with attempting to communicate secrets to a foreign power.");
                subj = nullptr;
                cable("Line KR", "The man who wrote the letter", "The newspapers have the arrest of " + p->name + " this morning. He had been under observation, they say, 'for some weeks' - that is, from about the time his letter reached us.\n\n"
                      "He told nobody. We told nobody outside this station. Somebody in this station told them.\n\nLINE KR");
            }
            break;
        case P_LOSTOFFER:
            subj = nullptr;
            g.standCentre = clampf(g.standCentre - 7, 0, 100);
            cable("Centre", "The letter you did not answer", "An officer of one of the adversary's secret departments was convicted yesterday of attempting to pass documents to this Embassy. In court it was said that he pushed three letters through a Soviet letter-box, "
                  "and that 'the Russians never answered'. Centre has read the evidence with attention. The material he was offering would have been the most valuable the station had produced in ten years.\n\nCENTRE");
            break;
        case P_REFERRAL: subj = nullptr; referralArrives(x.ref); break;
        case P_LINEN_PKG:
            subj = nullptr;
            g.lineNPkgs++;
            cable("Centre", "Line N package", "A sealed package for Directorate S arrived in last night's pouch and is in your safe. You are not to open it. It is to be placed as instructed.\n\nCENTRE");
            break;
        default: break;
    }
    subj = nullptr;
}

void Sim::monthly() {
    int sal = 0, subs = 0;
    for (auto& p : g.persons) {
        p.paid90 = (int)std::lround(p.paid90 * 0.67f);
        if (p.stage == AGENT) { sal += p.salary; p.owed += p.salary; }
    }
    for (auto& o : g.officers) if (o.role == ROLE_SUB && !o.gone) subs++;
    int basic = (int)std::lround(P(60) * (1 + 0.25f * subs));
    int allot = basic + (int)std::lround(sal * 0.8f);
    money(allot, "Monthly operational advance");
    if (!onLeave(g.day)) g.paperwork += 3 + T().role;
    const Officer* rz = rezident();
    int tp = rz && T().role < 3 ? rz->temper : 0;
    float par = 26 * (1 + 0.3f * T().role) * (tp == 1 ? 1.15f : tp == 2 ? 0.85f : 1.f);
    int away = 0;
    for (int dd = g.day - 30; dd < g.day; dd++) if (onLeave(dd)) away++;
    par *= 1 - std::min(30, away) / 30.f;                    // a month on leave is not held against you
    float score = g.mSpotted * 3 + std::min(g.mMeetings, 8 + 4 * T().role) * 1.5f + g.mRolls * 2 + std::min(g.mValue / 12.f, 15.f * (1 + 0.5f * T().role))
                  - (g.overdueDays > 0 ? (tp == 4 ? 16 : tp == 3 ? 0 : 8) : 0) - (g.cover < 40 ? (tp == 4 ? 16 : 8) : 0);
    float delta = clampf((score - par) / 3.f, -3, 5);
    if (tp == 1) delta *= 1.5f;                              // a careerist: loud praise, long memory
    if (tp == 3) delta *= 0.6f;                              // an old hand: little moves him
    if (tp == 2 && g.you.heat > 45) delta -= 2;              // cautious: the watchers' interest in you worries him
    // yesterday's successes are forgotten, and so in time are yesterday's failures: both standings drift back towards the ordinary
    g.standRez = clampf(g.standRez + delta + (g.standRez > 50 ? -1.5f : (g.standRez < 35 ? 1.5f : 0.5f)), 0, 100);
    g.standCentre = clampf(g.standCentre + (g.standCentre > 45 ? -1.5f : 0.5f), 0, 100);
    // RYAN: what you report, and what it costs
    std::string ryan;
    if (g.ryanPolicy == 0) { g.standCentre = clampf(g.standCentre - 1.5f, 0, 100); ryan = "RYAN return submitted: no indicators observed. Centre notes 'insufficient attention to the task'.\n"; }
    else if (g.ryanPolicy == 1) { g.standCentre = clampf(g.standCentre + 2, 0, 100); g.paperwork += 3; ryan = "RYAN return submitted: nine indicators reported with commentary (three hours). Centre notes the rezidentura's vigilance and asks for more detail next month.\n"; }
    std::string verdict = delta >= 3 ? "Good work. Keep to this pace." : delta >= 0 ? "Satisfactory." : delta > -3 ? "Below what is expected of this post."
                          : "Unsatisfactory. Results are what is asked of you, not explanations.";
    static const char* const GOOD[] = {"", " The station's figures went to the Department with your name at the top.", "", " Carry on.", ""};
    static const char* const BAD[] = {"", " The Department reads these figures too. So do I.", "", " It happens. Next month.", " The forms exist for a reason."};
    verdict += delta >= 3 ? GOOD[tp] : delta < 0 ? BAD[tp] : "";
    if (tp == 2 && g.you.heat > 45) verdict += " And you are being followed too often. I want fewer risks taken, not more.";
    if (tp == 3 && g.overdueDays > 0) verdict += " Your reports are late; nobody will die of it.";
    Date prev = dateOf(g.day - 1);
    cable(T().chief, std::string("Monthly review - ") + MONTH[prev.m],
          fstr("New contacts: %d. Meetings held: %d. Acts aborted: %d. Material graded: %d.\n", g.mSpotted, g.mMeetings, g.mAborts, g.mRolls)
              + (g.overdueDays > 0 ? "Reports are in arrears.\n" : "") + (g.cover < 40 ? "The Ambassador's office complains that you neglect your cover duties. That endangers more than yourself.\n" : "") + ryan
              + "\n" + verdict + fstr("\n\nOperational advance for the month: %d %s (basic %d, plus four-fifths of agreed agent remuneration). Monthly accounts are due: %d hours.\n\n", allot, T().currency, basic, 3 + T().role)
              + upper(T().chief));
    int rent = 0;
    for (auto& s : g.sites) if (s.flat && !s.retired) rent += s.rent;
    if (rent > 0) {
        int paid = std::min(rent, g.funds);
        money(-paid, "Safe flats - the month's rent, through the cut-out");
        if (paid < rent) {
            for (auto& s : g.sites) if (s.flat && !s.retired) s.neighbours += 6;
            alert("The rent of the safe flats was paid late: the landlord came round himself to ask for it.");
        }
    }
    officeMonthly();
    if (g.phase != PH_PLAY) return;
    monthRecord();
    krVehicles();
    cityChanges();
    g.mSpotted = g.mMeetings = g.mAborts = g.mRolls = 0; g.mValue = 0;
}

void Sim::requirements() {
    for (auto& q : g.reqs) {
        if (!q.closed && q.due >= 0 && g.day > q.due) {
            q.closed = true; q.failed = true;
            g.standCentre -= q.kind == RK_LINEN ? 10 : 6;
            cable("Centre", fstr("Requirement No. %d lapsed", q.id),
                  q.kind == RK_LINEN ? "The Line N task was not carried out within the time set. Directorate S has had to make other arrangements at short notice. Centre notes the failure.\n\nCENTRE"
                  : q.kind == RK_ACTIVE ? "Service A's material on '" + q.title + "' was not placed within the time set. The occasion has passed. Centre notes the failure.\n\nCENTRE"
                  : "No adequate material was received on '" + q.title + "' within the time set. The information has lost its value. Centre notes the failure.\n\nCENTRE");
        }
    }
    int open = 0;
    for (auto& q : g.reqs) if (!q.closed && q.kind != RK_LINEN) open++;
    if (open < 5 + T().role && g.day - g.lastReqIssue >= 12) {
        if (Requirement* q = nextRequirement()) {
            g.lastReqIssue = g.day;
            cable("Centre", fstr("New requirement No. %d: ", q->id) + q->title, q->text + "\n\n" + (q->due >= 0 ? "Required by " + fmtDay(q->due) + "." : std::string("Standing requirement.")) + "\n\nCENTRE");
        }
    }
    // Line N: an illegal somewhere in the country needs a package
    if (g.day - g.lastLineN > 170 && g.day - g.postStart > 75 && g.postEnd - g.day > 100 && pct(4)) {
        g.lastLineN = g.day;
        Requirement q;
        q.id = g.nextReqNo++; q.kind = RK_LINEN; q.sector = SECURITY; q.issued = g.day; q.due = g.day + 45; q.priority = 1; q.need = 1;
        q.district = 4 + r().n(4);
        q.title = std::string("Line N: package for ") + T().district[q.district];
        q.text = std::string("LINE N TASK. A package for an officer of Directorate S will reach you by the next pouch. It is to be placed in a dead drop in ") + T().district[q.district]
                 + " which has NEVER been used, which you have cased at least twice, and which is known to as few people as possible. "
                   "You will never learn who collects it. If the place is under observation when he comes, you will learn that.";
        g.reqs.push_back(q);
        pend(g.day + 5 + r().n(6), P_LINEN_PKG, 0, 0);
        cable("Centre", fstr("New requirement No. %d: ", q.id) + q.title, q.text + "\n\nRequired by " + fmtDay(q.due) + ".\n\nCENTRE");
    }
}

void Sim::walkIn() {
    bool spy = !T().spies.empty() && pct(15);          // the best penetrations in the history of the service walked in
    if (spy && g.decisions.empty() && pct(75)) { walkInLetter(); return; }
    Person np = spy ? makeSpy(0.5f) : makePerson(r().n(4), 0.5f);
    np.walkIn = true;
    np.rapport = 30; np.commitment = 35; np.stage = DEVELOPING;
    if (!np.dangle) np.tru[A_MONEY] = np.app[A_MONEY] = rnd(65, 92);
    g.persons.push_back(np);
    Person& p = g.persons.back();
    subj = &p;
    g.totSpotted++; g.mSpotted++;
    file(p, "WALK-IN. Pushed a letter through the door of the Consulate offering 'documents of interest' for payment.");
    cable(T().role == 3 ? "Duty officer" : "Rezident", "A volunteer",
          "A letter was pushed through the Consulate door last night. The writer gives his name as " + p.name + ", " + p.job + ", " + p.employer
              + ", and offers documents for money." + (spy ? " If he is what he says, he is an officer of a foreign service offering himself." : "") + "\n\nHalf of these are provocations and the other half are our best agents. He is yours to assess. Trace him before anything else is done.");
    subj = nullptr;
}

void Sim::history(const Date& d) {
    const Theatre& t = T();
    for (size_t i = 0; i < t.hist.size(); i++) {
        const HistT& h = t.hist[i];
        if (h.y != d.y || h.m != d.m || h.d != d.d || has(g.histDone, (int)i)) continue;
        g.histDone.push_back((int)i);
        g.a4Base += h.a4Base;
        g.a4 = clampf(g.a4 + h.a4Now, 0.3f, 2.4f);
        g.you.heat = clampf(g.you.heat + h.heat, 0, 100);
        std::string text = h.text;
        switch (h.code) {
            case H_DEFECTION: {
                Officer* who = nullptr;
                for (auto& o : g.officers) if (!o.gone && o.defectDay >= 0) who = &o;     // chosen when the posting began
                if (!who) {
                    // he was reported and recalled in time: the morning that should have been a disaster is an ordinary one
                    g.a4Base -= h.a4Base; g.you.heat = clampf(g.you.heat - h.heat, 0, 100);
                    text = "Centre informs the rezidentura that an officer formerly of this station, recalled on a colleague's report, has admitted that he was in touch with the adversary and had fixed a date to go over to them. "
                           "The date was to have been today.\n\nThe agents and the places he knew of are still ours.\n\nREZIDENT";
                    break;
                }
                std::string lost;
                for (auto& p : g.persons) if (p.live() && has(p.witting, who->id)) lost += "  " + p.full() + "\n";
                defection(*who);
                text = who->title() + " of this rezidentura has failed to report and must be presumed to be in the hands of the adversary.\n\n"
                       "Assume that your identity as an officer of the service is known. He knew what he had seen and what he had been told:\n"
                       + (lost.empty() ? "  (none of your people, so far as the record shows)\n" : lost)
                       + "\nOperational acts only with my personal sanction.\n\nREZIDENT";
                break;
            }
            case H_FOOT: {
                float chance = clampf(8 + g.you.heat * 0.9f, 10, 92);
                if (pct(chance)) {
                    endPosting(END_EXPELLED, "EXPELLED - 24 SEPTEMBER 1971",
                               "The British government has today expelled ninety Soviet officials and barred fifteen more from returning. Your name is on the list.\n\n"
                               "There is no disgrace in it: half the rezidentura is on the same aeroplane. Your agents pass to whoever is left.");
                    return;
                }
                for (auto& o : g.officers) if (o.role == ROLE_COLLEAGUE && !o.gone && pct(55)) o.gone = true;
                text = "The British have expelled ninety of our people and barred fifteen more. You are not on the list.\n\n"
                       "Understand what that means. The watchers who could not cover two hundred of us can now cover every one of those who remain, every day. "
                       "Put your agents on ice or run them with the greatest care.\n\nACTING REZIDENT";
                break;
            }
            case H_RYAN:
                text = "At the all-union conference the Chairman announced that the principal task of the service is now to detect preparations by the main adversary for a surprise nuclear missile attack. "
                       "The operation is called RYAN.\n\nEvery rezidentura in a NATO capital will report on a list of indicators every fortnight: lights at night in government buildings, movements of officials, "
                       "the price paid for donor blood, the slaughter of cattle. Reports are required whether or not anything has been observed.\n\nYour line will make the return for " + std::string(T().city) + ".\n\nCENTRE";
                addDecision(DK_RYAN, 0, "Operation RYAN - your line's returns",
                    "You have read the list of indicators. Half of them are things that happen in any Western capital every working day; the other half could not be observed by anyone without an agent in the President's bedroom. "
                    "The men who wrote it believe an attack is being prepared. Your sources say that nothing of the kind is in train.\n\nThe return goes in every month over your signature. Decide how your line will make it. You may be asked again.",
                    {"Report what is observed - which is nothing - and say so", "Report in the sense Centre expects: find the indicators and send them (three more hours a month)"});
                break;
            case H_ABLE:
                text = "FLASH. NATO forces have begun an exercise of nuclear release procedure with new message formats and radio silence. American bases are reported on alert. "
                       "Centre requires IMMEDIATE reporting of any indicator of preparation for a first strike. All officers will remain at their posts.\n\nCENTRE";
                if (g.ryanPolicy == 1) { g.paperwork += 10; g.fatigue = clampf(g.fatigue + 20, 0, 100); g.standCentre = clampf(g.standCentre + 3, 0, 100); }
                else { g.standCentre = clampf(g.standCentre - 5, 0, 100); g.paperwork += 3; }
                break;
            case H_WALL:
                text = "The GDR opened its frontier crossings to the West last night. People are sitting on the wall in Berlin. Nobody in this Embassy was told beforehand, and nobody at Centre appears to have been told either.\n\n"
                       "Sources who work for us out of conviction will be asking themselves what they are convinced of. See them.";
                for (auto& p : g.persons) if (p.live()) {
                    p.tru[A_IDEO] = clampf(p.tru[A_IDEO] - 12, 0, 100); p.app[A_IDEO] = clampf(p.app[A_IDEO] - 12, 0, 100);
                    if (p.stage == AGENT && !controlled(p) && p.tru[A_MONEY] < 45) { p.stress += 18; p.commitment = clampf(p.commitment - 15, 0, 100); }
                }
                break;
            case H_ORPHANS:
                text = "Demonstrators have occupied the headquarters of the Ministry for State Security in Berlin. Files are being carried out by the armful. "
                       "Assume that the friends' knowledge of our operations, and their own agent records, will reach the West German services in whole or in part.";
                addDecision(DK_ORPHANS, 0, "The friends' sources",
                    "A colonel of the friends' foreign service, a man you have dealt with for three years, asked to see you alone. His service has weeks to live. "
                    "He has three sources in " + std::string(g.theatre == TH_BONN ? "the Bonn ministries" : "the West Berlin Senate and the Allied missions") + " who have never been met by anyone but him, and he would rather give them to you than see them abandoned. He can introduce them once.\n\n"
                    "They are productive, trained and frightened. Their names are also in a card index in the Normannenstrasse, and nobody can tell you whether that index has been burned or sold.",
                    {"Take over all three", "Take over only the one with the best access", "Decline. Their files are not in your safe and never will be"});
                break;
            case H_UNIFY:
                g.wallDown = true;
                text = "The GDR ceased to exist at midnight. From today the security services of the Federal Republic have authority in what was its capital, and the friends' service is being wound up by the people it worked against. "
                       "Its officers are being questioned, and some of them are answering.\n\nKarlshorst is an island. Treat every district of the city as hostile ground, and every source ever known to the friends as named.";
                break;
            case H_JANATA: {
                bool forecast = false;
                for (auto& q : g.reqs) if (q.title == "The general election" && q.closed && !q.failed) forecast = true;
                g.standCentre = clampf(g.standCentre + (forecast ? 4.f : -6.f), 0, 100);
                text = std::string("The Prime Minister has lost the general election, and her own seat. Centre's forecast to the leadership, built on this rezidentura's reporting, was for a comfortable majority.\n\n")
                       + (forecast ? "Your own sources said otherwise and were not believed. The file shows it, and it has been noted where such things are noted."
                                   : "An explanation is required of why a rezidentura with this many agents in the governing party did not see it coming.") + "\n\nCENTRE";
                break;
            }
            default: break;
        }
        if (!text.empty()) cable(h.from, h.title, text);
    }
}
