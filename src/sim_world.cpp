// sim_world.cpp - the world around the work: the weather and the light, the host country's
// holidays and the Embassy's receptions, safe flats, Centre's verification of agents, and the
// "operational game" played back at the other side when they try to recruit you.
#include "sim.h"
#include <cmath>

// ============================================================================ climate and light
namespace {
struct Climate {
    float lat;                 // degrees north
    int noon;                  // clock time of solar noon, standard time, minutes
    float t[12];               // mean daily temperature by month, C
    float wet[12];             // chance of rain or snow on a day
    float fog[12];             // chance of fog on a dry day
};
const Climate& climate(int th) {
    static const Climate MOSCOW = {55.75f, 12 * 60 + 30, {-9, -8, -2, 6, 13, 17, 19, 17, 11, 5, -1, -6},
                                   {.45f, .40f, .35f, .35f, .35f, .40f, .45f, .40f, .40f, .45f, .50f, .50f}, {.04f, .04f, .04f, .03f, .02f, .02f, .02f, .03f, .05f, .07f, .07f, .05f}};
    static const Climate LONDON = {51.5f, 12 * 60, {5, 5, 7, 9, 13, 16, 18, 18, 15, 11, 8, 6},
                                   {.50f, .42f, .42f, .40f, .38f, .35f, .35f, .38f, .38f, .45f, .48f, .50f}, {.10f, .08f, .05f, .03f, .02f, .02f, .02f, .03f, .05f, .08f, .12f, .12f}};
    static const Climate VIENNA = {48.2f, 11 * 60 + 54, {0, 2, 6, 11, 16, 19, 21, 20, 16, 10, 5, 1},
                                   {.40f, .38f, .40f, .40f, .42f, .42f, .40f, .38f, .35f, .35f, .42f, .42f}, {.10f, .07f, .04f, .02f, .02f, .02f, .02f, .03f, .05f, .09f, .12f, .12f}};
    static const Climate DELHI = {28.6f, 12 * 60 + 21, {14, 17, 23, 29, 33, 33, 31, 30, 29, 26, 20, 15},
                                  {.06f, .06f, .05f, .04f, .06f, .25f, .50f, .50f, .25f, .04f, .02f, .04f}, {.25f, .12f, .03f, .01f, .01f, .01f, .01f, .01f, .01f, .03f, .10f, .25f}};
    static const Climate WASH = {38.9f, 12 * 60 + 8, {1, 3, 8, 13, 19, 24, 27, 26, 22, 15, 9, 4},
                                 {.32f, .30f, .35f, .33f, .35f, .32f, .32f, .30f, .28f, .26f, .30f, .32f}, {.04f, .04f, .03f, .02f, .02f, .02f, .02f, .02f, .03f, .04f, .04f, .04f}};
    static const Climate BERLIN = {52.5f, 12 * 60 + 6, {0, 1, 4, 9, 14, 17, 19, 18, 15, 10, 5, 1},
                                   {.42f, .38f, .38f, .36f, .38f, .40f, .42f, .40f, .36f, .36f, .42f, .44f}, {.09f, .07f, .04f, .02f, .02f, .02f, .02f, .03f, .05f, .09f, .11f, .10f}};
    static const Climate BONN = {50.7f, 12 * 60 + 32, {2, 3, 6, 10, 14, 17, 19, 18, 15, 11, 6, 3},
                                 {.45f, .40f, .42f, .40f, .42f, .42f, .42f, .40f, .38f, .40f, .45f, .47f}, {.10f, .08f, .05f, .03f, .02f, .02f, .02f, .03f, .07f, .12f, .14f, .12f}};
    switch (th) {
        case TH_MOSCOW: return MOSCOW;
        case TH_LONDON: return LONDON;
        case TH_VIENNA: return VIENNA;
        case TH_DELHI: return DELHI;
        case TH_WASHINGTON: return WASH;
        case TH_BONN: return BONN;
        default: return BERLIN;
    }
}
/** Summer time: the clock an hour ahead of the sun. */
bool summerTime(int th, Date d) {
    switch (th) {
        case TH_LONDON: return true;                                   // British Standard Time: GMT+1 all year, October 1968 - October 1971
        case TH_MOSCOW: return d.y >= 1981 && d.m >= 4 && d.m <= 9;
        case TH_WASHINGTON: return d.m >= 5 && d.m <= 10;
        case TH_DELHI: return false;
        default: return d.y >= 1980 && d.m >= 4 && d.m <= 9;            // Austria and the Federal Republic from 1980
    }
}
int nthWeekday(int y, int m, int dow, int n) {                      // dow 0 = Monday; n = 1.. (or -1: the last)
    if (n > 0) {
        int d0 = dayOf(y, m, 1);
        int first = d0 + ((dow - dateOf(d0).dow) + 7) % 7;
        return first + 7 * (n - 1);
    }
    int dl = dayOf(y, m + 1 > 12 ? 1 : m + 1, 1) - 1;
    if (m == 12) dl = dayOf(y + 1, 1, 1) - 1;
    return dl - ((dateOf(dl).dow - dow) + 7) % 7;
}
int easter(int y) {                                                // the Western Easter Sunday (Meeus)
    int a = y % 19, b = y / 100, c = y % 100, d = b / 4, e = b % 4, f = (b + 8) / 25, gg = (b - f + 1) / 3;
    int h = (19 * a + b - d - gg + 15) % 30, i = c / 4, k = c % 4, l = (32 + 2 * e + 2 * i - h - k) % 7, m = (a + 11 * h + 22 * l) / 451;
    int month = (h + l - 7 * m + 114) / 31, day = ((h + l - 7 * m + 114) % 31) + 1;
    return dayOf(y, month, day);
}
}  // namespace

