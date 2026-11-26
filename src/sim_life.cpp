// sim_life.cpp - the life around the work: the cars the watchers use and your own log of them,
// how you get to an act, the agents' own lives (illness, promotion, a posting abroad ...), and the
// monthly returns that the Returns screen draws.
#include "sim.h"

// ============================================================================ the watchers' cars
namespace {
struct CarList { std::vector<const char*> makes; };

const std::vector<const char*>& makesFor(int theatre, int year) {
    static const std::vector<const char*> MOSCOW = {"Volga GAZ-21", "Moskvich-408", "Volga GAZ-21", "Zaporozhets", "Moskvich-412"};
    static const std::vector<const char*> LONDON = {"Ford Cortina", "Ford Zephyr", "Hillman Minx", "Morris Oxford", "Austin 1100", "Vauxhall Victor", "Rover 2000", "Morris Minor"};
    static const std::vector<const char*> VIENNA = {"Opel Rekord", "VW Beetle", "Fiat 124", "Steyr-Puch 500", "Ford Taunus", "Peugeot 504", "Mercedes 220"};
    static const std::vector<const char*> DELHI = {"Hindustan Ambassador", "Premier Padmini", "Standard Herald", "Willys Jeep", "Hindustan Ambassador"};
    static const std::vector<const char*> WASH = {"Ford LTD", "Chevrolet Impala", "Plymouth Fury", "Dodge Aspen", "AMC Concord", "Chevrolet Caprice", "Ford Fairmont"};
    static const std::vector<const char*> GER79 = {"VW Golf", "Opel Ascona", "Ford Taunus", "VW Passat", "Mercedes 200", "BMW 316", "Audi 80"};
    static const std::vector<const char*> GER86 = {"VW Golf", "VW Passat", "Opel Kadett", "Ford Escort", "Audi 80", "BMW 318", "Mercedes 190"};
    switch (theatre) {
        case TH_MOSCOW: return MOSCOW;
        case TH_LONDON: return LONDON;
        case TH_VIENNA: return VIENNA;
        case TH_DELHI: return DELHI;
        case TH_WASHINGTON: return WASH;
        default: return year < 1985 ? GER79 : GER86;
    }
}
const char* const COLOURS[] = {"grey", "dark blue", "beige", "dark green", "black", "white", "maroon", "light blue", "brown"};
const char* const RU_LETTERS[] = {"А", "Б", "В", "Г", "Д", "Е", "К", "Л", "М", "Н", "О", "П", "Р", "С", "Т", "У", "Х"};
}  // namespace

std::string Sim::makePlate() {
    auto L = [&]() { static const char* A = "ABCDEFGHJKLMNPRSTVWXY"; return A[r().n(21)]; };
    int y = dateOf(g.day).y;
    switch (g.theatre) {
        case TH_MOSCOW: return fstr("%02d-%02d МО", r().n(100), r().n(100)) + RU_LETTERS[r().n(17)];
        case TH_LONDON: {          // the year letter at the end: C for 1965 ... J for 1970-71
            static const char SUF[] = "CDEFGHJK";
            int newest = std::max(0, std::min(7, y - 1965 + 1));
            return fstr("%c%c%c %d%c", L(), L(), L(), 100 + r().n(900), SUF[r().n(newest + 1)]);
        }
        case TH_VIENNA: return fstr("W %d.%03d", 1 + r().n(899), r().n(1000));
        case TH_DELHI: return fstr("D%c%c %d", "LHE"[r().n(3)], L(), 1000 + r().n(9000));
        case TH_WASHINGTON: { int k = r().n(3);
                              return k == 0 ? fstr("%d-%03d", 100 + r().n(900), r().n(1000)) : k == 1 ? fstr("%c%c%c-%03d", L(), L(), L(), r().n(1000)) : fstr("%c%c %d", L(), L(), 1000 + r().n(9000)); }
        case TH_BONN: { static const char* const AREA[] = {"BN", "BN", "BN", "K", "SU", "AW"};
                        return fstr("%s-%c%c %d", AREA[r().n(6)], L(), L(), 10 + r().n(990)); }
        default: return fstr("B-%c%c %d", L(), L(), 10 + r().n(990));
    }
}

void Sim::makeCar(Car& c) {
    const auto& m = makesFor(g.theatre, dateOf(g.day).y);
    c.make = m[r().n((int)m.size())];
    c.colour = COLOURS[r().n(9)];
    c.plate = makePlate();
}

/** A new posting: the adversary's fleet of surveillance cars, and an empty log. */
void Sim::makePool() {
    g.pool.clear();
    g.watchers.clear();
    int n = training() ? 6 : 10 + r().n(5);
    for (int i = 0; i < n; i++) { Car c; makeCar(c); g.pool.push_back(c); }
    g.replateDay = g.day + 140 + r().n(100);
}

