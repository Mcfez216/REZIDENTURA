// core.h - everything the game REMEMBERS. No rules live here (they are in sim_*.cpp).
//
//   Rng      our own random generator, so a world number gives the same world on every machine
//   Ar       the save-game archive: every struct has an io() that lists its fields once,
//            and that one list is used for both saving and loading
//   dates    day 0 = Monday 2 September 1968; everything else is counted from there
//   model    Person, Site, Roll, Order, Requirement, Officer ... and GameState, which holds it all
//
// Convention: fields the PLAYER MUST NEVER SEE DIRECTLY are marked HIDDEN. The screens only ever
// show estimates, reports and things an officer could really know.
#pragma once
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <type_traits>
#include <vector>

// ============================================================================ random numbers
struct Rng {
    uint64_t s = 1;
    uint64_t next() {                       // splitmix64
        uint64_t z = (s += 0x9E3779B97F4A7C15ull);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
        return z ^ (z >> 31);
    }
    float f() { return (float)((next() >> 40) / 16777216.0); }          // 0 <= f < 1
    int n(int k) { return k <= 0 ? 0 : (int)(next() % (uint64_t)k); }    // 0 .. k-1
    bool pct(float chance) { return f() * 100.f < chance; }
    float rnd(float lo, float hi) { return lo + f() * (hi - lo); }
    bool coin() { return (next() & 1) != 0; }
    float gauss() {                         // normal distribution, mean 0, deviation 1
        float u = std::max(f(), 1e-7f), v = f();
        return std::sqrt(-2.f * std::log(u)) * std::cos(6.2831853f * v);
    }
};

inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

// ============================================================================ save-game archive
struct Ar {
    std::vector<uint8_t> buf;
    size_t pos = 0;
    bool writing = true, ok = true;

    void raw(void* p, size_t n) {
        if (writing) { const uint8_t* b = (const uint8_t*)p; buf.insert(buf.end(), b, b + n); }
        else if (pos + n <= buf.size()) { std::memcpy(p, buf.data() + pos, n); pos += n; }
        else { ok = false; std::memset(p, 0, n); }
    }
    template <class T> Ar& operator&(T& v) {
        if constexpr (std::is_arithmetic<T>::value || std::is_enum<T>::value) raw(&v, sizeof(T));
        else v.io(*this);
        return *this;
    }
    Ar& operator&(std::string& s) {
        uint32_t n = (uint32_t)s.size(); raw(&n, 4);
        if (!writing) { if (n > buf.size()) { ok = false; n = 0; } s.resize(n); }
        if (n) raw(&s[0], n);
        return *this;
    }
    template <class T> Ar& operator&(std::vector<T>& v) {
        uint32_t n = (uint32_t)v.size(); raw(&n, 4);
        if (!writing) { if (n > buf.size()) { ok = false; n = 0; } v.clear(); v.resize(n); }
        for (auto& x : v) *this & x;
        return *this;
    }
    template <class K, class V> Ar& operator&(std::map<K, V>& m) {
        uint32_t n = (uint32_t)m.size(); raw(&n, 4);
        if (writing) { for (auto& kv : m) { K k = kv.first; *this & k & kv.second; } }
        else { m.clear(); if (n > buf.size()) { ok = false; n = 0; } for (uint32_t i = 0; i < n; i++) { K k{}; *this & k; *this & m[k]; } }
        return *this;
    }
    template <class T, size_t N> Ar& operator&(T (&v)[N]) { for (size_t i = 0; i < N; i++) *this & v[i]; return *this; }
    template <class T, size_t N> Ar& operator&(std::array<T, N>& v) { for (auto& x : v) *this & x; return *this; }
};

