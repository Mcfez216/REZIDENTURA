// sim_text.cpp - the texts the screens show. Nothing here changes the game.
// RULE: nothing marked HIDDEN in core.h may be printed here, except in careerText() after a posting is over.
#include "sim.h"
#include "records.h"

static std::string upper(std::string s) { for (auto& c : s) c = (char)toupper((unsigned char)c); return s; }
static std::string lower(std::string s) { for (auto& c : s) c = (char)tolower((unsigned char)c); return s; }
static const char* ROLE_NAME[] = {"case officer", "senior case officer", "head of line", "Rezident"};
static const char* OROLE[] = {"Rezident", "Security officer (Line KR)", "Technical officer", "Case officer (another line)", "Case officer (under you)"};

int Sim::unread() const { int n = 0; for (auto& c : g.cables) if (!c.read && !c.routine()) n++; return n; }

std::string Sim::status1() {
    if (training()) return upper(fmtDay(g.day)) + "    Senior Lieutenant " + g.officer + ", trainee    " + T().city + " - field exercises";
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
    std::string hol = holiday(d);
    if (!hol.empty()) add(hol + " (holiday)");
    if (dateOf(d).m == 11 && dateOf(d).d == 7 && !training()) add("Embassy reception");
    if (onLeave(d)) add("ON LEAVE");
    else if (d <= g.busyUntil && d >= g.day) add("TAKEN UP: " + g.busyWhy);
    if (g.planLevel >= 0 && d == g.planEnd) add("attestation");
    if (d <= g.standDownUntil && d >= g.day) add("STAND-DOWN");
    if (dateOf(d).dow == 3) add("pouch closes");
    if (dateOf(d).d == 1) add("advance and accounts");
    auto cs = g.csDays.find(d);
    if (cs != g.csDays.end() && cs->second.state == 2) add("CS TEAM");
    for (auto& p : g.persons)
        if (p.live() && p.handler == 0 && p.stage != SPOTTED && birthday(p, dateOf(d).y) == d) {
            int turns = dateOf(d).y - birthYear(p);
            add(p.label() + "'s birthday" + (turns % 10 == 0 ? fstr(" (%d)", turns) : std::string("")));
        }
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
        else sb += "Last read " + fmtShort(s.markSeenDay) + ": " + (s.markSeen == 0 ? "no mark" : s.markSeen == 1 ? "DROP LOADED" : s.markSeen == 2 ? "DANGER" : s.markSeen == 4 ? "THE ESCAPE SIGNAL" : "your call-out") + ". ";
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

std::string Sim::dossier(const Person& p, bool withHead) {
    std::string sb;
    if (withHead) {
        if (!p.codename.empty()) sb += "AGENT " + p.codename + "\n";
        sb += upper(p.name) + fstr(", %d\n", p.age) + p.job + "\n" + p.employer + "\n" + SECTOR_LABEL[p.sector] + "\nLives: " + T().district[p.district] + "\n\n";
    }
    if (p.service != SVC_NONE) sb += serviceNote(p) + "\n";
    if (p.walkIn) sb += "WALK-IN: he came to us.\n";
    if (p.orphan) sb += "TAKEN OVER from the friends' service; his name is in their records.\n";
    if (p.service != SVC_NONE || p.walkIn || p.orphan) sb += "\n";
    sb += std::string("Status: ") + STAGE_LABEL[p.stage] + "     First contact: " + (p.firstDay < g.postStart ? std::string("before your arrival") : fmtShort(p.firstDay)) + "\n";
    sb += fstr("Meetings: %d     Last contact: ", p.meetings) + (p.lastContact < 0 ? std::string("-") : fmtShort(p.lastContact)) + "\n";
    sb += "Relations: " + word(p.rapport, W_RAPPORT, 5) + "     Commitment: " + word(p.commitment, W_COMMIT, 5) + "\n";
    sb += std::string("Trace: ") + (p.traceState == 0 ? "not requested" : p.traceState == 1 ? "requested" : p.traceBad ? "ADVERSE - see file" : "answered - nothing adverse")
          + "     Sanction to recruit: " + (p.sanction == 0 ? "not requested" : p.sanction == 1 ? "with Centre" : p.sanction == 2 ? "GRANTED" : "refused") + "\n";
    sb += "Case officer: " + handlerName(p) + (p.handler && p.pitchAuth ? " (authorised to make the approach)" : "") + "\n";
    sb += "Also witting: " + wittingText(p.witting) + "\n";
    if (!p.note.empty()) { std::string n = p.note; for (auto& ch : n) if (ch == '\n') ch = ' '; sb += "YOUR NOTE: " + n + "\n"; }
    if (p.confid > 0) sb += std::string("Confidential contact: ") + (p.confid == 1 ? "registration asked of Centre" : "REGISTERED - each lunch or dinner yields a report") + "\n";
    if (p.play > 0) sb += std::string("Playback: ") + (p.play == 1 ? "proposed to Centre" : p.play == 2 ? fstr("RUNNING - his material is disregarded; he is given chicken feed (%d meetings)", p.playMeets) : "ended") + "\n";
    if (p.escape >= 2) sb += std::string("Escape plan: ") + (p.escape == 2 ? "STANDING - his signal is a man with a carrier bag at his signal site. Read that site often" : p.escape == 3 ? "STANDING" : "SIGNALLED AND ACKNOWLEDGED - the pick-up is on") + "\n";
    if (p.kompState > 0) {
        sb += std::string("Compromising material: ") + (p.kompState == 1 ? "sanction asked of Centre" : p.kompState == 2 ? "SANCTIONED - to be staged at your next dinner with him"
                         : p.kompState == 3 ? std::string("HELD (") + (p.kompKind == 0 ? "receipts" : "photographs") + ")" : "an attempt was made and failed") + "\n";
    }
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
        sb += "\nSKILLS (as your instruction has left them)\n";
        sb += "  " + pad("Security (own tradecraft)", 30) + word(p.craft, W_LEVEL, 5) + "\n";
        sb += "  " + pad("Photography of documents", 30) + word(p.skPhoto, W_LEVEL, 5) + "\n";
        sb += "  " + pad("Elicitation", 30) + word(p.skElicit, W_LEVEL, 5) + "\n";
        sb += "  " + pad("Talent-spotting", 30) + word(p.skSpot, W_LEVEL, 5) + (p.referrals ? fstr("  (%d colleague(s) put forward)", p.referrals) : std::string("")) + "\n";
        sb += "  " + pad("Communications", 30) + word(p.skComms, W_LEVEL, 5) + "\n";
        sb += std::string("  Verification by Centre: ") + (p.verify == 0 ? "not requested" : p.verify == 1 ? "requested" : p.verify == 2 ? "CONTROL QUESTIONS PUT - his next deliveries are being checked"
                         : p.verifyResult == "CONSISTENT" ? "answers CONSISTENT with what Centre knows" : p.verifyResult == "INVENTED" ? "MATERIAL INVENTED" : "answers FALSE - possible adversary control") + "\n";
        sb += fstr("  Remuneration: %d a month.  Owed: %d.  Paid to date: %d %s.\n", p.salary, std::max(0, p.owed), p.paidTotal, T().currency);
        sb += "\nCOMMUNICATIONS PLAN\n  Personal meetings: " + (p.meetEvery > 0 ? fstr("every %d days", p.meetEvery) : std::string("none regular")) + "\n";
        int cur = p.meetSites.empty() ? -1 : (int)((size_t)std::abs(p.meetIdx) % p.meetSites.size());
        for (int i = 0; i < (int)p.meetSites.size(); i++) { Site* s = site(p.meetSites[i]); sb += fstr("     %s %d) ", i == cur && p.nextMeet >= 0 ? ">" : " ", i + 1) + (s ? s->name : std::string("?")) + "\n"; }
        Site* ag = agreedSite(p);
        sb += "  Next agreed meeting: " + (p.nextMeet < 0 ? std::string("NONE") : fmtDay(p.nextMeet) + ", evening, " + (ag ? ag->name : std::string("?"))) + "\n";
        if (p.misses > 0) sb += fstr("  Failed meetings in a row: %d (three and regular contact is lost)\n", p.misses);
        sb += "  Dead drop: " + (p.dropEvery > 0 && site(p.dropSite) ? site(p.dropSite)->name + fstr(", serviced every %d days; next about ", p.dropEvery) + fmtShort(p.nextDrop) : std::string("none")) + "\n";
        sb += "  Signal site: " + (site(p.signalSite) ? site(p.signalSite)->name : std::string("none")) + "\n";
        if (p.exfil == 1) sb += "  Exfiltration: requested of Centre.\n";
        if (p.exfil == 2) sb += "  Exfiltration: PASSPORT AND ROUTE IN YOUR SAFE. Hand them over at a meeting.\n";
        if (p.bug == 1) sb += "  Listening device: with him, to be placed.\n";
        if (p.bug == 2) sb += "  Listening device: in place in his office; tapes reach the technical officer.\n";
        if (p.bug == 3) sb += "  Listening device: SILENT.\n";
    }
    if (Person* src = person(p.ciBy)) sb += "\nNOTE: source " + src->label() + " reported on this person on " + fmtShort(p.ciDay) + " (see file).\n";
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
    if (o.role == ROLE_REZIDENT) {
        int days = g.day - g.postStart;
        sb += "\nHOW HE RUNS THE STATION\n";
        if (days < 45) sb += "  Too early to say. Give it a month or two.\n";
        else sb += std::string("  He is ") + TEMPER_LABEL[o.temper] + ". " + TEMPER_TEXT[o.temper] + "\n";
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
                        : x.kind == P_RADIO ? "Broadcast to an agent" : x.kind == P_LINEN_PKG ? "Package for Line N" : x.kind == P_FLAT ? "Safe flat being rented"
                        : x.kind == P_VERIFY ? "Centre's control questions" : x.kind == P_GAME ? "Centre's answer on the operational game" : nullptr;
        if (!w) continue;
        Person* p = person(x.ref);
        sb += std::string("  ") + w + ((x.kind == P_TRACE || x.kind == P_SANCTION || x.kind == P_RADIO || x.kind == P_VERIFY) && p ? " (" + p->label() + ")" : std::string(""))
              + (x.kind == P_FLAT ? std::string(" (") + T().district[x.ref] + ")" : std::string("")) + "\n";
        any = true;
    }
    if (!any) sb += "  nothing\n";
    sb += "\nSAFE FLATS\n";
    bool fl = false;
    for (auto& f : g.sites) if (f.flat && !f.retired) { fl = true; sb += "  " + pad(shortSite(&f), 40) + fstr("%d %s a month", f.rent, T().currency) + fstr(", used %d time(s)\n", f.uses); }
    if (!fl) sb += "  none - a flat is rented through a cut-out (Rent a safe flat)\n";
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
    sb += fstr("THE PLAN FOR %d\n", g.planYear > 0 ? g.planYear : dateOf(g.day).y) + planText() + "\n";
    sb += "TODAY\n  " + weatherLine() + "\n  Tomorrow: " + g.forecast + ".\n";
    std::string hol = holiday(g.day);
    if (!hol.empty()) sb += "  " + hol + " - a public holiday.\n";
    std::string next;
    for (int d = g.day + 1; d < g.day + 60 && next.empty(); d++) if (!holiday(d).empty()) next = holiday(d) + ", " + fmtDay(d);
    if (!next.empty()) sb += "  Next public holiday: " + next + ".\n";
    if (g.game > 0 || !g.gameLog.empty()) {
        sb += "\nTHE OPERATIONAL GAME" + std::string(g.game == 1 ? " - proposed to Centre" : g.game == 2 ? " - RUNNING (Line KR control)" : " - closed") + "\n";
        if (g.game == 2) sb += "  " + g.gameName + fstr(". Meetings so far: %d. Next lunch about ", g.gameRounds) + fmtShort(g.gameNext) + ".\n";
        size_t a = 0;
        while (a < g.gameLog.size()) { size_t b = g.gameLog.find('\n', a); if (b == std::string::npos) b = g.gameLog.size(); sb += "  " + g.gameLog.substr(a, b - a) + "\n"; a = b + 1; }
    }
    sb += "\n";
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
          "The missing fifth, every lunch, and your officers' expenses come out of the basic. A supplementary advance can be asked of Centre (Material).\n";
    return sb;
}

