// sim_career.cpp - generators (people, sites, officers, requirements), the start and end of a
// posting, the years at Centre between postings, decisions, saving and loading.
#include "sim.h"

static std::string upper(std::string s) { for (auto& c : s) c = (char)toupper((unsigned char)c); return s; }

// ============================================================================ saving and loading
static const char MAGIC[4] = {'R', 'E', 'Z', '3'};

std::vector<uint8_t> Sim::save() {
    Ar a;
    a.writing = true;
    a.raw((void*)MAGIC, 4);
    a & g;
    return a.buf;
}

bool Sim::load(const std::vector<uint8_t>& data) {
    if (data.size() < 8 || std::memcmp(data.data(), MAGIC, 4) != 0) return false;
    Ar a;
    a.writing = false; a.buf = data; a.pos = 4;
    GameState t;
    a & t;
    if (!a.ok || t.theatre < 0 || t.theatre >= (int)THEATRES.size()) return false;
    g = t;
    rep = nullptr; subj = nullptr; attention = false;
    actAs(nullptr);
    attempted.clear(); gradeLines.clear();
    return true;
}

// ============================================================================ generators
std::string Sim::personName(bool maleOnly, bool& female) {
    const Theatre& t = T();
    for (int i = 0; i < 200; i++) {
        female = !maleOnly && r().n((int)(t.male.size() + t.female.size())) >= (int)t.male.size();
        const auto& first = female ? t.female : t.male;
        std::string n = std::string(first[r().n((int)first.size())]) + " " + t.last[r().n((int)t.last.size())];
        if (std::find(g.usedNames.begin(), g.usedNames.end(), n) == g.usedNames.end()) { g.usedNames.push_back(n); return n; }
    }
    return fstr("A. N. Other %d", g.nextId);
}

/** Create a new contact. dangleChance is the probability the adversary has put this person in your path. */
Person Sim::makePerson(int sector, float dangleChance) {
    const Theatre& t = T();
    Person p;
    p.id = g.nextId++;
    p.age = 24 + r().n(32);
    p.sector = sector;
    p.district = r().n(8);
    const JobT& j = t.jobs[sector][r().n((int)t.jobs[sector].size())];
    p.job = j.job; p.employer = j.employer;
    p.name = personName(j.maleOnly, p.female);
    p.empSec = clampf(j.sec + rnd(-8, 8), 2, 95);
    p.tru[A_ACCESS] = rnd((float)j.lo, (float)j.hi);
    p.tru[A_MONEY] = rnd(10, 92);
    p.tru[A_IDEO] = std::pow(r().f(), 1.6f) * 80 + 4;      // sympathy is scarce, and scarcer every year
    p.tru[A_EGO] = rnd(10, 92);
    p.tru[A_VULN] = std::pow(r().f(), 1.4f) * 75 + 4;
    p.tru[A_NERVE] = rnd(20, 92);
    p.tru[A_DISC] = rnd(22, 95);
    p.tru[A_HONEST] = rnd(30, 96);
    p.tru[A_INFL] = clampf(j.infl + rnd(-12, 12), 0, 97);
    p.diligence = rnd(45, 98);
    p.craft = rnd(5, 25);
    p.firstDay = g.day;
    for (int i = 0; i < N_ATTR; i++) p.app[i] = p.tru[i];
    if (p.tru[A_HONEST] < 50 && p.tru[A_MONEY] > 55 && pct(50)) {
        p.fabricator = true;
        p.app[A_ACCESS] = clampf(p.tru[A_ACCESS] + rnd(12, 28), 0, 97);
    }
    if (r().f() < dangleChance) makeDangle(p);
    for (int i = 0; i < N_ATTR; i++) { p.estV[i] = 50; p.estC[i] = 0; }
    p.estV[A_ACCESS] = (j.lo + j.hi) / 2.f; p.estC[A_ACCESS] = 20;
    p.estV[A_INFL] = (float)j.infl; p.estC[A_INFL] = 25;
    return p;
}

