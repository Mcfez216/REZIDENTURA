// sim_night.cpp - end of day: your officers' work, the night, the next dawn, Centre's answers, history.
#include "sim.h"

static std::string upper(std::string s) { for (auto& c : s) c = (char)toupper((unsigned char)c); return s; }
static bool has(const std::vector<int>& v, int x) { return std::find(v.begin(), v.end(), x) != v.end(); }

// ============================================================================ END OF DAY
void Sim::endDay() {
    if (g.phase != PH_PLAY || !g.decisions.empty()) return;
    rep = &g.reports[g.day];
    rep->clear();
    attempted.clear();
    say("OPERATIONS REPORT - " + upper(fmtDay(g.day)) + " - " + upper(T().city));
    say("");
    for (int b = 0; b < 3; b++) { sdrDone[b] = false; sdrRoute[b] = -1; }
    if (g.fatigue >= 92) {
        alert("You are ill with exhaustion and stayed in your flat all day. Today's orders were not carried out.");
        g.fatigue -= 30;
    } else {
        for (int b = 0; b < 3 && g.phase == PH_PLAY; b++) {
            Order o = order(g.day, b);
            say(upper(BLOCK_LABEL[b]) + " - " + OTYPE_LABEL[o.type]);
            resolve(o, b);
        }
    }
    if (g.phase == PH_PLAY) officersDay();
    if (g.phase == PH_PLAY) night();
    g.plan.erase(g.day);
    if (g.phase == PH_PLAY) { g.day++; dawn(); }
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
        // 3. work on one of his developmental contacts
        if (!acted && !o.gone && g.day - o.lastAct >= 2) {
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
        if (!acted && !o.gone && o.assign == AS_SPOT && (g.day % 7) == (o.id % 7) && pct(24 + o.nose * 0.2f)) {
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
    // --- you
    g.fatigue = std::max(0.f, g.fatigue - 10);
    if (!weekend(g.day)) g.cover = std::max(0.f, g.cover - 1.6f);
    for (int k = -1; k < (int)g.officers.size(); k++) {
        Actor& a = k < 0 ? g.you : g.officers[k].a;
        float floor = std::max(8.f, a.peakHeat * 0.3f);     // once carded you are never quite forgotten
        if (a.heat > floor) a.heat = std::max(floor, a.heat - 0.15f - 0.012f * (a.heat - floor));   // interest fades fastest when it is highest
    }
    if (g.cover < 20) g.you.heat += 0.6f; else if (g.cover < 40) g.you.heat += 0.3f;     // a "diplomat" who is never at his desk
    if (g.paperwork > 8) g.overdueDays++; else g.overdueDays = 0;
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
    if (g.standRez < 6)
        endPosting(END_RECALLED, "RECALLED TO MOSCOW", T().role == 3
            ? "The department has lost confidence in your direction of the rezidentura. You are recalled 'for consultations' and will not be returning."
            : "The Rezident has reported to Centre that you are unfit for operational work at this station. You are recalled 'for consultations' and will not be returning.");
}

void Sim::sendCable(const OutCable& c) {
    switch (c.kind) {
        case C_TRACE: pend(g.day + 6 + r().n(9), P_TRACE, c.ref, 0); break;
        case C_SANCTION: pend(g.day + 5 + r().n(8), P_SANCTION, c.ref, 0); break;
        case C_FUNDS: pend(g.day + 4 + r().n(6), P_FUNDS, 0, P(100)); break;
        case C_KR: pend(g.day + 14 + r().n(16), P_KR, c.ref, 0); break;
        case C_RADIO: pend(g.day + 2 + r().n(3), P_RADIO, c.ref, c.p1, c.p2, c.p3); break;
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
    bool work = !weekend(g.day);
    float before = p.suspicion;
    // --- an inherited source whose name is in somebody else's files
    if (p.exposeDay >= 0 && g.day >= p.exposeDay && !ctl) {
        p.exposeDay = -1;
        p.suspicion = 100;
        file(p, "Named in records that have reached the adversary.");
    }
    // --- collection
    if (p.tempo > 0 && work) {
        static const float mult[] = {0, 4, 10, 22};
        float access = ctl ? p.app[A_ACCESS] : p.tru[A_ACCESS];
        float pages = access / 100.f * mult[p.tempo] * rnd(0.5f, 1.5f);
        if (p.camera && p.film > 0) {
            Requirement* q = req(p.reqId);
            float match = q && !q->closed && q->kind == RK_COLLECT && q->sector == p.sector ? 1.f : 0.55f;
            float v = ctl ? rnd(26, 60) : (p.fabricator && pct(45) ? -1 : p.tru[A_ACCESS] * rnd(0.6f, 1.05f) * match);
            p.frameVal += pages * v;
            p.frames += (int)std::lround(pages);
            if (p.frames >= FRAMES) {
                float avg = p.frameVal / p.frames;
                Roll& x = newRoll(p);
                x.frames = FRAMES;
                if (avg < 0 || (p.fabricator && avg < p.tru[A_ACCESS] * 0.35f)) { x.genuine = false; avg = rnd(25, 55); }
                x.value = clampf(avg, 3, 100);
                x.state = R_AGENT;
                p.held.push_back(x.id);
                p.film--; p.frames = 0; p.frameVal = 0;
            }
        }
        if (!ctl) {
            float t = (float)p.tempo;
            p.suspicion += t * t * (p.empSec / 100.f) * (1 - p.craft / 150.f) * 0.11f + (100 - p.tru[A_DISC]) / 100.f * 0.03f * t;
            p.stress += t * 0.14f * (1.5f - p.tru[A_NERVE] / 100.f);
        }
    } else if (!ctl) {
        p.stress -= 0.3f;
        p.suspicion -= p.suspicion < 40 ? 0.15f : 0.4f;
    }
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
    // --- life
    if (!ctl && pct(0.25f)) {
        int k = r().n(4);
        if (k == 0) { p.tru[A_ACCESS] = p.app[A_ACCESS] = clampf(p.tru[A_ACCESS] + rnd(6, 15), 0, 97); p.warn.push_back("has been promoted and now sees a wider range of papers."); }
        else if (k == 1) { p.tru[A_ACCESS] = p.app[A_ACCESS] = clampf(p.tru[A_ACCESS] - rnd(10, 25), 5, 97); p.warn.push_back("has been moved to another section. His access is reduced."); }
        else if (k == 2) { p.stress += 15; p.warn.push_back("has trouble at home and is drinking more than he should."); }
        else { p.suspicion += p.empSec > 50 ? 8 : 2; p.warn.push_back("says routine security interviews are being held throughout his office."); }
    }
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
        if (p.suspicion >= 60) p.stress += 0.6f;         // a man who knows he is watched does not hide it well
        if (p.tru[A_NERVE] < 45 && pct(0.5f)) p.warn.push_back(pick(WARNFALSE));
        if (p.suspicion >= 100) { caught(p); return; }
        p.stress = clampf(p.stress, 0, 100);
        if (p.stress > 85 && pct(3)) { breakdown(p); return; }
    }
    // --- dead drop cycle
    if (p.dropEvery > 0 && p.dropSite >= 0 && p.nextDrop >= 0 && g.day >= p.nextDrop) serviceDrop(p);
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
    if (!ctl && pct((100 - p.diligence) * 0.5f + (p.stress > 80 ? 20 : 0))) { p.nextDrop = g.day + 2; return; }   // he put it off
    if (!ctl && p.suspicion >= 60 && sg && sg->mark == 2) { p.nextDrop = g.day + p.dropEvery; return; }
    if (!ctl) {
        if (s.staked) p.suspicion = std::max(p.suspicion + 30, 75.f);     // walked into a watched drop
        else if (p.suspicion >= 50 && pct(p.suspicion - 35)) stake(s);     // led the watchers to it
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
    s.exposure += 3;
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
    p.stage = ARRESTED;
    clearAppointments(p);
    p.nextMeet = -1; p.nextDrop = -1;
    for (int id : p.held) if (Roll* x = roll(id)) x->state = R_LOST;
    p.held.clear();
    for (int id : p.meetSites) if (Site* s = site(id)) s->burned = true;
    if (Site* s = site(p.dropSite)) stake(*s);
    if (Site* s = site(p.signalSite)) s->burned = true;
    ha.heat = clampf(ha.heat + 10, 0, 100);
    // a chief with twenty agents loses more of them than an officer with two, and is judged accordingly
    g.standRez -= 10 / (1 + 0.5f * T().role); g.standCentre -= 6 / (1 + 0.5f * T().role); g.totArrested++;
    file(p, "ARRESTED. Charged with espionage.");
    cable(T().chief, "Arrest of " + p.codename,
          "The newspapers report that " + p.name + fstr(", %d, ", p.age) + p.job + ", has been arrested and charged with espionage.\n\n"
              "Assume he is talking. Every site, signal and routine known to " + p.codename + " is finished as of today, and his case officer may be named in court. "
              "A full damage report is required.\n\n" + upper(T().chief));
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
    std::string note;
    if (!x.genuine && pct(p && p->fabricator ? 32 : 36))
        note = " NOTE: parts contradict information from reliable sources. The source requires verification.";
    x.grade = gr + (note.empty() ? "" : " (queried)");
    x.state = R_GRADED; x.stateDay = g.day;
    g.totRolls++; g.mRolls++; g.totValue += pts; g.mValue += pts;
    g.standCentre = clampf(g.standCentre + pts / (80.f * (1 + 0.5f * T().role)), 0, 100);
    credit(x.sector, pts, x.reqId);
    if (x.sector == SECURITY && x.genuine && v >= 40) g.secBonusUntil = g.day + 120;
    gradeLines.push_back(fstr("No. %d (", x.id) + src + (x.oral ? ", oral report" : ", film") + (x.cabled ? ", by cable summary" : "") + "): " + gr + "." + note);
}

// ============================================================================ DAWN
/** Is this officer followed today? Depends on how interested the adversary is in him and how stretched they are. */
void Sim::coverage(Actor& a, bool isYou) {
    (void)isYou;
    bool prev = false;
    for (int b = 0; b < 3; b++) {
        float p = clampf(0.04f + a.heat / 100.f * 0.72f, 0, 0.85f) * g.a4 * (b == 0 ? 0.7f : 1.f) * (weekend(g.day) ? 0.8f : 1.f);
        bool c = (b > 0 && r().f() < 0.55f) ? prev : r().f() < p;
        a.covered[b] = c; prev = c;
        a.teamQ[b] = (int)clampf(40 + a.heat * 0.4f + rnd(0, 15), 30, 90);
    }
}

void Sim::dawn() {
    actAs(nullptr);
    subj = nullptr;
    Date d = dateOf(g.day);
    if (g.day > g.postEnd) {
        endPosting(END_TOUR, "END OF POSTING", "Your tour in " + std::string(T().city) + " is complete. You hand over your agents, your sites and your safe to your successor and fly home by Aeroflot.");
        return;
    }
    // --- hidden surveillance picture for today
    g.a4 = clampf(g.a4 + r().gauss() * 0.12f + (g.a4Base - g.a4) * 0.2f, 0.3f, 2.4f);
    for (int i = 0; i < 8; i++) g.hot[i] = r().f() < T().density[i] * g.a4;
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
    if (d.d == 1 && g.day > g.postStart) monthly();
    history(d);
    if (g.phase != PH_PLAY) return;
    requirements();
    if (pct(T().walkIn) && g.day - g.postStart > 20) walkIn();
    // --- somebody takes an interest in you personally
    if (g.theatre > 0 && g.day - g.lastApproach > 240 && g.day - g.postStart > 90 && pct(0.10f + g.you.heat * 0.004f)) {
        g.lastApproach = g.day;
        addDecision(DK_APPROACH, 0, "An approach",
            std::string("At a reception last night a man you know slightly - a commercial counsellor, he says, though nobody has seen him do any commerce - took you aside. ")
            + "He knows your name and patronymic, your daughter's school, and the date your tour ends. He said that men of talent are not always valued at home, "
              "that he has friends who value them, and that he lunches on Thursdays at a place he named. Then he talked about fishing.\n\n"
              "It is a pitch, and a competent one. The rules say you report it to Line KR within twenty-four hours.",
            {"Report it to Line KR this morning (some hours of statements; you will be watched by your own side for a while)",
             "Say nothing. Nothing happened, and a report goes on your file for ever"});
    }
    if (g.rank < std::min(3, g.theatre + 1) && g.standCentre >= 72 && g.day - g.postStart > 365) {
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
        std::string kr = est < 22 ? "No sign that the adversary takes a special interest in you."
                         : est < 45 ? "Periodic coverage. You are probably carded as a suspected intelligence officer."
                         : est < 68 ? "Regular coverage. Assume you are identified as an officer of the service."
                         : "You are a priority target. KR advises that you stand down from all operational acts.";
        sb += "\nLINE KR weekly assessment of your position:\n  " + kr + "\n";
        g.krLog.push_back(pad(fmtShort(g.day), 11) + kr);
        // and, if you have officers, a line on each of them
        for (auto& o : g.officers) if (o.role == ROLE_SUB && !o.gone) {
            float e = o.a.heat + r().gauss() * 11;
            sb += "  " + o.title() + ": " + (e < 22 ? "not noticed." : e < 45 ? "periodic coverage." : e < 68 ? "regular coverage." : "priority target - stand him down.") + "\n";
        }
    }
    sb += "\nTODAY:\n";
    bool any = false;
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
            bool flag = controlled(*p) ? pct(35) : pct(4);
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
            bool guilty = o->mole && o->moleDay <= g.day;
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
            } else if (x.amt == 3) {                    // an approach you did not report
                g.standRez = clampf(g.standRez - 15, 0, 100); g.standCentre = clampf(g.standCentre - 8, 0, 100);
                g.paperwork += 6;
                cable("Line KR", "A matter you omitted to report", "It has come to the knowledge of this Line, by another channel, that an officer of the adversary made an approach to you some time ago. "
                      "There is no record of a report from you. You will write one now, in full, and explain the delay.\n\nLINE KR");
            }
            break;
        }
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
    g.paperwork += 3 + T().role;
    float par = 26 * (1 + 0.3f * T().role);
    float score = g.mSpotted * 3 + std::min(g.mMeetings, 8 + 4 * T().role) * 1.5f + g.mRolls * 2 + std::min(g.mValue / 12.f, 15.f * (1 + 0.5f * T().role))
                  - (g.overdueDays > 0 ? 8 : 0) - (g.cover < 40 ? 8 : 0);
    float delta = clampf((score - par) / 3.f, -3, 5);
    // yesterday's successes are forgotten, and so in time are yesterday's failures: both standings drift back towards the ordinary
    g.standRez = clampf(g.standRez + delta + (g.standRez > 50 ? -1.5f : (g.standRez < 35 ? 1.5f : 0.5f)), 0, 100);
    g.standCentre = clampf(g.standCentre + (g.standCentre > 45 ? -1.5f : 0.5f), 0, 100);
    // RYAN: what you report, and what it costs
    std::string ryan;
    if (g.ryanPolicy == 0) { g.standCentre = clampf(g.standCentre - 1.5f, 0, 100); ryan = "RYAN return submitted: no indicators observed. Centre notes 'insufficient attention to the task'.\n"; }
    else if (g.ryanPolicy == 1) { g.standCentre = clampf(g.standCentre + 2, 0, 100); g.paperwork += 3; ryan = "RYAN return submitted: nine indicators reported with commentary (three hours). Centre notes the rezidentura's vigilance and asks for more detail next month.\n"; }
    std::string verdict = delta >= 3 ? "Good work. Keep to this pace." : delta >= 0 ? "Satisfactory." : delta > -3 ? "Below what is expected of this post."
                          : "Unsatisfactory. Results are what is asked of you, not explanations.";
    Date prev = dateOf(g.day - 1);
    cable(T().chief, std::string("Monthly review - ") + MONTH[prev.m],
          fstr("New contacts: %d. Meetings held: %d. Acts aborted: %d. Material graded: %d.\n", g.mSpotted, g.mMeetings, g.mAborts, g.mRolls)
              + (g.overdueDays > 0 ? "Reports are in arrears.\n" : "") + (g.cover < 40 ? "The Ambassador's office complains that you neglect your cover duties. That endangers more than yourself.\n" : "") + ryan
              + "\n" + verdict + fstr("\n\nOperational advance for the month: %d %s (basic %d, plus four-fifths of agreed agent remuneration). Monthly accounts are due: %d hours.\n\n", allot, T().currency, basic, 3 + T().role)
              + upper(T().chief));
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
    Person np = makePerson(r().n(4), 0.5f);
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
              + ", and offers documents for money.\n\nHalf of these are provocations and the other half are our best agents. He is yours to assess. Trace him before anything else is done.");
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
                std::vector<Officer*> c;
                for (auto& o : g.officers) if (!o.gone && o.role == ROLE_COLLEAGUE) c.push_back(&o);
                Officer* who = nullptr;
                for (Officer* o : c) if (o->mole) who = o;                 // if there was a traitor all along, it is he who runs
                if (!who && !c.empty()) who = c[r().n((int)c.size())];
                std::string lost;
                if (who) {
                    for (auto& p : g.persons) if (p.live() && has(p.witting, who->id)) lost += "  " + p.full() + "\n";
                    defection(*who);
                }
                text = (who ? who->title() : std::string("An officer")) + " of this rezidentura has failed to report and must be presumed to be in the hands of the adversary.\n\n"
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
                       "the price paid for donor blood, the slaughter of cattle. Reports are required whether or not anything has been observed.\n\nYour line will make the return for Washington.\n\nCENTRE";
                addDecision(DK_RYAN, 0, "Operation RYAN - your line's returns",
                    "You have read the list of indicators. Half of them are things that happen in Washington every working day; the other half could not be observed by anyone without an agent in the President's bedroom. "
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
                    "He has three sources in the Bonn ministries who have never been met by anyone but him, and he would rather give them to you than see them abandoned. He can introduce them once.\n\n"
                    "They are productive, trained and frightened. Their names are also in a card index in the Normannenstrasse, and nobody can tell you whether that index has been burned or sold.",
                    {"Take over all three", "Take over only the one with the best access", "Decline. Their files are not in your safe and never will be"});
                break;
            default: break;
        }
        if (!text.empty()) cable(h.from, h.title, text);
    }
}
