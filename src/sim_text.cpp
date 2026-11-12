// sim_text.cpp - the texts the screens show. Nothing here changes the game.
// RULE: nothing marked HIDDEN in core.h may be printed here, except in careerText() after a posting is over.
#include "sim.h"

static std::string upper(std::string s) { for (auto& c : s) c = (char)toupper((unsigned char)c); return s; }
static std::string lower(std::string s) { for (auto& c : s) c = (char)tolower((unsigned char)c); return s; }
static const char* ROLE_NAME[] = {"case officer", "senior case officer", "head of line", "Rezident"};
static const char* OROLE[] = {"Rezident", "Security officer (Line KR)", "Technical officer", "Case officer (another line)", "Case officer (under you)"};

int Sim::unread() const { int n = 0; for (auto& c : g.cables) if (!c.read) n++; return n; }

std::string Sim::status1() {
    return upper(fmtDay(g.day)) + "    " + RANK[g.rank] + " " + g.officer + ", " + T().cover + "    " + T().city + " rezidentura (" + ROLE_NAME[T().role] + ")";
}
std::string Sim::status2() {
    static const char* const COVER[] = {"blown", "thin", "adequate", "good", "solid"};
    return fstr("Funds %d %s   |   Fatigue %d/100   |   Cover %s   |   Reports due %.1f h   |   %s: %s   |   Centre: %s",
                g.funds, T().currency, (int)std::lround(g.fatigue), word(g.cover, COVER, 5).c_str(), g.paperwork, T().role == 3 ? "Department" : T().chief,
                word(g.standRez, W_STAND, 5).c_str(), word(g.standCentre, W_STAND, 5).c_str());
}

std::string Sim::diary(int d) {
    std::string sb;
    auto add = [&](const std::string& s) { sb += (sb.empty() ? "" : "; ") + s; };
    if (dateOf(d).dow == 3) add("pouch closes");
    if (dateOf(d).d == 1) add("advance and accounts");
    auto cs = g.csDays.find(d);
    if (cs != g.csDays.end() && cs->second.state == 2) add("CS TEAM");
    for (auto& p : g.persons) if (p.stage == AGENT) {
        if (p.nextMeet == d) add(p.codename + " meeting" + (p.handler ? " (" + handlerName(p) + ")" : std::string("")));
        if (p.nextDrop == d) add(p.codename + " due at drop");
    }
    for (auto& q : g.reqs) if (!q.closed && q.due == d) add(fstr("Req. %d due", q.id));
    if (d == g.postEnd) add("END OF TOUR");
    return sb;
}

std::string Sim::siteNotes(const Site& s) {
    std::string sb;
    if (s.retired) sb += "struck off. ";
    if (s.type == ST_DROP) {
        if (s.cash > 0 || s.film > 0 || s.camera || s.tempoNote >= 0 || s.reqNote > -2) sb += "YOUR LOAD IS IN PLACE since " + fmtShort(s.loadedDay) + ". ";
    }
    if (s.type == ST_SIGNAL) {
        if (s.markSeenDay < 0) sb += "Not read yet. ";
        else sb += "Last read " + fmtShort(s.markSeenDay) + ": " + (s.markSeen == 0 ? "no mark" : s.markSeen == 1 ? "DROP LOADED" : s.markSeen == 2 ? "DANGER" : "your call-out") + ". ";
    }
    for (auto& p : g.persons) {
        if (p.stage != AGENT) continue;
        if (std::find(p.meetSites.begin(), p.meetSites.end(), s.id) != p.meetSites.end() || p.dropSite == s.id || p.signalSite == s.id) sb += "In " + p.codename + "'s plan. ";
    }
    if (!s.witting.empty()) sb += fstr("Known to %d other officer(s). ", (int)s.witting.size());
    if (s.exposure > 55) sb += "Over-used. ";
    return sb;
}

std::string Sim::wittingText(const std::vector<int>& w) {
    std::string sb;
    for (int id : w) if (Officer* o = officer(id)) sb += (sb.empty() ? "" : ", ") + o->title() + (o->gone ? " (gone)" : "");
    return sb.empty() ? "nobody but you and the " + std::string(T().role == 3 ? "department" : "Rezident") : sb;
}