std::string Sim::careerText() {
    std::string sb = std::string("SERVICE RECORD - ") + upper(std::string(RANK[g.rank]) + " " + g.officer) + "\nFirst Chief Directorate, Committee for State Security\n\n";
    if (!g.courseReport.empty()) sb += g.courseReport + "\n";
    if (g.record.empty()) sb += "First posting abroad. Nothing is yet entered.\n\n";
    static const char* const HOW[] = {"tour completed", "DECLARED PERSONA NON GRATA", "expelled with the rest of the station", "RECALLED"};
    for (auto& x : g.record) {
        const Theatre& t = THEATRES[x.theatre];
        sb += upper(t.city) + "  " + fmtShort(x.startDay) + " - " + fmtShort(x.endDay) + "   " + HOW[x.how] + "\n";
        sb += fstr("  Recruitments %d | agents at end %d | arrests %d | exfiltrated %d | material graded %d (worth %d) | requirements met %d\n", x.recruited, x.agentsAtEnd, x.arrested, x.exfil, x.rolls, (int)std::lround(x.value), x.reqDone);
        sb += "  Centre's verdict: " + x.verdict + fstr(" (%d/100)", (int)std::lround(x.score)) + ". Rank on leaving: " + RANK[x.rankAtEnd] + "." + (x.award.empty() ? "" : " " + x.award + ".") + "\n";
        if (!x.agents.empty()) sb += "  The files, opened afterwards:\n";
        for (auto& a : x.agents) sb += "    " + a + "\n";
        sb += "\n";
    }
    if (g.phase == PH_PLAY) sb += std::string("PRESENT ") + (training() ? "COURSE: " : "POSTING: ") + T().city + ", since " + fmtShort(g.postStart) + ". Ends " + fmtDay(g.postEnd) + ".\n\n";
    sb += "DECORATIONS AND COMMENDATIONS\n";
    if (g.awards.empty()) sb += "  none\n";
    for (auto& a : g.awards) sb += "  " + a + "\n";
    if (g.png + g.blown > 0) sb += fstr("\nExpelled or declared persona non grata: %d time(s). Known to Western services: %s.\n", g.blown, g.blown ? "yes" : "no");
    sb += "\nTHE CAREER (where you go next depends on how the last tour went)\n";
    static const char* SLOT[] = {"1968-1971", "1973-1977", "1979-1984", "1986-1991"};
    for (int slot = 0; slot < 4; slot++) {
        std::string line = std::string("  ") + SLOT[slot] + "  ";
        std::string opts;
        int done = -1;
        for (auto& x : g.record) if (THEATRES[x.theatre].slot == slot) done = x.theatre;
        for (int i = 0; i < (int)THEATRES.size(); i++) {
            const Theatre& t = THEATRES[i];
            if (t.slot != slot) continue;
            std::string name = std::string(t.city) + " (" + ROLE_NAME[t.role] + ")";
            if (done == i) name = "[" + name + " - served]";
            else if (g.phase == PH_PLAY && i == g.theatre) name = "[" + name + " - YOU ARE HERE]";
            else if (g.phase == PH_INTERLUDE && i == g.nextTheatre) name = "[" + name + " - ordered]";
            opts += (opts.empty() ? "" : "   or   ") + name;
        }
        sb += line + opts + "\n";
    }
    sb += "  August 1991, Moscow.\n";
    (void)lower;
    return sb;
}

