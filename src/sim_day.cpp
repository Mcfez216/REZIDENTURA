// sim_day.cpp - the day planner, staff actions, and everything that happens in the street.
#include "sim.h"
#include <cstdarg>

// ============================================================================ small helpers
const char* W_RAPPORT[5] = {"cold", "polite", "friendly", "warm", "close"};
const char* W_COMMIT[5] = {"none", "slight", "has done favours", "compromised", "fully committed"};
const char* W_LEVEL[5] = {"very low", "low", "moderate", "high", "very high"};
const char* W_STAND[5] = {"very poor", "poor", "fair", "good", "excellent"};

std::string word(float v, const char* const* w, int n) { return w[(int)(clampf(v, 0, 99.9f) / 100.f * n)]; }

std::string fstr(const char* fmt, ...) {
    char b[1024];
    va_list ap; va_start(ap, fmt); vsnprintf(b, sizeof b, fmt, ap); va_end(ap);
    return b;
}
int ulen(const std::string& s) { int n = 0; for (unsigned char c : s) if ((c & 0xC0) != 0x80) n++; return n; }
std::string pad(const std::string& s, int n) { std::string r = s; for (int i = ulen(s); i < n; i++) r += ' '; return r; }
std::string clip(const std::string& s, int n) {
    if (ulen(s) <= n) return s;
    size_t i = 0; int c = 0;
    while (i < s.size() && c < n - 1) { i++; while (i < s.size() && ((unsigned char)s[i] & 0xC0) == 0x80) i++; c++; }
    return s.substr(0, i) + ".";
}

/**
 * All the game's text is written in the masculine ("he", "his", "him"). When the person it is about
 * is a woman it is converted here, so that nobody has to write every sentence twice.
 */
std::string gx(const Person* p, const std::string& s) {
    if (!p || !p->female) return s;
    static const char* M[][2] = {{"himself", "herself"}, {"he", "she"}, {"He", "She"}, {"HE", "SHE"}, {"his", "her"}, {"His", "Her"}, {"him", "her"}};
    auto letter = [](unsigned char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c >= 0x80; };
    std::string out;
    size_t i = 0, n = s.size();
    while (i < n) {
        if (letter((unsigned char)s[i])) {
            size_t j = i;
            while (j < n && letter((unsigned char)s[j])) j++;
            std::string w = s.substr(i, j - i);
            for (auto& m : M) if (w == m[0]) { w = m[1]; break; }
            out += w; i = j;
        } else out += s[i++];
    }
    return out;
}

Person* Sim::person(int id) { for (auto& p : g.persons) if (p.id == id) return &p; return nullptr; }
Site* Sim::site(int id) { for (auto& s : g.sites) if (s.id == id) return &s; return nullptr; }
Roll* Sim::roll(int id) { for (auto& x : g.rolls) if (x.id == id) return &x; return nullptr; }
Requirement* Sim::req(int id) { for (auto& q : g.reqs) if (q.id == id) return &q; return nullptr; }
Officer* Sim::officer(int id) { for (auto& o : g.officers) if (o.id == id) return &o; return nullptr; }

std::string Sim::handlerName(const Person& p) {
    if (p.handler == 0) return "you";
    Officer* o = officer(p.handler);
    return o ? o->title() : "?";
}

void Sim::say(const std::string& s) { if (rep) rep->push_back(gx(subj, s)); }
void Sim::alert(const std::string& s) { if (rep) rep->push_back("  *** " + gx(subj, s)); attention = true; }
void Sim::file(Person& p, const std::string& s) { p.file.push_back(pad(fmtShort(g.day), 11) + gx(&p, s)); }
void Sim::cable(const std::string& from, const std::string& subject, const std::string& body) {
    Cable c;
    c.day = g.day; c.from = from; c.subject = subject; c.body = gx(subj, body);
    g.cables.push_back(c);
    attention = true;
}
/** Every movement of operational funds goes through here so that it lands in the accounts. */
void Sim::money(int delta, const std::string& what) {
    if (delta == 0) return;
    g.funds += delta;
    Ledger l;
    l.day = g.day; l.amt = delta; l.bal = g.funds; l.what = what;
    g.ledger.push_back(l);
}
void Sim::queue(int kind, int ref, int groups, const std::string& subject, int p1, int p2, int p3) {
    OutCable c;
    c.kind = kind; c.ref = ref; c.groups = groups; c.subject = subject; c.queued = g.day; c.p1 = p1; c.p2 = p2; c.p3 = p3;
    g.outQ.push_back(c);
}
void Sim::pend(int day, int kind, int ref, int amt, int p2, int p3) {
    Pending x;
    x.day = day; x.kind = kind; x.ref = ref; x.amt = amt; x.p2 = p2; x.p3 = p3;
    g.pending.push_back(x);
}
void Sim::witness(std::vector<int>& list, int officerId) {
    if (officerId <= 0) return;
    if (std::find(list.begin(), list.end(), officerId) == list.end()) list.push_back(officerId);
}
/** The adversary's interest in whoever is acting just now. */
void Sim::heat(float d) {
    A->heat = clampf(A->heat + d, 0, 100);
    if (A->heat > A->peakHeat) A->peakHeat = A->heat;
}

Site* Sim::agreedSite(const Person& p) {
    if (p.meetSites.empty()) return nullptr;
    return site(p.meetSites[(size_t)std::abs(p.meetIdx) % p.meetSites.size()]);
}

std::string Sim::shortSite(const Site* s) {
    if (!s) return "?";
    std::string n = s->name;
    size_t i = n.find(" [");
    std::string d = i != std::string::npos ? n.substr(i) : "";
    std::string base = i != std::string::npos ? n.substr(0, i) : n;
    return clip(base, 26) + d;
}

// ============================================================================ the day planner
Order Sim::routine(int day, int b) const {
    Order o;
    o.type = (weekend(day) || b == 2) ? O_REST : O_COVER;
    return o;
}
bool Sim::isSet(int day, int b) const { auto it = g.plan.find(day); return it != g.plan.end() && it->second[b].set; }
Order Sim::order(int day, int b) const { auto it = g.plan.find(day); return it != g.plan.end() && it->second[b].set ? it->second[b] : routine(day, b); }
void Sim::setOrder(int day, int b, Order o) { o.set = true; g.plan[day][b] = o; }
void Sim::clearOrder(int day, int b) { auto it = g.plan.find(day); if (it != g.plan.end()) it->second[b] = Order(); }

std::string Sim::describe(const Order& o) {
    Person* p = person(o.person);
    Site* s = site(o.site);
    std::string pn = p ? p->label() : "?";
    switch (o.type) {
        case O_COVER: return "Cover duties";
        case O_OFFICE: return "Paperwork";
        case O_REST: return "Rest";
        case O_FUNCTION: return std::string("Function: ") + SECTOR_SHORT[o.sector];
        case O_CASE: return "Case: " + shortSite(s);
        case O_SCOUT: return std::string("Scout ") + T().district[o.district] + " (" + SITETYPE_LABEL[o.siteType] + ")";
        case O_SDR: return o.route >= 0 && site(o.route) ? "SDR: " + shortSite(site(o.route)) : "SDR - improvised route";
        case O_MEET_OVERT: return "Social: " + pn + (o.am >= 0 ? " [Service A]" : "");
        case O_MEET_CLAN: return std::string(o.appointment ? "MEET* " : "MEET ") + pn + (o.pitch >= 0 ? " [PITCH]" : "") + " @ " + shortSite(s);
        case O_LOAD_DROP: return "LOAD drop for " + pn + ": " + shortSite(s);
        case O_UNLOAD_DROP: return "UNLOAD drop: " + shortSite(s);
        case O_READ_SIGNAL: return "Read signal: " + shortSite(s);
        case O_PLACE_SIGNAL: return "Call-out signal for " + pn;
        case O_LINE_N: return "LINE N load: " + shortSite(s);
        case O_CONTROL: return "CONTROL op: " + shortSite(s);
        default: return "?";
    }
}

bool Sim::isOp(int t) {
    return t == O_CASE || t == O_SCOUT || t == O_MEET_CLAN || t == O_LOAD_DROP || t == O_UNLOAD_DROP || t == O_LINE_N || t == O_CONTROL;
}

