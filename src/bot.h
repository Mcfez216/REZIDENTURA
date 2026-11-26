// bot.h - the test player's brain (see bot.cpp). Kept in a header so the UI test can let it play too.
#pragma once
#include "sim.h"
#include <cstdlib>

static inline bool has(const std::vector<int>& v, int x) { return std::find(v.begin(), v.end(), x) != v.end(); }

struct Bot {
    Sim& s;
    bool careful;
    GameState& g;
    Bot(Sim& sim, bool c) : s(sim), careful(c), g(sim.g) {}

    bool inPlan(int siteId) {
        for (auto& p : g.persons) if (p.stage == AGENT && (has(p.meetSites, siteId) || p.dropSite == siteId || p.signalSite == siteId)) return true;
        return false;
    }
    Site* best(int type, int minCasing, bool unusedOnly = false, int district = -1) {
        Site* b = nullptr;
        for (auto& x : g.sites) {
            if (x.type != type || x.retired || x.casing < minCasing) continue;
            if (unusedOnly && (x.uses > 0 || inPlan(x.id))) continue;
            if (district >= 0 && x.district != district) continue;
            if (!b || x.exposure < b->exposure) b = &x;
        }
        return b;
    }
    int count(int type, bool unused = false) { int n = 0; for (auto& x : g.sites) if (x.type == type && !x.retired && (!unused || (x.uses == 0 && !inPlan(x.id)))) n++; return n; }
    bool put(int b, Order o) { if (s.isSet(g.day, b)) return false; if (!s.validate(o, g.day, b).empty()) return false; s.setOrder(g.day, b, o); return true; }
    Order mk(int type) { Order o; o.type = type; o.policy = careful ? 1 : 2; return o; }
    void sdrBefore(int b) {
        if (b == 0 || s.isSet(g.day, b - 1)) return;
        Order o = mk(O_SDR);
        if (Site* rt = best(ST_ROUTE, 1)) if (rt->exposure < 40) o.route = rt->id;
        put(b - 1, o);
    }
    int queried(const Person& p) { int n = 0; for (auto& x : g.rolls) if (x.agentId == p.id && x.grade.find("queried") != std::string::npos) n++; return n; }
    bool warned(const Person& p) {
        int n = (int)p.file.size();
        for (int i = std::max(0, n - 4); i < n; i++) if (p.file[i].find("followed from") != std::string::npos || p.file[i].find("further conversation") != std::string::npos || p.file[i].find("clearance") != std::string::npos || p.file[i].find("security officer") != std::string::npos || p.file[i].find("searched") != std::string::npos || p.file[i].find("SURVEILLANCE") != std::string::npos || p.file[i].find("telephone clicks") != std::string::npos) return true;
        return false;
    }

    bool hotNow() { if (g.krLog.empty()) return false; const std::string& k = g.krLog.back(); return k.find("Regular coverage") != std::string::npos || k.find("priority target") != std::string::npos; }

    /** The training course: do what the instructor's card says. */
    void planCourse() {
        int day = g.day;
        auto at = [&](int d, int b, Order o) { if (!s.isSet(d, b) || !s.order(d, b).appointment) { o.policy = 1; std::string e = s.validate(o, d, b); if (e.empty()) s.setOrder(d, b, o); } };
        switch (g.tut) {
            case 1: { Order o = mk(O_SCOUT); o.district = 4; o.siteType = ST_MEET; at(day, 1, o); break; }
            case 2: { Order o = mk(O_CASE); o.site = g.tutMeet; at(day, 1, o); break; }
            case 3: { Order a = mk(O_SCOUT); a.district = 6; a.siteType = ST_DROP; at(day, 0, a);
                      Order b = mk(O_SCOUT); b.district = 1; b.siteType = ST_SIGNAL; at(day, 1, b); break; }
            case 4: { Order a = mk(O_SDR); at(day, 1, a); Order b = mk(O_CASE); b.site = g.tutDrop; at(day, 2, b); break; }
            case 5: { Order o = mk(O_MEET_OVERT); o.person = g.tutA; at(day, 1, o); break; }
            case 6: {
                s.requestFilm();
                Order a = mk(O_SDR); at(day + 1, 1, a);
                if (s.isSet(day + 1, 2) && s.order(day + 1, 2).appointment) {
                    Order m = s.order(day + 1, 2);
                    if (Person* b = s.person(m.person)) m.pay = b->owed;
                    m.train = true; m.set = true;
                    g.plan[day + 1][2] = m;
                }
                Order o = mk(O_OFFICE); at(day, 1, o);
                break;
            }
            case 7: { Order a = mk(O_SDR); at(day, 1, a); break; }
            case 8: { Order a = mk(O_READ_SIGNAL); a.site = g.tutSignal; at(day, 0, a); Order b = mk(O_SDR); at(day, 1, b);
                      Order c = mk(O_UNLOAD_DROP); c.site = g.tutDrop; at(day, 2, c); break; }
            case 9: {
                for (int id : {g.tutA, g.tutB}) if (Person* q = s.person(id)) for (auto& f : q->file) if (f.find("Note:") != std::string::npos && q->live()) { s.terminate(*q); break; }
                break;
            }
            default: break;
        }
    }