std::string Sim::siteReport(const Site& s) {
    std::string sb = std::string(s.flat ? "Safe flat" : SITETYPE_LABEL[s.type]) + " - " + T().district[s.district] + "\n" + s.name + "\n\n";
    if (s.casing == 0) sb += "Not yet cased.\n";
    else if (s.type == ST_ROUTE) sb += "Quality of the route: " + word(s.estSight, W_LEVEL, 5) + fstr(" (walked %d time(s); known %d/3)\n", s.uses, s.casing);
    else sb += "Natural cover: " + word(s.estCover, W_LEVEL, 5) + "\nView of the approaches: " + word(s.estSight, W_LEVEL, 5) + fstr("\nCased: %d/3\n", s.casing);
    sb += fstr("Used %d time(s); last ", s.uses) + (s.lastUsed < 0 ? std::string("never") : fmtShort(s.lastUsed)) + ".\n";
    if (s.exposure > 55) sb += "OVER-USED: a regular would notice a stranger's habits here.\n";
    else if (s.exposure > 30) sb += "Used often enough to leave a pattern.\n";
    sb += "Known to other officers: " + (s.witting.empty() ? std::string("none") : wittingText(s.witting)) + "\n";
    if (eastSide(s.district)) sb += "On our side of the frontier: the adversary cannot watch here.\n";
    if (s.flat) sb += fstr("A SAFE FLAT, rented through a cut-out for %d %s a month. Meetings are indoors and can be long; documents can be photographed properly. "
                           "Neighbours notice comings and goings.\n", s.rent, T().currency);
    std::string n = siteNotes(s);
    if (!n.empty()) sb += "\n" + n + "\n";
    return sb;
}