int Sim::sunTime(int day, bool set) const {
    const Climate& c = climate(g.theatre);
    Date d = dateOf(day);
    int n = day - dayOf(d.y, 1, 1) + 1;
    float decl = 23.44f * std::sin(6.2831853f * (284 + n) / 365.f) * 3.14159265f / 180.f;
    float lat = c.lat * 3.14159265f / 180.f;
    float cw = (std::sin(-0.833f * 3.14159265f / 180.f) - std::sin(lat) * std::sin(decl)) / (std::cos(lat) * std::cos(decl));
    float w = std::acos(clampf(cw, -1, 1)) * 180.f / 3.14159265f;
    int t = c.noon + (int)std::lround((set ? 1 : -1) * w / 15.f * 60.f);
    return t + (summerTime(g.theatre, d) ? 60 : 0);
}

/** Today's weather becomes yesterday's forecast's subject; tomorrow's is drawn, and forecast. */
void Sim::weather(bool first) {
    const Climate& c = climate(g.theatre);
    auto draw = [&](int day, int prevWx, int prevT, int& wx, int& t) {
        Date d = dateOf(day);
        float mean = c.t[d.m - 1];
        float prevMean = c.t[(dateOf(day - 1).m) - 1];
        t = (int)std::lround(mean + 0.55f * (prevT - prevMean) + r().gauss() * 2.6f);
        float wet = c.wet[d.m - 1] + (prevWx == WX_RAIN || prevWx == WX_SNOW ? 0.15f : 0);
        if (r().f() < wet) wx = t <= 0 || (t <= 2 && pct(50)) ? WX_SNOW : WX_RAIN;
        else if (t <= 18 && r().f() < c.fog[d.m - 1] * (prevWx == WX_FOG ? 2.2f : 1)) wx = WX_FOG;
        else wx = pct(45) ? WX_CLEAR : WX_CLOUD;
    };
    if (first) { draw(g.day, WX_CLOUD, (int)c.t[dateOf(g.day).m - 1], g.wx, g.temp); }
    else { g.wx = g.wxNext; g.temp = g.tempNext; }
    draw(g.day + 1, g.wx, g.temp, g.wxNext, g.tempNext);
    g.sunrise = sunTime(g.day, false);
    g.sunset = sunTime(g.day, true);
    int fw = pct(75) ? g.wxNext : r().n(5);
    if (fw == WX_SNOW && g.tempNext > 3) fw = WX_RAIN;
    int ft = g.tempNext + r().n(5) - 2;
    g.forecast = std::string(WX_LABEL[fw]) + fstr(", about %d C", ft);
}

std::string Sim::weatherLine() const {
    return std::string(WX_LABEL[g.wx]) + fstr(", %d C. Sunrise %02d:%02d, sunset %02d:%02d.", g.temp, g.sunrise / 60, g.sunrise % 60, g.sunset / 60, g.sunset % 60);
}