/** The adversary builds a dangle to look like exactly what a case officer hopes to find. */
void Sim::makeDangle(Person& p) {
    p.dangle = true;
    p.fabricator = false;
    p.app[A_ACCESS] = clampf(p.tru[A_ACCESS] + rnd(8, 20), 0, 96);
    if (r().coin()) { p.app[A_MONEY] = rnd(68, 90); p.app[A_IDEO] = rnd(10, 35); }
    else { p.app[A_IDEO] = rnd(62, 85); p.app[A_MONEY] = rnd(30, 55); }
    p.app[A_EGO] = rnd(55, 85);
    p.app[A_NERVE] = rnd(62, 85);
    p.app[A_DISC] = rnd(70, 92);
    p.app[A_HONEST] = rnd(72, 92);
    p.diligence = rnd(85, 99);
}

static std::string subst(std::string s, const std::string& key, const std::string& val) {
    size_t i = s.find(key);
    if (i != std::string::npos) s.replace(i, key.size(), val);
    return s;
}

Site Sim::makeSite(int type, int district) {
    const Theatre& t = T();
    Site s;
    s.id = g.nextId++;
    s.type = type; s.district = district;
    std::string name;
    for (int tries = 0; tries < 40; tries++) {
        if (type == ST_ROUTE) {
            int n = 0;
            for (auto& o : g.sites) if (o.type == ST_ROUTE) n++;
            name = fstr("Route %c: ", 'A' + (n % 26)) + t.street[district][r().n(3)] + ", then " + t.park[district] + ", then " + t.station[r().n(8)];
        } else {
            const auto& tp = type == ST_MEET ? t.meetT : (type == ST_DROP ? t.dropT : t.signalT);
            name = tp[r().n((int)tp.size())];
            name = subst(name, "%P", t.venue[r().n((int)t.venue.size())]);
            name = subst(name, "%S", t.street[district][r().n(3)]);
            name = subst(name, "%K", t.park[district]);
            name = subst(name, "%T", t.station[district]);
        }
        if (!name.empty() && name[0] >= 'a' && name[0] <= 'z') name[0] = (char)toupper(name[0]);
        name += std::string(" [") + t.district[district] + "]";
        bool clash = false;
        for (auto& o : g.sites) if (o.name == name) clash = true;
        if (!clash) break;
        name += fstr(" (%d)", s.id);
    }
    s.name = name;
    s.cover = clampf(rnd(25, 85) + t.dist[district] * 3, 5, 95);
    s.sight = rnd(20, 90);
    return s;
}

Site& Sim::inherit(int type, int district) {
    Site s = makeSite(type, district);
    s.casing = 2;
    s.exposure = rnd(15, 45);
    s.uses = 3 + r().n(9);
    casingEstimate(s);
    g.sites.push_back(s);
    return g.sites.back();
}

/** Issue the next requirement whose start month has arrived, or null if none is available. */
Requirement* Sim::nextRequirement() {
    const Theatre& t = T();
    int month = (g.day - g.postStart) / 30;
    int best = -1;
    for (int i = 0; i < (int)t.reqs.size(); i++) {
        if (std::find(g.usedReqTemplates.begin(), g.usedReqTemplates.end(), i) != g.usedReqTemplates.end()) continue;
        if (t.reqs[i].month > month) continue;
        if (best < 0 || r().f() < 0.35f) best = i;
    }
    if (best < 0) return nullptr;
    const ReqT& rt = t.reqs[best];
    g.usedReqTemplates.push_back(best);
    Requirement q;
    q.id = g.nextReqNo++;
    q.sector = rt.sector; q.title = rt.title; q.text = rt.text; q.kind = rt.kind;
    q.need = rt.kind == RK_COLLECT ? (float)std::lround(rt.need * 0.65f * (1 + 0.3f * t.role)) : (float)rt.need;
    q.issued = g.day;
    q.due = rt.span > 0 ? g.day + rt.span : -1;
    q.priority = rt.span > 0 ? 1 : 2;
    g.reqs.push_back(q);
    return &g.reqs.back();
}

std::string Sim::officerName() {
    for (int i = 0; i < 200; i++) {
        std::string n = std::string(RU_FIRST[r().n((int)RU_FIRST.size())]) + " " + RU_LAST[r().n((int)RU_LAST.size())];
        if (std::find(g.usedNames.begin(), g.usedNames.end(), n) == g.usedNames.end() && n.find(g.officer) == std::string::npos) { g.usedNames.push_back(n); return n; }
    }
    return "N. N.";
}