std::string Sim::districtText(int d) {
    const Theatre& t = T();
    std::string sb = upper(t.district[d]) + "\n";
    sb += std::string("Travel from the ") + (training() ? "School" : "Embassy") + ": " + (t.dist[d] == 0 ? "on the doorstep" : t.dist[d] == 1 ? "near" : t.dist[d] == 2 ? "moderate" : "far - a long route in itself") + "\n";
    if (eastSide(d)) sb += "On our side of the frontier.\n";
    int n[N_SITETYPE] = {};
    for (auto& s : g.sites) if (!s.retired && s.district == d) n[s.type]++;
    sb += fstr("Your sites: %d meeting, %d drop, %d signal, %d route\n", n[0], n[1], n[2], n[3]);
    int seen = 0, conf = 0, appr = 0;
    for (auto& l : g.sdrLog) if (l.district == d && g.day - l.day <= 90) { appr++; if (l.signs) seen++; if (l.signs == 2) conf++; }
    sb += fstr("Last 90 days: %d approaches, %d with surveillance seen (%d confirmed)\n", appr, seen, conf);
    int z = 0;
    for (auto& kv : g.zenit) if (g.day - kv.first <= 30 && kv.second.find(t.district[d]) != std::string::npos) z++;
    sb += fstr("Named by Zenit on %d morning(s) of the last 30\n", z);
    return sb;
}

