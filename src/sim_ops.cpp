// sim_ops.cpp - operations. Most of the work never ends: a contact is developed, an agent is run, a site wears out.
// An operation is the opposite: a name, a goal, a deadline, stages anybody can see, and a reckoning on the day.
// Centre (or the Rezident) offers one every couple of months, built from the station as it is:
//   OP_RECRUIT  a man with real access, who is leaving: recruit him before he goes
//   OP_LEAK     the adversary knows too much: find the traitor in the station - or say there is none
//   OP_RESCUE   one of your agents is close to arrest: get him out first
//   OP_FLASH    Moscow needs one thing, quickly, from your line
// Nothing hidden is shown: the stages are judged from what your own files say.
#include "sim.h"

static bool has(const std::string& s, const char* w) { return s.find(w) != std::string::npos; }
static uint32_t hsh32(uint32_t a, uint32_t b) { uint32_t h = a * 2654435761u ^ (b + 0x9E3779B9u); h ^= h >> 15; h *= 2246822519u; h ^= h >> 13; return h; }
static std::string upperStr(std::string s) { for (auto& c : s) c = (char)toupper((unsigned char)c); return s; }
static const char* const OPNAME[] = {"VYUGA", "ZARYA", "METEL", "GROM", "IRTYSH", "ALMAZ", "TUMAN", "PURGA", "KOSTYOR", "SAPSAN", "BEREG", "ORION", "VOLNA", "KREMEN", "SHTORM", "YANTAR"};

Operation* Sim::activeOp() { for (auto& o : g.ops) if (o.state == OPS_ACTIVE) return &o; return nullptr; }

static int controlOps(const GameState& g) {
    int n = 0;
    for (auto& o : g.officers) for (auto& f : o.file) if (has(f, "ontrol operation at") || has(f, "CONTROL OPERATION at")) n++;
    return n;
}
static bool inDanger(const Sim& s, const Person& p) {
    if (p.stage != AGENT || p.handler != 0) return false;
    int n = (int)p.file.size();
    for (int i = std::max(0, n - 6); i < n; i++)
        if (has(p.file[i], "searched") || has(p.file[i], "telephone clicks") || has(p.file[i], "followed from") || has(p.file[i], "further conversation")
            || has(p.file[i], "SURVEILLANCE OBSERVED ON THE AGENT")) return true;
    (void)s;
    return false;
}

std::vector<std::pair<std::string, bool>> Sim::opStages(const Operation& o) {
    std::vector<std::pair<std::string, bool>> v;
    Person* p = person(o.ref);
    Requirement* q = req(o.ref);
    switch (o.kind) {
        case OP_RECRUIT:
            v.push_back({"Relations at least friendly", p && (p->rapport >= 40 || p->stage == AGENT)});
            v.push_back({"A clean trace, and Centre's sanction", p && (p->sanction == 2 || p->stage == AGENT)});
            v.push_back({"Recruited", p && p->stage == AGENT});
            break;
        case OP_LEAK:
            v.push_back({"Run a marked-card control operation", controlOps(g) > o.ref2});
            v.push_back({"Name the traitor - or nobody (Planner: 'Name the traitor')", o.stage >= 9});
            break;
        case OP_RESCUE:
            v.push_back({"Ask Centre for his travel documents (Contacts: More actions)", p && (p->exfil >= 1 || p->stage == EXFILTRATED)});
            v.push_back({"The documents arrive", p && (p->exfil >= 2 || p->stage == EXFILTRATED)});
            v.push_back({"Hand them over at a meeting: he leaves that night", p && p->stage == EXFILTRATED});
            break;
        case OP_FLASH: {
            bool tasked = false;
            for (auto& a : g.persons) if (a.stage == AGENT && a.reqId == o.ref) tasked = true;
            v.push_back({"Task an agent of the line against it", tasked || (q && q->progress > 0)});
            v.push_back({"Material on it graded by Centre", q && q->progress > 0});
            v.push_back({"The requirement satisfied", q && q->closed && !q->failed});
            break;
        }
        default: break;
    }
    return v;
}