std::string Sim::commsText(const Person& p) {
    std::string sb = p.meetEvery > 0 ? fstr("meetings every %d days", p.meetEvery) : std::string("no regular meetings");
    sb += fstr(" at %d site(s)", (int)p.meetSites.size());
    if (p.dropEvery > 0 && site(p.dropSite)) sb += fstr("; drop every %d days at ", p.dropEvery) + shortSite(site(p.dropSite));
    else sb += "; no dead drop";
    sb += site(p.signalSite) ? "; signal site " + shortSite(site(p.signalSite)) : std::string("; no signal site");
    return sb;
}

std::string Sim::dossier(const Person& p) {
    std::string sb;
    if (!p.codename.empty()) sb += "AGENT " + p.codename + "\n";
    sb += upper(p.name) + fstr(", %d\n", p.age) + p.job + "\n" + p.employer + "\n" + SECTOR_LABEL[p.sector] + "\nLives: " + T().district[p.district] + "\n\n";
    sb += std::string("Status: ") + STAGE_LABEL[p.stage] + "     First contact: " + (p.firstDay < g.postStart ? std::string("before your arrival") : fmtShort(p.firstDay)) + "\n";
    sb += fstr("Meetings: %d     Last contact: ", p.meetings) + (p.lastContact < 0 ? std::string("-") : fmtShort(p.lastContact)) + "\n";
    sb += "Relations: " + word(p.rapport, W_RAPPORT, 5) + "     Commitment: " + word(p.commitment, W_COMMIT, 5) + "\n";
    sb += std::string("Trace: ") + (p.traceState == 0 ? "not requested" : p.traceState == 1 ? "requested" : p.traceBad ? "ADVERSE - see file" : "answered - nothing adverse")
          + "     Sanction to recruit: " + (p.sanction == 0 ? "not requested" : p.sanction == 1 ? "with Centre" : p.sanction == 2 ? "GRANTED" : "refused") + "\n";
    sb += "Case officer: " + handlerName(p) + (p.handler && p.pitchAuth ? " (authorised to make the approach)" : "") + "\n";
    sb += "Also witting: " + wittingText(p.witting) + "\n";
    sb += "\nASSESSMENT             your estimate         confidence\n";
    static const char* const CONF[] = {"none", "poor", "fair", "good", "high"};
    for (int i = 0; i < N_ATTR; i++) {
        sb += "  " + pad(ATTR[i], 23);
        if (p.estC[i] < 5) { sb += "?  not assessed\n"; continue; }
        int v = (int)std::lround(p.estV[i]), bars = (int)std::lround(v / 10.f);
        sb += "[";
        for (int k = 0; k < 10; k++) sb += k < bars ? '#' : '.';
        sb += "] " + pad(fstr("%d", v), 4) + pad(fstr("+/-%d", (int)std::lround((100 - p.estC[i]) * 0.3f)), 9) + word(p.estC[i], CONF, 5) + "\n";
    }
    if (p.stage == AGENT) {
        Requirement* q = req(p.reqId);
        sb += std::string("\nHANDLING\n  Tempo: ") + TEMPO[p.tempo] + "\n  Tasked: " + (q ? fstr("No. %d ", q->id) + q->title + (q->closed ? " (CLOSED - retask)" : "") : std::string("general collection")) + "\n";
        sb += std::string("  Equipment: ") + (p.camera ? "Minox camera" : "NO CAMERA") + fstr("; fresh cassettes held: %d", p.film) + (p.camera && p.film == 0 ? " (HE CANNOT PHOTOGRAPH)" : "") + "\n";
        sb += std::string("  Radio: ") + (p.radio ? fstr("one-way schedule; pad sheets left: %d", p.pads) : std::string("none")) + "\n";
        sb += "  Agent's own tradecraft: " + word(p.craft, W_LEVEL, 5) + "\n";
        sb += fstr("  Remuneration: %d a month.  Owed: %d.  Paid to date: %d %s.\n", p.salary, std::max(0, p.owed), p.paidTotal, T().currency);
        sb += "\nCOMMUNICATIONS PLAN\n  Personal meetings: " + (p.meetEvery > 0 ? fstr("every %d days", p.meetEvery) : std::string("none regular")) + "\n";
        int cur = p.meetSites.empty() ? -1 : (int)((size_t)std::abs(p.meetIdx) % p.meetSites.size());
        for (int i = 0; i < (int)p.meetSites.size(); i++) { Site* s = site(p.meetSites[i]); sb += fstr("     %s %d) ", i == cur && p.nextMeet >= 0 ? ">" : " ", i + 1) + (s ? s->name : std::string("?")) + "\n"; }
        Site* ag = agreedSite(p);
        sb += "  Next agreed meeting: " + (p.nextMeet < 0 ? std::string("NONE") : fmtDay(p.nextMeet) + ", evening, " + (ag ? ag->name : std::string("?"))) + "\n";
        if (p.misses > 0) sb += fstr("  Failed meetings in a row: %d (three and regular contact is lost)\n", p.misses);
        sb += "  Dead drop: " + (p.dropEvery > 0 && site(p.dropSite) ? site(p.dropSite)->name + fstr(", serviced every %d days; next about ", p.dropEvery) + fmtShort(p.nextDrop) : std::string("none")) + "\n";
        sb += "  Signal site: " + (site(p.signalSite) ? site(p.signalSite)->name : std::string("none")) + "\n";
    }
    bool head = false;
    int inHand = 0;
    for (auto& x : g.rolls) {
        if (x.agentId != p.id || x.state == R_AGENT || x.state == R_DROP || x.state == R_LOST) continue;
        if (!head) { sb += "\nPRODUCTION (what has reached you)\n"; head = true; }
        if (x.state != R_GRADED) { inHand++; continue; }
        sb += "  No. " + pad(fstr("%d", x.id), 6) + pad(x.oral ? "oral" : "film", 6) + pad(fmtShort(x.stateDay), 11) + x.grade + "\n";
    }
    if (inHand > 0) sb += fstr("  %d item(s) still in the chain between you and Centre's evaluators.\n", inHand);
    sb += "\nFILE\n";
    for (auto& f : p.file) sb += "  " + f + "\n";
    return gx(&p, sb);
}

