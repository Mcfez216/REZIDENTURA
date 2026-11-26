// sim.h - the rules. One call to endDay() resolves the three blocks of the current day, then the
// other officers' work, then the night (agents working, drops being serviced, the adversary
// investigating, the lab, the cipher room, the pouch), then rolls the dawn of the next day.
//
//   sim_day.cpp     orders and everything that happens in the street
//   sim_night.cpp   night, dawn, Centre's answers, the other officers, history
//   sim_career.cpp  starting a posting, ending it, the years at Centre, the end of the career
//   sim_spy.cpp     officers of foreign services, exfiltration, listening devices
//   sim_tutorial.cpp the training course
//   sim_life.cpp    the watchers' cars, getting about, agents' own lives, the monthly returns
//   sim_world.cpp   weather and light, holidays, safe flats, Centre's verification, the operational game
//   sim_events.cpp  the events deck, the changing city, agents' skills
//   sim_trade.cpp   chicken feed and playback, confidential contacts, walk-in letters, escape plans, the summons
//   sim_office.cpp  the year's plan and attestation, home and leave, your own money, your patron, compromise operations
//   sim_text.cpp    the texts the screens show (dossier, stores, station, accounts ...)
//
// The screens never change the game except through the public functions of this class.
#pragma once
#include "content.h"
#include "core.h"
#include <set>

std::string gx(const Person* p, const std::string& s);      // "he/his/him" -> "she/her/her" when p is a woman
std::string word(float v, const char* const* w, int n);
int ulen(const std::string& s);                             // length in characters (UTF-8 aware)
std::string pad(const std::string& s, int n);
std::string pad(const std::string& s, int n, char fill);
std::string clip(const std::string& s, int n);
std::string fstr(const char* fmt, ...);
bool strikeToday(const GameState& g);                       // a transport strike today (sim_events.cpp)
bool meetsAllowed(const GameState& g);                      // stood down, but agents' agreed meetings are kept

extern const char* W_RAPPORT[5];
extern const char* W_COMMIT[5];
extern const char* W_LEVEL[5];
extern const char* W_STAND[5];

class Sim {
public:
    /** When set, a meeting or a drop of yours that meets surveillance stops the day and asks you (DK_STREET). The screens set it from the settings. */
    bool askInStreet = false;
    /** Your own operational acts, as your surveillance log records them: when and where. Nothing hidden. */
    struct Pattern { int grid[7][3] = {}; int dist[8] = {}; int n = 0, slotDow = -1, slotB = -1, slotN = 0, topDist = -1, topDistN = 0; };
    Pattern myPattern(int days) const;
    std::string patternWord(const Pattern& p) const;        // how predictable it looks: "varied" ... "a timetable"
    // ---- birthdays: everybody has one, drawn from the world number, and Centre's files note the round ones
    int birthday(const Person& p, int year) const;          // the day (game day number) of his birthday in that year
    int birthYear(const Person& p) const { return dateOf(p.firstDay).y - p.age; }
    int nearBirthday(const Person& p, int day) const;       // the birthday within ten days of that day, or -99999
    bool presentDue(const Person& p, int day) const;        // a present on that day would be in time, and none has been given
    GameState g;
    /** Set when something happened that the player should look at ("run until event" stops on it). */
    bool attention = false;

    static const int FRAMES = 50;                           // exposures on one Minox cassette

    // ---- starting, saving, loading
    void newCareer(uint64_t seed, const std::string& officer, int difficulty = 1, bool withTraining = false);
    /** A career begun anywhere: at the course (TH_MOSCOW) or at any posting, with a background, difficulty and ironman rule. */
    struct CareerOpts { int start = TH_MOSCOW; int background = 0; int difficulty = 1; bool ironman = false; };
    void newCareer(uint64_t seed, const std::string& officer, const CareerOpts& o);
    static int startRank(int theatre);
    const Officer* rezident() const;
    // the service records (records.h): how this career ended, what it earned, and the line kept for it
    std::string endingKey() const;
    std::vector<int> careerCitations() const;
    std::string archiveLine() const;
    void skipTraining();
    std::vector<uint8_t> save();
    bool load(const std::vector<uint8_t>& data);