// ============================================================================ conveniences for the player
/** What wants doing, most pressing first. Built only from what you already know - nothing hidden is used. */
std::vector<Sim::Todo> Sim::todo() {
    std::vector<Todo> v;
    auto add = [&](int level, int tab, const std::string& t) { v.push_back({level, tab, t}); };
    Date d = dateOf(g.day);
    if (g.day <= g.standDownUntil) add(0, TD_PLAN, "Line KR's stand-down is in force until " + fmtShort(g.standDownUntil) + ".");
    if (g.day <= g.busyUntil) add(0, TD_PLAN, "You are taken up until " + fmtShort(g.busyUntil) + ": " + g.busyWhy + ".");
    if (unread() > 0) add(1, TD_CENTRE, fstr("%d unread cable%s.", unread(), unread() == 1 ? "" : "s"));
    // meetings
    for (auto& p : g.persons) {
        if (p.stage != AGENT || p.handler != 0) continue;
        if (p.nextMeet >= g.day && p.nextMeet <= g.day + 3) {
            Site* s = agreedSite(p);
            add(p.nextMeet == g.day ? 2 : 1, TD_PLAN, gx(&p, "Agreed meeting with " + p.codename + (p.nextMeet == g.day ? " THIS EVENING" : " on " + fmtDay(p.nextMeet)) + " at " + (s ? shortSite(s) : std::string("?")) + "."));
        }
        if (p.misses > 0) add(1, TD_PEOPLE, gx(&p, p.codename + fstr(" has missed %d meeting%s.", p.misses, p.misses == 1 ? "" : "s")));
        if (p.owed > 0 && p.owed >= p.salary) add(0, TD_PEOPLE, gx(&p, p.codename + fstr(" is owed %d %s.", p.owed, T().currency)));
        if (p.exfil == 2) add(1, TD_PLAN, gx(&p, "Travel documents for " + p.codename + " are in your safe: hand them over, or rehearse a standing escape plan, at a long meeting."));
        // the signal site: a standing escape plan is a promise to read it
        if (Site* sg = site(p.signalSite)) {
            int since = sg->markSeenDay < 0 ? 99 : g.day - sg->markSeenDay;
            bool wife = g.wifeHere && g.wifeSignal == sg->id;
            if (p.escape >= 2 && since >= 3) add(2, TD_PLAN, gx(&p, p.codename + " has a standing escape plan, and " + shortSite(sg) + (since >= 99 ? " has never been read." : fstr(" has not been read for %d days.", since))));
            else if (!wife && p.dropEvery > 0 && since > p.dropEvery + 2) add(1, TD_PLAN, gx(&p, "Signal site of " + p.codename + " (" + shortSite(sg) + (since >= 99 ? ") has never been read." : fstr(") not read for %d days.", since))));
            if (sg->markSeen == 1) {
                Site* dr = site(p.dropSite);
                add(since > 3 ? 2 : 1, TD_PLAN, gx(&p, p.codename + "'s 'drop loaded' mark was seen on " + fmtShort(sg->markSeenDay) + ": " + (dr ? shortSite(dr) : std::string("his drop")) + " is waiting to be emptied."));
            }
            if (sg->markSeen == 2) add(2, TD_PEOPLE, gx(&p, p.codename + "'s DANGER signal was seen on " + fmtShort(sg->markSeenDay) + "."));
        }
        if (p.verify == 3 && p.verifyResult != "CONSISTENT" && p.play == 0) add(1, TD_PEOPLE, gx(&p, p.codename + " failed Centre's verification. Decide what is to be done with him."));
    }
    // birthdays in the coming ten days, of the people you see yourself
    for (auto& p : g.persons) {
        if (!p.live() || p.handler != 0 || p.stage == SPOTTED) continue;
        int b = nearBirthday(p, g.day + 5);
        if (b < g.day || b > g.day + 10 || p.giftYear == dateOf(b).y) continue;
        int turns = dateOf(b).y - birthYear(p);
        bool round = turns % 10 == 0;
        add(round ? 1 : 0, TD_PLAN, gx(&p, p.label() + "'s birthday is on " + fmtDay(b) + (round ? fstr(" - his %dth. A present at a meeting near the day would be remembered; none would be noticed.", turns) : std::string(". A small present at a meeting near the day is a kindness."))));
    }
    // your own agents' equipment, as your file has it
    for (auto& p : g.persons) {
        if (p.stage != AGENT || p.handler != 0 || p.tempo <= 0) continue;
        if (!p.camera) add(0, TD_PEOPLE, gx(&p, p.codename + " has no camera: everything he gives you is oral. Issue one at a meeting or through his drop."));
        else if (p.film == 0) add(1, TD_PEOPLE, gx(&p, p.codename + " has no fresh cassettes left: he cannot photograph. Give him film at his next meeting, or load his drop."));
    }
    // incriminating acts in the next three days with nothing to show you whether you are followed
    if (!training())
        for (int dd = g.day; dd <= g.day + 2; dd++)
            for (int b = 0; b < 3; b++) {
                Order o = order(dd, b);
                if (!isSet(dd, b) || o.cs) continue;
                bool hot = o.type == O_MEET_CLAN || o.type == O_LOAD_DROP || o.type == O_UNLOAD_DROP || o.type == O_LINE_N;
                if (!hot || (b > 0 && order(dd, b - 1).type == O_SDR)) continue;
                std::string when = (dd == g.day ? std::string("today") : fmtDay(dd)) + ", " + BLOCK_LABEL[b];
                for (auto& c : when) c = (char)tolower((unsigned char)c);
                add(dd == g.day ? 2 : 1, TD_PLAN, describe(o) + " (" + when + "): " + (b == 0 ? "a morning act has no detection route before it. Only the short check en route will tell you whether you are followed."
                                                                                     : "no detection route in the block before it. Only the short check en route will tell you whether you are followed."));
            }
    // the cipher clerk's queue
    if (!g.outQ.empty() && g.day - g.outQ.front().queued >= 2)
        add(1, TD_CENTRE, fstr("%d cable%s waiting in the cipher queue since %s: the day's allotment of groups is not enough. Draft fewer, or shorter.", (int)g.outQ.size(), g.outQ.size() == 1 ? " has been" : "s have been", fmtShort(g.outQ.front().queued).c_str()));
    // what you have left lying in a drop
    for (auto& s : g.sites)
        if (!s.retired && s.type == ST_DROP && s.forAgent >= 0 && s.loadedDay >= 0 && g.day - s.loadedDay > 9)
            if (Person* p = person(s.forAgent)) add(1, TD_SITES, gx(p, "Your package for " + p->label() + " has lain in " + shortSite(&s) + " since " + fmtShort(s.loadedDay) + "."));
    // contacts
    for (auto& p : g.persons) {
        if (p.handler != 0 || !p.live() || p.stage == AGENT) continue;
        if (p.sanction == 2) add(1, TD_PEOPLE, gx(&p, "Centre has sanctioned the recruitment of " + p.name + ". The approach is yours to make."));
        if (p.kompState == 2) add(1, TD_PLAN, gx(&p, "The compromise operation against " + p.name + " is sanctioned: it is staged at your next dinner with him."));
        // the next step in the ordinary run of a case, from what your own file says
        if (p.stage == DEVELOPING && p.traceState == 0 && p.rapport >= 40) add(0, TD_PEOPLE, gx(&p, p.name + " is friendly now. Ask Centre for a trace on him before you go further."));
        if (p.stage == DEVELOPING && p.traceState == 2 && !p.traceBad && p.sanction == 0 && p.rapport >= 40) {
            float conf = 0;
            for (int i = 0; i < A_INFL; i++) conf += p.estC[i];
            if (conf / A_INFL >= 35) add(0, TD_PEOPLE, gx(&p, p.name + "'s trace is clean and your assessment of him is taking shape. If you mean to recruit him, propose it to Centre."));
        }
        if (p.stage == DEVELOPING && p.lastContact >= 0 && g.day - p.lastContact > 30) add(0, TD_PEOPLE, gx(&p, p.name + fstr(" has not been seen for %d days. Acquaintance fades.", g.day - p.lastContact)));
    }
    // material
    int inSafe = 0, ready = 0;
    for (auto& x : g.rolls) { if (x.state == R_YOU) inSafe++; if (x.state == R_READY) ready++; }
    if (inSafe + ready > 0) {
        int toThu = (3 - d.dow + 7) % 7;
        add(toThu == 0 ? 2 : 1, TD_MAT, fstr("%d item%s of material in your hands", inSafe + ready, inSafe + ready == 1 ? "" : "s") + (toThu == 0 ? ". THE POUCH CLOSES TONIGHT." : fstr(". The pouch closes in %d day%s.", toThu, toThu == 1 ? "" : "s")));
    }
    for (auto& q : g.reqs)
        if (!q.closed && q.due >= g.day && q.due - g.day <= 14 && q.progress < q.need) add(q.due - g.day <= 5 ? 2 : 1, TD_CENTRE, fstr("Requirement No. %d falls due in %d day%s: ", q.id, q.due - g.day, q.due - g.day == 1 ? "" : "s") + q.title);
    // yourself
    if (g.paperwork > 8) add(2, TD_PLAN, fstr("Contact reports are %d hours in arrears, and it is being noticed. Give a block to office work.", (int)std::lround(g.paperwork)));
    else if (g.paperwork > 5) add(1, TD_PLAN, fstr("%d hours of reports are waiting to be written.", (int)std::lround(g.paperwork)));
    if (g.fatigue > 70) add(1, TD_PLAN, "You are over-tired: you see less in the street, and everything goes worse. Rest.");
    if (!training() && g.cover < 40) add(1, TD_PLAN, "Your cover is wearing thin. Do the job you are supposed to be here to do.");
    if (!training() && g.funds < P(40)) add(1, TD_ACC, fstr("Operational funds are low: %d %s.", g.funds, T().currency));
    if (!training() && g.film < 2) add(0, TD_MAT, "You have almost no film in the safe.");
    if (!training() && g.wifeHere && g.home < 35) add(1, TD_HOME, "Things are bad at home. Evenings in, a weekend, something for the family.");
    if (!training() && g.leaveFrom > g.day) add(0, TD_HOME, "Leave begins on " + fmtDay(g.leaveFrom) + ". Tell your agents; think what will lie in the drops.");
    else if (!training() && g.leaveYear != d.y && g.day - g.postStart >= 240 && d.m >= 6 && !onLeave(g.day)) add(0, TD_HOME, "You have not taken this year's leave.");
    if (g.planLevel >= 0) {
        float t = clampf((g.day - g.planStart) / (float)std::max(1, g.planEnd - g.planStart), 0, 1);
        float f = 0.4f * std::min(1.f, (g.totRecruited - g.baseRec) / (float)std::max(1, g.planRec)) + 0.4f * std::min(1.f, (g.totValue - g.baseVal) / std::max(1.f, g.planVal))
                  + 0.2f * std::min(1.f, (g.totSpotted - g.baseSpot) / (float)std::max(1, g.planSpot));
        if (t > 0.4f && f < t - 0.25f) add(0, TD_STATION, fstr("The year is %d%% gone and the plan about %d%% met.", (int)std::lround(t * 100), (int)std::lround(f * 100)));
    }
    std::stable_sort(v.begin(), v.end(), [](const Todo& a, const Todo& b) { return a.level > b.level; });
    return v;
}