// ============================================================================ holidays
std::string Sim::holiday(int day) const {
    Date d = dateOf(day);
    int y = d.y, md = d.m * 100 + d.d;
    int e = easter(y);
    auto is = [&](int m, int dd) { return md == m * 100 + dd; };
    switch (g.theatre) {
        case TH_MOSCOW:
            if (is(1, 1)) return "New Year";
            if (is(3, 8)) return "International Women's Day";
            if (is(5, 1) || is(5, 2)) return "May Day";
            if (is(5, 9)) return "Victory Day";
            if (is(11, 7) || is(11, 8)) return "Anniversary of the Revolution";
            if (y < 1977 && is(12, 5)) return "Constitution Day";
            if (y >= 1977 && is(10, 7)) return "Constitution Day";
            return "";
        case TH_LONDON:
            if (is(12, 25)) return "Christmas Day";
            if (is(12, 26)) return "Boxing Day";
            if (day == e - 2) return "Good Friday";
            if (day == e + 1) return "Easter Monday";
            if (day == (y >= 1971 ? nthWeekday(y, 5, 0, -1) : e + 50)) return y >= 1971 ? "Spring Bank Holiday" : "Whit Monday";
            if (day == (y >= 1971 ? nthWeekday(y, 8, 0, -1) : nthWeekday(y, 8, 0, 1))) return "August Bank Holiday";
            return "";
        case TH_VIENNA:
            if (is(1, 1)) return "New Year";
            if (is(1, 6)) return "Epiphany";
            if (day == e + 1) return "Easter Monday";
            if (is(5, 1)) return "Labour Day";
            if (day == e + 39) return "Ascension";
            if (day == e + 50) return "Whit Monday";
            if (day == e + 60) return "Corpus Christi";
            if (is(8, 15)) return "Assumption";
            if (is(10, 26)) return "National Day";
            if (is(11, 1)) return "All Saints";
            if (is(12, 8)) return "Immaculate Conception";
            if (is(12, 25)) return "Christmas Day";
            if (is(12, 26)) return "St Stephen's Day";
            return "";
        case TH_DELHI:
            if (is(1, 26)) return "Republic Day";
            if (is(8, 15)) return "Independence Day";
            if (is(10, 2)) return "Gandhi Jayanti";
            return "";
        case TH_WASHINGTON:
            if (is(1, 1)) return "New Year's Day";
            if (y >= 1986 && day == nthWeekday(y, 1, 0, 3)) return "Martin Luther King Day";
            if (day == nthWeekday(y, 2, 0, 3)) return "Washington's Birthday";
            if (day == nthWeekday(y, 5, 0, -1)) return "Memorial Day";
            if (is(7, 4)) return "Independence Day";
            if (day == nthWeekday(y, 9, 0, 1)) return "Labor Day";
            if (day == nthWeekday(y, 10, 0, 2)) return "Columbus Day";
            if (is(11, 11)) return "Veterans Day";
            if (day == nthWeekday(y, 11, 3, 4)) return "Thanksgiving";
            if (is(12, 25)) return "Christmas Day";
            return "";
        default: {     // the Federal Republic and West Berlin
            if (is(1, 1)) return "New Year";
            if (day == e - 2) return "Good Friday";
            if (day == e + 1) return "Easter Monday";
            if (is(5, 1)) return "Labour Day";
            if (day == e + 39) return "Ascension";
            if (day == e + 50) return "Whit Monday";
            if (y < 1990 && is(6, 17)) return "Day of German Unity";
            if (y >= 1990 && is(10, 3)) return "Day of German Unity";
            int bb = nthWeekday(y, 11, 2, 1);                      // Repentance Day: the Wednesday before 23 November
            while (dateOf(bb + 7).d <= 22 && dateOf(bb + 7).m == 11) bb += 7;
            if (day == bb) return "Day of Prayer and Repentance";
            if (is(12, 25) || is(12, 26)) return "Christmas";
            return "";
        }
    }
}
bool Sim::dayOff(int day) const { return weekend(day) || !holiday(day).empty(); }