/** Returns "" if the order is legal for that slot, otherwise the reason it is not. */
std::string Sim::validate(const Order& o, int day, int b) {
    Person* p = person(o.person);
    Site* s = site(o.site);
    auto bad = [&](const std::string& m) { return gx(p, m); };
    bool usesPerson = o.type == O_MEET_OVERT || o.type == O_MEET_CLAN || o.type == O_LOAD_DROP || o.type == O_PLACE_SIGNAL;
    if (usesPerson && p && p->handler != 0 && !o.appointment)
        return bad(p->label() + " is run by " + handlerName(*p) + ". Take the case back yourself first (Contacts & Agents).");
    switch (o.type) {
        case O_FUNCTION:
            if (b == 0) return "Receptions, lectures and society meetings are afternoon or evening affairs.";
            break;
        case O_CASE:
            if (!s || s->retired) return "Choose a site or route.";
            break;
        case O_SDR:
            if (o.route >= 0) { Site* rt = site(o.route); if (!rt || rt->type != ST_ROUTE || rt->retired) return "Choose one of your prepared routes, or improvise."; }
            break;
        case O_MEET_OVERT:
            if (!p || !p->live()) return "Choose a contact.";
            if (p->stage == AGENT) return bad("Never meet a recruited agent openly. Use his communications plan.");
            break;
        case O_MEET_CLAN:
            if (!p || !p->live()) return "Choose a contact.";
            if (!s || s->type != ST_MEET || s->retired) return "Choose a meeting site.";
            if (p->stage == AGENT) {
                if (p->nextMeet != day || b != 2)
                    return bad(p->label() + " can only be met on the agreed date (" + (p->nextMeet < 0 ? "none agreed" : fmtDay(p->nextMeet) + ", evening")
                               + "). To see him sooner, place a call-out signal or send a radio instruction.");
                Site* ag = agreedSite(*p);
                if (ag && ag->id != s->id) return p->label() + " will go to the agreed site: " + ag->name + ".";
            }
            if (o.pitch >= 0 && p->stage == AGENT) return p->label() + " is already recruited.";
            break;
        case O_LOAD_DROP:
            if (!s || s->type != ST_DROP || s->retired) return "Choose a dead drop.";
            if (!p || p->stage != AGENT) return "Choose the agent the load is for.";
            if (p->dropSite != s->id) return bad(p->label() + " does not know this drop. It must be in his communications plan (brief him at a meeting).");
            break;
        case O_UNLOAD_DROP:
            if (!s || s->type != ST_DROP) return "Choose a dead drop.";
            break;
        case O_READ_SIGNAL:
            if (!s || s->type != ST_SIGNAL) return "Choose a signal site.";
            break;
        case O_PLACE_SIGNAL:
            if (!p || p->stage != AGENT) return "Choose an agent.";
            if (p->signalSite < 0) return bad(p->label() + " has no signal site in his communications plan.");
            if (p->meetSites.empty()) return bad(p->label() + " has no meeting site in his communications plan.");
            break;
        case O_LINE_N: {
            Requirement* q = req(o.lineN);
            if (!q || q->closed || q->kind != RK_LINEN) return "Choose an open Line N task.";
            if (g.lineNPkgs < 1) return "The package for the illegal has not yet arrived by pouch.";
            if (!s || s->type != ST_DROP || s->retired) return "Choose a dead drop.";
            if (s->district != q->district) return std::string("Centre has told the illegal to look in ") + T().district[q->district] + ". The drop must be there.";
            if (s->casing < 2) return "Line N will only accept a drop you have cased at least twice.";
            if (s->uses > 0) return "Line N will only accept a drop that has never been used.";
            for (auto& x : g.persons) if (x.stage == AGENT && x.dropSite == s->id) return "That drop belongs to an agent's communications plan.";
            break;
        }
        case O_CONTROL: {
            Officer* of = officer(o.officer);
            if (!of || of->gone) return "Choose the one officer who is to be told of the operation.";
            if (!s || s->type != ST_DROP || s->retired) return "Choose a dead drop for the marked card.";
            for (auto& x : g.persons) if (x.stage == AGENT && x.dropSite == s->id) return "Never use an agent's drop as a marked card.";
            break;
        }
        default: break;
    }
    if (o.am >= 0) {
        Requirement* q = req(o.am);
        if (!q || q->closed || q->kind != RK_ACTIVE) return "That active measure is no longer open.";
    }
    if (o.cs) { auto it = g.csDays.find(day); if (it == g.csDays.end() || it->second.state != 2) return "No countersurveillance team has been arranged for that day. Arrange one first (Day Planner)."; }
    return "";
}

// ============================================================================ staff actions
std::string Sim::requestTrace(Person& p) {
    if (p.traceState == 1) return "A trace on " + p.name + " is already outstanding.";
    p.traceState = 1;
    queue(C_TRACE, p.id, 60, "Trace request: " + p.name);
    g.paperwork += 0.5f;
    file(p, "Trace requested from Centre.");
    return "Trace request on " + p.name + " drafted (60 groups). It joins the cipher queue.";
}

std::string Sim::requestSanction(Person& p) {
    if (p.stage == AGENT) return p.label() + " is already recruited.";
    if (p.sanction == 1) return "A recruitment proposal for " + p.name + " is already with Centre.";
    p.sanction = 1;
    queue(C_SANCTION, p.id, 200, "Recruitment proposal: " + p.name);
    g.paperwork += 3;
    file(p, "Recruitment proposal submitted to Centre for sanction.");
    return "Recruitment proposal for " + p.name + " drafted (200 groups, 3 hours of paperwork). Centre will take one to two weeks.";
}

std::string Sim::requestFunds() {
    for (auto& c : g.outQ) if (c.kind == C_FUNDS) return "A funds request is already in the cipher queue.";
    for (auto& x : g.pending) if (x.kind == P_FUNDS) return "A funds request is already with Centre.";
    queue(C_FUNDS, 0, 80, "Request for supplementary operational funds");
    g.paperwork += 1;
    return fstr("Request for a supplementary advance of %d %s drafted (80 groups).", P(100), T().currency);
}

void Sim::orderResupply() {
    if (g.resupplyOrdered) return;
    g.resupplyOrdered = true;
    pend(g.day + 18 + r().n(8), P_RESUPPLY, 0, 0);
}

std::string Sim::requestFilm() {
    if (g.stationFilm < 5) { orderResupply(); return "The technical officer has no film to spare. Stock is on order from Centre by pouch."; }
    g.stationFilm -= 5;
    pend(g.day + 1 + r().n(3), P_FILM, 0, 5);
    if (g.stationFilm < 10) orderResupply();
    return "Five Minox cassettes requisitioned from station stores. Issue in one to three days.";
}

std::string Sim::requestCamera() {
    if (g.stationCameras < 1) { orderResupply(); return "No camera in station stores. One has been requested from Centre; allow three weeks."; }
    g.stationCameras--;
    pend(g.day + 2 + r().n(4), P_CAMERA, 0, 1);
    return "One Minox camera requisitioned. The technical officer will check and issue it in two to five days.";
}

std::string Sim::requestContainers() {
    pend(g.day + 1 + r().n(2), P_CONTAINER, 0, 3);
    return "Three concealment containers ordered from the technical officer. Ready in a day or two.";
}

std::string Sim::requestPads() {
    for (auto& x : g.pending) if (x.kind == P_PADS) return "One-time pads are already on order.";
    if (g.stationPads >= 4) {
        g.stationPads -= 4;
        pend(g.day + 1 + r().n(2), P_PADS, 0, 4);
        return "Four one-time pad sheets drawn from the cipher clerk's reserve. Issue in a day or two.";
    }
    pend(g.day + 16 + r().n(8), P_PADS, 0, 8);
    return "The reserve is exhausted. Eight pad sheets have been ordered from Centre by pouch; allow three weeks.";
}

/** Two officers to watch your back: your own people if you have them, otherwise whoever the Rezident can spare. */
std::vector<int> Sim::pickTeam() {
    std::vector<int> own, others;
    for (auto& o : g.officers) {
        if (o.gone) continue;
        if (o.role == ROLE_SUB && o.assign == AS_SUPPORT) own.push_back(o.id);
        else if (o.role == ROLE_COLLEAGUE) others.push_back(o.id);
    }
    std::vector<int> team;
    while (team.size() < 2 && !own.empty()) { int i = r().n((int)own.size()); team.push_back(own[i]); own.erase(own.begin() + i); }
    while (team.size() < 2 && !others.empty()) { int i = r().n((int)others.size()); team.push_back(others[i]); others.erase(others.begin() + i); }
    return team;
}

std::string Sim::requestCS(int day) {
    if (day <= g.day) return "The team must be arranged at least a day ahead.";
    if (g.csDays.count(day)) return "You have already asked for " + fmtDay(day) + ".";
    int week = day / 7, n = 0;
    for (auto& kv : g.csDays) if (kv.first / 7 == week && kv.second.state == 2) n++;
    int role = T().role, own = 0;
    for (auto& o : g.officers) if (!o.gone && o.role == ROLE_SUB && o.assign == AS_SUPPORT) own++;
    bool ok;
    std::string why;
    if (role >= 2) {                       // you detail your own officers
        ok = own >= 2 || (role == 3 && own >= 1);
        if (!ok) why = "You have fewer than two officers assigned to support. Change their assignments on the Station screen.";
        else if (n >= 4) { ok = false; why = "Your officers have covered four acts this week already and are showing themselves too often."; }
    } else {
        int limit = 2 + (own > 0 ? 1 : 0);
        ok = n < limit && pct(25 + g.standRez * 0.75f + (own > 0 ? 15 : 0));
        if (!ok) why = n >= limit ? "Refused: you have had your share of the team that week." : "Refused. The Rezident needs the officers elsewhere that day.";
    }
    CsDay c;
    c.state = ok ? 2 : 3;
    if (ok) {
        std::vector<int> team = pickTeam();
        if (team.size() < 2) { c.state = 3; g.csDays[day] = c; return "There are not two officers free to form a team that day."; }
        c.o1 = team[0]; c.o2 = team[1];
        g.csDays[day] = c;
        return "Arranged. " + officer(c.o1)->title() + " and " + officer(c.o2)->title() + " will cover your route on " + fmtDay(day)
               + ". Tick 'countersurveillance' on that day's operational order. They will see where you go and whom you meet.";
    }
    g.csDays[day] = c;
    return why;
}