std::string Sim::officerFile(const Officer& o) {
    std::string sb = upper(o.title()) + "\n" + OROLE[o.role] + (o.gone ? "  -  NO LONGER AT THE STATION" : "") + "\n\n";
    if (o.role == ROLE_SUB) {
        sb += "Tradecraft: " + word(o.a.tradecraft, W_LEVEL, 5) + "     Judgement of people: " + word(o.nose, W_LEVEL, 5) + "     Knowledge of the city: " + word(o.a.street, W_LEVEL, 5) + "\n";
        sb += std::string("Assignment: ") + ASSIGN_LABEL[o.assign] + (o.assign == AS_SPOT ? std::string(" - ") + SECTOR_SHORT[o.sector] + " circle" : std::string("")) + "\n";
        sb += std::string("Abort policy: ") + POLICY[o.policy] + "\n\nHIS CASES\n";
        bool any = false;
        for (auto& p : g.persons) if (p.live() && p.handler == o.id) {
            any = true;
            sb += "  " + pad(p.full(), 34) + pad(STAGE_LABEL[p.stage], 12) + (p.stage == AGENT ? "next meeting " + (p.nextMeet < 0 ? std::string("NONE") : fmtShort(p.nextMeet)) : "relations " + word(p.rapport, W_RAPPORT, 5)) + "\n";
        }
        if (!any) sb += "  none\n";
    }
    sb += "\nWHAT HE KNOWS OF YOUR WORK\n";
    bool any = false;
    for (auto& p : g.persons) if (!p.codename.empty() || p.live()) {
        if (p.handler != o.id && std::find(p.witting.begin(), p.witting.end(), o.id) == p.witting.end()) continue;
        sb += "  " + p.full() + "\n"; any = true;
    }
    int ns = 0;
    for (auto& s : g.sites) if (std::find(s.witting.begin(), s.witting.end(), o.id) != s.witting.end()) ns++;
    if (ns) { sb += fstr("  %d operational site(s)\n", ns); any = true; }
    if (!any) sb += o.role == ROLE_REZIDENT ? "  everything you report\n" : "  nothing, so far as the record shows\n";
    sb += "\nNOTES\n";
    if (o.file.empty()) sb += "  none\n";
    for (auto& f : o.file) sb += "  " + f + "\n";
    return sb;
}