/** On 7 November the Embassy gives its reception for the anniversary of the Revolution, and every officer is there. */
void Sim::embassyReception(int b) {
    Date d = dateOf(g.day);
    if (training() || b != 2 || !(d.m == 11 && d.d == 7)) return;
    Order o = order(g.day, 2);
    if (o.type == O_FUNCTION) {
        say("  It was the Embassy's own reception for the anniversary of the Revolution: four hundred guests, half of them from the ministries.");
        g.standRez = clampf(g.standRez + 1, 0, 100);
        if (pct(55)) spot(o.sector, "at the Embassy's reception for the anniversary", 0);
    } else {
        alert("You were not at the Embassy's reception for the anniversary of the Revolution. The Ambassador noticed, and so did the Rezident.");
        g.standRez = clampf(g.standRez - 3, 0, 100);
    }
}

// ============================================================================ safe flats
int Sim::flatCost(int district) const { return P(14) + P(4) * std::max(0, 3 - T().dist[district]); }

std::string Sim::rentFlat(int district) {
    if (training()) return "There are no safe flats on the course.";
    if (district < 0 || district > 7) return "Choose a district.";
    if (T().role == 0) return "A case officer on his first posting does not rent flats. That is for the Rezident to decide, and he has decided no.";
    if (eastSide(district)) return "On our side of the frontier the friends provide flats, and you need none of your own.";
    int n = 0, limit = T().role >= 3 ? 3 : 2;
    for (auto& s : g.sites) if (s.flat && !s.retired) n++;
    for (auto& x : g.pending) if (x.kind == P_FLAT) n++;
    if (n >= limit) return fstr("Centre allows the rezidentura %d safe flats at your level. Give one up first (Sites: strike it off).", limit);
    int rent = flatCost(district), deposit = rent * 3;
    if (g.funds < deposit) return fstr("A flat in %s needs %d %s now (three months in advance), and %d a month after. You hold %d.", T().district[district], deposit, T().currency, rent, g.funds);
    money(-deposit, std::string("Safe flat in ") + T().district[district] + " - three months' rent in advance, through a cut-out");
    pend(g.day + 14 + r().n(15), P_FLAT, district, rent);
    g.paperwork += 1.5f;
    return fstr("The support officer will rent a flat in %s through a cut-out: a local who signs the lease and never asks why. "
                "Three months paid in advance (%d %s); then %d a month. It will be ready in two to four weeks.", T().district[district], deposit, T().currency, rent);
}

/** After a meeting at a flat: what the neighbours saw, and what the watchers did. */
void Sim::flatMeeting(Site& s, Person& p, bool observed, bool longMeet) {
    s.neighbours += longMeet ? 9 : 5;
    s.exposure = std::max(0.f, s.exposure - (longMeet ? 8 : 4));      // indoors: no pattern in the street, but see neighbours
    if (observed && pct(45)) s.burned = true;                           // they followed you to the door
    if (s.neighbours > 55 && pct((s.neighbours - 55) * 1.5f)) s.burned = true;
    if (s.neighbours > 38 && pct(45)) say("    The concierge looked at you both for longer than a concierge needs to.");
    if (longMeet && p.stage == AGENT) p.stress = clampf(p.stress - 3, 0, 100);
}

/** At a flat, an agent can bring out the documents themselves, and you photograph them properly. */
void Sim::docSession(const Order& o, Person& p, Site& s) {
    (void)o;
    if (!s.flat) return;
    float access = controlled(p) ? p.app[A_ACCESS] : p.tru[A_ACCESS];
    if (access < 15) { say("    He had nothing worth bringing out of his office."); return; }
    if (g.film < 2) { say("    You had too little film in the safe to photograph what he brought."); return; }
    g.film -= 2;
    Roll& x = newRoll(p);
    x.frames = FRAMES * 2;
    x.value = x.genuine ? clampf(access * rnd(0.85f, 1.2f), 5, 100) : rnd(30, 62);
    if (p.fabricator && !controlled(p)) { x.genuine = false; x.value = rnd(25, 50); }
    x.state = R_YOU;
    paper(2);
    say(fstr("    He brought the files themselves out of his office for the evening. You photographed them on the kitchen table, two full cassettes (item No. %d), and he took them back before morning.", x.id));
    if (!controlled(p)) {
        p.suspicion += 2 + p.empSec / 12.f;
        p.stress = clampf(p.stress + 5, 0, 100);
        if (p.empSec > 50 && pct(p.empSec / 8.f)) {
            p.suspicion += 20;
            p.warn.push_back("says the registry asked where a file had been overnight. He told them he took it home to finish a report.");
        }
    }
    s.neighbours += 4;
}