/** A new working name for an agent. Centre does not mind what he is called so long as it is not his own name. */
std::string Sim::renameAgent(Person& p, const std::string& name) {
    std::string n;
    for (char c : name) if (isalnum((unsigned char)c) || c == '-' || c == ' ') n += (char)toupper((unsigned char)c);
    while (!n.empty() && n.back() == ' ') n.pop_back();
    while (!n.empty() && n.front() == ' ') n.erase(n.begin());
    if (p.codename.empty()) return "Only a recruited agent has a codename.";
    if (n.size() < 2 || n.size() > 14) return "A codename is one word of two to fourteen letters.";
    for (auto& q : g.persons) if (&q != &p && q.codename == n) return "That name is in use.";
    std::string old = p.codename;
    if (n == old) return "";
    p.codename = n;
    file(p, "Codename changed from " + old + " to " + n + ".");
    return old + " will be " + n + " from now on, in your files and in Centre's.";
}

/** The whole career as a plain text, for keeping. */
std::string Sim::personalFile() {
    std::string sb = "REZIDENTURA - PERSONAL FILE\n" + std::string(RANK[g.rank]) + " " + g.officer + "\n" + fmtDay(g.day) + ", " + T().city + "\n\n";
    sb += careerText() + "\n";
    if (!g.attest.empty()) { sb += "\nATTESTATIONS\n"; for (auto& a : g.attest) sb += "  " + a + "\n"; }
    sb += "\n" + privateText();
    if (!g.notes.empty()) sb += "\nNOTEBOOK\n" + g.notes + "\n";
    if (g.phase == PH_OVER) sb += "\n" + g.overTitle + "\n\n" + g.overText + "\n";
    return sb;
}