    /** The bot's answer to a question put to it. It never goes over to the other side. */
    int choose(const Decision& d) {
        int n = (int)d.opts.size();
        switch (d.kind) {
            case DK_SCENE: {                      // a pitch: the angle it had planned; an agent: calm him if he is frightened, else praise him
                Order o = s.order(g.day, std::max(0, std::min(2, d.ref)));
                if (n == 5) return o.pitch >= 0 && o.pitch < 4 ? o.pitch : 4;
                if (d.text.find("frightened") != std::string::npos || d.text.find("followed") != std::string::npos || d.text.find("searched") != std::string::npos) return 0;
                return careful ? (g.funds > s.P(60) ? 2 : 0) : 1;
            }
            case DK_STREET: return careful ? (d.title.rfind("SURVEILLANCE", 0) == 0 ? 0 : 1) : n - 1;   // careful: walk on from a confirmed team
            case DK_ORPHANS: return (int)(g.seed % 3);
            case DK_OP_OFFER: return careful || g.seed % 3 ? 0 : 1;
            case DK_OP_NAME: {                    // the officer a marked card caught, if any; otherwise nobody
                int i = 0;
                for (auto& x : g.officers) {
                    if (x.gone || !(x.role == ROLE_COLLEAGUE || x.role == ROLE_SUB || x.role == ROLE_TECH)) continue;
                    for (auto& f : x.file) if (f.find("stake-out was waiting") != std::string::npos) return i;
                    i++;
                }
                return n - 1;
            }
            case DK_APPROACH: if (d.ref == 1) return careful ? 0 : 1 + (int)(g.seed % 2);
                              return careful ? (g.seed % 3 == 0 ? 2 : 0) : (g.seed % 2 ? 2 : 1);
            case DK_PLAN: return careful ? 1 : 2;
            case DK_SUMMONS: return 0;
            case DK_WALKIN: if (d.title.find("answer") != std::string::npos) return d.text.find("GENUINE") != std::string::npos ? 0 : 2;
                            return careful ? 1 : 0;
            case DK_LEAVE: return careful ? (int)(g.day / 365 % 2) : 2;
            case DK_WIFE: return careful ? 0 : 1;
            case DK_GAME: return careful ? (g.gameRounds >= 3 ? 3 : 0) : (g.gameRounds >= 6 ? 3 : 1 + (int)((g.day / 7) % 2));
            case DK_GAME_OFFER: return careful ? 0 : 1;
            case DK_EVENT: return std::min(n - 1, careful ? 0 : 1 + (int)(g.day % 2));
            default: return std::min(n - 1, careful ? 0 : 1);
        }
    }
    void plan() {
        planDay();
        if (!s.training()) travel();
    }
    /** How to get to today's acts: the Embassy car to far places, a taxi now and then, meetings on foot. */
    void travel() {
        for (int b = 0; b < 3; b++) {
            if (!s.isSet(g.day, b)) continue;
            Order o = s.order(g.day, b);
            if (!Sim::isOp(o.type) || o.appointment) continue;
            Site* st = s.site(o.site);
            int d = st ? st->district : o.district;
            int far = s.T().dist[d];
            int want = 0;
            if (o.type == O_MEET_CLAN || o.type == O_LOAD_DROP || o.type == O_UNLOAD_DROP) want = (!careful && far >= 2 && g.day % 3 == 0) ? 2 : 0;
            else if (o.type == O_SCOUT) want = far >= 2 ? 1 : 0;                      // nothing there yet for the car to give away
            else want = careful ? 0 : (far >= 2 ? 1 : 0);
            if (want == o.travel) continue;
            o.travel = want;
            if (s.validate(o, g.day, b).empty()) s.setOrder(g.day, b, o);
        }
    }
    void planDay() {
        if (s.training()) { planCourse(); return; }
        const Theatre& t = s.T();
        int day = g.day;
        while (!g.decisions.empty()) s.decide(g.decisions[0].id, choose(g.decisions[0]));
        // an operation: a leak hunt is closed with a name a week before the day; a rescue starts with the documents
        if (Operation* op = s.activeOp()) {
            if (op->kind == OP_LEAK && g.day >= op->due - 7) { s.nameTraitor(); while (!g.decisions.empty()) s.decide(g.decisions[0].id, choose(g.decisions[0])); }
            if (op->kind == OP_RESCUE) if (Person* p = s.person(op->ref)) {
                if (p->exfil == 0) s.requestExfil(*p);
                // documents in the safe and his meeting a long way off: call him out
                if (p->exfil == 2 && p->stage == AGENT && p->signalSite >= 0 && !p->meetSites.empty() && (p->nextMeet < 0 || p->nextMeet > day + 5) && !s.isSet(day, 1)) {
                    Order c = mk(O_PLACE_SIGNAL); c.person = p->id; put(1, c);
                }
            }
        }
        // ---- stores and staff work
        if (g.film < 4) s.requestFilm();
        if (g.cameras < 1 && day % 7 == 0) s.requestCamera();
        if (g.containers < 1 && day % 11 == 0) s.requestContainers();
        if (t.role > 0 && g.pads < 2 && day % 30 == 0) s.requestPads();
        if (t.role > 0 && g.bugs == 0 && day % 45 == 0) s.requestBug();
        if (g.funds < s.P(40) && day % 9 == 0) s.requestFunds();
        if (day % 13 == 0) for (auto& x : g.rolls) if (x.state == R_READY && !x.cabled) { s.cableSummary(x); break; }
        // the officer's own affairs
        {
            Date dd = dateOf(day);
            if (careful) {
                if (dd.m >= 5 && dd.m <= 8 && g.leaveYear != dd.y && g.day > g.leaveTo && day % 7 == 0) s.bookLeave(day + 21);
                if (g.wifeHere && g.home < 48 && day % 5 == 0) s.buyHomeGift();
                if (g.purse >= 90 && day % 20 == 0) s.buyGoods();
                if (day % 50 == 0) for (int k = 0; k < 3; k++) if (!(g.assets & (1 << k)) && g.purse >= ASSET_COST[k] + 60) { s.buyAsset(k); break; }
                if (g.wifeSignal < 0 && g.home >= 50 && day % 15 == 0)
                    for (auto& p : g.persons) if (p.stage == AGENT && p.handler == 0 && p.signalSite >= 0) { s.setWifeSignal(p.signalSite); break; }
            } else {
                if (g.skim != 2) s.setSkim(2);
                if (g.purse >= 60 && day % 45 == 0) s.giftChief();
                // a short way with a contact who will not come of his own accord
                if (day % 9 == 0)
                    for (auto& p : g.persons)
                        if (p.stage == DEVELOPING && p.handler == 0 && p.kompState == 0 && p.meetings >= 3 && (p.estV[A_VULN] > 45 || p.estV[A_MONEY] > 55))
                            { s.prepareCompromise(p, p.estV[A_MONEY] > p.estV[A_VULN] ? 0 : 1); break; }
            }
        }
        // a safe flat for the agents, once there is money for one
        if (day % 30 == 0 && t.role > 0) {
            int flats = 0;
            for (auto& x : g.sites) if (x.flat && !x.retired) flats++;
            for (auto& x : g.pending) if (x.kind == P_FLAT) flats++;
            int dd = 4 + (int)(g.seed % 4);
            if (flats < (careful ? 1 : 2) && !s.eastSide(dd) && g.funds > s.flatCost(dd) * 6) s.rentFlat(dd);
        }
        // agents whose material Centre has queried are verified, and dropped if they fail it
        for (auto& p : g.persons) {
            if (p.stage != AGENT) continue;
            if (p.verify == 0 && queried(p) >= 1 && (careful || queried(p) >= 2)) s.requestVerify(p);
            auto filed = [&](const char* what) { for (auto& l : p.file) if (l.find(what) != std::string::npos) return true; return false; };
            if (p.verify == 3 && (p.verifyResult == "FALSE" || p.verifyResult == "INVENTED")) {
                // some are played back rather than dropped
                if (p.play == 1 || p.play == 2) continue;
                if (careful && p.verifyResult == "FALSE" && p.handler == 0 && (g.seed + p.id) % 2 == 0 && !filed("Proposed to Centre: that he is under")) s.proposePlayback(p);
                else s.terminate(p);
                continue;
            }
            // a way out, standing, for the agents worth it
            if (careful && p.handler == 0 && p.exfil == 0 && p.escape == 0 && p.signalSite >= 0 && p.estV[A_ACCESS] > 62 && day % 13 == 0 && !filed("Exfiltration requested")) s.requestExfil(p);
        }
        if (careful && day % 7 == 0)
            for (auto& p : g.persons) if (p.stage == DEVELOPING && p.handler == 0 && p.confid == 0 && p.meetings >= 4 && p.rapport >= 45) { s.registerContact(p); break; }
        if (!careful && day % 15 == 0) s.padReport();
        // ---- officers: spread the work
        std::vector<Officer*> subs;
        for (auto& o : g.officers) if (o.role == ROLE_SUB && !o.gone) subs.push_back(&o);
        if (!subs.empty()) {
            int mineAgents = 0;
            for (auto& p : g.persons) if (p.stage == AGENT && p.handler == 0) mineAgents++;
            for (auto& p : g.persons) {
                if (p.stage == AGENT && p.handler == 0 && mineAgents > 2 && subs.size() > 1) { if (s.setHandler(p, subs[1 + p.id % (subs.size() - 1)]->id).find("now run by") != std::string::npos) mineAgents--; }
                if (p.live() && p.handler != 0 && p.sanction == 2 && !p.pitchAuth) p.pitchAuth = true;
            }
            if (day % 60 == 0) for (size_t i = 0; i < subs.size(); i++) subs[i]->policy = careful ? 1 : 2;
            // bring an agent back off the ice once things have been quiet for a season
            if (day % 30 == 0) for (auto& p : g.persons) if (p.stage == AGENT && p.handler != 0 && p.tempo == 0 && !warned(p) && day - p.firstDay > 120) s.instruct(p, 1, -2);
        }
        for (auto& p : g.persons) {
            if (!p.live() || p.stage == AGENT) continue;
            if (p.traceState == 0 && (p.rapport > 22 || p.walkIn || p.handler != 0)) s.requestTrace(p);
            float conf = 0;
            for (int i = 0; i < A_INFL; i++) conf += p.estC[i];
            conf /= A_INFL;
            if ((p.sanction == 0 || (p.sanction == 3 && !p.traceBad && day % 40 == 0)) && p.traceState == 2 && !p.traceBad && conf > (careful ? 42 : 30) && p.rapport > 40) s.requestSanction(p);
            if (careful && p.traceBad) s.terminate(p);
        }
        for (auto& p : g.persons) if (p.live() && careful) {
            int notes = 0;
            for (auto& f : p.file) if (f.find("Note:") != std::string::npos || f.find("observed by the adversary") != std::string::npos || f.find("KNOWN TO THE ADVERSARY") != std::string::npos) notes++;
            if (queried(p) >= 2 || (queried(p) >= 1 && notes >= 1) || notes >= 2) s.terminate(p);
        }
        // ---- treachery: act on a positive marked card
        for (auto& o : g.officers) if (!o.gone && !g.krPending) for (auto& f : o.file) if (f.find("stake-out was waiting") != std::string::npos) { s.denounce(o); break; }
        // ---- countersurveillance for the next few evenings with a meeting
        if (careful) for (auto& p : g.persons) if (p.stage == AGENT && p.handler == 0 && p.nextMeet > day && p.nextMeet <= day + 2 && !g.csDays.count(p.nextMeet)) s.requestCS(p.nextMeet);

        if (g.fatigue > 62) { for (int b = 0; b < 3; b++) if (!s.isSet(day, b)) { Order o = mk(O_REST); s.setOrder(day, b, o); } }
        // ---- an agreed meeting this evening
        if (s.isSet(day, 2) && s.order(day, 2).appointment) {
            Order o = s.order(day, 2);
            Person& p = *s.person(o.person);
            o.policy = careful ? 1 : 2;
            o.pay = std::max(0, std::min(p.owed, g.funds));
            o.film = p.camera && p.film < 2 ? std::min(2, g.film) : 0;
            o.camera = !p.camera;
            o.train = true;
            o.standing = p.exfil == 2;
            o.present = !o.brush && s.presentDue(p, day);
            if (Operation* op = s.activeOp()) {
                if (op->kind == OP_RESCUE && op->ref == p.id && p.exfil == 2) { o.exfil = true; o.standing = false; o.longMeet = true; o.brush = false; }
            }
            o.trainWhat = p.craft < 55 ? 0 : (p.camera && p.skPhoto < 50) ? 1 : p.skSpot < 40 ? 3 : p.skComms < 50 ? 4 : 2;
            if (Site* at = s.agreedSite(p)) o.docs = at->flat && g.film >= 4 && !warned(p) && p.estV[A_ACCESS] > 40 && day % 2 == 0;
            o.longMeet = true;
            o.pads = (g.pads > 0 && p.pads < 1) ? 1 : 0;
            // a man about to be arrested is got out; a well-placed one is given a device for his office
            bool late = false;
            for (auto& f : p.file) if (f.find("followed from") != std::string::npos || f.find("further conversation") != std::string::npos || f.find("about to be ARRESTED") != std::string::npos) late = true;
            if (careful && late && p.exfil == 0) s.requestExfil(p);
            if (p.exfil == 2 && late) { o.exfil = true; o.standing = false; o.longMeet = true; o.brush = false; }
            if (g.bugs > 0 && p.bug == 0 && p.estV[A_ACCESS] > 62 && !late) { o.bug = true; o.longMeet = true; }
            o.tempo = (careful && warned(p)) ? 0 : (careful ? 1 : 2);
            if (careful && (warned(p) || hotNow())) { o.collect = !warned(p); o.longMeet = !warned(p); o.policy = 0; }
            auto cs = g.csDays.find(day);
            o.cs = cs != g.csDays.end() && cs->second.state == 2;
            for (auto& q : g.reqs) if (!q.closed && q.kind == RK_COLLECT && q.sector == p.sector) { o.reqId = q.id; break; }
            if (Operation* op = s.activeOp()) if (op->kind == OP_FLASH) if (Requirement* fq = s.req(op->ref)) if (!fq->closed && fq->sector == p.sector) o.reqId = fq->id;
            // move him on to sites of our own when we have them
            Site* a = nullptr; Site* b2 = nullptr;
            for (auto& x : g.sites) if (x.type == ST_MEET && !x.retired && x.casing >= 2 && x.exposure < 40) { if (!a) a = &x; else if (!b2) b2 = &x; }
            if (a && b2) { o.setMeetSites = true; o.meetSites = {a->id, b2->id}; }
            if (Site* d = best(ST_DROP, 2)) if (p.dropSite < 0 || (s.site(p.dropSite) && s.site(p.dropSite)->exposure > 50)) { o.dropSite = d->id; o.dropEvery = 14; }
            if (p.signalSite < 0) if (Site* sg = best(ST_SIGNAL, 1)) o.signalSite = sg->id;
            o.set = true;
            g.plan[day][2] = o;
            sdrBefore(2);
        }
        bool wk = s.dayOff(day);
        // Line KR says they are on to us: go quiet until they lose interest
        bool hot = false, veryHot = false;
        if (careful && !g.krLog.empty()) { const std::string& k = g.krLog.back(); hot = k.find("Regular coverage") != std::string::npos || k.find("priority target") != std::string::npos; veryHot = k.find("priority target") != std::string::npos; }
        if (hot && s.isSet(day, 2) && s.order(day, 2).appointment) { g.plan[day][2].policy = 0; }
        // cover first: an officer who is never at his desk is an officer the watchers follow
        bool desk = !wk && g.cover < 66;
        // ---- dead drops of my own agents
        for (auto& p : g.persons) {
            if (p.stage != AGENT || p.handler != 0 || p.dropEvery <= 0 || p.nextDrop < 0) continue;
            int since = day - (p.nextDrop - p.dropEvery);
            Site* sg = s.site(p.signalSite);
            if (since == 1 && sg && careful) { Order o = mk(O_READ_SIGNAL); o.site = sg->id; put(1, o); Order c = mk(O_CASE); c.site = p.dropSite; put(2, c); }
            if (since == 2 && !veryHot && (!careful || !sg || sg->markSeen == 1)) { Order o = mk(O_UNLOAD_DROP); o.site = p.dropSite; if (put(1, o)) sdrBefore(1); }
        }
        // ---- Line N
        for (auto& q : g.reqs) {
            if (q.closed || q.kind != RK_LINEN) continue;
            Site* d = best(ST_DROP, 0, true, q.district);
            if (!d) { Order o = mk(O_SCOUT); o.district = q.district; o.siteType = ST_DROP; put(wk ? 0 : 2, o); }
            else if (d->casing < 2) { Order o = mk(O_CASE); o.site = d->id; put(wk ? 1 : 2, o); }
            else if (g.lineNPkgs > 0) { Order o = mk(O_LINE_N); o.site = d->id; o.lineN = q.id; if (put(1, o)) sdrBefore(1); }
        }
        // ---- a marked card now and then
        if (day % 97 == 0 && careful && !hot) {
            Site* d = best(ST_DROP, 1, true);
            std::vector<Officer*> c;
            for (auto& o : g.officers) if (!o.gone && (o.role == ROLE_COLLEAGUE || o.role == ROLE_SUB)) c.push_back(&o);
            if (d && !c.empty()) { Order o = mk(O_CONTROL); o.site = d->id; o.officer = c[(day / 97) % c.size()]->id; if (put(1, o)) sdrBefore(1); }
        }
        // ---- my developmental contacts
        Person* ripe = nullptr; Person* lunch = nullptr;
        for (auto& p : g.persons) {
            if (!p.live() || p.stage == AGENT || p.handler != 0) continue;
            if (p.sanction == 2 && ((p.rapport > 48 && p.commitment > 28) || p.kompState == 3) && day - p.lastPitchDay > 60) ripe = &p;
            else if (day - std::max(p.lastContact, p.firstDay) >= 6 && (!lunch || p.rapport > lunch->rapport)) lunch = &p;
        }
        if (hot) { ripe = nullptr; if (veryHot) lunch = nullptr; }
        if (ripe) {
            Site* ms = best(ST_MEET, 1);
            if (ms) {
                Order o = mk(O_MEET_CLAN);
                o.person = ripe->id; o.site = ms->id;
                static const int at[] = {A_MONEY, A_IDEO, A_EGO};
                int bi = 0;
                for (int i = 1; i < 3; i++) if (ripe->estV[at[i]] > ripe->estV[at[bi]]) bi = i;
                o.pitch = ripe->kompState == 3 ? 3 : bi; o.pay = std::min(s.P(20), g.funds); o.camera = true; o.film = 2; o.tempo = careful ? 1 : 2;
                if (Site* d = best(ST_DROP, 1)) { o.dropSite = d->id; o.dropEvery = 14; }
                if (Site* sg = best(ST_SIGNAL, 1)) o.signalSite = sg->id;
                if (put(2, o)) sdrBefore(2);
            }
        }
        if (lunch && (!wk || desk)) {
            Order o = mk(O_MEET_OVERT);
            o.person = lunch->id;
            o.present = s.presentDue(*lunch, day);
            o.favour = lunch->rapport > 35 && lunch->commitment < 60;
            if (lunch->rapport > 45 && lunch->estV[A_MONEY] > 50 && g.funds > s.P(60) && day % 3 == 0) o.pay = s.P(10);
            if (lunch->estV[A_INFL] > 45 && lunch->commitment > 30) for (auto& q : g.reqs) if (!q.closed && q.kind == RK_ACTIVE) { o.am = q.id; break; }
            put(desk || lunch->kompState == 2 ? 2 : 1, o);
        }
        // ---- sites of our own
        int need = -1;
        if (count(ST_MEET) < 4) need = ST_MEET; else if (count(ST_DROP, true) < 2) need = ST_DROP; else if (count(ST_SIGNAL) < 2) need = ST_SIGNAL; else if (count(ST_ROUTE) < 2) need = ST_ROUTE;
        if (need >= 0 && !hot && (wk || (day % 5 == 0 && !desk))) { Order o = mk(O_SCOUT); o.siteType = need; o.district = 4 + (day % 4); put(wk ? 1 : 2, o); }
        for (auto& x : g.sites) if (!x.retired && x.casing < 2 && wk) { Order o = mk(O_CASE); o.site = x.id; if (put(0, o)) break; }
        // ---- people
        int live = 0;
        for (auto& p : g.persons) if (p.live() && p.stage != AGENT && p.handler == 0) live++;
        if (live < 4 && !wk && day % 2 == 0) { Order o = mk(O_FUNCTION); o.sector = (day / 2) % N_SECTOR; put(2, o); }
        if (g.paperwork > 6) { Order o = mk(O_OFFICE); if (wk) put(0, o) || put(1, o); else if (!desk || g.paperwork > 10) put(1, o) || put(2, o); else put(2, o); }
        // radio: put a warned agent on ice at once
        if (careful) for (auto& p : g.persons) if (p.stage == AGENT && p.radio && p.pads > 0 && p.tempo > 0 && warned(p) && day % 5 == 0) s.radioMsg(p, 0, -2, false);
    }
};