/** Every few months the watchers change the plates on some of their cars, and replace others. */
void Sim::replate() {
    if (g.pool.empty() || g.day < g.replateDay) return;
    for (auto& c : g.pool) {
        if (!pct(35)) continue;
        if (pct(30)) makeCar(c);         // a different car altogether
        else c.plate = makePlate();     // the same car, new plates
    }
    g.replateDay = g.day + 140 + r().n(100);
}

/** The cars on you in this block (the team takes three of the pool). */
static void teamCars(const GameState& g, int b, int out[3]) {
    int n = (int)g.pool.size();
    uint32_t h = (uint32_t)g.day * 2654435761u + (uint32_t)b * 40503u;
    for (int i = 0; i < 3; i++) { h ^= h >> 13; h *= 1274126177u; out[i] = n ? (int)(h % (uint32_t)n) : -1; }
}

/** How much the log helps today: each car of today's team that you know by its plate. */
float Sim::recognise(int b) {
    if (g.pool.empty()) return 0;
    int cars[3];
    teamCars(g, b, cars);
    float bonus = 0;
    for (int i = 0; i < 3; i++) {
        if (cars[i] < 0) continue;
        for (auto& w : g.watchers)
            if (!w.struck && w.plate == g.pool[cars[i]].plate) { bonus += 6; break; }
    }
    return std::min(bonus, 14.f);
}

/** After an approach: what you noticed goes into the log. */
std::string Sim::noteVehicle(int b, int district, int signs, bool cov) {
    if (signs == 0 || g.pool.empty()) return "";
    Car seen;
    int poolIdx = -1;
    if (cov) {
        if (!pct(signs == 2 ? 70 : 40)) return "";
        int cars[3];
        teamCars(g, b, cars);
        poolIdx = cars[r().n(3)];
        seen = g.pool[poolIdx];
    } else {
        if (!pct(55)) return "";       // a false alarm: an innocent car goes into the log
        makeCar(seen);
    }
    for (auto& w : g.watchers)
        if (w.plate == seen.plate && w.make == seen.make) {
            w.seen++; w.lastDay = g.day;
            if (std::find(w.districts.begin(), w.districts.end(), district) == w.districts.end()) w.districts.push_back(district);
            return " The " + w.colour + " " + w.make + ", " + w.plate + ", again" + (w.struck ? " (you had struck it off)." : fstr(" - seen %d times now.", w.seen));
        }
    Watcher w;
    w.id = g.nextId++;
    w.make = seen.make; w.colour = seen.colour; w.plate = seen.plate;
    w.firstDay = w.lastDay = g.day; w.seen = 1; w.districts.push_back(district); w.pool = poolIdx;
    g.watchers.push_back(w);
    return " You noted a " + w.colour + " " + w.make + ", " + w.plate + " (Surveillance).";
}

void Sim::strikeWatcher(int id, bool struck) {
    for (auto& w : g.watchers) if (w.id == id) w.struck = struck;
}

/** Once a month Line KR checks one entry of your log against what it knows. */
void Sim::krVehicles() {
    std::vector<Watcher*> open;
    for (auto& w : g.watchers) if (w.kr == 0 && !w.struck) open.push_back(&w);
    if (open.empty() || !pct(60)) return;
    Watcher& w = *open[r().n((int)open.size())];
    std::string what = w.colour + " " + w.make + ", " + w.plate;
    std::string ans;
    if (w.pool < 0) { w.kr = 2; ans = "is registered to a private owner with no connection to the services. Strike it off."; }
    else if (g.pool[w.pool].plate != w.plate) { w.kr = 3; ans = "was one of theirs, but the plates have been changed or the car withdrawn. It will not be seen again under that number."; }
    else { w.kr = 1; ans = "is registered to a firm the " + std::string(T().watchers) + " use as cover. It is one of theirs."; }
    cable("Line KR", "Vehicles in your log", "Of the vehicles in your surveillance log, the " + what + " " + ans + "\n\nLINE KR");
}

