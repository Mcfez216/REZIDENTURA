// core.h - everything the game REMEMBERS. No rules live here (they are in sim_*.cpp).
//
//   Rng      our own random generator, so a world number gives the same world on every machine
//   Ar       the save-game archive: every struct has an io() that lists its fields once,
//            and that one list is used for both saving and loading
//   dates    day 0 = Monday 1 January 1968; everything else is counted from there
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
    uint32_t ver = 12;                       // the version of the save being read (older saves lack the newer fields)

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
inline const int EPOCH = daysFromCivil(1968, 1, 1);
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

enum Stage { SPOTTED, DEVELOPING, AGENT, BROKEN, ARRESTED, TERMINATED, EXFILTRATED };
inline const char* STAGE_LABEL[] = {"Spotted", "Developing", "Agent", "Contact broken", "Arrested", "Terminated", "Exfiltrated"};

/** Is this person an officer of somebody else's intelligence or security service? */
enum Service { SVC_NONE, SVC_SECURITY, SVC_ADVERSARY, SVC_THIRD };
//             civilian   the local security service (the people who follow you)
//                                       an intelligence service of the main adversary or its close allies
//                                                      the service of some third country
inline const char* SERVICE_LABEL[] = {"", "local security service", "adversary's intelligence service", "third-country service"};

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
inline const char* TRAVEL[] = {"On foot and by public transport", "Embassy car (diplomatic plates)", "Taxi"};
inline const char* SKILL_LABEL[] = {"Security (his own tradecraft)", "Photography of documents", "Elicitation (drawing out colleagues)", "Talent-spotting", "Communications (drops, signals, radio)"};
inline const char* SKILL_SHORT[] = {"Security", "Photography", "Elicitation", "Talent-spotting", "Communications"};
inline const char* PITCH[] = {"Money", "Ideology", "Ego / recognition", "Coercion"};

// Material states - the supply chain from the agent's office to Moscow.
enum { R_AGENT, R_DROP, R_YOU, R_LAB, R_READY, R_POUCH, R_CENTRE, R_GRADED, R_LOST };
inline const char* RSTATE[] = {"Held by agent", "In dead drop", "With the case officer", "Rezidentura lab queue", "Developed - awaiting pouch",
                               "In diplomatic pouch", "At Centre - under evaluation", "Evaluated", "LOST"};

// Pending (delayed) event kinds.
enum { P_TRACE = 1, P_SANCTION, P_FUNDS, P_FILM, P_CAMERA, P_CONTAINER, P_GRADE, P_POUCH_ARRIVE, P_RETURN,
       P_RESUPPLY, P_KR, P_RADIO, P_PADS, P_SECRET, P_LINEN_PKG, P_EXFIL, P_BUG, P_FLAT, P_VERIFY, P_GAME, P_REFERRAL, P_KOMP, P_SKIM, P_WALKIN, P_PLAY, P_CONFID, P_LETTER, P_KOBA, P_ESCAPE, P_LOSTOFFER };
// Outgoing cable kinds (they queue for the cipher clerk).
enum { C_TRACE = 1, C_SANCTION, C_FUNDS, C_SUMMARY, C_KR, C_RADIO, C_EXFIL, C_FLAT, C_VERIFY, C_GAME, C_KOMP, C_PLAY, C_CONFID };

// Requirement kinds.
enum { RK_COLLECT, RK_ACTIVE, RK_LINEN };

// Officer roles and assignments.
enum { ROLE_REZIDENT, ROLE_KR, ROLE_TECH, ROLE_COLLEAGUE, ROLE_SUB };
enum { AS_SUPPORT, AS_SPOT, AS_HANDLE, AS_REST };
inline const char* ASSIGN_LABEL[] = {"Support you (countersurveillance on call)", "Spotting", "Handling / developing his cases", "Stood down"};