// ============================================================================ the service records
std::string Sim::endingKey() const {
    const std::string& t = g.overTitle;
    if (t == "DEFECTOR") return "defector";
    if (t == "LEFORTOVO") return "lefortovo";
    if (t == "A DESK IN THE ARCHIVES") return "archives";
    if (t == "TRANSFERRED TO THE RESERVE") return "reserve";
    if (t == "MOSCOW, AUGUST 1991") {
        float avg = 0;
        for (auto& x : g.record) avg += x.score;
        if (!g.record.empty()) avg /= g.record.size();
        return g.rank >= 4 ? "1991-general" : (g.rank == 3 && avg >= 45) ? "1991-kept" : "1991-retired";
    }
    return "";
}

std::vector<int> Sim::careerCitations() const {
    std::vector<int> v;
    bool full = g.overTitle == "MOSCOW, AUGUST 1991";
    int recruited = 0;
    bool banner = false, network = false, nobody = false, wash = false, bonn = false, png = false, taken = false, clean = true, out = false;
    for (auto& r : g.record) {
        if (THEATRES[r.theatre].training) continue;
        recruited += r.recruited;
        if (r.award.find("Red Banner") != std::string::npos) banner = true;
        if (r.agentsAtEnd >= 6) network = true;
        if (r.exfil > 0) out = true;
        if (r.recruited >= 3 && r.arrested == 0) nobody = true;
        if (r.theatre == TH_WASHINGTON && r.how == END_TOUR) wash = true;
        if (r.theatre == TH_BONN) bonn = true;
        if (r.how == END_PNG || r.how == END_EXPELLED) png = true;
        if (r.how != END_TOUR) clean = false;
        for (auto& a : r.agents) if (a.find("PROVOCATION") != std::string::npos) taken = true;
    }
    if (full) v.push_back(CI_FULL);
    if (g.rank >= 4) v.push_back(CI_GENERAL);
    if (banner) v.push_back(CI_BANNER);
    if (full && clean) v.push_back(CI_CLEAN);
    if (recruited >= 10) v.push_back(CI_RECRUITER);
    if (network) v.push_back(CI_NETWORK);
    if (nobody) v.push_back(CI_NOBODY);
    if (wash) v.push_back(CI_WASHINGTON);
    if (bonn) v.push_back(CI_BONN);
    if (full && g.difficulty >= 2) v.push_back(CI_HARD);
    if (full && g.ironman) v.push_back(CI_IRONMAN);
    if (png) v.push_back(CI_PNG);
    if (taken) v.push_back(CI_TAKEN);
    if (out || g.totExfil > 0) v.push_back(CI_BROUGHT_OUT);
    if (full && THEATRES[g.startTheatre].slot >= 1) v.push_back(CI_LATE);
    return v;
}