// ============================================================================ verification by Centre
std::string Sim::requestVerify(Person& p) {
    if (p.stage != AGENT) return "Centre verifies recruited agents.";
    if (training()) return "Not on the course.";
    if (p.verify == 1 || p.verify == 2) return "The verification of " + p.codename + " is already in hand.";
    p.verify = 1;
    p.verifyResult.clear();
    queue(C_VERIFY, p.id, 60, "Verification of " + p.codename);
    g.paperwork += 1;
    file(p, "Verification requested of Centre.");
    return "Request drafted (60 groups). Centre will prepare control questions - things it already knows the answers to - and check his next two deliveries against them. "
           "It takes some weeks, and Centre's analysts are not always right.";
}

void Sim::verifyRoll(Person& p, Roll& x) {
    if (p.verify != 2) return;
    bool flagged = false;
    std::string why;
    if (!x.genuine) {
        if (pct(p.fabricator && !controlled(p) ? 60 : 55)) { flagged = true; why = p.fabricator && !controlled(p) ? "invented" : "false"; }
    } else if (pct(7)) { flagged = true; why = "false"; }      // the analysts can be wrong
    p.verifyLeft--;
    if (!flagged && p.verifyLeft > 0) return;
    p.verify = 3;
    Person* keep = subj; subj = &p;
    if (flagged) {
        p.verifyResult = why == "invented" ? "INVENTED" : "FALSE";
        file(p, "VERIFICATION: answers to control questions " + std::string(why == "invented" ? "show the material is invented." : "are false in particulars Centre can check."));
        cable("Centre", "Verification of " + p.codename,
              "VERIFICATION OF " + p.codename + ". His answers to the control questions are " + (why == "invented" ? "INVENTED: they agree with nothing, including each other. Centre assesses that the source fabricates his material."
                                                                                                                   : "FALSE in particulars Centre is able to check. Centre assesses that the source may be under the adversary's control.")
              + "\n\nIt is for the rezidentura to propose what is to be done with him. Centre reminds you that verification is evidence, not proof.\n\nCENTRE");
    } else {
        p.verifyResult = "CONSISTENT";
        file(p, "VERIFICATION: answers to control questions agree with what Centre knows.");
        cable("Centre", "Verification of " + p.codename, "VERIFICATION OF " + p.codename + ". His answers to the control questions agree with what Centre knows. No grounds for doubt were found.\n\nCENTRE");
    }
    subj = keep;
}

// ============================================================================ the operational game
namespace {
const char* const GAME_NAMES[] = {"Mr Hargreave", "Mr Ellison", "Herr Brandauer", "Mr Whitcombe", "Monsieur Darnand", "Mr Kessler", "Mr Pryor", "Mr Lindqvist"};
const char* const GAME_TOPICS[] = {"the Rezident's habits and his health", "who in the station works on the scientific line", "your own money, and whether it is enough",
                                   "the routine of the cipher room", "the next round of postings", "how the Embassy chooses its drivers",
                                   "your opinion of the new General Secretary", "where you spend your holidays", "your marriage"};
}

void Sim::gameStart() {
    g.game = 1;
    queue(C_GAME, 0, 100, "Proposal for an operational game");
}

/** Centre's answer to the proposal to play the approach back at the other side. */
void Sim::gameSanction() {
    if (g.game != 1) return;
    if (!pct(std::min(90.f, 60 + g.standCentre / 4))) {
        g.game = 3;
        cable("Centre", "Your proposal", "Centre does NOT sanction an operational game. The approach is noted; avoid the person concerned and vary your movements. "
              "Centre reminds you that such games are proposed by Line KR, not by the officer approached.\n\nCENTRE");
        return;
    }
    g.game = 2;
    g.gameName = GAME_NAMES[r().n(8)];
    g.gameNext = g.day + 5 + r().n(6);
    g.gameRounds = 0; g.gameFound = 0;
    g.gameDoubt = 10;
    g.gameSuspect = clampf(15 + g.you.heat / 3, 0, 60);
    g.gameLog = fmtShort(g.day) + "  Centre sanctions the game against " + g.gameName + ".\n";
    cable("Centre", "Operational game sanctioned",
          "Centre SANCTIONS an operational game. You will go to the Thursday lunch and let " + g.gameName + " believe that you are interested. "
          "Line KR will give you material prepared by Centre - true, checkable and worthless - and will hear every word through the device you will carry.\n\n"
          "The purpose is to learn what the other side knows of us and whom they have among us, not to give them anything. "
          "You will report every meeting in full. If you begin to enjoy it, say so.\n\nCENTRE");
}