void Sim::opOffer(int kind) {
    Operation o;
    o.id = g.nextId++;
    o.kind = kind; o.start = g.day; o.state = OPS_OFFERED;
    o.name = OPNAME[(g.ops.size() + g.seed + g.theatre * 5) % (sizeof OPNAME / sizeof OPNAME[0])];
    std::string from = T().role == 3 ? "Centre" : "the Rezident";
    switch (kind) {
        case OP_RECRUIT: {
            Person* best = nullptr;
            for (auto& p : g.persons)
                if (p.live() && p.handler == 0 && p.stage != AGENT && p.estV[A_ACCESS] >= 55 && !p.traceBad && (!best || p.estV[A_ACCESS] > best->estV[A_ACCESS])) best = &p;
            if (!best) {                          // Centre points one out
                int sec = r().n(N_SECTOR);
                Person np = makePerson(sec, T().dangle * hard());
                np.tru[A_ACCESS] = np.app[A_ACCESS] = clampf(rnd(70, 90), 0, 97);
                np.estV[A_ACCESS] = 70; np.estC[A_ACCESS] = 30;
                g.persons.push_back(np);
                best = &g.persons.back();
                file(*best, "Pointed out by Centre as a target of the first importance. You are to arrange to meet him.");
            }
            o.ref = best->id;
            o.due = g.day + 100 + r().n(30);
            o.title = "Recruit " + best->name;
            o.brief = gx(best, best->name + ", " + best->job + ", " + best->employer + ", is to be posted out of " + T().city + " on " + fmtDay(o.due)
                      + ". What he sees now, nobody we have sees. Centre wants him recruited before he goes.");
            o.stakes = "Succeed, and Centre's thanks go on your file with a bonus to operational funds. Fail, and that is noted too. A refusal reported to his security officer will not help either.";
            break;
        }
        case OP_LEAK: {
            o.ref = -1;
            for (auto& x : g.officers) if (!x.gone && x.mole && x.moleDay <= g.day) o.ref = x.id;
            o.ref2 = controlOps(g);
            o.due = g.day + 75;
            o.title = "Find the leak";
            o.brief = "The adversary has known too much of late: sites burned that should have been clean, and a man of ours taken who should not have been. Directorate K's analysts think the leak may be in this station - or want to be sure it is not. "
                      "You are to find out, and name the man to " + from + " by " + fmtDay(o.due) + ".";
            o.stakes = "Name the traitor, and he is removed and you are commended. Accuse an innocent comrade, and everybody will know it. Say there is nobody when there is somebody, and the leak goes on. The marked card (a control operation) and the Case board are your instruments.";
            break;
        }
        case OP_RESCUE: {
            Person* a = nullptr;
            for (auto& p : g.persons) if (inDanger(*this, p) && p.exfil == 0 && p.escape == 0) { a = &p; break; }
            if (!a) return;
            o.ref = a->id;
            o.due = g.day + 50;
            o.title = "Bring " + a->codename + " out";
            o.brief = gx(a, a->codename + "'s last reports frighten " + from + ": his office is being looked at, and so, very likely, is he. If they take him, he talks, and everything he knows dies with his freedom. Get him out of " + std::string(T().country) + " by " + fmtDay(o.due) + ".");
            o.stakes = gx(a, "The documents take a week or two; his next meeting may be further off than that - a call-out signal or a broadcast brings him sooner. Bring him out, and the service will remember that you did not abandon a man. Lose him, and you will answer for it - and every agent who hears of it will wonder about himself.");
            break;
        }
        case OP_FLASH: {
            Person* a = nullptr;
            for (auto& p : g.persons) if (p.stage == AGENT && p.handler == 0 && p.camera) { a = &p; break; }
            if (!a) return;
            static const char* const WHAT[N_SECTOR] = {"the Cabinet's real position at next month's NATO ministerial meeting", "the deployment schedule of the new submarine squadron",
                                                       "the test results of the new airborne radar", "the names of the officers on the Soviet desk of the security service"};
            Requirement q;
            q.id = g.nextReqNo++; q.kind = RK_COLLECT; q.sector = a->sector; q.issued = g.day; q.due = g.day + 60; q.priority = 1; q.need = 32;
            q.title = std::string("FLASH: ") + WHAT[a->sector];
            q.text = "Required at short notice for a meeting of the leadership. Documents, not opinions.";
            g.reqs.push_back(q);
            o.ref = q.id;
            o.due = q.due;
            o.title = std::string("Flash: ") + WHAT[a->sector];
            o.brief = std::string("Moscow wants ") + WHAT[a->sector] + ", by " + fmtDay(o.due) + ". It is wanted for a meeting of the leadership. Your line is the one that can get it.";
            o.stakes = "Deliver, and it is read at the very top. Fail, and the requirement lapses with your name on it. The pouch closes on Thursdays and takes a week and more to be graded: plan backwards from the date.";
            break;
        }
        default: return;
    }
    g.ops.push_back(o);
    g.lastOpDay = g.day;
    addDecision(DK_OP_OFFER, o.id, "Operation " + o.name + ": " + o.title, o.brief + "\n\n" + o.stakes + "\n\nDeadline: " + fmtDay(o.due) + ".",
                {"Take it on", kind == OP_LEAK ? "Decline. Let Directorate K do its own work" : "Decline. The station has enough on its hands"});
}