    // ---- lookups
    const Theatre& T() const { return THEATRES[g.theatre]; }
    bool training() const { return T().training; }
    /** How hard the world pushes back: 0.75 forgiving, 1 standard, 1.25 unforgiving. */
    float hard() const { return g.difficulty == 0 ? 0.75f : (g.difficulty == 2 ? 1.25f : 1.f); }
    /** A district on our own side of a frontier, where the adversary cannot follow anybody. */
    bool eastSide(int district) const { return T().east[district] && !g.wallDown; }
    Person* person(int id);
    Site* site(int id);
    Roll* roll(int id);
    Requirement* req(int id);
    Officer* officer(int id);
    int P(int base) const { return (int)std::lround(base * T().price); }   // a price in local money
    int cipherQuota() const { return 300 + 100 * T().role; }
    int labPerDay() const { return 2 + T().role; }
    float& lang() { return g.langs[T().lang]; }
    std::string handlerName(const Person& p);
    bool controlled(const Person& p) const { return p.dangle || p.doubled; }
    Site* agreedSite(const Person& p);
    std::string shortSite(const Site* s);

    // ---- the day planner
    Order routine(int day, int b) const;
    bool isSet(int day, int b) const;
    Order order(int day, int b) const;
    void setOrder(int day, int b, Order o);
    void clearOrder(int day, int b);
    std::string describe(const Order& o);
    std::string validate(const Order& o, int day, int b);   // empty = legal

    // ---- staff actions (no time cost, but cables queue)
    std::string requestTrace(Person& p);
    std::string requestSanction(Person& p);
    std::string requestFunds();
    std::string requestFilm();
    std::string requestCamera();
    std::string requestContainers();
    std::string requestPads();
    std::string requestCS(int day);
    std::string cableSummary(Roll& x);
    std::string terminate(Person& p);
    std::string setHandler(Person& p, int officerId);
    std::string radioMsg(Person& p, int tempo, int reqId, bool callout);
    std::string instruct(Person& p, int tempo, int reqId);
    std::string requestExfil(Person& p);
    std::string requestBug();
    std::string denounce(Officer& o);
    void strikeWatcher(int id, bool struck);
    std::string rentFlat(int district);
    int flatCost(int district) const;
    std::string requestVerify(Person& p);
    bool dayOff(int day) const;
    std::string holiday(int day) const;
    std::string weatherLine() const;
    // ---- the officer's own affairs (sim_office.cpp)
    std::string bookLeave(int startDay);
    std::string cancelLeave();
    bool onLeave(int day) const;
    std::string setWifeSignal(int siteId);
    std::string buyHomeGift();
    std::string buyGoods();
    std::string giftChief();
    std::string setSkim(int tenths);
    std::string buyAsset(int k);
    std::string prepareCompromise(Person& p, int kind);
    int kompCost(int kind) const;
    std::string privateText();
    // ---- the trade (sim_trade.cpp)
    std::string proposePlayback(Person& p);
    std::string registerContact(Person& p);
    std::string padReport();
    // ---- conveniences for the player (sim_text.cpp)
    enum { TD_PLAN, TD_PEOPLE, TD_SITES, TD_MAT, TD_CENTRE, TD_STATION, TD_ACC, TD_HOME };
    struct Todo { int level; int tab; std::string text; };       // level: 2 urgent, 1 soon, 0 worth knowing; tab: which screen deals with it
    std::vector<Todo> todo();
    std::string renameAgent(Person& p, const std::string& name);
    std::string personalFile();
    std::string planText();
    void retireSite(Site& s) { s.retired = true; }
    void decide(int decisionId, int option);
    void chooseInterlude(int option);

    // ---- the turn
    void endDay();
    static bool isOp(int t);