std::string Sim::cableSummary(Roll& x) {
    if (x.state != R_READY) return "Only developed material awaiting the pouch can be summarised by cable.";
    if (x.cabled) return "Already in the cipher queue.";
    x.cabled = true;
    queue(C_SUMMARY, x.id, 150, fstr("Summary of material No. %d", x.id));
    g.paperwork += 1.5f;
    return "Summary drafted (150 groups, 1.5 hours). Faster, but Centre grades a summary lower than the documents.";
}

std::string Sim::terminate(Person& p) {
    if (!p.live()) return "Nothing to do.";
    clearAppointments(p);
    p.stage = p.stage == AGENT ? TERMINATED : BROKEN;
    p.nextMeet = -1; p.nextDrop = -1; p.tempo = 0;
    for (int id : p.held) if (Roll* x = roll(id)) x->state = R_LOST;
    p.held.clear();
    file(p, "Contact terminated on your decision.");
    if (controlled(p)) g.you.heat = clampf(g.you.heat + 4, 0, 100);
    return p.label() + ": contact terminated. File closed.";
}

std::string Sim::setHandler(Person& p, int officerId) {
    if (!p.live()) return "The file is closed.";
    if (p.handler == officerId) return "No change.";
    if (officerId != 0) {
        Officer* o = officer(officerId);
        if (!o || o->gone || o->role != ROLE_SUB) return "Only an officer under your own command can be given a case.";
        int n = 0;
        for (auto& x : g.persons) if (x.live() && x.handler == officerId) n++;
        if (n >= 5) return o->title() + " already has five cases. More and he will make mistakes on all of them.";
        witness(p.witting, officerId);
        for (int id : p.meetSites) if (Site* s = site(id)) witness(s->witting, officerId);
        if (Site* s = site(p.dropSite)) witness(s->witting, officerId);
        if (Site* s = site(p.signalSite)) witness(s->witting, officerId);
        if (o->assign != AS_HANDLE) o->assign = AS_HANDLE;
    }
    p.handler = officerId;
    p.pitchAuth = false;
    if (p.stage == AGENT) {
        p.rapport = clampf(p.rapport - 8, 0, 100); p.stress = clampf(p.stress + 6, 0, 100);
        // an agreed meeting moves from one diary to the other
        clearAppointments(p);
        if (officerId == 0) schedule(p);
    }
    file(p, officerId == 0 ? "Case taken back by you personally." : "Case handed to " + handlerName(p) + ".");
    return gx(&p, p.label() + (officerId == 0 ? " is now run by you personally." : " is now run by " + handlerName(p) + ". He meets him under the existing communications plan, pays him from line funds, and reports to you."));
}

std::string Sim::radioMsg(Person& p, int tempo, int reqId, bool callout) {
    if (p.stage != AGENT) return "Radio instructions are for recruited agents.";
    if (!p.radio) return gx(&p, p.label() + " has no receiving schedule. Issue him one, with pad sheets, at a meeting.");
    if (p.pads < 1) return gx(&p, p.label() + " has used all his pad sheets and cannot decipher a broadcast.");
    if (callout && p.meetSites.empty()) return gx(&p, p.label() + " has no meeting site in his plan.");
    queue(C_RADIO, p.id, 60, "Broadcast request: instruction to " + p.codename, tempo, reqId, callout ? 1 : 0);
    g.paperwork += 0.5f;
    return gx(&p, "Request drafted (60 groups). Centre will put the groups out on his schedule within a few days of receiving it. "
                  "You will not know whether he heard them until you see what he does.");
}

/** Tell the handling officer what the agent is to do. He passes it on at the next meeting. */
std::string Sim::instruct(Person& p, int tempo, int reqId) {
    if (p.stage != AGENT || p.handler == 0) return "Instructions through a handling officer are for agents run by one of your officers.";
    if (tempo >= 0) p.instrTempo = tempo;
    if (reqId > -2) p.instrReq = reqId;
    return gx(&p, handlerName(p) + " has your instruction and will give it to " + p.label() + " at the next meeting"
              + (p.nextMeet >= 0 ? " (" + fmtDay(p.nextMeet) + ")." : ". There is no meeting agreed: consider a broadcast or taking the case back."));
}

std::string Sim::denounce(Officer& o) {
    if (o.gone) return "He is no longer here.";
    if (o.role == ROLE_REZIDENT || o.role == ROLE_KR) return "You do not report the Rezident or the security officer to themselves. That is a matter for a different game.";
    if (g.krPending) return "Line KR already has one report from you under investigation.";
    g.krPending = true;
    queue(C_KR, o.id, 180, "Personal, for the head of Directorate K");
    g.paperwork += 2;
    o.file.push_back(pad(fmtShort(g.day), 11) + "You reported your suspicions of him to Directorate K.");
    return "Your report on " + o.title() + " is drafted in your own hand cipher (180 groups). Directorate K will take some weeks. "
           "If you are wrong you will have accused a comrade.";
}

// ============================================================================ appointments
void Sim::clearAppointments(const Person& p) {
    for (auto& kv : g.plan)
        for (auto& o : kv.second)
            if (o.set && o.appointment && o.person == p.id) o = Order();
}

/** Put the agent's next agreed meeting on your planner (or leave it to his handling officer). */
void Sim::schedule(Person& p) {
    clearAppointments(p);
    if (p.stage != AGENT || p.nextMeet < 0) return;
    // one officer cannot meet two agents on the same evening
    bool moved = true;
    while (moved) {
        moved = false;
        for (auto& q : g.persons)
            if (&q != &p && q.stage == AGENT && q.handler == p.handler && q.nextMeet == p.nextMeet) { p.nextMeet++; moved = true; }
    }
    Site* s = agreedSite(p);
    if (!s || p.handler != 0) return;
    Order o;
    o.type = O_MEET_CLAN; o.person = p.id; o.site = s->id; o.appointment = true;
    if (isSet(p.nextMeet, 2) && !order(p.nextMeet, 2).appointment)
        cable("Rezidentura duty officer", "Diary conflict",
              "Your planner already had an order for the evening of " + fmtDay(p.nextMeet) + ", when " + p.codename + " will be waiting at " + s->name
                  + ". The agreed meeting has replaced it.");
    setOrder(p.nextMeet, 2, o);
}

void Sim::fallback(Person& p) {
    p.callout = false;
    if (p.stage != AGENT) return;
    p.misses++;
    if (p.misses >= 3) {
        p.nextMeet = -1;
        clearAppointments(p);
        alert("Three consecutive meetings with " + p.label() + " have failed. Regular contact is lost; only a call-out signal, a broadcast or his dead drop remains.");
        file(p, "Regular contact lost after three failed meetings.");
        return;
    }
    p.meetIdx++;
    p.nextMeet = g.day + 7;
    schedule(p);
    Site* s = agreedSite(p);
    say("    Fall-back: same hour in seven days, " + (s ? s->name : std::string("?")) + " (" + fmtDay(p.nextMeet) + ").");
}

void Sim::nextAppointment(Person& p) {
    p.misses = 0; p.callout = false;
    if (p.meetEvery <= 0 && p.signalSite < 0 && !p.radio) {
        p.meetEvery = 60;
        say("    (Standing rules require a reserve meeting where there is neither signal site nor radio: set at 60 days.)");
    }
    if (p.meetEvery > 0 && !p.meetSites.empty()) {
        p.meetIdx++;
        p.nextMeet = g.day + p.meetEvery;
        schedule(p);
        Site* s = agreedSite(p);
        say("    Next meeting agreed: " + fmtDay(p.nextMeet) + ", evening, " + (s ? s->name : std::string("?")) + ".");
    } else {
        p.nextMeet = -1;
        clearAppointments(p);
        say("    No further personal meeting agreed. Contact by dead drop, signal and radio only.");
    }
}

// ============================================================================ assessment
void Sim::learn(Person& p, int i, float power) {
    float k = power * (0.5f + noseNow() / 100.f);
    p.estC[i] = std::min(95.f, p.estC[i] + k * (100 - p.estC[i]) / 100.f);
    float noise = r().gauss() * (100 - p.estC[i]) * 0.22f;
    p.estV[i] = clampf(p.app[i] + noise, 0, 100);
}
void Sim::learnSome(Person& p, int n, float power) { for (int k = 0; k < n; k++) learn(p, r().n(N_ATTR), power); }

static const char* const TELLS[] = {
    "Note: he steered the talk twice towards your own duties at the Embassy.",
    "Note: he agreed to everything without asking what the risks were.",
    "Note: he knew the way to the meeting place better than he should have.",
    "Note: his account of his office routine does not match what he said last time.",
    "Note: he was not curious about why a diplomat should want this. Most people are.",
    "Note: he was early, and had chosen the seat with the view of the door."};

void Sim::tells(Person& p) {
    float c = controlled(p) ? 4 + noseNow() * 0.08f : 1.2f;
    if (pct(c)) { std::string t = pick(TELLS); file(p, t); say("    " + t); }
}

static const char* const MOOD[] = {"calm and businesslike", "somewhat tense", "nervous - smoking continuously", "frightened"};
std::string Sim::mood(Person& p) {
    float s = controlled(p) ? rnd(8, 30) : p.stress + r().gauss() * 8;
    return word(s, MOOD, 4);
}