/** Called each morning. Judges the operation that is running, and now and then offers a new one. */
void Sim::opsDawn() {
    if (training() || g.phase != PH_PLAY) return;
    if (g.scenario) {                         // a scenario ends when its operation is decided, or its time runs out
        if (Operation* o = activeOp()) (void)o;
        bool decided = false;
        for (auto& o : g.ops) if (o.state == OPS_WON || o.state == OPS_LOST) decided = true;
        if (decided || g.day > g.scenarioEnd) { scenarioFinish(false, ""); return; }
    }
    if (Operation* o = activeOp()) {
        Person* p = person(o->ref);
        Requirement* q = req(o->ref);
        auto st = opStages(*o);
        int done = 0;
        for (auto& s : st) if (s.second) done++; else break;
        o->stage = std::max(o->stage, done);
        switch (o->kind) {
            case OP_RECRUIT:
                if (p && p->stage == AGENT) opEnd(*o, true, gx(p, p->name + " is recruited, as " + p->codename + "."));
                else if (!p || !p->live()) opEnd(*o, false, gx(p, (p ? p->name : std::string("The target")) + " is lost to us."));
                else if (g.day > o->due) opEnd(*o, false, gx(p, p->name + " has left " + T().city + ", unrecruited."));
                break;
            case OP_RESCUE:
                if (p && p->stage == EXFILTRATED) opEnd(*o, true, gx(p, p->codename + " is safe in the Soviet Union."));
                else if (!p || p->stage == ARRESTED) opEnd(*o, false, gx(p, (p ? p->codename : std::string("The agent")) + " was arrested before he could be brought out."));
                else if (!p->live() || g.day > o->due) opEnd(*o, false, gx(p, p->codename + " was not brought out in time."));
                break;
            case OP_FLASH:
                if (q && q->closed && !q->failed) opEnd(*o, true, "The material was read at the meeting it was wanted for.");
                else if (!q || q->failed || g.day > o->due + 1) opEnd(*o, false, "The material did not arrive in time.");
                break;
            case OP_LEAK:
                if (g.day > o->due && o->stage < 9) opEnd(*o, false, "The day came, and you named nobody.");
                break;
            default: break;
        }
        return;
    }
    // a man of yours in danger does not wait for the calendar
    bool offered = g.scenario != 0;           // in a scenario there is one operation, and it is already running
    for (auto& o : g.ops) if (o.state == OPS_OFFERED) offered = true;
    if (offered || !g.decisions.empty() || g.day - g.postStart < 30) return;
    for (auto& p : g.persons)
        if (inDanger(*this, p) && p.exfil == 0 && p.escape == 0) {
            bool before = false;
            for (auto& o : g.ops) if (o.kind == OP_RESCUE && o.ref == p.id) before = true;
            if (!before) { opOffer(OP_RESCUE); return; }
        }
    if (g.day - g.lastOpDay < 90 + (int)(hsh32((uint32_t)g.seed, (uint32_t)g.lastOpDay) % 60)) return;
    bool leakDone = false, mole = false, agent = false;
    for (auto& o : g.ops) if (o.kind == OP_LEAK) leakDone = true;
    for (auto& x : g.officers) if (!x.gone && x.mole && x.moleDay <= g.day) mole = true;
    for (auto& p : g.persons) if (p.stage == AGENT && p.handler == 0 && p.camera) agent = true;
    int roll = r().n(100);
    if (!leakDone && ((mole && roll < 45) || roll < 8)) opOffer(OP_LEAK);
    else if (agent && roll < 55) opOffer(OP_FLASH);
    else opOffer(OP_RECRUIT);
}