// ============================================================================ a new career, a new posting
void Sim::newCareer(uint64_t seed, const std::string& officerName_) {
    g = GameState();
    g.seed = seed;
    g.rng.s = seed * 0x9E3779B97F4A7C15ull + 12345;
    std::string n = officerName_;
    while (!n.empty() && n.back() == ' ') n.pop_back();
    while (!n.empty() && n.front() == ' ') n.erase(n.begin());
    if (!n.empty()) g.officer = n;
    rep = nullptr; subj = nullptr;
    actAs(nullptr);
    startPosting(0, true);
}

void Sim::startPosting(int theatre, bool first) {
    g.theatre = theatre;
    const Theatre& t = T();
    g.postStart = dayOf(t.sy, t.sm, t.sd);
    g.postEnd = dayOf(t.ey, t.em, t.ed);
    g.day = g.postStart;
    g.phase = PH_PLAY;
    rep = nullptr; subj = nullptr;
    actAs(nullptr);
    // --- a clean desk
    g.persons.clear(); g.sites.clear(); g.rolls.clear(); g.reqs.clear(); g.cables.clear(); g.outQ.clear(); g.pending.clear();
    g.officers.clear(); g.decisions.clear(); g.plan.clear(); g.reports.clear(); g.csDays.clear(); g.ledger.clear(); g.sdrLog.clear();
    g.zenit.clear(); g.krLog.clear(); g.usedReqTemplates.clear(); g.histDone.clear(); g.usedNames.clear();
    g.mSpotted = g.mMeetings = g.mAborts = g.mRolls = 0; g.mValue = 0;
    g.totSpotted = g.totRecruited = g.totRolls = g.totReqDone = g.totArrested = g.totAborts = 0; g.totValue = 0;
    g.you.street = 8;
    g.you.heat = g.you.peakHeat = clampf(t.heat0 + g.blown * t.blownHeat, 0, 80);
    g.fatigue = 10; g.cover = 70; g.standRez = 50; g.paperwork = 2; g.overdueDays = 0;
    if (first) g.standCentre = 45;
    g.funds = 0;
    money((int)std::lround(P(120) * (1 + 0.5f * t.subs)), "Balance taken over from your predecessor");
    g.film = 6 + 2 * t.role; g.cameras = t.role > 0 ? 1 : 0; g.containers = 3; g.pads = t.role > 0 ? 2 : 0; g.lineNPkgs = 0;
    g.stationFilm = 40; g.stationCameras = 2 + t.role; g.stationPads = 6; g.resupplyOrdered = false;
    g.secBonusUntil = -1; g.ryanPolicy = -1; g.krPending = false; g.warnedLow = false;
    g.a4Base = t.a4Base; g.a4 = t.a4Base;
    g.lastReqIssue = g.day; g.lastFundsReq = -9999; g.lastLineN = g.day - 60; g.lastApproach = g.day;

    // --- the station
    auto addOfficer = [&](int role, int rank) -> Officer& {
        Officer o;
        o.id = g.nextId++; o.role = role; o.rank = rank; o.name = officerName();
        o.a.tradecraft = rnd(30, 62); o.a.street = rnd(25, 60); o.a.heat = o.a.peakHeat = clampf(t.heat0 + rnd(-6, 12), 4, 60);
        o.nose = rnd(30, 62); o.diligence = rnd(55, 95);
        g.officers.push_back(o);
        return g.officers.back();
    };
    if (t.role < 3) addOfficer(ROLE_REZIDENT, 3);
    addOfficer(ROLE_KR, 2);
    addOfficer(ROLE_TECH, 1);
    for (int i = 0; i < t.colleagues; i++) addOfficer(ROLE_COLLEAGUE, r().n(3));
    for (int i = 0; i < t.subs; i++) {
        Officer& o = addOfficer(ROLE_SUB, std::max(0, std::min(g.rank - 1, r().n(3))));
        o.assign = i == 0 ? AS_SUPPORT : (i % 2 ? AS_SPOT : AS_HANDLE);
        if (t.subs == 1) o.assign = AS_SPOT;
        o.sector = i % N_SECTOR;
        if (t.role == 1) { o.a.tradecraft = rnd(24, 40); o.nose = rnd(25, 45); }      // a first-tour officer
    }
    // --- treachery. Most stations are clean. You do not know whether this one is.
    static const float MOLE[] = {22, 30, 40, 35};
    if (pct(MOLE[theatre % 4])) {
        std::vector<int> cand;
        for (size_t i = 0; i < g.officers.size(); i++) if (g.officers[i].role == ROLE_COLLEAGUE || g.officers[i].role == ROLE_SUB) cand.push_back((int)i);
        if (!cand.empty()) {
            Officer& o = g.officers[cand[r().n((int)cand.size())]];
            o.mole = true;
            o.moleDay = g.postStart + r().n(std::max(1, (g.postEnd - g.postStart) * 6 / 10));
        }
    }
    std::vector<int> subs;
    for (auto& o : g.officers) if (o.role == ROLE_SUB) subs.push_back(o.id);

    // --- inherited sites
    Site& m1 = inherit(ST_MEET, 4); int m1id = m1.id;
    int m2id = inherit(ST_MEET, 5).id;
    int d1id = inherit(ST_DROP, 7).id;
    int sgid = inherit(ST_SIGNAL, 0).id;
    if (pct(15)) site(d1id)->burned = true;
    if (t.role >= 1) inherit(ST_ROUTE, 3);
    int m3id = -1, d2id = -1, sg2id = -1;
    if (t.role >= 2) { m3id = inherit(ST_MEET, 6).id; d2id = inherit(ST_DROP, 6).id; sg2id = inherit(ST_SIGNAL, 1).id; inherit(ST_MEET, 7); }
    for (int i = 0; i < 4 + t.role; i++) nextRequirement();

    // --- the agents your predecessor hands over
    auto handOver = [&](int sector, int handler, int meetA, int meetB, int drop, int signal, int firstMeet) -> int {
        Person a = makePerson(sector, 0);
        a.fabricator = false;
        for (int i = 0; i < N_ATTR; i++) a.app[i] = a.tru[i];
        a.tru[A_ACCESS] = a.app[A_ACCESS] = clampf(a.tru[A_ACCESS], 50, 78);
        a.stage = AGENT;
        a.codename = CODENAMES[g.codenameIdx++ % CODENAMES.size()];
        a.rapport = rnd(30, 42); a.commitment = 80; a.stress = rnd(20, 40);
        a.suspicion = pct(12) ? rnd(28, 38) : rnd(0, 12);
        a.camera = true; a.film = 2; a.tempo = 2; a.craft = rnd(25, 40);
        a.salary = P(30 + r().n(5) * 5); a.owed = a.salary;
        a.meetEvery = 28; a.meetSites = {meetA, meetB};
        a.dropEvery = 14; a.dropSite = drop; a.signalSite = signal;
        a.nextMeet = g.day + firstMeet; a.nextDrop = g.day + firstMeet + 4;
        a.sanction = 2; a.traceState = 2; a.firstDay = g.day - 400; a.meetings = 14; a.lastContact = g.day - 23;
        a.handler = handler;
        if (handler) a.witting.push_back(handler);
        for (auto& q : g.reqs) if (q.kind == RK_COLLECT && q.sector == sector) a.reqId = q.id;
        for (int i = 0; i < N_ATTR; i++) { a.estC[i] = 45; a.estV[i] = clampf(a.app[i] + r().gauss() * 10, 0, 100); }
        a.file.push_back(gx(&a, pad(fstr("%d", dateOf(g.day).y - 1), 11) + "Recruited by your predecessor's line on a material basis. Photographs documents at his place of work with a Minox."));
        a.file.push_back(pad(fmtShort(g.day), 11) + (handler ? "Taken over by " + officer(handler)->title() + " under the existing communications plan." : std::string("Handed over to you by the departing officer.")));
        // the predecessor's friends at the station know of him
        for (auto& o : g.officers) if (o.role == ROLE_COLLEAGUE && pct(18)) a.witting.push_back(o.id);
        g.persons.push_back(a);
        schedule(g.persons.back());
        return a.id;
    };
    int mine = handOver(r().coin() ? MILITARY : SCITECH, 0, m1id, m2id, d1id, sgid, 5);
    std::string others;
    if (t.role >= 2) {
        int id2 = handOver(POLITICAL, subs.size() > 1 ? subs[1] : 0, m3id, m2id, d2id, sg2id, 9);
        others += person(id2)->codename + " (" + handlerName(*person(id2)) + ")";
        if (t.role >= 3) {
            int id3 = handOver(SECURITY, subs.size() > 2 ? subs[2] : 0, m2id, m3id, -1, -1, 12);
            person(id3)->dropEvery = 0;
            others += ", " + person(id3)->codename + " (" + handlerName(*person(id3)) + ")";
        }
    }
    // --- a developmental contact he also leaves you
    {
        Person c = makePerson(POLITICAL, 0.15f);
        c.stage = DEVELOPING; c.rapport = rnd(26, 36); c.commitment = rnd(8, 18);
        c.firstDay = g.day - 90; c.meetings = 4; c.lastContact = g.day - 20;
        for (int i = 0; i < N_ATTR; i++) { c.estC[i] = 25; c.estV[i] = clampf(c.app[i] + r().gauss() * 17, 0, 100); }
        c.file.push_back(pad(fmtShort(g.day - 80), 11) + "Met by your predecessor at a reception. Four lunches since. Assessment incomplete.");
        g.persons.push_back(c);
    }
    Person& a = *person(mine);
    Person& c = g.persons.back();
    std::string names;
    for (int id : subs) names += (names.empty() ? "" : ", ") + officer(id)->title();
    std::string rolePara =
        t.role == 0 ? "I expect: contact reports written up without delay, new contacts every month, and no heroics."
        : t.role == 1 ? "You are a senior officer now, and I attach " + names + " to you. He is in his first tour. Give him work (Station screen), watch what he does with it, and remember that his mistakes are entered against your name as well as his."
        : t.role == 2 ? "You head the line. Under you: " + names + ". You decide who spots, who handles which case, and who watches whose back (Station screen; Contacts & Agents for the cases). You will run fewer agents yourself than you would like. That is what the post means."
        : "You are appointed Rezident. Your operational officers: " + names + ". You answer to this department for everything they do and everything they fail to do. Nobody will approve your operations for you, and nobody will share the blame.";
    subj = &a;
    cable(t.role == 3 ? "Centre" : "Rezident", "Arrival - your duties",
          "Comrade " + g.officer + ",\n\nYou are posted to the " + t.city + " rezidentura under cover of " + t.cover + ", " + t.embassy + ". "
              "Your cover duties are real work and the Ambassador expects to see you doing them.\n\n" + rolePara + "\n\n"
              "You take over personally agent " + a.codename + " (" + a.job + ", " + a.employer + ") and one developmental contact, " + c.name + ". "
              + (others.empty() ? "" : "Also inherited by your officers: " + others + ". ")
              + "The files and the inherited operational sites are in your safe. Treat every inherited site as known through your predecessor's habits - re-case them and find your own.\n\n"
              "First meeting with " + a.codename + ": " + fmtDay(a.nextMeet) + ", evening, " + site(m1id)->name + ". He is owed last month's money.\n\n"
              + t.local + "\n\n" + (t.role == 3 ? "HEAD OF THE FOURTH DEPARTMENT" : "REZIDENT"));
    subj = nullptr;
    cable("Centre", "Requirements",
          "The current information requirements for your line are attached (see the Centre screen). Material is graded on receipt by pouch. "
          "Use the cipher channel only where the delay of the pouch would reduce the value of the information.\n\nCENTRE");
    attention = false;
    dawn();
}