// ============================================================================ dates
// Civil-date arithmetic (Howard Hinnant's public-domain algorithms).
inline int daysFromCivil(int y, int m, int d) {
    y -= m <= 2;
    const int era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153u * (unsigned)(m + (m > 2 ? -3 : 9)) + 2) / 5 + (unsigned)d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (int)doe - 719468;
}
struct Date { int y, m, d, dow; };          // dow: 0 = Monday ... 6 = Sunday
inline const int EPOCH = daysFromCivil(1968, 9, 2);
inline int dayOf(int y, int m, int d) { return daysFromCivil(y, m, d) - EPOCH; }
inline Date dateOf(int day) {
    int z = day + EPOCH + 719468;
    const int era = (z >= 0 ? z : z - 146096) / 146097;
    const unsigned doe = (unsigned)(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153;
    Date r;
    r.d = (int)(doy - (153 * mp + 2) / 5 + 1);
    r.m = (int)(mp < 10 ? mp + 3 : mp - 9);
    r.y = (int)yoe + era * 400 + (r.m <= 2);
    r.dow = (((day % 7) + 7) % 7);          // day 0 is a Monday
    return r;
}
inline bool weekend(int day) { return dateOf(day).dow >= 5; }
inline const char* DOW[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
inline const char* MON[] = {"", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
inline const char* MONTH[] = {"", "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
inline std::string fmtDay(int day) {         // "Mon 2 Sep 1968"
    if (day < -100000) return "-";
    Date d = dateOf(day); char b[40];
    std::snprintf(b, sizeof b, "%s %d %s %d", DOW[d.dow], d.d, MON[d.m], d.y);
    return b;
}
inline std::string fmtShort(int day) {       // "2 Sep 68"
    Date d = dateOf(day); char b[40];
    std::snprintf(b, sizeof b, "%d %s %02d", d.d, MON[d.m], d.y % 100);
    return b;
}

// ============================================================================ the model
enum Sector { POLITICAL, MILITARY, SCITECH, SECURITY, N_SECTOR };
inline const char* SECTOR_LABEL[] = {"Political (Line PR)", "Military", "Science & Technology (Line X)", "Security services (Line KR)"};
inline const char* SECTOR_SHORT[] = {"Political", "Military", "Sci-tech", "Security"};

enum Stage { SPOTTED, DEVELOPING, AGENT, BROKEN, ARRESTED, TERMINATED };
inline const char* STAGE_LABEL[] = {"Spotted", "Developing", "Agent", "Contact broken", "Arrested", "Terminated"};

enum SiteType { ST_MEET, ST_DROP, ST_SIGNAL, ST_ROUTE, N_SITETYPE };
inline const char* SITETYPE_LABEL[] = {"Meeting site", "Dead drop", "Signal site", "Detection route"};

inline const char* BLOCK_LABEL[] = {"Morning", "Afternoon", "Evening"};

/** Everything you can put in one block of the day planner. */
enum OType { O_COVER, O_OFFICE, O_REST, O_FUNCTION, O_CASE, O_SCOUT, O_SDR, O_MEET_OVERT, O_MEET_CLAN,
             O_LOAD_DROP, O_UNLOAD_DROP, O_READ_SIGNAL, O_PLACE_SIGNAL, O_LINE_N, O_CONTROL, N_OTYPE };
inline const char* OTYPE_LABEL[] = {
    "Cover duties", "Rezidentura: paperwork", "Rest", "Attend function / spot contacts", "Case a site or walk a route",
    "Scout district for a new site or route", "Surveillance detection route", "Overt social meeting", "Clandestine meeting",
    "Load dead drop", "Unload dead drop", "Read signal site", "Place call-out signal",
    "Line N task: load drop for an illegal", "Control operation (marked card)"};

// Attribute indexes for a person. tru[] is reality, app[] is what they SHOW, est*[] is your file.
enum { A_ACCESS, A_MONEY, A_IDEO, A_EGO, A_VULN, A_NERVE, A_DISC, A_HONEST, A_INFL, N_ATTR };
inline const char* ATTR[] = {"Access", "Money motive", "Ideological sympathy", "Ego / resentment", "Vulnerability",
                             "Nerve", "Discretion", "Reliability", "Influence (press etc.)"};
inline const char* TEMPO[] = {"On ice (no collection)", "Low", "Normal", "High"};
inline const char* POLICY[] = {"Strict - abort on any doubt", "Normal - abort on confirmed surveillance", "Press on regardless"};
inline const char* PITCH[] = {"Money", "Ideology", "Ego / recognition", "Coercion"};

// Material states - the supply chain from the agent's office to Moscow.
enum { R_AGENT, R_DROP, R_YOU, R_LAB, R_READY, R_POUCH, R_CENTRE, R_GRADED, R_LOST };
inline const char* RSTATE[] = {"Held by agent", "In dead drop", "With the case officer", "Rezidentura lab queue", "Developed - awaiting pouch",
                               "In diplomatic pouch", "At Centre - under evaluation", "Evaluated", "LOST"};

// Pending (delayed) event kinds.
enum { P_TRACE = 1, P_SANCTION, P_FUNDS, P_FILM, P_CAMERA, P_CONTAINER, P_GRADE, P_POUCH_ARRIVE, P_RETURN,
       P_RESUPPLY, P_KR, P_RADIO, P_PADS, P_SECRET, P_LINEN_PKG };
// Outgoing cable kinds (they queue for the cipher clerk).
enum { C_TRACE = 1, C_SANCTION, C_FUNDS, C_SUMMARY, C_KR, C_RADIO };

// Requirement kinds.
enum { RK_COLLECT, RK_ACTIVE, RK_LINEN };

// Officer roles and assignments.
enum { ROLE_REZIDENT, ROLE_KR, ROLE_TECH, ROLE_COLLEAGUE, ROLE_SUB };
enum { AS_SUPPORT, AS_SPOT, AS_HANDLE, AS_REST };
inline const char* ASSIGN_LABEL[] = {"Support you (countersurveillance on call)", "Spotting", "Handling / developing his cases", "Stood down"};

inline const char* RANK[] = {"Captain", "Major", "Lieutenant-Colonel", "Colonel", "Major-General"};
inline const char* RANK_ABBR[] = {"Capt.", "Maj.", "Lt-Col.", "Col.", "Maj-Gen."};

// Game phases.
enum { PH_PLAY, PH_INTERLUDE, PH_OVER };
// How a posting ended.
enum { END_TOUR, END_PNG, END_EXPELLED, END_RECALLED };

/** Somebody who walks the streets for the service: you, or one of the other officers. */
struct Actor {
    float tradecraft = 40, street = 8;
    float heat = 14, peakHeat = 14;         // HIDDEN - the adversary's interest in this officer
    bool covered[3] = {false, false, false}; // HIDDEN - today's surveillance, rolled at dawn
    int teamQ[3] = {50, 50, 50};             // HIDDEN - quality of the team on him
    template <class A> void io(A& a) { a & tradecraft & street & heat & peakHeat & covered & teamQ; }
};

struct Person {
    int id = 0;
    std::string name, job, employer, codename;
    int age = 0;
    bool female = false;
    int sector = 0, district = 0, stage = SPOTTED;

    float tru[N_ATTR] = {};                  // HIDDEN - reality
    float app[N_ATTR] = {};                  // HIDDEN - what the person presents
    float estV[N_ATTR] = {};                 // your assessment
    float estC[N_ATTR] = {};                 // your confidence in it, 0-100
    float empSec = 0;                        // HIDDEN - how good the employer's security is
    float craft = 0;                         // agent's own tradecraft (you train this)
    float diligence = 0;                     // HIDDEN - how reliably they service drops / signals / radio
    bool dangle = false;                     // HIDDEN - controlled by the adversary from the start
    bool doubled = false;                    // HIDDEN - caught and turned
    bool fabricator = false;                 // HIDDEN - pads / invents material
    bool walkIn = false, orphan = false;
    int trapDay = -1;                        // HIDDEN - when the adversary intends to close the trap
    int exposeDay = -1;                      // HIDDEN - an inherited source whose name is in somebody else's files

    float rapport = 0, commitment = 0;       // HIDDEN exact values, shown as words
    float stress = 0;                        // HIDDEN, you read their mood at meetings
    float suspicion = 0;                     // HIDDEN - investigation progress at his office
    bool knownAdv = false;                   // HIDDEN - the adversary has noticed your contact with them

    // --- agent handling
    int tempo = 0, reqId = -1;
    bool camera = false, radio = false;
    int film = 0, frames = 0, pads = 0;
    float frameVal = 0;                      // HIDDEN running quality of the cassette in the camera
    std::vector<int> held;                   // finished roll ids still with the agent
    int salary = 0, owed = 0, paidTotal = 0, paid90 = 0;

    // --- communications plan
    int meetEvery = 0;
    std::vector<int> meetSites;
    int meetIdx = 0, nextMeet = -1, misses = 0;
    bool callout = false;
    int dropEvery = 0, dropSite = -1, signalSite = -1, nextDrop = -1;
    int fetchDay = -1;                       // when a handling officer means to empty the drop

    // --- bureaucracy
    int sanction = 0;                        // 0 none, 1 requested, 2 granted, 3 refused
    int traceState = 0;                      // 0 none, 1 requested, 2 answered
    bool traceBad = false;
    int firstDay = 0, lastContact = -1, meetings = 0, overtObserved = 0, lastPitchDay = -9999, lastAmDay = -9999;
    std::vector<std::string> warn;           // things they will tell you next meeting
    std::vector<std::string> file;           // the dossier log

    // --- who runs him, and who knows
    int handler = 0;                         // 0 = you, otherwise an officer id
    bool pitchAuth = false;                  // you have authorised the handling officer to make the approach
    bool recommended = false;                // the handling officer has already told you he thinks the man is ripe
    int instrTempo = -1, instrReq = -2;      // your standing instruction to the handling officer, passed on at his next meeting
    std::vector<int> witting;                // officer ids who know this person is ours

    std::string label() const { return codename.empty() ? name : codename; }
    std::string full() const { return codename.empty() ? name : codename + " (" + name + ")"; }
    bool live() const { return stage == SPOTTED || stage == DEVELOPING || stage == AGENT; }

    template <class A> void io(A& a) {
        a & id & name & job & employer & codename & age & female & sector & district & stage & tru & app & estV & estC
          & empSec & craft & diligence & dangle & doubled & fabricator & walkIn & orphan & trapDay & exposeDay
          & rapport & commitment & stress & suspicion & knownAdv & tempo & reqId & camera & radio & film & frames & pads
          & frameVal & held & salary & owed & paidTotal & paid90 & meetEvery & meetSites & meetIdx & nextMeet & misses
          & callout & dropEvery & dropSite & signalSite & nextDrop & fetchDay & sanction & traceState & traceBad
          & firstDay & lastContact & meetings & overtObserved & lastPitchDay & lastAmDay & warn & file
          & handler & pitchAuth & recommended & instrTempo & instrReq & witting;
    }
};

struct Site {
    int id = 0;
    std::string name;
    int type = ST_MEET, district = 0;
    float cover = 50, sight = 50;            // HIDDEN - natural cover; how well you can see watchers there (for a route: its quality)
    float estCover = 50, estSight = 50;      // what your casing says
    int casing = 0;                          // 0-3
    float exposure = 0;                      // pattern built up by use; decays slowly
    int uses = 0, lastUsed = -1;
    bool burned = false;                     // HIDDEN - the adversary knows the site
    bool staked = false;                     // HIDDEN - they are physically waiting there
    int stakedDay = -1;
    bool retired = false;
    // dead drop contents
    std::vector<int> rolls;
    int cash = 0, film = 0;
    bool camera = false, container = false;
    int tempoNote = -1, reqNote = -2;
    int forAgent = -1, loadedDay = -1;
    // signal: HIDDEN truth 0 none, 1 "drop loaded", 2 DANGER, 3 your call-out
    int mark = 0, markSeen = -1, markSeenDay = -1;
    std::vector<int> witting;                // officer ids who know the site
    template <class A> void io(A& a) {
        a & id & name & type & district & cover & sight & estCover & estSight & casing & exposure & uses & lastUsed
          & burned & staked & stakedDay & retired & rolls & cash & film & camera & container & tempoNote & reqNote
          & forAgent & loadedDay & mark & markSeen & markSeenDay & witting;
    }
};

struct Roll {
    int id = 0, agentId = 0, reqId = -1, frames = 0, state = 0, stateDay = 0, shotDay = 0, sector = 0;
    float value = 0;                         // HIDDEN until graded
    bool genuine = true;                     // HIDDEN
    bool oral = false, cabled = false;
    std::string grade;
    template <class A> void io(A& a) { a & id & agentId & reqId & frames & state & stateDay & shotDay & sector & value & genuine & oral & cabled & grade; }
};

struct Order {
    bool set = false;                        // false = "follow routine"
    int type = O_COVER;
    int person = -1, site = -1, district = 0, sector = 0, siteType = ST_MEET;
    int policy = 1;                          // abort policy, see POLICY
    bool cs = false;                         // countersurveillance team requested for this act
    int route = -1;                          // prepared route to walk (SDR), -1 = improvise
    // meeting agenda
    bool longMeet = true, train = false, favour = false, collect = true;
    int pay = 0, film = 0, tempo = -1, reqId = -2, pitch = -1, pads = 0;
    bool camera = false, container = false;
    int am = -1;                             // active-measure requirement to place through this person
    // communications plan changes (-1 / -2 = leave alone)
    int meetEvery = -1, dropEvery = -1, dropSite = -2, signalSite = -2;
    bool setMeetSites = false;
    std::vector<int> meetSites;
    bool appointment = false;                // generated from an agent's communications plan
    // special operations
    int officer = -1;                        // control operation: the one colleague who is told
    int lineN = -1;                          // Line N requirement being carried out
    template <class A> void io(A& a) {
        a & set & type & person & site & district & sector & siteType & policy & cs & route & longMeet & train & favour & collect
          & pay & film & tempo & reqId & pitch & pads & camera & container & am & meetEvery & dropEvery & dropSite & signalSite
          & setMeetSites & meetSites & appointment & officer & lineN;
    }
};
typedef std::array<Order, 3> DayPlan;

struct Requirement {
    int id = 0, issued = 0, due = -1, priority = 2, sector = 0, kind = RK_COLLECT, district = -1;
    std::string title, text;
    float need = 100, progress = 0;
    bool closed = false, failed = false;
    template <class A> void io(A& a) { a & id & issued & due & priority & sector & kind & district & title & text & need & progress & closed & failed; }
};

struct Cable {
    int day = 0;
    std::string from, subject, body;
    bool read = false;
    template <class A> void io(A& a) { a & day & from & subject & body & read; }
};

struct OutCable {
    int kind = 0, ref = 0, groups = 0, queued = 0, p1 = -1, p2 = -2, p3 = 0;
    std::string subject;
    template <class A> void io(A& a) { a & kind & ref & groups & queued & p1 & p2 & p3 & subject; }
};

struct Pending {
    int day = 0, kind = 0, ref = 0, amt = 0, p2 = -2, p3 = 0;
    template <class A> void io(A& a) { a & day & kind & ref & amt & p2 & p3; }
};

struct Ledger {
    int day = 0, amt = 0, bal = 0;
    std::string what;
    template <class A> void io(A& a) { a & day & amt & bal & what; }
};

struct SdrLog {
    int day = 0, block = 0, district = 0, signs = 0;   // signs: 0 nothing, 1 possible, 2 confirmed
    std::string act, route;
    bool abort = false;
    template <class A> void io(A& a) { a & day & block & district & signs & act & route & abort; }
};

struct Officer {
    int id = 0, role = ROLE_COLLEAGUE, rank = 0;
    std::string name;
    Actor a;
    float nose = 40, diligence = 70;         // assessment of people; reliability
    bool mole = false;                       // HIDDEN - works for the adversary
    int moleDay = -1;                        // HIDDEN - from which day
    int defectDay = -1;                      // HIDDEN - the day he goes over
    bool cautious = false;                   // HIDDEN - a mole who knows he has been looked at
    bool gone = false;
    int assign = AS_SUPPORT, sector = 0, policy = 1;
    int lastAct = -99;
    std::vector<std::string> file;
    std::string title() const { return std::string(RANK_ABBR[rank]) + " " + name; }
    template <class A> void io(A& ar) { ar & id & role & rank & name & a & nose & diligence & mole & moleDay & defectDay & cautious & gone & assign & sector & policy & lastAct & file; }
};

struct Decision {
    int id = 0, kind = 0, ref = 0, day = 0;
    std::string title, text;
    std::vector<std::string> opts;
    template <class A> void io(A& a) { a & id & kind & ref & day & title & text & opts; }
};
enum { DK_APPROACH = 1, DK_RYAN, DK_ORPHANS };

struct CsDay {
    int state = 0;                           // 1 requested, 2 approved, 3 refused
    int o1 = -1, o2 = -1;                    // the two officers detailed
    template <class A> void io(A& a) { a & state & o1 & o2; }
};

/** The record of one finished posting, for the Career screen and the final reckoning. */
struct PostRec {
    int theatre = 0, startDay = 0, endDay = 0, how = END_TOUR, rankAtEnd = 0, cause = 0;   // cause of a PNG: 1 provocation, 2 turned agent, 3 watched
    int spotted = 0, recruited = 0, agentsAtEnd = 0, arrested = 0, rolls = 0, reqDone = 0, aborts = 0;
    float value = 0, score = 0;
    std::string verdict, award;
    std::vector<std::string> agents;         // one line per agent, with the truth about each
    template <class A> void io(A& a) { a & theatre & startDay & endDay & how & rankAtEnd & cause & spotted & recruited & agentsAtEnd & arrested & rolls & reqDone & aborts & value & score & verdict & award & agents; }
};

struct GameState {
    uint32_t version = 3;
    uint64_t seed = 0;
    Rng rng;
    int day = 0;                             // 0 = Monday 2 September 1968
    std::string officer = "Sokolov";
    int rank = 0;
    int phase = PH_PLAY;

    // --- the career
    int theatre = 0;                         // index into THEATRES
    int postStart = 0, postEnd = 0;
    int png = 0, recalls = 0, blown = 0, poorPostings = 0;
    std::vector<PostRec> record;
    std::vector<std::string> awards;
    std::string interludeTitle, interludeText;
    std::vector<std::string> interludeOpts;
    std::string overTitle, overText;
    float langs[2] = {58, 20};               // English, German

    // --- you
    Actor you;
    float assessment = 40, fatigue = 10;
    float cover = 70;                        // credibility of your cover job
    float standRez = 50, standCentre = 45;
    float paperwork = 2;                     // hours of reports outstanding
    int overdueDays = 0;
    int funds = 120;
    int film = 6, cameras = 0, containers = 3, pads = 0, lineNPkgs = 0;
    int stationFilm = 40, stationCameras = 2, stationPads = 6;
    bool resupplyOrdered = false;
    int secBonusUntil = -1;                  // good Line KR material sharpens your eye for watchers
    int ryanPolicy = -1;                     // -1 not yet asked, 0 report as observed, 1 report what Centre expects

    // --- the world of this posting
    std::vector<Person> persons;
    std::vector<Site> sites;
    std::vector<Roll> rolls;
    std::vector<Requirement> reqs;
    std::vector<Cable> cables;
    std::vector<OutCable> outQ;
    std::vector<Pending> pending;
    std::vector<Officer> officers;
    std::vector<Decision> decisions;
    std::map<int, DayPlan> plan;
    std::map<int, std::vector<std::string>> reports;
    std::map<int, CsDay> csDays;
    std::vector<Ledger> ledger;
    std::vector<SdrLog> sdrLog;
    std::map<int, std::string> zenit;        // day -> what the morning summary said
    std::vector<std::string> krLog;          // Line KR's weekly assessments, as given
    int nextId = 1, nextReqNo = 1, codenameIdx = 0;
    std::vector<int> usedReqTemplates, histDone;
    std::vector<std::string> usedNames;

    // --- today's (hidden) surveillance picture, rolled at dawn
    float a4 = 0.8f;                         // HIDDEN - how stretched the watchers are
    float a4Base = 0.85f;                    // HIDDEN - long-run level; history moves it
    int lastReqIssue = -99, lastFundsReq = -9999, lastLineN = -9999, lastApproach = -9999;
    bool hot[8] = {};                        // HIDDEN - districts saturated today
    std::string brief;
    int groupsToday = 0;
    bool krPending = false;
    bool warnedLow = false;                  // your chief has already told you that you are close to recall

    // --- counters for the monthly review and the posting record
    int mSpotted = 0, mMeetings = 0, mAborts = 0, mRolls = 0;
    float mValue = 0;
    int totSpotted = 0, totRecruited = 0, totRolls = 0, totReqDone = 0, totArrested = 0, totAborts = 0;
    float totValue = 0;

    template <class A> void io(A& a) {
        a & version & seed & rng.s & day & officer & rank & phase & theatre & postStart & postEnd & png & recalls & blown & poorPostings
          & record & awards & interludeTitle & interludeText & interludeOpts & overTitle & overText & langs
          & you & assessment & fatigue & cover & standRez & standCentre & paperwork & overdueDays & funds
          & film & cameras & containers & pads & lineNPkgs & stationFilm & stationCameras & stationPads & resupplyOrdered
          & secBonusUntil & ryanPolicy & persons & sites & rolls & reqs & cables & outQ & pending & officers & decisions
          & plan & reports & csDays & ledger & sdrLog & zenit & krLog & nextId & nextReqNo & codenameIdx & usedReqTemplates
          & histDone & usedNames & a4 & a4Base & lastReqIssue & lastFundsReq & lastLineN & lastApproach & hot & brief
          & groupsToday & krPending & warnedLow & mSpotted & mMeetings & mAborts & mRolls & mValue & totSpotted & totRecruited & totRolls
          & totReqDone & totArrested & totAborts & totValue;
    }
};