void Sim::opEnd(Operation& o, bool won, const std::string& why) {
    o.state = won ? OPS_WON : OPS_LOST;
    o.ended = g.day;
    std::string from = T().role == 3 ? "Centre" : (o.kind == OP_RESCUE ? "Rezident" : "Centre");
    Person* keep = subj; subj = nullptr;
    if (won) {
        g.opsWon++;
        g.standCentre = clampf(g.standCentre + (o.kind == OP_LEAK ? 12 : 8), 0, 100);
        g.standRez = clampf(g.standRez + 5, 0, 100);
        if (o.kind != OP_LEAK) money(P(50), "Bonus from Centre: operation " + o.name);
        cable(from, "Operation " + o.name + " - COMPLETED", why + "\n\nOperation " + o.name + " (" + o.title + ") is closed as a success. "
              + (o.kind != OP_LEAK ? fstr("A bonus of %d %s is credited to operational funds. ", P(50), T().currency) : std::string(""))
              + "It has been entered on your file.\n\n" + upperStr(from));
    } else {
        g.opsLost++;
        g.standCentre = clampf(g.standCentre - 3, 0, 100);
        g.standRez = clampf(g.standRez - 2, 0, 100);
        cable(from, "Operation " + o.name + " - FAILED", why + "\n\nOperation " + o.name + " (" + o.title + ") is closed as a failure. The reasons will be examined.\n\n" + upperStr(from));
    }
    subj = keep;
    attention = true;
}

std::string Sim::nameTraitor() {
    Operation* o = activeOp();
    if (!o || o->kind != OP_LEAK) return "There is no leak hunt under way.";
    for (auto& d : g.decisions) if (d.kind == DK_OP_NAME) return "";
    std::vector<std::string> opts;
    for (auto& x : g.officers) if (!x.gone && (x.role == ROLE_COLLEAGUE || x.role == ROLE_SUB || x.role == ROLE_TECH)) opts.push_back(x.title());
    opts.push_back("Nobody at this station. The leak is elsewhere");
    addDecision(DK_OP_NAME, o->id, "Operation " + o->name + ": the name",
                "You are to give " + std::string(T().role == 3 ? "Centre" : "the Rezident") + " one name, or none. Directorate K will act on it.\n\n"
                "Think of who knew of the sites that burned, who knew of the man who was taken, and what your marked cards showed. The Case board has the strings.",
                opts);
    return "";
}