void Sim::gameRound() {
    if (g.game != 2 || g.day < g.gameNext || !g.decisions.empty()) return;
    if (g.gameRounds >= 4 && g.gameSuspect < 45 && pct(40)) {
        addDecision(DK_GAME_OFFER, 0, "The offer",
                    "At the end of lunch " + g.gameName + " stopped talking about fishing. He said that his friends have been patient, that they know what you are paid and what you would be paid, "
                    "and that a man of your experience could live very comfortably under another name: a house, a pension, a new start for your family. They would need a month to arrange it, and an answer by Thursday.\n\n"
                    "Line KR has heard the tape. So, presumably, has Centre.",
                    {"Refuse him, report it, and close the game. It has gone as far as it should.",
                     "Say you will think about it, and keep the game going",
                     "Accept. Go over to them."});
        return;
    }
    std::string topic = GAME_TOPICS[r().n(9)];
    std::string hint = g.gameSuspect < 30 ? "Line KR thinks he still believes you." : g.gameSuspect < 55 ? "Line KR thinks he has begun to wonder." : "Line KR thinks he is no longer sure of you, and is testing.";
    std::string doubt = g.gameDoubt > 45 ? "\n\nCentre has asked, in a cable you were not shown, why this is taking so long." : "";
    addDecision(DK_GAME, 0, fstr("The operational game: lunch No. %d", g.gameRounds + 1),
                "Thursday is lunch with " + g.gameName + ". Last time he came back twice to " + topic + ". " + hint + doubt + "\n\nWhat do you give him this time, and what do you ask?",
                {"Chicken feed only - Centre's prepared material, true and worthless - and ask nothing (safe; you learn little; he grows impatient)",
                 "The better material Centre allows, and steer him towards what his people want to know",
                 "Press him: ask what they already know about our people",
                 "Close the game: tell Line KR it is finished"});
}