// ============================================================================ surveillance and the approach to an act
static const char* const WEAK[] = {
    "a grey saloon seen twice, possibly coincidence", "a man in a raincoat on two different platforms",
    "a woman with a shopping bag who changed buses when you did", "a taxi that idled too long at the kerb", "the same face in two shop windows"};
static const char* const STRONG[] = {
    "the same green van at three checkpoints", "a foot team of three leap-frogging through the side streets",
    "a man who ran for the train and then got off again at the next stop", "two cars taking turns behind the bus",
    "a watcher at the cinema corner muttering into a coat sleeve"};

bool Sim::csToday(const Order& o) const {
    if (!o.cs || !me()) return false;
    auto it = g.csDays.find(g.day);
    return it != g.csDays.end() && it->second.state == 2;
}

/** The officers who covered the act have seen the man and the place. */
void Sim::csWitness(Person* p, Site* s) {
    auto it = g.csDays.find(g.day);
    if (it == g.csDays.end()) return;
    for (int id : {it->second.o1, it->second.o2}) {
        if (p) witness(p->witting, id);
        if (s) witness(s->witting, id);
    }
}

/**
 * Everything that happens between leaving the Embassy and reaching the place.
 * A full route in the block before gives a much better chance of seeing watchers than the short check en route.
 */
Sim::Ap Sim::approach(const Order& o, int b, int district, Site* st) {
    Ap a;
    bool cov = A->covered[b];
    int q = A->teamQ[b];
    if (!cov && g.hot[district] && pct(25)) { cov = true; q = 55; }
    bool longSdr = me() ? (b > 0 && sdrDone[b - 1]) : (O->a.tradecraft > 30);
    Site* route = me() && longSdr ? site(sdrRoute[b - 1]) : nullptr;
    bool cs = csToday(o);
    float fat = fatigueNow();
    float fatPen = fat > 70 ? 15 : (fat > 50 ? 6 : 0);
    float det = (longSdr ? 66 : 36) + (A->tradecraft - 50) * 0.5f + A->street * 0.25f - q * 0.3f - fatPen
                + (cs ? 25 : 0) + (g.secBonusUntil >= g.day ? 10 : 0) + (st ? (st->sight - 50) * 0.15f : 0);
    if (longSdr && me()) {
        if (route) {
            det += (route->sight - 50) * 0.3f + route->casing * 2;
            if (route->exposure > 45) det -= (route->exposure - 45) * 0.5f;     // they know where you will turn
            if (route->burned) det -= 15;
        } else det -= 5;                                                        // made up as you went along
    }
    det = clampf(det, 4, 95);
    int signs = 0;
    if (cov) {
        float x = r().f() * 100;
        if (x < det) signs = 2; else if (x < det + (100 - det) * 0.35f) signs = 1;
        if (A->heat < 60) heat(longSdr ? 1.2f : 0.4f);   // a diplomat seen dry-cleaning himself is noted as an intelligence officer
    } else {
        float fa = clampf((cs ? 3 : 13) - A->tradecraft * 0.08f + fat * 0.05f, 1, 25);
        if (pct(fa)) signs = 1;
    }
    std::string how = std::string(longSdr ? (route ? "prepared route" : "improvised route") : "short check en route") + (cs ? ", countersurveillance team out" : "");
    if (signs == 0) a.txt = "SDR (" + how + "): no surveillance detected.";
    else if (signs == 1) a.txt = "SDR (" + how + "): POSSIBLE surveillance - " + pick(WEAK) + ".";
    else a.txt = "SDR (" + how + "): SURVEILLANCE CONFIRMED - " + pick(STRONG) + ".";
    a.abort = (signs == 2 && o.policy != 2) || (signs == 1 && o.policy == 0);
    if (a.abort) {
        a.txt += " Act aborted; the route was completed as an innocent errand.";
        if (me()) { g.mAborts++; g.totAborts++; }
        if (signs == 2) A->tradecraft = std::min(92.f, A->tradecraft + 0.25f);
    } else if (signs > 0) a.txt += me() ? " You pressed on." : " He pressed on.";
    a.observed = cov && !a.abort;
    a.signs = signs;
    if (me()) {
        SdrLog l;
        l.day = g.day; l.block = b; l.district = district; l.signs = signs; l.abort = a.abort;
        l.act = OTYPE_LABEL[o.type]; l.route = how + (route ? ": " + shortSite(route) : "");
        g.sdrLog.push_back(l);
    }
    A->tradecraft = std::min(92.f, A->tradecraft + 0.06f);
    A->street = std::min(95.f, A->street + 0.12f + T().dist[district] * 0.04f);
    return a;
}

/** The adversary has seen one of us with this person. */
void Sim::advNotices(Person& p) {
    if (!p.knownAdv) { if (pct(60)) p.knownAdv = true; return; }
    p.overtObserved++;
    if (p.stage == AGENT || controlled(p)) return;
    float x = r().f() * 100;
    if (p.empSec > 45 && x < 30) {
        p.stage = BROKEN;
        file(p, "Telephoned to cancel future engagements: his department has 'reminded staff to report contacts with Soviet-bloc officials'. Contact broken.");
        alert(p.name + " has broken off contact (see his file).");
    } else if (x < 38) {
        makeDangle(p);          // interviewed, and agreed to play along. You are told nothing.
    }
}

void Sim::stake(Site& s) { s.staked = true; s.burned = true; s.stakedDay = g.day; }

/** Caught with the material on the table. */
void Sim::flagrante(Person* p, Site* s) {
    std::string truth = p && p->dangle ? "Centre's post-mortem concludes that " + p->label() + " was under the control of the adversary from the first contact."
                        : p && p->doubled ? "Centre's post-mortem concludes that " + p->label() + " had been detected and turned some time before the arrest."
                        : "Centre's post-mortem concludes that the agent or the site had been under observation for some time.";
    pngCause = p && p->dangle ? 1 : (p && p->doubled ? 2 : (s && s->staked ? 4 : 3));
    if (me()) {
        say("    As the material changed hands men closed in from three sides. You were held for two hours until the Consul arrived, and photographed with the evidence on the table.");
        endPosting(END_PNG, "PERSONA NON GRATA",
                   "You were detained in the act of receiving secret material" + (p ? " from " + p->full() : std::string("")) + (s ? " at " + s->name : std::string(""))
                       + ".\n\nThe host government has declared you persona non grata. You have seven days to leave " + T().country + ". The newspapers have your photograph.\n\n" + truth);
        return;
    }
    // one of your officers
    Officer* o = O;
    o->gone = true;
    alert(o->title() + " WAS DETAINED IN THE ACT" + (p ? " with " + p->label() : std::string("")) + (s ? " at " + s->name : std::string("")) + ". He has been declared persona non grata.");
    g.standRez = clampf(g.standRez - 10 / (1 + 0.5f * T().role), 0, 100); g.standCentre = clampf(g.standCentre - 8 / (1 + 0.5f * T().role), 0, 100);
    g.you.heat = clampf(g.you.heat + 6, 0, 100);
    if (p && p->live()) {
        clearAppointments(*p);
        p->stage = controlled(*p) ? TERMINATED : ARRESTED;
        p->nextMeet = -1; p->nextDrop = -1;
        if (!controlled(*p)) g.totArrested++;
        file(*p, controlled(*p) ? "Exposed as an instrument of the adversary at the arrest of his case officer." : "ARRESTED together with his case officer.");
    }
    std::string took;
    for (auto& x : g.persons) if (x.live() && x.handler == o->id) { x.handler = 0; x.pitchAuth = false; if (x.stage == AGENT) schedule(x); took += (took.empty() ? "" : ", ") + x.label(); }
    Person* keep = subj; subj = nullptr;
    cable(T().role == 3 ? "Centre" : "Rezident", "Expulsion of " + o->title(),
          o->title() + " was detained by " + T().adversary + " in the act of an operational meeting and has been expelled.\n\n" + truth
              + (took.empty() ? "" : "\n\nHis cases revert to you personally: " + took + ". Assume everything he knew about them is known.")
              + "\n\nAn officer's failure is his chief's failure.");
    subj = keep;
}

// ============================================================================ resolving the day's orders
void Sim::spot(int sector, const std::string& how, int handler) {
    float dangle = T().dangle + (g.you.heat > 45 ? 0.08f : 0);
    Person p = makePerson(sector, dangle);
    p.handler = handler;
    if (handler) witness(p.witting, handler);
    g.persons.push_back(p);
    Person& q = g.persons.back();
    g.mSpotted++; g.totSpotted++;
    file(q, "Met " + how + ". " + q.job + ", " + q.employer + ".");
    alert("New contact " + how + ": " + q.name + fstr(", %d, ", q.age) + q.job + ", " + q.employer + ".");
}

void Sim::casingEstimate(Site& s) {
    float n = (3 - s.casing) * 11 + 3.f;
    s.estCover = clampf(s.cover + r().gauss() * n, 0, 100);
    s.estSight = clampf(s.sight + r().gauss() * n, 0, 100);
}