    // ---- texts for the screens (sim_text.cpp)
    std::string status1();
    std::string status2();
    std::string diary(int day);
    std::string siteNotes(const Site& s);
    std::string dossier(const Person& p, bool withHead = true);
    std::string siteReport(const Site& s);
    std::string districtText(int d);
    std::string serviceNote(const Person& p);
    std::string officerFile(const Officer& o);
    std::string storesText();
    std::string stationText();
    std::string survSummary();
    std::string accSummary();
    std::string careerText();
    std::string summary();
    std::string commsText(const Person& p);
    std::string wittingText(const std::vector<int>& w);
    int unread() const;

private:
    struct StreetHalt { int block; std::string title, text; std::vector<std::string> opts; };
    void endDayInner();
    std::string streetStakes(const Order& o, Site* st);
    void birthdayMeeting(const Order& o, Person& p);
    Rng& r() { return g.rng; }
    float rnd(float lo, float hi) { return g.rng.rnd(lo, hi); }
    bool pct(float c) { return g.rng.pct(c); }
    template <size_t N> const char* pick(const char* const (&a)[N]) { return a[g.rng.n((int)N)]; }

    // --- who is acting, and who the text is about
    std::vector<std::string>* rep = nullptr;
    Person* subj = nullptr;
    Actor* A = nullptr;                                     // the officer in the street just now
    Officer* O = nullptr;                                   // null = you
    bool me() const { return O == nullptr; }
    void actAs(Officer* o) { O = o; A = o ? &o->a : &g.you; }
    float fatigueNow() const { return me() ? g.fatigue : 25.f; }
    float noseNow() const { return me() ? g.assessment : O->nose; }
    void tire(float f) { if (me()) g.fatigue += f; }
    void paper(float h) { g.paperwork += me() ? h : h * 0.3f; }

    bool sdrDone[3] = {false, false, false};
    int sdrRoute[3] = {-1, -1, -1};
    std::set<int> attempted;
    std::vector<std::string> gradeLines;
    int pngCause = 0;

    void say(const std::string& s);
    void alert(const std::string& s);
    void file(Person& p, const std::string& s);
    void cable(const std::string& from, const std::string& subject, const std::string& body);
    void money(int delta, const std::string& what);
    void queue(int kind, int ref, int groups, const std::string& subject, int p1 = -1, int p2 = -2, int p3 = 0);
    void pend(int day, int kind, int ref, int amt, int p2 = -2, int p3 = 0);
    void witness(std::vector<int>& list, int officerId);
    void heat(float d);

    // --- foreign services, exfiltration, devices (sim_spy.cpp)
    Person makeSpy(float dangleChance);
    void ciReport(Person& src);
    void krTip();
    void exfiltrate(Person& p);
    void bugNight(Person& p, bool work);
    void arrest(Person& p, const std::string& how);

    // --- the watchers' cars, getting about, agents' lives, the monthly returns (sim_life.cpp)
    void makePool();
    std::string makePlate();
    void makeCar(Car& c);
    float recognise(int b);
    std::string noteVehicle(int b, int district, int signs, bool cov);
    void replate();
    void krVehicles();
    void lifeEvent(Person& p);
    void leaveForPost(Person& p);
    void postingDecision(Person& p);
    void monthRecord();

    // --- the weather, flats, verification, the operational game (sim_world.cpp)
    void weather(bool first);
    int sunTime(int day, bool set) const;
    void flatMeeting(Site& s, Person& p, bool observed, bool longMeet);
    void docSession(const Order& o, Person& p, Site& s);
    void verifyRoll(Person& p, Roll& x);
    void gameStart();
    void gameSanction();
    void gameRound();
    void gameDecide(int kind, int option);
    void embassyReception(int b);

    // --- events, the changing city, agents' skills (sim_events.cpp)
    void events();
    void eventDecide(int ref, int option);
    void cityChanges();
    void referral(Person& p);
    void referralArrives(int agentId);
    void trainAgent(Person& p, int what, bool flat);
    void initSkills(Person& p, bool officer);