std::string Sim::storesText() {
    std::string sb = "YOUR SAFE\n";
    sb += fstr("  Operational funds ......... %d %s\n  Minox cassettes (fresh) ... %d\n  Minox cameras ............. %d\n  Concealment containers .... %d\n  One-time pad sheets ....... %d\n",
               g.funds, T().currency, g.film, g.cameras, g.containers, g.pads);
    if (g.lineNPkgs > 0) sb += fstr("  SEALED PACKAGE FOR LINE N . %d\n", g.lineNPkgs);
    sb += "\nSTATION STORES (technical officer)\n" + std::string("  Film: ") + (g.stationFilm >= 15 ? "adequate" : g.stationFilm >= 5 ? "low" : "NONE") + "     Cameras: " + (g.stationCameras > 0 ? "available" : "NONE")
          + "     Pads: " + (g.stationPads >= 4 ? "available" : "by pouch only") + (g.resupplyOrdered ? "     (resupply on order from Centre)" : "") + "\n";
    sb += "\nON ORDER / EXPECTED\n";
    bool any = false;
    for (auto& x : g.pending) {
        const char* w = x.kind == P_FILM ? "Film cassettes" : x.kind == P_CAMERA ? "Minox camera" : x.kind == P_CONTAINER ? "Concealment containers" : x.kind == P_FUNDS ? "Centre's answer on funds"
                        : x.kind == P_TRACE ? "Trace reply" : x.kind == P_SANCTION ? "Sanction decision" : x.kind == P_PADS ? "One-time pads" : x.kind == P_KR ? "Directorate K's findings"
                        : x.kind == P_RADIO ? "Broadcast to an agent" : x.kind == P_LINEN_PKG ? "Package for Line N" : nullptr;
        if (!w) continue;
        Person* p = person(x.ref);
        sb += std::string("  ") + w + ((x.kind == P_TRACE || x.kind == P_SANCTION || x.kind == P_RADIO) && p ? " (" + p->label() + ")" : std::string("")) + "\n";
        any = true;
    }
    if (!any) sb += "  nothing\n";
    sb += fstr("\nCIPHER QUEUE  (your allotment: %d groups a day; sent yesterday: %d)\n", cipherQuota(), g.groupsToday);
    if (g.outQ.empty()) sb += "  empty\n";
    for (auto& c : g.outQ) sb += "  " + pad(fstr("%d gr", c.groups), 9) + c.subject + "\n";
    sb += fstr("\nLAB: develops %d cassettes a day.   POUCH: leaves every Thursday night; four days to Moscow.\n", labPerDay());
    int n[9] = {};
    for (auto& x : g.rolls) if (x.state >= 0 && x.state < 9) n[x.state]++;
    sb += fstr("PIPELINE: with you %d | lab %d | awaiting pouch %d | in pouch %d | at Centre %d | graded %d | lost %d\n", n[R_YOU], n[R_LAB], n[R_READY], n[R_POUCH], n[R_CENTRE], n[R_GRADED], n[R_LOST]);
    sb += fstr("\nPAPERWORK OUTSTANDING: %.1f hours.  Over 8 hours is 'in arrears' and counts against you.\n", g.paperwork);
    return sb;
}