void Sim::resolve(const Order& o, int b) {
    actAs(nullptr);
    subj = (o.type == O_MEET_OVERT || o.type == O_MEET_CLAN || o.type == O_LOAD_DROP || o.type == O_PLACE_SIGNAL) ? person(o.person) : nullptr;
    switch (o.type) {
        case O_COVER:
            g.cover = std::min(100.f, g.cover + 1.6f); g.fatigue += 3;
            if (A->covered[b]) heat(-0.3f);
            if (pct(2.5f)) spot(pct(60) ? POLITICAL : r().n(4), "through your cover work", 0);
            break;
        case O_OFFICE: {
            float done = std::min(g.paperwork, 4.5f);
            g.paperwork -= done; g.fatigue += 2; g.cover = std::min(100.f, g.cover + 0.5f);
            say(fstr("  %.1f hours of reports written up. Outstanding: %.1f h.", done, g.paperwork));
            break;
        }
        case O_REST:
            g.fatigue = std::max(0.f, g.fatigue - 22);
            break;
        case O_FUNCTION: {
            g.fatigue += 6; g.cover = std::min(100.f, g.cover + 0.8f); g.paperwork += 0.5f;
            money(-std::min(g.funds, P(3)), std::string("Function - fares, drinks (") + SECTOR_SHORT[o.sector] + " circle)");
            if (A->covered[b]) heat(-0.2f);
            if (pct(20 + lang() * 0.2f)) {
                spot(o.sector, "at a function", 0);
                if (pct(10)) spot(o.sector, "at the same function", 0);
            } else {
                static const char* const DULL[] = {"A dull evening. Nobody of operational interest.", "Useful for your cover, nothing more.",
                                                   "You collected visiting cards from people with no access to anything."};
                say(std::string("  ") + pick(DULL));
            }
            lang() = std::min(95.f, lang() + 0.08f);
            break;
        }
        case O_CASE: caseSite(o, b); break;
        case O_SCOUT: scout(o, b); break;
        case O_SDR: {
            sdrDone[b] = true; sdrRoute[b] = o.route; g.fatigue += 9;
            Site* rt = site(o.route);
            if (rt) {
                say("  Two hours on " + rt->name + ", watching your own back.");
                rt->uses++; rt->lastUsed = g.day; rt->exposure += 14;
                if (A->covered[b] && pct(50)) rt->burned = true;
                if (rt->exposure > 45) say("    You have walked this route too often. A team that knows it need not follow you round it.");
            } else say("  Two hours of buses, shops and back streets, watching your own back.");
            if (b == 2 || !isOp(order(g.day, b + 1).type)) say("  (No operational act followed the route.)");
            break;
        }
        case O_MEET_OVERT: meetOvert(o, b); break;
        case O_MEET_CLAN: meetClan(o, b); break;
        case O_LOAD_DROP: loadDrop(o, b); break;
        case O_UNLOAD_DROP: unloadDrop(o, b); break;
        case O_READ_SIGNAL: readSignal(o, b); break;
        case O_PLACE_SIGNAL: placeSignal(o, b); break;
        case O_LINE_N: lineN(o, b); break;
        case O_CONTROL: controlOp(o, b); break;
        default: break;
    }
    subj = nullptr;
    g.fatigue = clampf(g.fatigue, 0, 100);
    heat(0);
}

void Sim::caseSite(const Order& o, int b) {
    Site* s = site(o.site);
    if (!s) { say("  Order void."); return; }
    bool route = s->type == ST_ROUTE;
    say(std::string(route ? "  Walking " : "  Casing ") + s->name + ".");
    tire(7 + T().dist[s->district] * 2);
    Ap a = approach(o, b, s->district, route ? nullptr : s);
    say("    " + a.txt);
    if (a.abort) return;
    // a pass to look at the place is the cheapest way to find men waiting at it
    if (s->staked && pct(35 + A->tradecraft / 3.f + s->sight / 4.f + s->casing * 8)) {
        std::string up = s->name;
        for (auto& ch : up) ch = (char)toupper((unsigned char)ch);
        alert("SOMETHING WRONG AT " + up + ": the same two men on the bench as you passed in each direction. You walked on. Treat the site as compromised.");
        paper(0.5f);
        return;
    }
    if (a.observed && pct(60)) s->burned = true;
    if (s->casing < 3) s->casing++;
    casingEstimate(*s);
    paper(0.5f);
    if (route) say(fstr("    Route known %d/3. Quality (cover for stops, chances to look back): %s.", s->casing, word(s->estSight, W_LEVEL, 5).c_str()));
    else say(fstr("    Casing level now %d/3. Natural cover %s, view of approaches %s.", s->casing, word(s->estCover, W_LEVEL, 5).c_str(), word(s->estSight, W_LEVEL, 5).c_str()));
    if (s->exposure > 55) say("    The place has been used too often: a regular would notice a stranger's habits here.");
}

void Sim::scout(const Order& o, int b) {
    bool route = o.siteType == ST_ROUTE;
    say(std::string("  Looking for a ") + (route ? "route through " : std::string(SITETYPE_LABEL[o.siteType]) + " in ") + T().district[o.district] + ".");
    tire(9 + T().dist[o.district] * 2);
    Ap a = approach(o, b, o.district, nullptr);
    say("    " + a.txt);
    if (a.abort) return;
    if (!pct(65 + A->street * 0.3f)) { say("    Nothing suitable found."); return; }
    Site s = makeSite(o.siteType, o.district);
    if (A->street > 35 || pct(A->tradecraft)) {      // an experienced eye picks the better of two candidates
        Site t = makeSite(o.siteType, o.district);
        if (t.cover + t.sight > s.cover + s.sight) s = t;
    }
    s.casing = 1;
    if (a.observed && pct(50)) s.burned = true;
    casingEstimate(s);
    g.sites.push_back(s);
    paper(0.5f);
    if (route) say("    Worked out: " + s.name + ". First impression of its quality: " + word(s.estSight, W_LEVEL, 5) + ".");
    else say("    Found: " + s.name + ". First impression: cover " + word(s.estCover, W_LEVEL, 5) + ", view of approaches " + word(s.estSight, W_LEVEL, 5) + ".");
}

void Sim::meetOvert(const Order& o, int b) {
    Person* pp = person(o.person);
    if (!pp || !pp->live() || pp->stage == AGENT) { say("  Order void: contact not available."); return; }
    Person& p = *pp;
    const char* what = b == 0 ? "Coffee" : (b == 1 ? "Lunch" : "Dinner");
    int cost = P(b == 0 ? 1 : (b == 1 ? 4 : 9));
    tire(5);
    if (!pct(55 + p.rapport * 0.45f) && !controlled(p)) {
        say(std::string("  ") + what + " with " + p.name + ": he telephoned his apologies. Another time.");
        return;
    }
    cost = std::min(cost, g.funds);
    money(-cost, std::string(what) + " with " + p.name);
    say(std::string("  ") + what + " with " + p.name + fstr(" (%d %s).", cost, T().currency));
    socialGain(p, 1.0f);
    if (o.favour) favour(p);
    if (p.live() && o.pay > 0) gift(p, o.pay);
    if (p.live() && o.am >= 0) placeAM(o, p);
    if (o.pitch >= 0) say("    You do not make a recruitment approach across a restaurant table. (Use a clandestine meeting.)");
    learnSome(p, 2, 18);
    tells(p);
    if (A->covered[b]) {
        heat(-0.2f);
        advNotices(p);
        // nobody runs a detection route to a lunch; but an officer with his eyes open sometimes notices the next table
        if (pct(12 + A->tradecraft * 0.35f)) {
            say("    You had company: a couple at the next table who ordered nothing and left when you did.");
            file(p, "A meeting with him was observed by the adversary.");
        }
    }
    p.lastContact = g.day; p.meetings++; g.mMeetings++; paper(1);
    if (me()) { g.assessment = std::min(92.f, g.assessment + 0.1f); lang() = std::min(95.f, lang() + 0.05f); }
    else O->nose = std::min(88.f, O->nose + 0.05f);
}

void Sim::socialGain(Person& p, float mult) {
    float lg = me() ? lang() : 60.f;
    float chem = 0.6f + lg / 200.f + (p.app[A_EGO] > 60 ? 0.15f : 0);
    float gain = rnd(4, 9) * chem * mult * (p.rapport > 60 ? 0.5f : 1);
    p.rapport = clampf(p.rapport + gain, 0, 100);
    if (p.stage == SPOTTED && p.rapport >= 25) {
        p.stage = DEVELOPING;
        file(p, "Relationship established. Moved to active development.");
    }
    say(std::string("    Relations: ") + word(p.rapport, W_RAPPORT, 5) + ".");
}

void Sim::favour(Person& p) {
    float best = std::max(p.app[A_MONEY], std::max(p.app[A_IDEO], p.app[A_EGO]));
    float chance = 0.5f * p.rapport + 0.3f * best + 8;
    if (controlled(p)) chance = 90;
    if (pct(chance)) {
        p.commitment = clampf(p.commitment + rnd(8, 18), 0, 100);
        learn(p, A_ACCESS, 22);
        static const char* const FAV[] = {"an internal telephone directory", "a trade journal not on public sale", "his written opinion on a matter in the newspapers",
                                          "an unclassified staff circular", "an introduction to a colleague"};
        std::string t = std::string("Agreed to a small favour (") + pick(FAV) + ").";
        file(p, t);
        say("    " + t + " Commitment: " + word(p.commitment, W_COMMIT, 5) + ".");
        if (pct(30)) credit(p.sector, rnd(2, 6), -1);
    } else {
        p.rapport = clampf(p.rapport - 4, 0, 100);
        say("    He turned the request aside. Too early, or the wrong request.");
        if (p.rapport < 18) {
            p.stage = BROKEN;
            file(p, "Took offence at a request and has ended the acquaintance.");
            alert(p.name + " has ended the acquaintance.");
        }
    }
}