// Where the officer came from. Each starts the career with different strengths.
inline const char* BACKGROUND_LABEL[] = {"Political line", "Counter-intelligence", "Institute of Languages", "Well connected"};
inline const char* BACKGROUND_TEXT[] = {
    "Line PR, the Higher School and two years on the British desk. Sound at everything, outstanding at nothing.",
    "Six years in the Second Chief Directorate, watching foreigners in Moscow. You know how surveillance works from the inside - and how hard it is to make a stranger like you.",
    "The Maurice Thorez Institute, then interpreting for delegations. Your languages are excellent and you read people well; the street is new to you.",
    "Your father-in-law sits on a Central Committee commission. Centre reads your reports kindly and doors open; the work itself you will learn on the job."};
// How a Rezident runs his station. You learn which kind you have by working for him.
inline const char* TEMPER_LABEL[] = {"steady", "a careerist", "cautious", "an old hand", "a stickler"};
inline const char* TEMPER_TEXT[] = {
    "He judges by results over months, not weeks.",
    "He wants numbers for his own returns. Good months are praised loudly; bad ones are remembered.",
    "He fears a scandal more than a thin month. Attention from the watchers worries him more than a short return.",
    "He has seen everything twice. Neither praise nor blame moves him far, and he forgives late paperwork.",
    "Paperwork, cover duties and the rules. Late reports and a neglected cover cost twice as much with him."};
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

    // --- an officer of a foreign service, and what he has told you about your own affairs
    int service = SVC_NONE;
    std::vector<int> told;                   // counter-intelligence items he has already given (so he does not repeat himself)
    int ciBy = -1, ciDay = -9999;            // on the person NAMED by a source: which source, and when
    // --- getting him out; a listening device in his office
    int exfil = 0;                           // 0 nothing, 1 asked of Centre, 2 travel documents are in your safe
    int bug = 0;                             // 0 none, 1 he has the device, 2 planted and live, 3 silent
    int bugDay = -1, bugUntil = -1, bugNext = -1;   // HIDDEN: when he plants it, when the battery dies, when the next tape is due
    // --- his skills, besides his own security (craft): you raise them by instruction at long meetings
    float skPhoto = 0, skElicit = 0, skSpot = 0, skComms = 0;
    int referrals = 0;                       // colleagues he has put your way
    // --- verification by Centre: 0 none, 1 asked, 2 control questions put to him, 3 done
    int verify = 0, verifyLeft = 0;
    int giftYear = -1;                       // the year whose birthday has been marked (or missed) - version 13
    std::string verifyResult;
    // --- his own life
    int sickUntil = -1;                      // off work, photographing nothing
    int postDay = -1, postGo = 0;            // a posting abroad: when he leaves; 1 = agreed that he goes
    bool postAsked = false;                  // you have been asked what to do about it
    std::string postCity;
    std::vector<int> witting;                // officer ids who know this person is ours
    // --- compromising material prepared against him: 0 none, 1 asked of Centre, 2 sanctioned (staged at the next long social meeting), 3 held, 4 failed
    int kompState = 0, kompKind = 0, kompDay = -9999;
    float komp = 0;                          // HIDDEN - how hard it really bites
    int shortPaid = 0;                       // HIDDEN from the books - what you kept back from his money
    // --- the trade (sim_trade.cpp)
    int fed = 0;                             // HIDDEN - deliveries made while under the adversary's control (the first are chicken feed)
    int play = 0, playMeets = 0;             // playback: 0 none, 1 proposed to Centre, 2 running, 3 ended
    int confid = 0, lastConfid = -9999;      // confidential contact: 0 no, 1 registration asked, 2 registered
    int escape = 0, escapeDay = -1;          // standing escape plan: 0 none, 2 rehearsed and standing, 3 he has signalled, 4 seen: the pick-up is on
    std::string note;                        // the player's own pencilled note on the file

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
          & handler & pitchAuth & recommended & instrTempo & instrReq & witting
          & service & told & ciBy & ciDay & exfil & bug & bugDay & bugUntil & bugNext
          & sickUntil & postDay & postGo & postAsked & postCity & verify & verifyLeft & verifyResult
          & skPhoto & skElicit & skSpot & skComms & referrals;
        if (a.ver >= 9) a & kompState & kompKind & kompDay & komp & shortPaid;
        if (a.ver >= 10) a & fed & play & playMeets & confid & lastConfid & escape & escapeDay;
        if (a.ver >= 11) a & note;
        if (a.ver >= 13) a & giftYear;
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
    // a safe flat (a meeting site indoors, rented through a cut-out)
    bool flat = false;
    int rent = 0;
    float neighbours = 0;                    // HIDDEN - how much the neighbours and the concierge have noticed
    template <class A> void io(A& a) {
        a & id & name & type & district & cover & sight & estCover & estSight & casing & exposure & uses & lastUsed
          & burned & staked & stakedDay & retired & rolls & cash & film & camera & container & tempoNote & reqNote
          & forAgent & loadedDay & mark & markSeen & markSeenDay & witting & flat & rent & neighbours;
    }
};

