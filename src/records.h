// records.h - the service records kept across careers: the citations a career can earn, and the ways a
// career can end. The game writes one line per finished career to "service-records.txt" beside the saves.
#pragma once

struct Citation { const char* title; const char* text; };
enum {
    CI_FULL, CI_GENERAL, CI_BANNER, CI_CLEAN, CI_RECRUITER, CI_NETWORK, CI_NOBODY, CI_WASHINGTON, CI_BONN,
    CI_HARD, CI_IRONMAN, CI_PNG, CI_TAKEN, CI_LATE, CI_ALLBACK, N_CITATION
};
inline const Citation CITATIONS[N_CITATION] = {
    {"Full term", "Serve until August 1991."},
    {"General's stars", "Retire as a Major-General."},
    {"Red Banner", "Be awarded the Order of the Red Banner for a tour."},
    {"Clean record", "Reach 1991 without being expelled or recalled."},
    {"Recruiter", "Recruit ten agents in one career."},
    {"A network", "Leave six agents in place at the end of a tour."},
    {"Nobody lost", "Recruit three agents in one tour and lose none of them."},
    {"The main adversary", "Complete a tour in Washington."},
    {"Rezident", "Command the station in Bonn."},
    {"Unforgiving", "Reach 1991 on the Unforgiving setting."},
    {"One life", "Reach 1991 in an ironman career."},
    {"Persona non grata", "Be expelled by name."},
    {"Taken in", "Recruit a man who was the adversary's all along."},
    {"Late start", "Reach 1991 in a career begun after 1968."},
    {"Every road", "Finish careers with all four backgrounds."},     // counted across careers, not within one
};

struct Ending { const char* key; const char* title; };
inline const Ending ENDINGS[] = {
    {"1991-general", "August 1991 - a department in the new service"},
    {"1991-kept", "August 1991 - kept on"},
    {"1991-retired", "August 1991 - retired to a bank in the Arbat"},
    {"archives", "A desk in the archives"},
    {"reserve", "Transferred to the reserve"},
    {"lefortovo", "Lefortovo"},
    {"defector", "Defector"},
};
constexpr int N_ENDING = (int)(sizeof(ENDINGS) / sizeof(ENDINGS[0]));