std::string Sim::stationText() {
    const Theatre& t = T();
    std::string sb = upper(std::string(RANK[g.rank]) + " " + g.officer) + "\n" + t.cover + "\n" + t.embassy + "\n";
    sb += fstr("Post: %s.   Day %d of this posting; the tour ends %s.   World number %llu.\n\n", ROLE_NAME[t.role], g.day - g.postStart + 1, fmtDay(g.postEnd).c_str(), (unsigned long long)g.seed);
    auto line = [&](const std::string& k, const std::string& v) { return "  " + pad(k + " ", 27, '.') + " " + v + "\n"; };
    sb += "PROFESSIONAL\n" + line("Tradecraft", word(g.you.tradecraft, W_LEVEL, 5)) + line("Assessment of people", word(g.assessment, W_LEVEL, 5))
          + line(t.lang == 0 ? "English" : "German", word(lang(), W_LEVEL, 5)) + line(std::string("Knowledge of ") + t.city, word(g.you.street, W_LEVEL, 5)) + "\n";
    sb += "CONDITION\n" + line("Fatigue", fstr("%d/100", (int)std::lround(g.fatigue))) + line("Cover credibility", fstr("%d/100", (int)std::lround(g.cover))) + "\n";
    sb += "STANDING\n  With the " + std::string(t.chief) + ": " + word(g.standRez, W_STAND, 5) + "\n  With Centre: " + word(g.standCentre, W_STAND, 5) + "\n\n";
    sb += "DISTRICTS (travel effort from the Embassy)\n";
    for (int i = 0; i < 8; i++) sb += "  " + pad(t.district[i], 20) + (t.dist[i] == 0 ? "on the doorstep" : t.dist[i] == 1 ? "near" : t.dist[i] == 2 ? "moderate" : "far - a long SDR in itself") + "\n";
    sb += "\nThe adversary's watchers are concentrated where their targets are, and thin out with distance from the centre. Distance costs you fatigue.\n";
    return sb;
}

// pad with a custom fill character (used for dotted leaders)
std::string pad(const std::string& s, int n, char fill) { std::string r = s; for (int i = ulen(s); i < n; i++) r += fill; return r; }

std::string Sim::survSummary() {
    const Theatre& t = T();
    std::string sb = "YOUR OWN OBSERVATIONS\n\nEvery approach to an operational act is entered here: what check you made, what you saw, what you did. "
                     "\"Nothing seen\" is not \"nobody there\" - a short check en route misses a good team more often than it finds one.\n\n";
    int n[8][3] = {}, tot[3] = {}, recent[3] = {};
    for (auto& l : g.sdrLog) {
        n[l.district][l.signs]++; tot[l.signs]++;
        if (g.day - l.day <= 60) recent[l.signs]++;
    }
    sb += "BY DISTRICT            approaches   possible   confirmed\n";
    for (int i = 0; i < 8; i++)
        sb += "  " + pad(t.district[i], 22) + pad(fstr("%d", n[i][0] + n[i][1] + n[i][2]), 13) + pad(fstr("%d", n[i][1]), 11) + fstr("%d\n", n[i][2]);
    sb += "  " + pad("ALL", 22) + pad(fstr("%d", tot[0] + tot[1] + tot[2]), 13) + pad(fstr("%d", tot[1]), 11) + fstr("%d\n", tot[2]);
    sb += "  " + pad("Last 60 days", 22) + pad(fstr("%d", recent[0] + recent[1] + recent[2]), 13) + pad(fstr("%d", recent[1]), 11) + fstr("%d\n", recent[2]);
    sb += "\nLINE KR - WEEKLY ASSESSMENTS OF YOUR POSITION (newest first)\n";
    if (g.krLog.empty()) sb += "  none yet - issued on Mondays\n";
    for (int i = (int)g.krLog.size() - 1; i >= 0 && i >= (int)g.krLog.size() - 26; i--) sb += "  " + g.krLog[i] + "\n";
    return sb;
}