// ============================================================================ the end of a posting
std::string Sim::summary() {
    int agents = 0;
    for (auto& p : g.persons) if (p.stage == AGENT) agents++;
    return std::string("POSTING RECORD - ") + RANK[g.rank] + " " + g.officer + ", " + T().city + ", " + fmtShort(g.postStart) + " to " + fmtShort(std::min(g.day, g.postEnd)) + "\n"
           + fstr("  Contacts spotted ............ %d\n  Recruitments ................ %d\n  Agents active at the end .... %d\n  Agents lost to arrest ....... %d\n"
                  "  Items of material graded .... %d\n  Total evaluated worth ....... %d\n  Requirements satisfied ...... %d\n  Operational acts aborted .... %d\n",
                  g.totSpotted, g.totRecruited, agents, g.totArrested, g.totRolls, (int)std::lround(g.totValue), g.totReqDone, g.totAborts)
           + "  Standing with the " + T().chief + ": " + word(g.standRez, W_STAND, 5) + "\n  Standing with Centre: " + word(g.standCentre, W_STAND, 5) + "\n";
}

void Sim::endPosting(int how, const std::string& title, const std::string& text) {
    const Theatre& t = T();
    PostRec rec;
    rec.theatre = g.theatre; rec.startDay = g.postStart; rec.endDay = std::min(g.day, g.postEnd); rec.how = how;
    rec.cause = how == END_PNG ? pngCause : 0;
    rec.spotted = g.totSpotted; rec.recruited = g.totRecruited; rec.arrested = g.totArrested; rec.rolls = g.totRolls;
    rec.reqDone = g.totReqDone; rec.aborts = g.totAborts; rec.value = g.totValue;
    for (auto& p : g.persons) {
        if (p.codename.empty()) continue;
        if (p.stage == AGENT) rec.agentsAtEnd++;
        std::string truth = p.dangle ? "A PROVOCATION - the adversary's man when recruited" : p.doubled ? "caught and TURNED by the adversary" : p.fabricator ? "genuine, but invented part of his material" : "genuine";
        rec.agents.push_back(pad(p.codename, 10) + pad(clip(p.name + ", " + p.job, 46), 48) + pad(STAGE_LABEL[p.stage], 16) + truth);
    }
    for (auto& o : g.officers) if (o.mole) rec.agents.push_back("  (" + o.title() + " of the rezidentura worked for the adversary from " + fmtShort(o.moleDay) + (o.gone ? "; he did not finish the tour.)" : "; he was never caught.)"));
    // what Centre weighs: product first, then requirements met, then the network you leave behind, less the people you lost
    float score = 30 * std::min(1.5f, g.totValue / t.valuePar) + g.totReqDone * 2 + std::min(g.totRecruited, 8) * 1.f + std::min(rec.agentsAtEnd, 6) * 2
                  - g.totArrested * 5 / (1 + 0.5f * t.role) + (g.standCentre - 50) * 0.25f;
    if (how == END_PNG) score -= 20;
    if (how == END_RECALLED) score -= 35;
    if (g.theatre == 2 && g.ryanPolicy == 0) score += 4;
    score = clampf(score, 0, 100);
    rec.score = score;
    rec.verdict = score >= 72 ? "Outstanding" : score >= 50 ? "Good" : score >= 30 ? "Satisfactory" : "Unsatisfactory";
    std::string notes;
    int before = g.rank;
    if (score >= 50 && g.rank < std::min(3, g.theatre + 1)) g.rank++;
    else if (score >= 30 && g.rank < g.theatre) g.rank++;
    if (g.theatre == 3 && score >= 72 && g.rank == 3) g.rank = 4;
    if (g.rank > before) notes += std::string("Promoted to ") + RANK[g.rank] + ".\n";
    if (score >= 72) rec.award = "Order of the Red Banner";
    else if (score >= 58) rec.award = "Order of the Red Star";
    if (!rec.award.empty()) { g.awards.push_back(fstr("%d  ", dateOf(g.day).y) + rec.award + " (" + t.city + ")"); notes += "Awarded the " + rec.award + ".\n"; }
    if (g.theatre == 2 && g.ryanPolicy == 0) notes += "A note on file records that the Washington line's RYAN returns of 1981-84 were later judged the more accurate.\n";
    rec.rankAtEnd = g.rank;
    if (score < 25) g.poorPostings++; else g.poorPostings = 0;
    if (how == END_PNG) { g.png++; g.blown++; }
    if (how == END_EXPELLED) g.blown++;
    if (how == END_RECALLED) g.recalls++;
    std::string record = summary();
    g.record.push_back(rec);
    g.standCentre = clampf(40 + (score - 45) * 0.35f, 20, 75);
    g.decisions.clear();
    attention = true;

    std::string eval = text + "\n\n------------------------------------------------------------\nCENTRE'S EVALUATION OF YOUR TOUR IN " + upper(t.city) + "\n\n"
                       + record + "\n  Verdict: " + rec.verdict + ".\n" + (notes.empty() ? "" : "  " + subst(notes, "\n", "\n  ")) + "\n"
                       "The files have been opened. What each of your agents really was is entered on the Career screen.\n";
    bool last = g.theatre + 1 >= (int)THEATRES.size();
    if (!last && g.png >= 3)
        finishCareer("TRANSFERRED TO THE RESERVE", eval + "\nThree governments have now expelled you by name. No country will accredit you and no cover will hold. You are transferred to the active reserve and given a post in a ministry's foreign-relations department, where you will read what other men bring home.");
    else if (!last && (g.recalls >= 2 || g.poorPostings >= 2))
        finishCareer("A DESK IN THE ARCHIVES", eval + "\nThe personnel directorate has concluded that you will not be sent abroad again. A desk in the archives awaits you, and the long afternoons that go with it.");
    else if (last) {
        float avg = 0;
        for (auto& x : g.record) avg += x.score;
        avg /= g.record.size();
        std::string fate = g.rank >= 4 ? "You kept your general's stars through the reorganisation. When the First Chief Directorate became a service of its own in December, you were given a department of it. The files you spent twenty years filling moved with you, three floors up."
            : (g.rank == 3 && avg >= 45) ? "You were kept on. In the new service the colonels who had run agents were worth more than the generals who had run meetings, and for a few years nobody had time to ask what any of it had been for."
            : "In the reductions of 1992 you were retired with the pension of your rank and a certificate. A bank in the Arbat took you on to look after what it called its security. The work was familiar.";
        finishCareer("MOSCOW, AUGUST 1991",
            eval + "\n------------------------------------------------------------\nMOSCOW, AUGUST 1991\n\n"            + std::string(how == END_TOUR ? "You came home in July." : "You had been home for some time, at a desk.") + " On the morning of the 19th of August there were tanks on the Garden Ring, and the Chairman of the Committee was a member of the State Committee for the State of Emergency. "
            "By the 22nd he was under arrest, and a crane was lifting the statue from its plinth in the square under the windows where you had reported for duty as a lieutenant.\n\n"
            "The First Chief Directorate sat out those three days at Yasenevo and issued no orders. In October it was detached from the Committee. In December the state you had served ceased to exist; "
            "your agents, those who were left, went on servicing their dead drops for a country that was no longer there.\n\n" + fate);
    } else {
        const Theatre& nx = THEATRES[g.theatre + 1];
        g.phase = PH_INTERLUDE;
        g.interludeTitle = title;
        g.interludeText = eval + "\n------------------------------------------------------------\n" + nx.tour + "\n\n"
                          "Next posting: " + nx.city + ", from " + fmtDay(dayOf(nx.sy, nx.sm, nx.sd)) + ", as " + nx.cover + "."
                          + (g.blown > 0 && nx.blownHeat > 0 ? "\n\nYou are known to the Western services by name and photograph. Expect attention from your first day." : "")
                          + "\n\nBefore you leave, the personnel directorate allows you one course:";
        g.interludeOpts = {"Advanced operational course at the Red Banner Institute (tradecraft)",
                           "Faculty of agent psychology (assessment of people)",
                           std::string("Language immersion: ") + (nx.lang == 0 ? "English" : "German")};
    }
}

