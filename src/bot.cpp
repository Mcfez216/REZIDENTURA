// bot.cpp - a test player with no screen. It plays whole careers on many worlds and prints how
// they went. Use it after changing a rule to see what you did to the balance, and to catch crashes.
//
//   bot [careers] [careful|reckless] [immortal] [first-seed]
//
// "immortal" wipes the expulsion count between postings so that every career reaches 1991
// (useful for exercising the late game).
#include "bot.h"

int main(int argc, char** argv) {
    int n = argc > 1 ? atoi(argv[1]) : 20;
    bool careful = true, immortal = false;
    uint64_t first = 1;
    for (int i = 2; i < argc; i++) {
        std::string a = argv[i];
        if (a == "reckless") careful = false; else if (a == "careful") careful = true; else if (a == "immortal") immortal = true; else first = (uint64_t)atoll(argv[i]);
    }
    int reached[4] = {}, n91 = 0, saveFail = 0;
    double score[4] = {}, value[4] = {}, recruited[4] = {}, arrested[4] = {};
    int how[4][4] = {}, cnt[4] = {}, moleCaught = 0, moles = 0, cause[4][8] = {};
    double days[4] = {};
    std::map<std::string, int> endings;
    for (uint64_t seed = first; seed < first + (uint64_t)n; seed++) {
        Sim s;
        s.newCareer(seed, "Testov");
        Bot bot(s, careful);
        int guard = 0, checked = 0;
        while (s.g.phase != PH_OVER && guard++ < 12000) {
            if (s.g.phase == PH_INTERLUDE) { if (immortal) { s.g.png = 0; s.g.recalls = 0; s.g.poorPostings = 0; } s.chooseInterlude((int)(seed % 3)); continue; }
            bot.plan();
            // save / load round trip: a loaded game must play on exactly as the original does
            if (guard % 700 == 350 && checked < 6) {
                checked++;
                std::vector<uint8_t> buf = s.save();
                Sim t;
                if (!t.load(buf)) { saveFail++; printf("LOAD FAILED seed %llu day %d\n", (unsigned long long)seed, s.g.day); }
                else {
                    Sim u; u.load(buf);
                    Bot b1(t, careful), b2(u, careful);
                    for (int k = 0; k < 25 && t.g.phase == PH_PLAY; k++) { b1.plan(); t.endDay(); b2.plan(); u.endDay(); }
                    if (t.save() != u.save()) { saveFail++; printf("NON-DETERMINISTIC after load: seed %llu day %d\n", (unsigned long long)seed, s.g.day); }
                }
            }
            if (getenv("BOTDBG") && s.g.phase == PH_PLAY) {
                static int lastT = -1; static double od = 0, cov = 0, nd = 0, pw = 0, fat = 0; static int kr = 0;
                if (s.g.theatre != lastT) { if (lastT >= 0) printf("   [dbg] theatre %d: days %.0f overdue%% %.0f avg cover %.0f avg paperwork %.1f avg fatigue %.0f\n", lastT, nd, 100 * od / nd, cov / nd, pw / nd, fat / nd); lastT = s.g.theatre; od = cov = nd = pw = fat = 0; }
                nd++; if (s.g.paperwork > 8) od++; cov += s.g.cover; pw += s.g.paperwork; fat += s.g.fatigue;
                if (dateOf(s.g.day).d == 2) printf("   [dbg] %s rez %.0f centre %.0f funds %d heat %.0f agents %d\n", fmtShort(s.g.day).c_str(), s.g.standRez, s.g.standCentre, s.g.funds, s.g.you.heat, (int)std::count_if(s.g.persons.begin(), s.g.persons.end(), [](const Person& p) { return p.stage == AGENT; }));
                if (dateOf(s.g.day).d == 2 && getenv("BOTDBG")[0] == '2') for (int i = (int)s.g.cables.size() - 1; i >= 0 && i > (int)s.g.cables.size() - 12; i--) if (s.g.cables[i].subject.find("Monthly review") == 0) { printf("        %s\n", s.g.cables[i].body.substr(0, s.g.cables[i].body.find('\n')).c_str()); break; }
                (void)kr;
            }
            float h0 = s.g.you.heat; int th0 = s.g.theatre; std::string key;
            if (getenv("HEATDBG") && s.g.phase == PH_PLAY) for (int b = 0; b < 3; b++) { Order o = s.order(s.g.day, b); if (o.type != O_COVER && o.type != O_REST && o.type != O_OFFICE && o.type != O_SDR) key += std::string(key.empty() ? "" : "+") + (o.type == O_MEET_CLAN ? (o.appointment ? "AGENTMEET" : (o.pitch >= 0 ? "PITCH" : "CLANCONTACT")) : OTYPE_LABEL[o.type]); }
            s.endDay();
            if (getenv("HEATDBG") && s.g.phase == PH_PLAY && s.g.theatre == th0) { static std::map<std::string, std::pair<double, int>> acc; static int nn = 0; auto& e = acc[key.empty() ? "(quiet day)" : key]; e.first += s.g.you.heat - h0; e.second++;
                if (++nn % 60000 == 0 || (seed == first + (uint64_t)n - 1 && guard > 11990)) {} 
                static bool reg = false; if (!reg) { reg = true; static auto* pacc = &acc; atexit([]() { for (auto& kv : *pacc) if (kv.second.second > 30) printf("   heat %+8.0f over %6d days (%+.2f/day)  %s\n", kv.second.first, kv.second.second, kv.second.first / kv.second.second, kv.first.c_str()); }); } }
            if (s.g.funds < 0 || s.g.fatigue != s.g.fatigue || s.g.standCentre != s.g.standCentre) { printf("BAD STATE seed %llu day %d\n", (unsigned long long)seed, s.g.day); break; }
        }
        for (auto& rcd : s.g.record) {
            int t = rcd.theatre;
            reached[t]++; score[t] += rcd.score; value[t] += rcd.value; recruited[t] += rcd.recruited; arrested[t] += rcd.arrested; how[t][rcd.how]++; cnt[t]++; cause[t][rcd.cause & 7]++; days[t] += rcd.endDay - rcd.startDay;
            for (auto& a : rcd.agents) if (a.find("worked for the adversary") != std::string::npos) { moles++; if (a.find("did not finish") != std::string::npos) moleCaught++; }
        }
        endings[s.g.overTitle]++;
        if (s.g.overTitle.find("1991") != std::string::npos) n91++;
        if (n <= 12) { printf("seed %llu: %s, %s |", (unsigned long long)seed, s.g.overTitle.c_str(), RANK[s.g.rank]); static const char* H[] = {"tour", "PNG", "expelled", "RECALLED"}; for (auto& rc : s.g.record) printf(" %s %s %.0f (%dd, arr %d) |", THEATRES[rc.theatre].city, H[rc.how], rc.score, rc.endDay - rc.startDay, rc.arrested); printf("\n"); }
    }
    printf("\n%d careers (%s%s)\n", n, careful ? "careful" : "reckless", immortal ? ", immortal" : "");
    for (int t = 0; t < 4; t++) if (cnt[t])
        printf("  %-11s played %3d  days %5.0f  score %5.1f  worth %6.0f  recruited %4.1f  arrested %4.1f  | tour %d  PNG %d (dangle %d, turned %d, agent watched %d, drop staked %d)  expelled %d  recalled %d\n", THEATRES[t].city, cnt[t], days[t] / cnt[t], score[t] / cnt[t], value[t] / cnt[t],
               recruited[t] / cnt[t], arrested[t] / cnt[t], how[t][0], how[t][1], cause[t][1], cause[t][2], cause[t][3], cause[t][4], how[t][2], how[t][3]);
    for (auto& e : endings) printf("  ending: %-32s %d\n", e.first.c_str(), e.second);
    printf("  traitors in stations: %d, removed before the end of the tour: %d\n", moles, moleCaught);
    printf("  save/load failures: %d\n", saveFail);
    return saveFail ? 1 : 0;
}