void Sim::gift(Person& p, int amount) {
    int amt = std::min(amount, g.funds);
    if (amt <= 0) return;
    if (p.app[A_MONEY] < 30) {
        p.rapport = clampf(p.rapport - 3, 0, 100);
        learn(p, A_MONEY, 30);
        say("    He would not take the envelope and was embarrassed by it.");
        return;
    }
    money(-amt, "Gift to " + p.name + " (receipt held)");
    p.paidTotal += amt; p.paid90 += amt;
    p.commitment = clampf(p.commitment + std::min(16.f, amt / T().price * p.app[A_MONEY] / 100.f * 0.6f), 0, 100);
    p.app[A_VULN] = clampf(p.app[A_VULN] + 6, 0, 100);
    p.tru[A_VULN] = clampf(p.tru[A_VULN] + 6, 0, 100);
    learn(p, A_MONEY, 30);
    file(p, fstr("Accepted %d %s 'for his trouble' and signed for it.", amt, T().currency));
    say(fstr("    He accepted %d %s 'for his trouble' and signed a receipt. Commitment: %s.", amt, T().currency, word(p.commitment, W_COMMIT, 5).c_str()));
}

/** Service A: getting a prepared text into print through somebody with a name or a column. */
void Sim::placeAM(const Order& o, Person& p) {
    Requirement* q = req(o.am);
    if (!q || q->closed || q->kind != RK_ACTIVE) return;
    if (g.day - p.lastAmDay < 30) { say("    He has handled material for you too recently; a second piece so soon would mark him."); return; }
    paper(1);
    bool ctl = controlled(p);
    float chance = ctl ? 92 : clampf(p.app[A_INFL] * 0.5f + p.commitment * 0.4f + p.rapport * 0.15f - 14, 3, 92);
    if (!pct(chance)) {
        p.rapport = clampf(p.rapport - 5, 0, 100);
        learn(p, A_INFL, 20);
        say("    You showed him the Service A material. He would not touch it: 'I cannot put my name to that.'");
        return;
    }
    p.lastAmDay = g.day;
    if (ctl || !pct(p.tru[A_INFL] * 0.85f + 10)) {
        learn(p, A_INFL, 25);
        file(p, "Took Service A material (" + q->title + "). Nothing has appeared.");
        say("    He took the Service A material and promised to see what could be done. (Nothing came of it.)");
    } else {
        q->progress += 1;
        learn(p, A_INFL, 30);
        g.standCentre = clampf(g.standCentre + 1.5f, 0, 100);
        file(p, "PLACED Service A material (" + q->title + ").");
        say(fstr("    He took the Service A material. It has appeared, in his own words, where it will be read. Placements: %d of %d.", (int)q->progress, (int)q->need));
        if (q->progress >= q->need) {
            q->closed = true; g.totReqDone++;
            g.standCentre = clampf(g.standCentre + 6, 0, 100); g.standRez = clampf(g.standRez + 3, 0, 100);
            Person* keep = subj; subj = nullptr;
            cable("Centre", fstr("Requirement No. %d carried out", q->id), "Service A reports that the material on '" + q->title + "' has been taken up and repeated by third parties who do not know its origin. The measure is considered accomplished.\n\nCENTRE");
            subj = keep;
        }
    }
    if (pct(ctl ? 35 : 3 + (100 - p.tru[A_DISC]) * 0.06f)) {
        heat(6); g.standCentre = clampf(g.standCentre - 3, 0, 100);
        p.knownAdv = true;
        if (p.stage == AGENT && !ctl) p.suspicion += 12;
        alert("The piece handled by " + p.label() + " has been denounced in print as a Soviet fabrication, and questions are being asked about where it came from.");
    }
}

// ---------------------------------------------------------------------------- clandestine meeting
void Sim::meetClan(const Order& o, int b) {
    Person* pp = person(o.person);
    Site* sp = site(o.site);
    if (!pp || !sp || !pp->live()) { say("  Order void: contact not available."); return; }
    Person& p = *pp; Site& s = *sp;
    bool agent = p.stage == AGENT;
    attempted.insert(p.id);
    say("  Meeting with " + p.label() + " at " + s.name + ".");
    tire(9 + T().dist[s.district] * 3);
    Ap a = approach(o, b, s.district, &s);
    say("    " + a.txt);
    if (a.abort) {
        if (agent) { p.stress += 5; fallback(p); }
        return;
    }
    float show = agent ? 95 - p.stress / 5.f : 45 + p.rapport * 0.4f + p.commitment * 0.3f - (p.commitment < 15 ? 20 : 0);
    if (agent && p.callout) show = show * p.diligence / 100.f;
    if (controlled(p)) show = 98;
    if (!pct(show)) {
        say("    " + p.label() + " did not appear. After fifteen minutes the place was left.");
        s.exposure += 4;
        if (agent) fallback(p);
        else p.rapport = clampf(p.rapport - 2, 0, 100);
        paper(0.5f);
        return;
    }
    bool cs = csToday(o);
    bool observed = a.observed;
    if (agent && !controlled(p) && p.suspicion >= 50 && pct((p.suspicion - 35) * 1.5f)) {
        float seen = 18 + A->tradecraft / 4.f + p.craft / 3.f + (s.sight - 50) * 0.2f + (cs ? 20 : 0);
        if (pct(seen)) {
            alert(p.label() + " ARRIVED WITH COMPANY: two men off the same bus, one took the table by the door. No contact was made.");
            file(p, "SURVEILLANCE OBSERVED ON THE AGENT at a meeting. He must be assumed to be under investigation.");
            s.burned = true;
            fallback(p);
            paper(2);
            return;
        }
        observed = true;
    }
    if (s.burned && pct(50)) observed = true;
    if (controlled(p)) { s.burned = true; if (A->heat < 38) heat(38 - A->heat); }     // whoever meets their man is a case officer, and is carded as one
    if (cs) csWitness(&p, &s);
    if (!me()) { witness(p.witting, O->id); witness(s.witting, O->id); }

    // a controlled agent's masters choose their evening: once they are ready, most meetings are the one
    // (nothing can be "caught in the act" at a meeting where no material changes hands)
    bool handover = o.collect && !p.held.empty();
    if (agent && controlled(p) && p.trapDay >= 0 && g.day >= p.trapDay && handover && pct(60)) { flagrante(&p, &s); return; }
    if (agent && observed && p.suspicion >= 75 && handover && pct(35)) { flagrante(&p, &s); return; }

    say(std::string("    Meeting held (") + (o.longMeet ? "about two hours" : "ten minutes") + ").");
    if (agent) agentAgenda(o, p, s); else contactAgenda(o, p, s);
    if (g.phase != PH_PLAY) return;

    s.uses++; s.lastUsed = g.day; s.exposure += o.longMeet ? 12 : 6;
    if (s.exposure > 60 && pct(s.exposure - 60)) s.burned = true;
    if (observed) {
        heat(5);
        if (agent) p.suspicion += 18;
        advNotices(p);
    }
    p.lastContact = g.day; p.meetings++; g.mMeetings++; paper(2);
    if (me()) g.assessment = std::min(92.f, g.assessment + 0.12f);
    if (p.stage == AGENT) nextAppointment(p);
}

Roll& Sim::newRoll(Person& p) {
    Roll x;
    x.id = g.nextId++;
    x.agentId = p.id; x.sector = p.sector; x.reqId = p.reqId; x.shotDay = g.day; x.stateDay = g.day;
    x.genuine = !controlled(p);
    g.rolls.push_back(x);
    return g.rolls.back();
}