/** One line of service-records.txt: fields separated by tabs. */
std::string Sim::archiveLine() const {
    auto clean = [](std::string s) { for (auto& c : s) if (c == '\t' || c == '\n' || c == '\r') c = ' '; return s; };
    std::string posts, cites;
    int recruited = 0;
    for (auto& r : g.record) {
        if (THEATRES[r.theatre].training) continue;
        if (!posts.empty()) posts += ", ";
        static const char* const HOW[] = {"", " (declared PNG)", " (expelled)", " (recalled)"};
        posts += std::string(THEATRES[r.theatre].city) + " " + r.verdict + HOW[std::max(0, std::min(3, r.how))];
        recruited += r.recruited;
    }
    for (int c : careerCitations()) cites += (cites.empty() ? "" : ",") + std::to_string(c);
    Date d = dateOf(g.day);
    return "1\t" + clean(g.officer) + "\t" + RANK[g.rank] + "\t" + std::to_string(g.background) + "\t" + std::to_string(g.startTheatre) + "\t"
           + std::to_string(g.difficulty) + "\t" + (g.ironman ? "1" : "0") + "\t" + std::to_string(g.seed) + "\t" + fstr("%d-%02d", d.y, d.m) + "\t"
           + endingKey() + "\t" + clean(posts) + "\t" + std::to_string(recruited) + "\t" + cites + "\t" + std::to_string((int)g.awards.size());
}

// ============================================================================ your own pattern
Sim::Pattern Sim::myPattern(int days) const {
    Pattern p;
    std::set<long long> seen;                     // one act per block, however many checks it took
    for (auto& l : g.sdrLog) {
        if (g.day - l.day > days || l.district < 0 || l.district >= 8) continue;
        if (!seen.insert((long long)l.day * 4 + l.block).second) continue;
        int dw = dateOf(l.day).dow;
        p.grid[dw][l.block]++; p.dist[l.district]++; p.n++;
    }
    for (int d = 0; d < 7; d++) for (int b = 0; b < 3; b++) if (p.grid[d][b] > p.slotN) { p.slotN = p.grid[d][b]; p.slotDow = d; p.slotB = b; }
    for (int i = 0; i < 8; i++) if (p.dist[i] > p.topDistN) { p.topDistN = p.dist[i]; p.topDist = i; }
    return p;
}
std::string Sim::patternWord(const Pattern& p) const {
    if (p.n < 3) return "too few acts to say";
    float slot = p.slotN / (float)p.n, dist = p.topDistN / (float)p.n;
    float k = std::max(slot * 1.4f, dist);
    return k < 0.35f ? "varied - hard to predict" : k < 0.5f ? "some habits showing" : k < 0.7f ? "predictable" : "a timetable";
}