void Sim::opDecide(const Decision& d, int option) {
    Operation* o = nullptr;
    for (auto& x : g.ops) if (x.id == d.ref) o = &x;
    if (!o) return;
    if (d.kind == DK_OP_OFFER) {
        if (option == 0) { o->state = OPS_ACTIVE; o->start = g.day; }
        else {
            o->state = OPS_DECLINED; o->ended = g.day;
            g.standCentre = clampf(g.standCentre - (o->kind == OP_LEAK ? 3 : 1), 0, 100);
            if (o->kind == OP_FLASH) if (Requirement* q = req(o->ref)) { q->closed = true; q->failed = true; }
        }
        return;
    }
    // DK_OP_NAME: the same list, in the same order, as when the question was put
    std::vector<Officer*> cand;
    for (auto& x : g.officers) if (!x.gone && (x.role == ROLE_COLLEAGUE || x.role == ROLE_SUB || x.role == ROLE_TECH)) cand.push_back(&x);
    o->stage = 9;
    Officer* mole = nullptr;
    for (auto& x : g.officers) if (!x.gone && x.mole && x.moleDay <= g.day) mole = &x;
    if (option >= (int)cand.size()) {
        if (!mole) opEnd(*o, true, "Directorate K's inquiry has found the leak in a ministry in Moscow, not in your station. Your judgement is noted.");
        else opEnd(*o, false, "You named nobody. The leak has not stopped.");
        return;
    }
    Officer* x = cand[(size_t)option];
    if (x == mole) {
        x->gone = true;
        x->file.push_back(pad(fmtShort(g.day), 11) + "RECALLED AND ARRESTED. Directorate K confirms he worked for the adversary.");
        for (auto& q : g.persons) {
            if (std::find(q.witting.begin(), q.witting.end(), x->id) != q.witting.end())
                q.file.push_back(pad(fmtShort(g.day), 11) + "KNOWN TO THE ADVERSARY through the traitor " + x->name + ". Everything since he learned of this case is suspect.");
            if (q.live() && q.handler == x->id) { q.handler = 0; q.pitchAuth = false; if (q.stage == AGENT) schedule(q); }
        }
        g.you.heat = std::max(8.f, g.you.heat - 8);
        g.awards.push_back(fstr("%d  Commended by the Chairman for the exposure of a traitor (", dateOf(g.day).y) + T().city + ")");
        opEnd(*o, true, x->title() + " was recalled on a pretext and has confessed. The files he betrayed are marked on the Contacts screen.");
    } else {
        x->diligence = std::max(20.f, x->diligence - 15);
        x->file.push_back(pad(fmtShort(g.day), 11) + "Named by you as the source of a leak. Cleared by Directorate K. He knows who named him.");
        g.standRez = clampf(g.standRez - 6, 0, 100);
        opEnd(*o, false, x->title() + " has been cleared by Directorate K. " + (mole ? "The leak goes on." : "The leak was never in the station.") + " He knows it was you.");
    }
}

// ============================================================================ scenarios
const ScenarioDef SCENARIOS[] = {
    {"A full career", "", TH_MOSCOW, 0},
    {"The man from the Admiralty - London, 1968",
     "Your first posting. Centre has its eye on a man with access to secrets nobody of ours has seen, and he is leaving the city in a few months. Recruit him before he goes - without being thrown out of the country first.",
     TH_LONDON, OP_RECRUIT},
    {"Somebody in the station - Vienna, 1973",
     "Sources are being lost and sites are burning. Centre believes one of your colleagues is working for the other side. You have a few weeks, the marked card and the Case board. Name him - and be right.",
     TH_VIENNA, OP_LEAK},
    {"Get him out - Washington, 1979",
     "The best agent you have inherited has seen men outside his house. The FBI is closing in. Get the travel documents, get him to a meeting, and get him out of America before they take him.",
     TH_WASHINGTON, OP_RESCUE},
    {"Flash - Berlin, 1979",
     "Moscow wants one thing, quickly, for a meeting at the very top. Your line can get it - if you can task the right man and get his film into the pouch in time.",
     TH_BERLIN1, OP_FLASH},
};
const int N_SCENARIO = (int)(sizeof SCENARIOS / sizeof SCENARIOS[0]);