std::string Sim::accSummary() {
    std::string sb = std::string("OPERATIONAL ACCOUNT - ") + RANK[g.rank] + " " + g.officer + "\n\n" + fstr("  Cash in your safe ......... %d %s\n\n", g.funds, T().currency);
    Date today = dateOf(g.day);
    for (int back = 0; back < 3; back++) {
        int m = today.m - back, y = today.y;
        while (m < 1) { m += 12; y--; }
        int in = 0, out = 0;
        for (auto& l : g.ledger) { Date d = dateOf(l.day); if (d.y == y && d.m == m) { if (l.amt > 0) in += l.amt; else out -= l.amt; } }
        sb += "  " + pad(std::string(MONTH[m]) + fstr(" %d", y), 18) + "received " + pad(fstr("%d", in), 7) + fstr("spent %d\n", out);
    }
    sb += "\nAGENTS                 agreed/month   owed now   paid to date\n";
    bool any = false;
    for (auto& p : g.persons) {
        if (p.codename.empty()) continue;
        any = true;
        sb += "  " + pad(p.codename + (p.stage == AGENT ? "" : " (closed)"), 22) + pad(fstr("%d", p.salary), 15) + pad(fstr("%d", std::max(0, p.owed)), 11) + fstr("%d\n", p.paidTotal);
    }
    if (!any) sb += "  none\n";
    sb += "\nMoney left in a dead drop is entered as spent on the day you load it. If the agent never collects it and you take it back, it is entered as received. If somebody else finds it, it is simply gone.\n\n"
          "The advance arrives on the 1st of the month: a basic sum (larger if you have officers under you) plus four-fifths of the remuneration agreed with your agents. "
          "The missing fifth, every lunch, and your officers' expenses come out of the basic. A supplementary advance can be asked of Centre (Material & Stores).\n";
    return sb;
}

std::string Sim::careerText() {
    std::string sb = std::string("SERVICE RECORD - ") + upper(std::string(RANK[g.rank]) + " " + g.officer) + "\nFirst Chief Directorate, Committee for State Security\n\n";
    if (g.record.empty()) sb += "First posting abroad. Nothing is yet entered.\n";
    static const char* const HOW[] = {"tour completed", "DECLARED PERSONA NON GRATA", "expelled with the rest of the station", "RECALLED"};
    for (auto& x : g.record) {
        const Theatre& t = THEATRES[x.theatre];
        sb += upper(t.city) + "  " + fmtShort(x.startDay) + " - " + fmtShort(x.endDay) + "   " + HOW[x.how] + "\n";
        sb += fstr("  Recruitments %d | agents at end %d | arrests %d | material graded %d (worth %d) | requirements met %d\n", x.recruited, x.agentsAtEnd, x.arrested, x.rolls, (int)std::lround(x.value), x.reqDone);
        sb += "  Centre's verdict: " + x.verdict + fstr(" (%d/100)", (int)std::lround(x.score)) + ". Rank on leaving: " + RANK[x.rankAtEnd] + "." + (x.award.empty() ? "" : " " + x.award + ".") + "\n";
        if (!x.agents.empty()) sb += "  The files, opened afterwards:\n";
        for (auto& a : x.agents) sb += "    " + a + "\n";
        sb += "\n";
    }
    if (g.phase == PH_PLAY) sb += std::string("PRESENT POSTING: ") + T().city + ", since " + fmtShort(g.postStart) + ". Tour ends " + fmtDay(g.postEnd) + ".\n\n";
    sb += "DECORATIONS AND COMMENDATIONS\n";
    if (g.awards.empty()) sb += "  none\n";
    for (auto& a : g.awards) sb += "  " + a + "\n";
    if (g.png + g.blown > 0) sb += fstr("\nExpelled or declared persona non grata: %d time(s). Known to Western services: %s.\n", g.blown, g.blown ? "yes" : "no");
    sb += "\nTHE CAREER AHEAD\n";
    for (size_t i = 0; i < THEATRES.size(); i++) {
        const Theatre& t = THEATRES[i];
        sb += fstr("  %d-%d  ", t.sy, t.ey) + pad(t.city, 12) + ROLE_NAME[t.role] + ((int)i < (int)g.record.size() ? "   (done)" : ((int)i == g.theatre && g.phase == PH_PLAY ? "   <- you are here" : "")) + "\n";
    }
    sb += "  August 1991, Moscow.\n";
    (void)lower;
    return sb;
}