void Sim::agentAgenda(const Order& o, Person& p, Site& s) {
    (void)s;
    for (auto& w : p.warn) alert(p.label() + " " + w);
    for (auto& w : p.warn) file(p, "Agent " + w);
    p.warn.clear();
    say("    Agent's manner: " + mood(p) + ".");
    // material
    if (o.collect && !p.held.empty()) {
        int n = 0;
        for (int id : p.held) if (Roll* x = roll(id)) { x->state = R_YOU; x->stateDay = g.day; n++; }
        p.held.clear();
        say(fstr("    Received %d exposed cassette%s.", n, n == 1 ? "" : "s"));
    } else if (o.collect) say("    He brought no film.");
    else if (!p.held.empty()) say("    On your instruction no material changed hands. He is still holding exposed film.");
    if (o.longMeet) {
        Roll& x = newRoll(p);
        x.oral = true; x.frames = 0;
        x.value = x.genuine ? p.tru[A_ACCESS] * rnd(0.15f, 0.4f) : rnd(10, 30);
        x.state = R_YOU;
        paper(1.5f);
        say(fstr("    Oral debriefing taken; the written report will go forward as item No. %d.", x.id));
    }
    // money
    int amt = std::min(o.pay, g.funds);
    if (amt > 0) {
        money(-amt, "Remuneration, agent " + p.codename + " (receipt held)");
        p.owed -= amt; p.paidTotal += amt; p.paid90 += amt;
        if (p.tru[A_MONEY] > 40) p.rapport = clampf(p.rapport + std::min(6.f, amt * 3.f / (p.salary + P(10))), 0, 100);
        say(fstr("    Paid %d %s against receipt.", amt, T().currency) + (p.owed > 0 ? fstr(" Still owed: %d.", p.owed) : std::string("")));
    } else if (p.owed > 0) say(fstr("    He was not paid. Owed: %d %s.", p.owed, T().currency));
    // supplies
    int nf = std::min(o.film, g.film);
    if (nf > 0) { g.film -= nf; p.film += nf; say(fstr("    Issued %d fresh cassette%s. He now holds %d.", nf, nf == 1 ? "" : "s", p.film)); }
    if (o.camera && !p.camera) {
        if (g.cameras > 0) { g.cameras--; p.camera = true; say("    Issued a Minox camera and showed him how to load it."); file(p, "Issued Minox camera."); }
        else say("    There was no camera to give him.");
    }
    int np = std::min(o.pads, g.pads);
    if (np > 0) {
        g.pads -= np; p.pads += np;
        if (!p.radio) { p.radio = true; file(p, "Issued a one-way radio schedule and one-time pads. He owns a short-wave receiver."); }
        say(fstr("    Issued %d one-time pad sheet%s and rehearsed the broadcast schedule. He now holds %d.", np, np == 1 ? "" : "s", p.pads));
    } else if (o.pads > 0) say("    You had no pad sheets to give him.");
    // instruction
    if (o.train && o.longMeet) {
        p.craft = std::min(80.f, p.craft + 5 + A->tradecraft * 0.05f);
        say("    Security instruction given: routes to meetings, handling of the camera at work, what to say if questioned.");
    }
    if (o.tempo >= 0 && o.tempo != p.tempo) {
        p.tempo = o.tempo;
        say(std::string("    Tempo of collection set to: ") + TEMPO[p.tempo] + ".");
        file(p, std::string("Tempo set to ") + TEMPO[p.tempo] + ".");
    }
    if (o.reqId > -2 && o.reqId != p.reqId) {
        p.reqId = o.reqId;
        Requirement* q = req(p.reqId);
        say("    Tasked against: " + (q ? q->title : std::string("general collection")) + ".");
    }
    if (o.am >= 0) placeAM(o, p);
    commsPlan(o, p);
    float relief = (o.longMeet ? rnd(8, 16) : rnd(2, 5)) * (0.5f + p.rapport / 100.f);
    p.stress = clampf(p.stress - relief, 0, 100);
    p.rapport = clampf(p.rapport + (o.longMeet ? rnd(1, 4) : 0.5f), 0, 100);
    if (o.longMeet) learnSome(p, 2, 20);
    tells(p);
}

void Sim::commsPlan(const Order& o, Person& p) {
    bool ch = false;
    if (o.meetEvery >= 0 && o.meetEvery != p.meetEvery) { p.meetEvery = o.meetEvery; ch = true; }
    if (o.setMeetSites && !o.meetSites.empty()) {
        std::vector<int> ok;
        for (int id : o.meetSites) { Site* t = site(id); if (t && t->type == ST_MEET && !t->retired) ok.push_back(id); }
        if (!ok.empty() && ok != p.meetSites) { p.meetSites = ok; p.meetIdx = -1; ch = true; }
    }
    if (o.dropEvery >= 0 && o.dropEvery != p.dropEvery) { p.dropEvery = o.dropEvery; ch = true; }
    if (o.dropSite > -2 && o.dropSite != p.dropSite) { p.dropSite = o.dropSite; ch = true; }
    if (o.signalSite > -2 && o.signalSite != p.signalSite) { p.signalSite = o.signalSite; ch = true; }
    if (ch) {
        p.nextDrop = (p.dropEvery > 0 && p.dropSite >= 0) ? g.day + p.dropEvery : -1;
        say("    New communications plan briefed and repeated back: " + commsText(p) + ".");
        file(p, "Communications plan changed: " + commsText(p) + ".");
    }
}

void Sim::contactAgenda(const Order& o, Person& p, Site& s) {
    socialGain(p, o.longMeet ? 1.1f : 0.4f);
    if (o.favour) favour(p);
    if (!p.live()) return;
    if (o.pay > 0 && o.pitch < 0) gift(p, o.pay);
    if (o.am >= 0 && o.pitch < 0) placeAM(o, p);
    learnSome(p, o.longMeet ? 3 : 1, 20);
    tells(p);
    if (o.pitch >= 0) pitch(o, p, s);
}

void Sim::pitch(const Order& o, Person& p, Site& s) {
    paper(3);
    p.lastPitchDay = g.day;
    say(std::string("    RECRUITMENT APPROACH (") + PITCH[o.pitch] + ").");
    if (p.sanction != 2) {
        g.standRez -= 12; g.standCentre -= 5;
        alert("The approach was made WITHOUT Centre's sanction. There will be something said about it.");
    }
    static const int attr[] = {A_MONEY, A_IDEO, A_EGO, A_VULN};
    float motive = p.tru[attr[o.pitch]];
    float score = 0.40f * motive + 0.25f * p.rapport + 0.35f * p.commitment - 40;
    if (o.pitch == 3) score = 0.75f * motive + 0.15f * p.commitment - 30 - p.tru[A_NERVE] * 0.15f;
    float chance = clampf(score / 50.f * 100.f, 2, 92);
    bool yes = controlled(p) ? pct(92) : pct(chance);
    if (yes) {
        p.stage = AGENT;
        int n = (int)CODENAMES.size();
        p.codename = std::string(CODENAMES[g.codenameIdx % n]) + (g.codenameIdx >= n ? "-2" : "");
        g.codenameIdx++;
        p.commitment = std::max(p.commitment, 70.f);
        p.salary = P(o.pitch == 0 ? 20 + (int)std::lround(p.tru[A_MONEY] * 0.7f / 5) * 5 : (int)std::lround(p.tru[A_MONEY] * 0.3f / 5) * 5);
        p.owed = 0;
        p.tempo = o.tempo >= 0 ? o.tempo : 1;
        if (o.pitch == 3) { p.stress += 30; p.tru[A_HONEST] = clampf(p.tru[A_HONEST] - 25, 0, 100); p.rapport = clampf(p.rapport - 25, 0, 100); }
        for (auto& q : g.reqs) if (!q.closed && q.kind == RK_COLLECT && q.sector == p.sector) { p.reqId = q.id; break; }
        if (o.reqId > -2) p.reqId = o.reqId;
        if (controlled(p)) p.trapDay = g.day + 120 + r().n(280);
        if (p.meetSites.empty()) p.meetSites.push_back(s.id);
        if (p.meetEvery <= 0) p.meetEvery = 21;
        commsPlan(o, p);
        int amt = std::min(o.pay, g.funds);
        if (amt > 0) { money(-amt, "First payment, agent " + p.codename + " (receipt held)"); p.paidTotal += amt; p.paid90 += amt; say(fstr("    First payment: %d %s against a signed receipt.", amt, T().currency)); }
        int nf = std::min(o.film, g.film);
        if (nf > 0) { g.film -= nf; p.film += nf; }
        if (o.camera && g.cameras > 0) { g.cameras--; p.camera = true; }
        int np = std::min(o.pads, g.pads);
        if (np > 0) { g.pads -= np; p.pads += np; p.radio = true; }
        g.totRecruited++; g.standRez = clampf(g.standRez + 8, 0, 100); g.standCentre = clampf(g.standCentre + 4, 0, 100);
        std::string lower = PITCH[o.pitch];
        for (auto& ch : lower) ch = (char)tolower((unsigned char)ch);
        file(p, "RECRUITED" + (me() ? std::string("") : " by " + O->title()) + " on the basis of " + lower + ". Codename " + p.codename
                    + fstr(". Agreed remuneration %d %s a month.", p.salary, T().currency));
        alert(p.name + " has agreed to co-operate. Codename " + p.codename + fstr(". He asks %d %s a month.", p.salary, T().currency)
              + (p.camera ? "" : " He has no camera: until he is issued one he can only report orally."));
        return;
    }
    // refusal
    float report = (100 - p.tru[A_DISC]) * 0.5f + p.empSec * 0.2f + (o.pitch == 3 ? 20 : 0);
    if (pct(report)) {
        heat(10); g.standRez -= 8 / (1 + 0.5f * T().role);
        p.knownAdv = true;
        if (pct(30)) {     // the adversary sends him back
            makeDangle(p);
            p.commitment = 30;
            pend(g.day + 9 + r().n(14), P_RETURN, p.id, 0);
            file(p, "Refused the approach and left abruptly.");
            alert(p.name + " refused and left abruptly.");
        } else {
            p.stage = BROKEN;
            file(p, "Refused the approach. It is believed he reported it to his security officer.");
            alert(p.name + " refused. The Embassy has since received an oral protest about 'activities incompatible with diplomatic status'.");
        }
    } else if (pct(55)) {
        p.commitment = clampf(p.commitment - 15, 0, 100); p.rapport = clampf(p.rapport - 10, 0, 100);
        file(p, "Declined the approach but did not break contact. Asked for time.");
        alert(p.name + " declined - 'not now' - but will go on meeting.");
    } else {
        p.stage = BROKEN;
        file(p, "Refused the approach and ended the relationship. No sign that he reported it.");
        alert(p.name + " refused and does not wish to meet again.");
    }
}