void Sim::gameDecide(int kind, int option) {
    if (kind == DK_GAME_OFFER) {
        if (option == 2) {
            finishCareer("DEFECTOR",
                         "You told " + g.gameName + " yes. A month later, on a Sunday, you took your family for a drive and did not come back. The debriefing took a year, in a house with a lake and two men who were always polite. "
                         "In Moscow you were tried in your absence and sentenced to death. Your file at Centre was closed with one word written across the cover in red pencil.\n\n"
                         "Whatever you told them, the Committee changed its codes, its routes and its people within the month, and wrote you out of its histories. Some of the officers who had served with you were never posted abroad again.");
            return;
        }
        if (option == 0) {
            g.game = 3;
            float credit = 6 + 3.f * g.gameFound + std::min(6, g.gameRounds);
            g.standCentre = clampf(g.standCentre + credit, 0, 100);
            g.standRez = clampf(g.standRez + 3, 0, 100);
            g.gameLog += fmtShort(g.day) + "  The offer refused; the game closed.\n";
            cable("Line KR", "The operational game - closed", "The game is closed on the offer, which is where such games should end. The adversary has spent a year and a great deal of money on an officer who was ours throughout, "
                  "and has told us more about itself than it learned about us. Centre has expressed its appreciation.\n\nLINE KR");
            heat(4);
            return;
        }
        g.gameSuspect += 10; g.gameDoubt += 8;
        g.gameNext = g.day + 21 + r().n(14);
        g.gameLog += fmtShort(g.day) + "  The offer: you asked for time.\n";
        return;
    }
    // a round of the game
    if (option == 3) {
        g.game = 3;
        g.standCentre = clampf(g.standCentre + 2 + 2.f * g.gameFound + std::min(4, g.gameRounds / 2), 0, 100);
        g.gameLog += fmtShort(g.day) + "  You closed the game.\n";
        cable("Line KR", "The operational game - closed", fmtShort(g.day) + ": the game is closed at your request after " + fstr("%d", g.gameRounds) + " meetings. Avoid the man and the places he uses.\n\nLINE KR");
        return;
    }
    g.gameRounds++;
    float learn = option == 0 ? 15 : option == 1 ? 45 : 70;
    g.gameSuspect += option == 0 ? 8 + g.gameRounds * 2 : option == 1 ? -4 : 18;
    g.gameDoubt += option == 0 ? -2 : option == 1 ? 7 : 2;
    g.gameSuspect = clampf(g.gameSuspect, 0, 100); g.gameDoubt = clampf(g.gameDoubt, 0, 100);
    std::string what = "He talked about fishing, and paid.";
    if (pct(learn)) {
        // what can be learned is whatever is true: a source in the station, an agent who is theirs, a place they know
        std::vector<int> moles, ctl, burned;
        for (auto& o : g.officers) if (o.mole && !o.gone) moles.push_back(o.id);
        for (auto& p : g.persons) if (p.stage == AGENT && controlled(p)) ctl.push_back(p.id);
        for (auto& s : g.sites) if (s.burned && !s.retired) burned.push_back(s.id);
        int k = r().n(4);
        if (g.gameSuspect > 55 && pct(40)) {            // they have begun to play it back at us: a false lead
            std::vector<Officer*> clean;
            for (auto& o : g.officers) if (!o.mole && !o.gone && o.role != ROLE_REZIDENT) clean.push_back(&o);
            if (!clean.empty()) {
                Officer* o = clean[r().n((int)clean.size())];
                what = "He let slip, as if by accident, that they have a friend in your station - and described " + o->title() + " closely enough to be recognised.";
            }
        } else if (k == 0 && !moles.empty() && option == 2) {
            Officer* o = officer(moles[r().n((int)moles.size())]);
            what = "He knew the duty roster of your station for last month. Only a man inside could have given it: by his questions, an officer of the rank of " + std::string(RANK[o->rank]) + ".";
            g.gameFound++;
        } else if (k <= 1 && !ctl.empty()) {
            Person* p = person(ctl[r().n((int)ctl.size())]);
            what = "He asked about the " + p->employer + " in a way that showed they already know who there talks to us.";
            p->ciBy = -1; p->ciDay = g.day;
            file(*p, "In the operational game the adversary's questions suggested they know of a source at " + p->employer + ".");
            g.gameFound++;
        } else if (k <= 2 && !burned.empty()) {
            Site* s = site(burned[r().n((int)burned.size())]);
            what = "He suggested, smiling, that you avoid " + s->name + " - 'it is much too public'.";
            g.gameFound++;
        } else {
            float h = g.you.heat;
            what = std::string("His questions showed what they think of you: ") + (h < 30 ? "a minor figure they hope to use." : h < 60 ? "an officer of the service, watched from time to time." : "an important officer, watched closely.");
        }
    }
    g.gameLog += fmtShort(g.day) + fstr("  Lunch %d: ", g.gameRounds) + what + "\n";
    cable("Line KR", fstr("The operational game: lunch %d", g.gameRounds), "From the tape of Thursday's lunch with " + g.gameName + ": " + what + "\n\nLINE KR");
    if (g.gameSuspect >= 75) {
        g.game = 3;
        heat(8);
        g.gameLog += fmtShort(g.day) + "  He did not come to the next lunch.\n";
        cable("Line KR", "The operational game - ended by the other side", g.gameName + " did not come to lunch, and the restaurant says the table was cancelled. The other side has drawn its conclusions. "
              "Expect them to make life difficult for you for a while.\n\nLINE KR");
        return;
    }
    if (g.gameDoubt >= 70) {
        g.game = 3;
        g.standCentre = clampf(g.standCentre - 10, 0, 100);
        g.gameLog += fmtShort(g.day) + "  Centre closed the game.\n";
        cable("Centre", "The operational game", "Centre has decided to close the operation. The material passed has been of a quality Centre did not authorise, and the reports of the meetings have not explained why. "
              "You will submit a full written account of every meeting.\n\nCENTRE");
        return;
    }
    g.gameNext = g.day + 21 + r().n(14);
}