struct Roll {
    int id = 0, agentId = 0, reqId = -1, frames = 0, state = 0, stateDay = 0, shotDay = 0, sector = 0;
    float value = 0;                         // HIDDEN until graded
    bool genuine = true;                     // HIDDEN
    bool oral = false, cabled = false;
    bool tech = false;                       // a tape from a listening device rather than film
    std::string grade;
    template <class A> void io(A& a) { a & id & agentId & reqId & frames & state & stateDay & shotDay & sector & value & genuine & oral & cabled & tech & grade; }
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
    bool brush = false;                      // a brush pass: seconds, packages only, no talk
    bool exfil = false;                      // hand over travel documents: he leaves tonight
    bool bug = false;                        // give him a listening device to plant in his office
    int travel = 0;                          // how you get there: see TRAVEL
    bool docs = false;                       // at a safe flat: he brings the documents themselves, and you photograph them
    int trainWhat = 0;                       // what the instruction is in: see SKILL_LABEL
    bool present = false;                    // a present for his birthday (version 13)
    bool standing = false;                   // rehearse the escape plan and leave it standing (he stays in place)
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
          & setMeetSites & meetSites & appointment & officer & lineN & brush & exfil & bug & travel & docs & trainWhat;
        if (a.ver >= 10) a & standing;
        if (a.ver >= 13) a & present;
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
    /** The everyday traffic - grades of material, stores issued, the car log - filed without being marked as new. */
    bool routine() const {
        return subject.rfind("Evaluation of material", 0) == 0 || subject == "Stores issued" || subject == "Station stores" || subject == "Vehicles in your log";
    }
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
    int temper = 0;                          // a Rezident's way of running his station (TEMPER_LABEL)
    int assign = AS_SUPPORT, sector = 0, policy = 1;
    int lastAct = -99;
    int waryUntil = -1;                      // he saw surveillance, or Line KR named him: lying low until then (version 14)
    std::vector<std::string> file;
    std::string title() const { return std::string(RANK_ABBR[rank]) + " " + name; }
    template <class A> void io(A& ar) { ar & id & role & rank & name & a & nose & diligence & mole & moleDay & defectDay & cautious & gone & assign & sector & policy & lastAct & file; if (ar.ver >= 12) ar & temper; if (ar.ver >= 14) ar & waryUntil; }
};

struct Decision {
    int id = 0, kind = 0, ref = 0, day = 0;
    std::string title, text;
    std::vector<std::string> opts;
    template <class A> void io(A& a) { a & id & kind & ref & day & title & text & opts; }
};
enum { DK_APPROACH = 1, DK_RYAN, DK_ORPHANS, DK_POSTED, DK_GAME, DK_GAME_OFFER, DK_EVENT, DK_PLAN, DK_LEAVE, DK_WIFE, DK_WALKIN, DK_SUMMONS, DK_STREET,
       DK_OP_OFFER, DK_OP_NAME };

