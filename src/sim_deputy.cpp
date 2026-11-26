// sim_deputy.cpp - the deputy. A good officer's day is mostly routine: cover work, reports, rest, the agents'
// money and film, stores on order, a detection route before every act, the signal sites read. With the deputy on,
// all of that is put in order each morning, and the player is left with what needs a decision.
// He fills only blocks you have left to routine, and never changes an order you gave.
#include "sim.h"

void Sim::usualAgenda(Order& o, const Person& p, int day) const {
    o.collect = true;
    if (!o.brush) o.longMeet = true;
    o.pay = std::max(0, std::min(p.owed, g.funds));
    o.film = p.camera && p.film < 2 ? std::min(2 - p.film, g.film) : 0;
    o.camera = !p.camera && g.cameras > 0;
    const float lv[] = {p.craft, p.skPhoto, p.skElicit, p.skSpot, p.skComms};
    int weak = 0;
    for (int i = 1; i < 5; i++) if (lv[i] < lv[weak]) weak = i;
    o.train = o.longMeet && lv[weak] < 60;
    o.trainWhat = weak;
    o.present = !o.brush && presentDue(p, day);
}

void Sim::deputyPlan() {
    if (!g.deputy || g.phase != PH_PLAY || training() || g.deputyDay == g.day) return;
    g.deputyDay = g.day;
    std::vector<std::string> did;
    auto pending = [&](int kind) { for (auto& x : g.pending) if (x.kind == kind) return true; return false; };
    bool busy = g.day <= g.busyUntil || onLeave(g.day);
    // ---- stores, before they run out
    if (g.film < 4 && !pending(P_FILM)) { requestFilm(); did.push_back("film requisitioned"); }
    bool needCam = false;
    for (auto& p : g.persons) if (p.stage == AGENT && p.handler == 0 && !p.camera) needCam = true;
    if (needCam && g.cameras < 1 && !pending(P_CAMERA)) { requestCamera(); did.push_back("a camera requisitioned"); }
    if (g.containers < 1 && !pending(P_CONTAINER)) { requestContainers(); did.push_back("containers ordered"); }
    if (g.funds < P(40) && g.day - g.lastFundsReq > 60 && !pending(P_FUNDS)) {
        bool queued = false;
        for (auto& c : g.outQ) if (c.kind == C_FUNDS) queued = true;
        if (!queued) { requestFunds(); did.push_back("supplementary funds asked of Centre"); }
    }
    // ---- today: the agenda of an agreed meeting nobody has looked at, a route before every act
    for (int b = 0; b < 3 && !busy; b++) {
        if (!isSet(g.day, b)) continue;
        Order o = order(g.day, b);
        Person* p = person(o.person);
        if (o.appointment && o.type == O_MEET_CLAN && p && p->stage == AGENT && o.pay == 0 && o.film == 0 && !o.camera && !o.train && !o.brush) {
            usualAgenda(o, *p, g.day);
            setOrder(g.day, b, o);
            did.push_back("the agenda for " + p->label() + "'s meeting filled in");
        }
    }
    // a detection route before every act of the coming week, in the block before it, so that you see them in the planner
    int routes = 0;
    for (int d = g.day; d < g.day + 7 && d <= g.postEnd; d++)
        for (int b = 1; b < 3; b++) {
            if (d <= g.busyUntil || onLeave(d)) continue;
            int t = order(d, b).type;
            bool hot = t == O_MEET_CLAN || t == O_LOAD_DROP || t == O_UNLOAD_DROP || t == O_LINE_N || t == O_CONTROL;
            if (!hot || isSet(d, b - 1)) continue;
            Order r;
            r.type = O_SDR;
            Site* best = nullptr;                // the least-used prepared route; a route walked too often is worse than none
            for (auto& s : g.sites) if (s.type == ST_ROUTE && !s.retired && s.casing >= 1 && s.exposure < 40 && (!best || s.exposure < best->exposure)) best = &s;
            r.route = best ? best->id : -1;
            if (validate(r, d, b - 1).empty()) { setOrder(d, b - 1, r); routes++; }
        }
    if (routes) did.push_back(fstr("%d detection route%s booked before the week's acts", routes, routes == 1 ? "" : "s"));
    // ---- reports, rest, and the signal sites - in blocks left to routine
    auto freeBlock = [&](bool weekdayOnly) {
        if (busy) return -1;
        for (int b : {1, 0, 2}) {
            if (isSet(g.day, b)) continue;
            if (weekdayOnly && dayOff(g.day)) continue;
            if (b < 2 && isSet(g.day, b + 1) && Sim::isOp(order(g.day, b + 1).type)) continue;   // that one may want a route
            return b;
        }
        return -1;
    };
    if (g.paperwork > 6) {
        int b = freeBlock(false);
        if (b >= 0) { Order o; o.type = O_OFFICE; setOrder(g.day, b, o); did.push_back(fstr("%.0f hours of reports written up", std::min(g.paperwork, 4.5f))); }
    }
    if (g.fatigue > 65) {
        int b = freeBlock(false);
        if (b >= 0 && order(g.day, b).type != O_REST) { Order o; o.type = O_REST; setOrder(g.day, b, o); did.push_back("an afternoon off: you are tired"); }
    }
    for (auto& p : g.persons) {
        if (p.stage != AGENT || p.handler != 0) continue;
        Site* sg = site(p.signalSite);
        if (!sg || sg->retired || (g.wifeHere && g.wifeSignal == sg->id)) continue;
        int since = sg->markSeenDay < 0 ? 99 : g.day - sg->markSeenDay;
        int every = p.escape >= 2 ? 3 : (p.dropEvery > 0 ? std::max(3, p.dropEvery / 2) : 99);
        if (since < every) continue;
        int b = freeBlock(false);
        if (b < 0) break;
        Order o; o.type = O_READ_SIGNAL; o.site = sg->id;
        if (validate(o, g.day, b).empty()) { setOrder(g.day, b, o); did.push_back(p.label() + "'s signal site read"); }
        break;                                    // one walk a day is enough
    }
    if (!did.empty()) {
        std::string s;
        for (size_t i = 0; i < did.size(); i++) s += (i ? "; " : "") + did[i];
        deputyLog = "Your deputy: " + s + ".";
    } else deputyLog.clear();
}