// ============================================================================ the agents' own lives
void Sim::lifeEvent(Person& p) {
    if (p.postDay >= 0 && g.day >= p.postDay) { leaveForPost(p); return; }
    if (controlled(p) || !pct(0.30f)) return;
    bool spySvc = p.service == SVC_ADVERSARY || p.service == SVC_SECURITY;
    switch (r().n(10)) {
        case 0:
            p.tru[A_ACCESS] = p.app[A_ACCESS] = clampf(p.tru[A_ACCESS] + rnd(6, 15), 0, 97);
            p.warn.push_back("has been promoted and now sees a wider range of papers.");
            break;
        case 1:
            p.tru[A_ACCESS] = p.app[A_ACCESS] = clampf(p.tru[A_ACCESS] - rnd(10, 25), 5, 97);
            p.warn.push_back("has been moved to another section. His access is reduced.");
            break;
        case 2:
            p.stress += 15;
            p.warn.push_back("has trouble at home and is drinking more than he should.");
            break;
        case 3:
            p.suspicion += p.empSec > 50 ? 8 : 2;
            p.warn.push_back("says routine security interviews are being held throughout his office.");
            break;
        case 4:
            p.sickUntil = g.day + 14 + r().n(40);
            p.warn.push_back("has been ill - some weeks in hospital - and has photographed nothing.");
            break;
        case 5: {
            if (p.age > 58 || p.postDay >= 0 || p.codename.empty()) break;
            static const char* const CITY[] = {"Brussels", "Paris", "Geneva", "Rome", "Ottawa", "Tokyo", "Ankara", "Canberra", "Lisbon", "Oslo"};
            p.postCity = CITY[r().n(10)];
            p.postDay = g.day + 50 + r().n(50);
            p.postGo = 0; p.postAsked = false;
            p.warn.push_back("has been offered a posting to " + p.postCity + " for three years, from " + fmtDay(p.postDay) + ". He has not yet answered.");
            break;
        }
        case 6:
            if (p.age < 56) break;
            p.tru[A_ACCESS] = p.app[A_ACCESS] = std::max(4.f, p.tru[A_ACCESS] * 0.5f);
            p.warn.push_back("is to retire within the year. His access is already being reduced as he hands over.");
            break;
        case 7:
            if (g.theatre == TH_WASHINGTON || spySvc) {
                bool trouble = p.suspicion > 15 || p.tru[A_NERVE] < 40;
                if (trouble) p.suspicion += 20 * (1.3f - p.tru[A_NERVE] / 100.f) * (1 - p.craft / 150.f);
                p.stress += 10;
                p.warn.push_back(trouble ? "was given a polygraph examination and was called back for a second session. He says he passed."
                                         : "was given a routine polygraph examination. He is sure he passed.");
            } else {
                if (p.empSec > 60) p.suspicion += 4;
                p.warn.push_back("must renew his security clearance: a long form, and an interview about his foreign contacts.");
            }
            break;
        case 8:
            p.stress += 8;
            p.tru[A_DISC] = std::max(5.f, p.tru[A_DISC] - 6);
            p.warn.push_back("has a new partner, who must not know.");
            break;
        default:
            p.tru[A_MONEY] = p.app[A_MONEY] = std::max(3.f, p.tru[A_MONEY] - 20);
            p.commitment = clampf(p.commitment - 8, 0, 100);
            p.warn.push_back("has come into money - an inheritance - and talks of giving it all up.");
            break;
    }
}

/** At the meeting where he tells you of the posting, you have to say what he should do. */
void Sim::postingDecision(Person& p) {
    if (p.postDay < 0 || p.postAsked || p.stage != AGENT) return;
    p.postAsked = true;
    addDecision(DK_POSTED, p.id, "A posting abroad: " + p.codename,
                gx(&p, p.codename + " (" + p.name + ", " + p.job + ") has been offered a three-year posting to " + p.postCity + ", leaving " + fmtDay(p.postDay) + ".\n\n"
                           "If he goes, he leaves your network; Centre will hand him to the rezidentura there, and credits you with the man. "
                           "If he refuses, he keeps his present access - but people ask why a man turns down a good posting."),
                {gx(&p, "Let him go. Centre hands him to the rezidentura in " + p.postCity + "."),
                 gx(&p, "Let him go - but use the weeks that are left: collection at High tempo until he leaves."),
                 gx(&p, "Persuade him to turn the posting down.")});
}

void Sim::leaveForPost(Person& p) {
    std::string city = p.postCity;
    p.postDay = -1;
    if (p.stage != AGENT) return;
    clearAppointments(p);
    p.stage = TERMINATED;
    p.nextMeet = -1; p.nextDrop = -1;
    for (int id : p.held) if (Roll* x = roll(id)) x->state = R_LOST;
    p.held.clear();
    float worth = p.tru[A_ACCESS];
    g.standCentre = clampf(g.standCentre + 1.5f + worth / 40.f, 0, 100);
    file(p, "Left for " + city + ". His file has gone to Centre; the rezidentura there will make contact.");
    Person* keep = subj; subj = &p;
    alert(p.codename + " has left for " + city + ". Centre has his file and will hand him to the rezidentura there.");
    cable("Centre", "Agent " + p.codename + " - transfer", p.codename + " has taken up his posting in " + city + ". Contact will be restored there by the local rezidentura on the conditions you established. "
                                                            "Centre thanks the rezidentura for a source kept in good order.\n\nCENTRE");
    subj = keep;
}

// ============================================================================ the monthly returns
void Sim::monthRecord() {
    MonthRec m;
    m.day = g.day - 1; m.theatre = g.theatre;
    m.spotted = g.mSpotted; m.meetings = g.mMeetings; m.aborts = g.mAborts; m.rolls = g.mRolls; m.value = g.mValue;
    m.rez = g.standRez; m.centre = g.standCentre; m.funds = g.funds; m.kr = g.krEst;
    for (auto& p : g.persons) if (p.stage == AGENT) m.agents++;
    g.months.push_back(m);
}