/** An operation: a piece of work with a name, a goal, a deadline, stages you can see, and a reckoning at the end (sim_ops.cpp). */
enum { OP_RECRUIT = 1, OP_LEAK, OP_RESCUE, OP_FLASH };
enum { OPS_OFFERED, OPS_ACTIVE, OPS_WON, OPS_LOST, OPS_DECLINED };
struct Operation {
    int id = 0, kind = 0, ref = -1, ref2 = -1, state = OPS_OFFERED, start = 0, due = 0, ended = -1, stage = 0;
    std::string name, title, brief, stakes;
    template <class A> void io(A& a) { a & id & kind & ref & ref2 & state & start & due & ended & stage & name & title & brief & stakes; }
};
inline const char* KOMP_LABEL[] = {"Money against signed receipts", "An arranged acquaintance, photographed", "Photographs of him taking an envelope from you"};
inline const char* PLAN_LABEL[] = {"modest", "firm", "ambitious"};
inline const char* ASSET_LABEL[] = {"A Zhiguli, through the cheque shop", "A cooperative flat in Moscow", "A dacha outside Moscow"};
inline const int ASSET_COST[] = {260, 600, 1100};
// the weather of the day
enum { WX_CLEAR, WX_CLOUD, WX_RAIN, WX_FOG, WX_SNOW };
inline const char* WX_LABEL[] = {"clear", "overcast", "rain", "fog", "snow"};

struct CsDay {
    int state = 0;                           // 1 requested, 2 approved, 3 refused
    int o1 = -1, o2 = -1;                    // the two officers detailed
    template <class A> void io(A& a) { a & state & o1 & o2; }
};

/** One of the cars the watchers use. HIDDEN: you only ever see what you wrote down. */
struct Car {
    std::string make, colour, plate;
    template <class A> void io(A& a) { a & make & colour & plate; }
};
/** An entry in your own log of the vehicles you believe the watchers use. */
struct Watcher {
    int id = 0;
    std::string make, colour, plate;
    int firstDay = 0, lastDay = 0, seen = 0;
    std::vector<int> districts;
    int kr = 0;                              // Line KR's word on it: 0 none, 1 theirs, 2 innocent, 3 no longer in use
    bool struck = false;                     // you have struck it off
    int pool = -1;                           // HIDDEN - which of their cars it is (-1: an innocent car)
    template <class A> void io(A& a) { a & id & make & colour & plate & firstDay & lastDay & seen & districts & kr & struck & pool; }
};
/** The month's returns, kept for the whole career. */
struct MonthRec {
    int day = 0, theatre = 0, spotted = 0, meetings = 0, aborts = 0, rolls = 0, agents = 0, funds = 0;
    float value = 0, rez = 0, centre = 0, kr = -1;     // kr: Line KR's estimate (as told), -1 none that month
    template <class A> void io(A& a) { a & day & theatre & spotted & meetings & aborts & rolls & agents & funds & value & rez & centre & kr; }
};

/** The record of one finished posting, for the Career screen and the final reckoning. */
struct PostRec {
    int theatre = 0, startDay = 0, endDay = 0, how = END_TOUR, rankAtEnd = 0, cause = 0;   // cause of a PNG: 1 provocation, 2 turned agent, 3 watched
    int spotted = 0, recruited = 0, agentsAtEnd = 0, arrested = 0, rolls = 0, reqDone = 0, aborts = 0, exfil = 0;
    float value = 0, score = 0;
    std::string verdict, award;
    std::vector<std::string> agents;         // one line per agent, with the truth about each
    template <class A> void io(A& a) { a & theatre & startDay & endDay & how & rankAtEnd & cause & exfil & spotted & recruited & agentsAtEnd & arrested & rolls & reqDone & aborts & value & score & verdict & award & agents; }
};