void Sim::finishCareer(const std::string& title, const std::string& text) {
    g.phase = PH_OVER;
    g.overTitle = title;
    g.overText = text + "\n\n" + careerText();
    attention = true;
}

void Sim::chooseInterlude(int option) {
    if (g.phase != PH_INTERLUDE) return;
    const Theatre& nx = THEATRES[g.theatre + 1];
    if (option == 0) g.you.tradecraft = std::min(92.f, g.you.tradecraft + 8);
    else if (option == 1) g.assessment = std::min(92.f, g.assessment + 8);
    else g.langs[nx.lang] = std::min(95.f, g.langs[nx.lang] + 18);
    if (g.langs[nx.lang] < 35) g.langs[nx.lang] = 35;      // nobody is posted without the working minimum
    startPosting(g.theatre + 1, false);
}

// ============================================================================ decisions
void Sim::decide(int decisionId, int option) {
    Decision d;
    bool found = false;
    for (size_t i = 0; i < g.decisions.size(); i++) if (g.decisions[i].id == decisionId) { d = g.decisions[i]; g.decisions.erase(g.decisions.begin() + i); found = true; break; }
    if (!found) return;
    actAs(nullptr);
    subj = nullptr;
    switch (d.kind) {
        case DK_APPROACH:
            if (option == 0) {
                g.standRez = clampf(g.standRez + 3, 0, 100); g.paperwork += 4;
                g.you.heat = std::max(8.f, g.you.heat - 2);
                cable("Line KR", "Your report of an approach", "Your report is noted. You acted correctly. You will avoid the person concerned, vary your movements, and expect that for some weeks your own movements will be of interest to this Line as well as to the adversary.\n\nLINE KR");
            } else if (pct(35)) pend(g.day + 30 + r().n(150), P_SECRET, 0, 3);
            break;
        case DK_RYAN:
            g.ryanPolicy = option;
            break;
        case DK_ORPHANS: {
            int n = option == 0 ? 3 : (option == 1 ? 1 : 0);
            static const int sec[] = {POLITICAL, MILITARY, SECURITY};
            Site* ms = nullptr;
            for (auto& s : g.sites) if (s.type == ST_MEET && !s.retired && (!ms || s.exposure < ms->exposure)) ms = &s;
            if (!ms && n > 0) ms = &inherit(ST_MEET, 4);
            int msId = ms ? ms->id : -1;
            std::string got;
            for (int i = 0; i < n; i++) {
                Person a = makePerson(sec[i], 0);
                a.fabricator = false;
                for (int k = 0; k < N_ATTR; k++) a.app[k] = a.tru[k];
                a.tru[A_ACCESS] = a.app[A_ACCESS] = rnd(i == 0 ? 78.f : 62.f, 92);
                a.stage = AGENT; a.orphan = true;
                a.codename = CODENAMES[g.codenameIdx++ % CODENAMES.size()];
                a.rapport = rnd(18, 30); a.commitment = 85; a.stress = rnd(45, 65); a.craft = rnd(50, 70);
                a.camera = true; a.film = 2; a.tempo = 1;
                a.salary = P(40); a.owed = 0;
                a.meetEvery = 28; a.meetSites = {msId}; a.nextMeet = g.day + 10 + i * 3;
                a.sanction = 2; a.traceState = 2; a.firstDay = g.day; a.lastContact = g.day;
                a.exposeDay = pct(60) ? g.day + 30 + r().n(330) : -1;
                for (auto& q : g.reqs) if (!q.closed && q.kind == RK_COLLECT && q.sector == a.sector) { a.reqId = q.id; break; }
                for (int k = 0; k < N_ATTR; k++) { a.estC[k] = 30; a.estV[k] = clampf(a.app[k] + r().gauss() * 15, 0, 100); }
                a.file.push_back(gx(&a, pad(fmtShort(g.day), 11) + "TAKEN OVER from the friends' foreign service. Run by them for some years; never met by us before today. His name is in their records."));
                g.persons.push_back(a);
                schedule(g.persons.back());
                got += (got.empty() ? "" : ", ") + a.codename + " (" + a.job + ", " + a.employer + ")";
            }
            if (n > 0) cable("Centre", "Sources taken over from the friends", "Centre notes that you have taken over: " + got + ".\n\nThey are yours. So is the question of what the other side already knows about them.\n\nCENTRE");
            else g.standCentre = clampf(g.standCentre - 2, 0, 100);
            break;
        }
        default: break;
    }
}