    // --- the plan, home and leave, the purse and the patron, compromise operations (sim_office.cpp)
    void planDawn(const Date& d);
    void planDecide(int option);
    void attestation(bool endOfTour);
    void homeNight();
    void officeMonthly();
    void officeHistory(const Date& d);
    void wifeDecide(int option);
    void leaveDawn();
    void leaveDecide(int option);
    int payAgent(Person& p, int amt);
    void skimExposed(const std::string& how);
    void patronFalls(const std::string& how);
    void stageCompromise(Person& p);
    void kompSanction(Person& p);
    void dirtDecide(int option);
    // --- chicken feed, confidential contacts, the letter through the door, escape plans, the summons (sim_trade.cpp)
    void playSanction(Person& p);
    void playbackMeet(Person& p, Site& s);
    void confidRegistered(Person& p);
    void confidTalk(Person& p);
    void walkInLetter();
    void letterTakenUp(bool genuine, int svc);
    void walkInDecide(int ref, int option);
    void letterAnswer(int ref);
    void rehearseEscape(Person& p);
    void escapeSignal(Person& p);
    void escapeSeen(Site& s);
    void escapePickup(Person& p);
    void summons();
    void summonsDecide(int option);

    // --- the training course (sim_tutorial.cpp)
    void tutorialSetup();
    void tutorialDawn();
    void tutorialNight();
    void tutorialIssue();
    void endTraining(bool completed);

    // --- generators (sim_career.cpp)
    int pickNext(float score, std::string& why);
    std::string personName(bool maleOnly, bool& female);
    Person makePerson(int sector, float dangleChance);
    void makeDangle(Person& p);
    Site makeSite(int type, int district);
    Site& inherit(int type, int district);
    Requirement* nextRequirement();
    std::string officerName();
    void startPosting(int theatre, bool first);
    void endPosting(int how, const std::string& title, const std::string& text);
    void finishCareer(const std::string& title, const std::string& text);
    void casingEstimate(Site& s);

    // --- the day (sim_day.cpp)
    struct Ap { bool abort = false, observed = false; std::string txt; int signs = 0; };
    Ap approach(const Order& o, int b, int district, Site* site);
    void resolve(const Order& o, int b);
    void spot(int sector, const std::string& how, int handler);
    void caseSite(const Order& o, int b);
    void scout(const Order& o, int b);
    void meetOvert(const Order& o, int b);
    void meetClan(const Order& o, int b);
    void agentAgenda(const Order& o, Person& p, Site& s);
    void brushAgenda(const Order& o, Person& p);
    void contactAgenda(const Order& o, Person& p, Site& s);
    void commsPlan(const Order& o, Person& p);
    void pitch(const Order& o, Person& p, Site& s);
    void placeAM(const Order& o, Person& p);
    void socialGain(Person& p, float mult);
    void favour(Person& p);
    void gift(Person& p, int amount);
    void loadDrop(const Order& o, int b);
    void unloadDrop(const Order& o, int b);
    void readSignal(const Order& o, int b);
    void placeSignal(const Order& o, int b);
    void lineN(const Order& o, int b);
    void controlOp(const Order& o, int b);
    bool stakeoutSeen(const Order& o, Site& s);
    void stake(Site& s);
    void advNotices(Person& p);
    void flagrante(Person* p, Site* s);
    void learn(Person& p, int i, float power);
    void learnSome(Person& p, int n, float power);
    void tells(Person& p);
    std::string mood(Person& p);
    Roll& newRoll(Person& p);
    void csWitness(Person* p, Site* s);
    bool csToday(const Order& o) const;

    // --- appointments
    void clearAppointments(const Person& p);
    void schedule(Person& p);
    void fallback(Person& p);
    void nextAppointment(Person& p);

    // --- night and dawn (sim_night.cpp)
    void officersDay();
    void night();
    void sendCable(const OutCable& c);
    void agentNight(Person& p);
    void serviceDrop(Person& p);
    void caught(Person& p);
    void breakdown(Person& p);
    void contactNight(Person& p);
    void credit(int sector, float pts, int reqId);
    void grade(Roll& x);
    void dawn();
    void coverage(Actor& a, bool isYou);
    float habitLift(int b) const;
    void arrive(const Pending& x);
    void monthly();
    void requirements();
    void walkIn();
    void history(const Date& d);
    void moles();
    void defection(Officer& o);
    void blowTo(Officer& o, bool everything);
    void addDecision(int kind, int ref, const std::string& title, const std::string& text, std::vector<std::string> opts);
    void orderResupply();
    std::vector<int> pickTeam();
};