void Sim::scenarioSetup(int k) {
    const ScenarioDef& sc = SCENARIOS[k];
    g.scenario = k;
    g.lastOpDay = g.day;
    if (sc.opKind == OP_LEAK) {               // in this story there is a traitor, and he is already at work
        bool any = false;
        for (auto& o : g.officers) if (o.mole && !o.gone) { any = true; o.moleDay = std::min(o.moleDay, g.day); }
        if (!any) {
            std::vector<Officer*> c;
            for (auto& o : g.officers) if (!o.gone && (o.role == ROLE_COLLEAGUE || o.role == ROLE_SUB)) c.push_back(&o);
            if (!c.empty()) { Officer* m = c[r().n((int)c.size())]; m->mole = true; m->moleDay = g.day; }
        }
    }
    if (sc.opKind == OP_RESCUE) {             // and here an agent of yours is already being looked at
        for (auto& p : g.persons) if (p.stage == AGENT && p.handler == 0) {
            p.suspicion = std::max(p.suspicion, 50.f);
            p.warn.push_back("is certain he is followed from his office every evening. He asks what will happen to his family.");
            file(p, "Agent is certain he is followed from his office every evening. (Reported by the departing officer.)");
            if (p.signalSite < 0) for (auto& s : g.sites) if (s.type == ST_SIGNAL && !s.retired) { p.signalSite = s.id; break; }
            break;
        }
    }
    opOffer(sc.opKind);
    for (auto& d : g.decisions) if (d.kind == DK_OP_OFFER) { g.decisions.erase(std::find_if(g.decisions.begin(), g.decisions.end(), [&](const Decision& x) { return x.id == d.id; })); break; }
    Operation* op = nullptr;
    for (auto& o : g.ops) if (o.state == OPS_OFFERED) { o.state = OPS_ACTIVE; op = &o; }
    g.scenarioEnd = op ? op->due + 1 : g.day + 120;
    Person* keep = subj; subj = nullptr;
    cable("Centre", std::string("SCENARIO: ") + sc.name, std::string(sc.blurb) + "\n\n" + (op ? "Operation " + op->name + ": " + op->title + ".\n\n" + op->brief + "\n\n" + op->stakes : std::string(""))
          + "\n\nThe scenario ends on " + fmtDay(g.scenarioEnd) + ", or as soon as the operation is decided. Its stages are pinned above the morning brief on the Planner. Your deputy runs the routine.\n\nCENTRE");
    subj = keep;
}

/** The end of a scenario: the operation decided, the time run out, or the posting cut short. */
void Sim::scenarioFinish(bool forced, const std::string& why) {
    Operation* op = nullptr;
    for (auto& o : g.ops) if (o.state != OPS_DECLINED) op = &o;
    bool won = op && op->state == OPS_WON && !forced;
    int agents = 0;
    for (auto& p : g.persons) if (p.stage == AGENT) agents++;
    float score = (won ? 60.f : 0.f) + std::min(20.f, g.totValue / 40.f) + std::min(15, agents * 5) + std::min(10, g.totRecruited * 5) - g.totArrested * 10 - (forced ? 30 : 0);
    score = clampf(score, 0, 100);
    std::string grade = score >= 85 ? "Order of the Red Banner" : score >= 65 ? "Order of the Red Star" : score >= 45 ? "A commendation from the Chairman" : score >= 25 ? "A note on your file" : "A reprimand";
    std::string sb = std::string(SCENARIOS[g.scenario].name) + "\n\n";
    sb += forced ? why + "\n\n" : (op ? std::string(won ? "The operation succeeded. " : "The operation failed. ") : std::string("")) + "\n";
    sb += fstr("Days: %d.   Material graded: %d (worth %d).   Recruited: %d.   Agents active at the end: %d.   Agents lost to arrest: %d.\n\n", g.day - g.postStart, g.totRolls, (int)std::lround(g.totValue), g.totRecruited, agents, g.totArrested);
    sb += fstr("SCORE: %d / 100  -  ", (int)std::lround(score)) + grade + ".\n\n";
    sb += "The files have been opened. What your agents really were:\n";
    for (auto& p : g.persons) {
        if (p.codename.empty()) continue;
        std::string truth = p.dangle ? "A PROVOCATION - the adversary's man from the start" : p.doubled ? "caught and TURNED by the adversary" : p.fabricator ? "genuine, but invented part of his material" : "genuine";
        sb += "  " + pad(p.codename, 10) + pad(clip(p.name + ", " + p.job, 46), 48) + pad(STAGE_LABEL[p.stage], 14) + truth + "\n";
    }
    for (auto& o : g.officers) if (o.mole) sb += "  (" + o.title() + " of the rezidentura worked for the adversary" + (o.gone ? "; he was taken.)\n" : "; he was never caught.)\n");
    g.phase = PH_OVER;
    g.overTitle = std::string(won ? "SCENARIO WON" : "SCENARIO LOST") + fstr("  -  %d / 100", (int)std::lround(score));
    g.overText = sb;
    g.archived = true;                        // a scenario is not a career: it does not go into the service records
    attention = true;
}
