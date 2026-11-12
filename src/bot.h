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

    void plan() {
        const Theatre& t = s.T();
        int day = g.day;
        while (!g.decisions.empty()) s.decide(g.decisions[0].id, g.decisions[0].kind == DK_ORPHANS ? (int)(g.seed % 3) : (careful ? 0 : 1));
        // ---- stores and staff work
        if (g.film < 4) s.requestFilm();
        if (g.cameras < 1 && day % 7 == 0) s.requestCamera();
        if (g.containers < 1 && day % 11 == 0) s.requestContainers();
        if (t.role > 0 && g.pads < 2 && day % 30 == 0) s.requestPads();
        if (g.funds < s.P(40) && day % 9 == 0) s.requestFunds();
        if (day % 13 == 0) for (auto& x : g.rolls) if (x.state == R_READY && !x.cabled) { s.cableSummary(x); break; }
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
            o.train = p.craft < 60;
            o.longMeet = true;
            o.pads = (g.pads > 0 && p.pads < 1) ? 1 : 0;
            o.tempo = (careful && warned(p)) ? 0 : (careful ? 1 : 2);
            if (careful && (warned(p) || hotNow())) { o.collect = !warned(p); o.longMeet = !warned(p); o.policy = 0; }
            auto cs = g.csDays.find(day);
            o.cs = cs != g.csDays.end() && cs->second.state == 2;
            for (auto& q : g.reqs) if (!q.closed && q.kind == RK_COLLECT && q.sector == p.sector) { o.reqId = q.id; break; }
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
        bool wk = weekend(day);
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
            if (p.sanction == 2 && p.rapport > 48 && p.commitment > 28 && day - p.lastPitchDay > 60) ripe = &p;
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
                o.pitch = bi; o.pay = std::min(s.P(20), g.funds); o.camera = true; o.film = 2; o.tempo = careful ? 1 : 2;
                if (Site* d = best(ST_DROP, 1)) { o.dropSite = d->id; o.dropEvery = 14; }
                if (Site* sg = best(ST_SIGNAL, 1)) o.signalSite = sg->id;
                if (put(2, o)) sdrBefore(2);
            }
        }
        if (lunch && (!wk || desk)) {
            Order o = mk(O_MEET_OVERT);
            o.person = lunch->id;
            o.favour = lunch->rapport > 35 && lunch->commitment < 60;
            if (lunch->rapport > 45 && lunch->estV[A_MONEY] > 50 && g.funds > s.P(60) && day % 3 == 0) o.pay = s.P(10);
            if (lunch->estV[A_INFL] > 45 && lunch->commitment > 30) for (auto& q : g.reqs) if (!q.closed && q.kind == RK_ACTIVE) { o.am = q.id; break; }
            put(desk ? 2 : 1, o);
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

