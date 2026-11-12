// sim.h - the rules. One call to endDay() resolves the three blocks of the current day, then the
// other officers' work, then the night (agents working, drops being serviced, the adversary
// investigating, the lab, the cipher room, the pouch), then rolls the dawn of the next day.
//
//   sim_day.cpp     orders and everything that happens in the street
//   sim_night.cpp   night, dawn, Centre's answers, the other officers, history
//   sim_career.cpp  starting a posting, ending it, the years at Centre, the end of the career
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

extern const char* W_RAPPORT[5];
extern const char* W_COMMIT[5];
extern const char* W_LEVEL[5];
extern const char* W_STAND[5];

class Sim {
public:
    GameState g;
    /** Set when something happened that the player should look at ("run until event" stops on it). */
    bool attention = false;

    static const int FRAMES = 50;                           // exposures on one Minox cassette

    // ---- starting, saving, loading
    void newCareer(uint64_t seed, const std::string& officer);
    std::vector<uint8_t> save();
    bool load(const std::vector<uint8_t>& data);

    // ---- lookups
    const Theatre& T() const { return THEATRES[g.theatre]; }
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
    std::string denounce(Officer& o);
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
    std::string dossier(const Person& p);
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

    // --- generators (sim_career.cpp)
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
