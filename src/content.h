// content.h - the shape of a posting ("theatre"). The four theatres themselves are in content.cpp,
// which is the only file you need to edit to change places, names, jobs, requirements or history.
#pragma once
#include <vector>

struct JobT { const char* job; const char* employer; int lo, hi; int sec; int infl; bool maleOnly; };
//            post            where                 access range  employer's security  influence  held only by men at the time

struct ReqT { int sector; const char* title; const char* text; int month; int need; int span; int kind; };
//            month = earliest month of the posting it can be issued; need = points (collection) or placements (active measure);
//            span = days allowed, 0 = standing requirement

struct SpyT { const char* job; const char* employer; int lo, hi; int service; };
//            an officer of a foreign service you may meet: post, service, access range, kind (see Service in core.h)

struct Pt { float x, y; };                  // a point on the city map, 0..1 across and down

enum { H_NONE, H_DEFECTION, H_FOOT, H_RYAN, H_ABLE, H_ORPHANS, H_WALL, H_QUIET, H_UNIFY, H_JANATA };
struct HistT { int y, m, d; const char* from; const char* title; const char* text; float a4Base; float a4Now; float heat; int code; };
//            a4Base = lasting change in the watchers' effort, a4Now = change for the next few days, heat = change in their interest in YOU

// The postings. The career has four slots; which posting fills slots 1-3 depends on how the last tour went.
enum { TH_MOSCOW, TH_LONDON, TH_VIENNA, TH_DELHI, TH_WASHINGTON, TH_BERLIN1, TH_BONN, TH_BERLIN2, N_THEATRE };

struct Theatre {
    int slot = 0;                           // -1 the training course, 0..3 the four postings of the career
    bool training = false;
    bool friends = false;                   // the local service is an ally: traces come quickly and catch more
    bool east[8] = {};                      // districts on our own side of a frontier: nobody follows you there
    float gradeMult = 1;                    // how much Centre cares for product from this place
    const char* cityRu = "";                // the city in Russian, for document headings
    Pt at[8] = {};                          // where each district lies on the map
    std::vector<Pt> river, wall;            // map decoration
    const char *city, *country, *cover, *embassy, *adversary, *watchers, *currency, *chief;
    int lang;                               // 0 English, 1 German
    int sy, sm, sd, ey, em, ed;             // first and last day of the posting
    int role;                               // 0 case officer, 1 senior officer, 2 line chief, 3 Rezident
    int subs, colleagues;                   // officers under you; other officers of the station
    float price;                            // local price level (London 1968 = 1)
    float a4Base, heat0, blownHeat, dangle, walkIn, valuePar;
    const char* district[8];
    int dist[8];                            // travel effort from the Embassy
    float density[8];                       // HIDDEN - chance the watchers are working the district on a given day
    const char* street[8][3];
    const char* park[8];
    const char* station[8];
    std::vector<const char*> venue, meetT, dropT, signalT;
    std::vector<const char*> male, female, last;
    std::vector<JobT> jobs[4];
    std::vector<SpyT> spies;
    std::vector<ReqT> reqs;
    std::vector<HistT> hist;
    const char* local;                      // the Rezident's words on local conditions
    const char* tour;                       // what you did at Centre before this posting
};

extern const std::vector<Theatre> THEATRES;
extern const std::vector<const char*> CODENAMES, RU_FIRST, RU_LAST;