// ---------------------------------------------------------------------------- dead drops and signals
void Sim::loadDrop(const Order& o, int b) {
    Site* sp = site(o.site);
    Person* pp = person(o.person);
    if (!sp || !pp) { say("  Order void."); return; }
    Site& s = *sp; Person& p = *pp;
    say("  Loading " + s.name + " for " + p.label() + ".");
    tire(8 + T().dist[s.district] * 3);
    Ap a = approach(o, b, s.district, &s);
    say("    " + a.txt);
    if (a.abort) return;
    if (s.staked && stakeoutSeen(o, s)) return;
    int cash = std::min(o.pay, g.funds), nf = std::min(o.film, g.film);
    bool cam = o.camera && g.cameras > 0;
    if (cash == 0 && nf == 0 && !cam && o.tempo < 0 && o.reqId == -2) { say("    Nothing to load."); return; }
    money(-cash, "Cash loaded in dead drop for " + p.codename);
    g.film -= nf; if (cam) g.cameras--;
    s.cash += cash; s.film += nf; s.camera = s.camera || cam;
    if (o.tempo >= 0) s.tempoNote = o.tempo;
    if (o.reqId > -2) s.reqNote = o.reqId;
    if (o.container && g.containers > 0) { g.containers--; s.container = true; }
    s.forAgent = p.id; s.loadedDay = g.day; s.uses++; s.lastUsed = g.day; s.exposure += 5;
    paper(1);
    if (csToday(o)) csWitness(&p, &s);
    say(std::string("    Loaded: ") + (cash > 0 ? fstr("%d %s, ", cash, T().currency) : std::string("")) + (nf > 0 ? fstr("%d cassettes, ", nf) : std::string(""))
        + (cam ? "Minox camera, " : "") + (o.tempo >= 0 || o.reqId > -2 ? "written instructions, " : "") + (s.container ? "in a concealment container." : "wrapped in oilcloth."));
    if (a.observed || s.staked) { stake(s); heat(8); }
}

/** True if the stake-out was noticed in time and the officer walked on. */
bool Sim::stakeoutSeen(const Order& o, Site& s) {
    float det = 25 + A->tradecraft / 3.f + s.sight / 4.f + s.casing * 8 + (csToday(o) ? 20 : 0);
    if (pct(det)) {
        std::string up = s.name;
        for (auto& ch : up) ch = (char)toupper((unsigned char)ch);
        alert("SOMETHING WRONG AT " + up + ": a van with misted windows, a courting couple who do not talk. No approach was made. Treat the site as compromised.");
        paper(1);
        return true;
    }
    return false;
}

void Sim::unloadDrop(const Order& o, int b) {
    Site* sp = site(o.site);
    if (!sp) { say("  Order void."); return; }
    Site& s = *sp;
    say("  Unloading " + s.name + ".");
    tire(8 + T().dist[s.district] * 3);
    Ap a = approach(o, b, s.district, &s);
    say("    " + a.txt);
    if (a.abort) return;
    bool has = !s.rolls.empty() || s.cash > 0 || s.film > 0 || s.camera;
    if (csToday(o)) csWitness(nullptr, &s);
    if (!me()) witness(s.witting, O->id);
    if (s.staked) {
        if (stakeoutSeen(o, s)) return;
        if (has && pct(50)) {              // as often as not they let it run, to see who else comes
            Person* owner = s.rolls.empty() ? person(s.forAgent) : (roll(s.rolls[0]) ? person(roll(s.rolls[0])->agentId) : nullptr);
            flagrante(owner, &s);
            return;
        }
        heat(20);
    }
    s.uses++; s.lastUsed = g.day; s.exposure += 5; paper(1);
    if (!has) { say("    The drop was empty."); if (a.observed) { s.burned = true; heat(6); } return; }
    int n = 0;
    for (int id : s.rolls) if (Roll* x = roll(id)) { x->state = R_YOU; x->stateDay = g.day; n++; }
    s.rolls.clear();
    if (n > 0) say(fstr("    Recovered %d exposed cassette%s.", n, n == 1 ? "" : "s"));
    if (s.cash > 0 || s.film > 0 || s.camera) {
        money(s.cash, "Uncollected dead-drop load recovered");
        g.film += s.film; if (s.camera) g.cameras++;
        say("    Your own earlier load had not been collected; it was taken back.");
        s.cash = 0; s.film = 0; s.camera = false; s.forAgent = -1; s.tempoNote = -1; s.reqNote = -2;
    }
    s.container = false;
    for (auto& p : g.persons) if (p.dropSite == s.id && p.signalSite >= 0) { Site* sg = site(p.signalSite); if (sg && sg->mark == 1) sg->mark = 0; }
    if (a.observed) {
        stake(s); heat(8);
        for (auto& p : g.persons) if (p.stage == AGENT && p.dropSite == s.id && !controlled(p)) p.suspicion += 10;
    }
}

void Sim::readSignal(const Order& o, int b) {
    Site* sp = site(o.site);
    if (!sp) { say("  Order void."); return; }
    Site& s = *sp;
    tire(3 + T().dist[s.district]);
    g.cover = std::min(100.f, g.cover + 0.6f);
    s.markSeen = s.mark; s.markSeenDay = g.day;
    if (A->covered[b] && pct(10)) s.burned = true;
    std::string t = s.mark == 0 ? "no mark." : (s.mark == 1 ? "the 'drop loaded' mark is up." : (s.mark == 2 ? "THE DANGER SIGNAL IS UP." : "your own call-out mark is still there."));
    if (s.mark == 2) alert("Signal site " + s.name + ": " + t); else say("  You walked past " + s.name + ": " + t);
}

void Sim::placeSignal(const Order& o, int b) {
    Person* pp = person(o.person);
    if (!pp || pp->signalSite < 0 || pp->stage != AGENT || !site(pp->signalSite)) { say("  Order void."); return; }
    Person& p = *pp;
    Site& s = *site(p.signalSite);
    tire(4 + T().dist[s.district]);
    s.mark = 3; s.uses++; s.lastUsed = g.day; s.exposure += 3;
    if (A->covered[b] && pct(40)) s.burned = true;
    p.misses = 0;
    p.nextMeet = g.day + 3; p.callout = true;
    schedule(p);
    Site* ag = agreedSite(p);
    say("  Call-out mark placed at " + s.name + ". If " + p.label() + " sees it he will come to " + (ag ? ag->name : std::string("?")) + " on the evening of " + fmtDay(p.nextMeet) + ".");
}

/** Line N: a package for an illegal you will never meet, in a drop that has never been used. */
void Sim::lineN(const Order& o, int b) {
    Site* sp = site(o.site);
    Requirement* q = req(o.lineN);
    if (!sp || !q || q->closed || g.lineNPkgs < 1) { say("  Order void."); return; }
    Site& s = *sp;
    say("  Line N task: loading " + s.name + ".");
    tire(8 + T().dist[s.district] * 3);
    Ap a = approach(o, b, s.district, &s);
    say("    " + a.txt);
    if (a.abort) return;
    if (s.staked && stakeoutSeen(o, s)) return;
    g.lineNPkgs--;
    paper(1);
    s.uses++; s.lastUsed = g.day;
    if (csToday(o)) csWitness(nullptr, &s);
    bool blown = a.observed || s.staked || s.burned;
    for (int id : s.witting) { Officer* of = officer(id); if (of && of->mole && of->moleDay <= g.day) blown = true; }
    q->closed = true;
    s.retired = true;                       // it belongs to Line N now
    say("    Package placed; safety signal set. The drop is struck off your list - it is Line N's from tonight.");
    // whether it went well you learn later, from Centre, and without explanation
    if (blown) { pend(g.day + 6 + r().n(20), P_SECRET, q->id, 2); heat(8); }
    else pend(g.day + 5 + r().n(10), P_SECRET, q->id, 1);
}

/**
 * The marked card. A drop is "loaded" with nothing, and exactly one colleague is told beforehand.
 * If men are waiting at the place, either the place was already known - or he told them.
 */
void Sim::controlOp(const Order& o, int b) {
    Site* sp = site(o.site);
    Officer* of = officer(o.officer);
    if (!sp || !of || of->gone) { say("  Order void."); return; }
    Site& s = *sp;
    say("  Control operation at " + s.name + ". Told beforehand: " + of->title() + " only.");
    witness(s.witting, of->id);
    if (of->mole && of->moleDay <= g.day && pct(of->cautious ? 35 : 75)) stake(s);
    tire(8 + T().dist[s.district] * 3);
    Ap a = approach(o, b, s.district, &s);
    say("    " + a.txt);
    if (a.abort) return;
    paper(1);
    s.uses++; s.lastUsed = g.day; s.exposure += 5;
    if (s.staked) {
        if (stakeoutSeen(o, s)) {
            of->file.push_back(pad(fmtShort(g.day), 11) + "CONTROL OPERATION at " + s.name + ": a stake-out was waiting. He alone had been told.");
            say("    Only " + of->title() + " had been told of this place and this hour. (Unless the place was known already.)");
            return;
        }
        heat(8);
    } else if (a.observed) { s.burned = true; heat(4); }
    of->file.push_back(pad(fmtShort(g.day), 11) + "Control operation at " + s.name + ": nothing seen at the site.");
    say("    Dummy package placed and recovered. Nothing seen at the site.");
}