struct GameState {
    uint32_t version = 16;
    std::string notes;                       // the player's notebook
    int background = 0;                      // where the officer came from (BACKGROUND_LABEL)
    int startTheatre = 0;                    // where the career was begun
    bool ironman = false;                    // one career, one file: no saving copies, no going back
    bool archived = false;                   // the career's end has been entered in the service records
    int lastPad = -9999;                     // the last time the newspapers were written up as a source
    float padded = 0;                        // how much of that there has been lately
    bool padCaught = false;                  // Centre has noticed, this year
    // --- the year's work plan, and the attestation each December
    int planYear = 0, planLevel = -1, planStart = 0, planEnd = 0;   // planLevel: -1 no plan in force, else see PLAN_LABEL
    int planRec = 0, planSpot = 0;           // what you undertook: recruitments, new contacts ...
    float planVal = 0;                       // ... and evaluation points of material
    int baseRec = 0, baseSpot = 0;           // where the counters stood when the plan began
    float baseVal = 0;
    std::vector<std::string> attest;         // the attestations, kept for the Career screen
    // --- private life
    float home = 65;                         // HIDDEN - how things are at home (shown as words)
    bool wifeHere = true;
    int wifeSignal = -1;                     // the signal site your wife passes on her shopping
    int leaveYear = 0, leaveFrom = -1, leaveTo = -1;
    int lastHomeGift = -9999, lastChiefGift = -9999;
    // --- the private purse (kept in London-1968 pounds; shown in local money)
    int purse = 30;
    int skim = 0;                            // tenths of every agent's payment that you keep back
    int skimmed = 0;                         // what you have kept back at this station (local money)
    int goods = 0;                           // Western goods put by for taking home
    float patron = 20;                       // HIDDEN - how well your protector at Centre thinks of you
    std::string patronName;
    bool patronSaved = false;                // he has already spoken for you once at this station
    float dirt = 0;                          // HIDDEN - what the other side holds against you personally
    float gru = 50;                          // relations with the neighbours (the military intelligence rezidentura)
    int assets = 0;                          // what your savings have bought at home: bit 0 a car, 1 a cooperative flat, 2 a dacha
    int difficulty = 1;                      // 0 forgiving, 1 standard, 2 unforgiving
    int nextTheatre = -1;                    // where Centre is sending you next (decided when a posting ends)
    bool wallDown = false;                   // Berlin: after unification the East is no longer safe ground
    int bugs = 0;                            // listening devices in your safe
    int totExfil = 0;
    // --- the training course
    int tut = 0;                             // lesson number; 0 = not on the course
    int tutTries = 0, tutMarks = 0, tutMax = 0;
    int tutA = -1, tutB = -1;                // the two role-players
    int tutMeet = -1, tutDrop = -1, tutSignal = -1, tutRoll = -1;
    std::string tutText;                     // the instructor's current task
    std::vector<std::string> tutLog;         // his remarks so far
    std::string courseReport;                // kept for the Career screen
    // --- the watchers' cars, and the monthly returns
    std::vector<Car> pool;                   // HIDDEN
    std::vector<Watcher> watchers;
    int replateDay = -1;                     // HIDDEN - when they next change plates
    std::vector<MonthRec> months;
    float krEst = -1;                        // Line KR's last estimate of the interest in you (as told)
    // --- the weather and the light
    int wx = WX_CLOUD, wxNext = WX_CLOUD, temp = 10, tempNext = 10, sunset = 18 * 60, sunrise = 6 * 60;
    std::string forecast;
    // --- the other side's approach to you, played back under Line KR's control (an "operational game")
    int game = 0;                            // 0 none, 1 asked Centre, 2 running, 3 over
    int gameNext = -1, gameRounds = 0, gameFound = 0;
    float gameDoubt = 0, gameSuspect = 0;    // HIDDEN - Centre's doubt about you; the other side's doubt about the game
    std::string gameName, gameLog;
    // --- events of the day and the changing city
    int busyUntil = -1;                      // until this day your days belong to somebody else (a delegation, the doctor ...)
    int standDownUntil = -1;                 // Line KR has stood the station down: no operational acts
    std::string busyWhy;
    std::map<int, int> evLast;               // event -> the day it last happened
    // HIDDEN - what the adversary's analysts have learned of your habits from what their teams saw (version 14):
    // weight of your operational acts by weekday and block, and by district; and how hard they are pushing back
    float habit[7][3] = {}, habitD[8] = {}, advAlert = 0;
    int streetForce[3] = {-1, -1, -1};
    bool deputy = false;                     // your deputy runs the routine (version 15)
    std::vector<Operation> ops;              // operations offered, running and finished this posting (version 16)
    int lastOpDay = -9999, opsWon = 0, opsLost = 0;
    int deputyDay = -1;                      // the last day he planned      // what you decided in the street today, block by block (the day is replayed with it)
    float distMod[8] = {1, 1, 1, 1, 1, 1, 1, 1};   // HIDDEN - how hard the watchers are working each district, as the months go by
    int lastCarDay = -99;
    uint64_t seed = 0;
    Rng rng;
    int day = 0;                             // 0 = Monday 1 January 1968
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
        a & version;
        a.ver = version;
        if (!a.writing) version = 16;
        a & difficulty & nextTheatre & wallDown & bugs & totExfil & tut & tutTries & tutMarks & tutMax & tutA & tutB
          & tutMeet & tutDrop & tutSignal & tutRoll & tutText & tutLog & courseReport & pool & watchers & replateDay & months & krEst
          & wx & wxNext & temp & tempNext & sunset & sunrise & forecast & game & gameNext & gameRounds & gameFound & gameDoubt & gameSuspect & gameName & gameLog
          & busyUntil & standDownUntil & busyWhy & evLast & distMod & lastCarDay
          & seed & rng.s & day & officer & rank & phase & theatre & postStart & postEnd & png & recalls & blown & poorPostings
          & record & awards & interludeTitle & interludeText & interludeOpts & overTitle & overText & langs
          & you & assessment & fatigue & cover & standRez & standCentre & paperwork & overdueDays & funds
          & film & cameras & containers & pads & lineNPkgs & stationFilm & stationCameras & stationPads & resupplyOrdered
          & secBonusUntil & ryanPolicy & persons & sites & rolls & reqs & cables & outQ & pending & officers & decisions
          & plan & reports & csDays & ledger & sdrLog & zenit & krLog & nextId & nextReqNo & codenameIdx & usedReqTemplates
          & histDone & usedNames & a4 & a4Base & lastReqIssue & lastFundsReq & lastLineN & lastApproach & hot & brief
          & groupsToday & krPending & warnedLow & mSpotted & mMeetings & mAborts & mRolls & mValue & totSpotted & totRecruited & totRolls
          & totReqDone & totArrested & totAborts & totValue;
        if (a.ver >= 9)
            a & planYear & planLevel & planStart & planEnd & planRec & planSpot & planVal & baseRec & baseSpot & baseVal & attest
              & home & wifeHere & wifeSignal & leaveYear & leaveFrom & leaveTo & lastHomeGift & lastChiefGift
              & purse & skim & skimmed & goods & patron & patronName & patronSaved & dirt & gru & assets;
        if (a.ver >= 10) a & lastPad & padded & padCaught;
        if (a.ver >= 11) a & notes;
        if (a.ver >= 12) a & background & startTheatre & ironman & archived;
        if (a.ver >= 14) a & habit & habitD & advAlert & streetForce;
        if (a.ver >= 15) a & deputy & deputyDay;
        if (a.ver >= 16) a & ops & lastOpDay & opsWon & opsLost;
    }
};
