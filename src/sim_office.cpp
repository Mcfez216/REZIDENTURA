// sim_office.cpp - the officer himself, as distinct from his agents: the year's work plan and the
// December attestation; his wife, his leave, his own money and his protector at Centre; what the
// other side may come to hold against him; and the compromise operations he mounts against others.
#include "sim.h"
#include <algorithm>
#include <cmath>

extern const char* W_HOME[5];
const char* W_HOME[5] = {"very bad - she has stopped speaking of it", "strained", "tolerable", "good", "happy"};
extern const char* W_PATRON[5];
const char* W_PATRON[5] = {"He does not know your name", "He knows your name", "He asks after you", "He speaks for you when your name comes up", "You are his man, and it is known"};
extern const char* W_GRU[5];
const char* W_GRU[5] = {"hostile", "cold", "correct", "good", "close"};
extern const char* W_KOMP[5];
const char* W_KOMP[5] = {"slight", "slight", "moderate", "strong", "very strong"};

static std::string upper(std::string s) { for (auto& c : s) c = (char)toupper((unsigned char)c); return s; }

// ============================================================================ the plan and the attestation
/** Each January (and soon after you arrive) you undertake what the year will bring; each December you are judged on it. */
void Sim::planDawn(const Date& d) {
    if (training()) return;
    // the attestation
    if (g.planLevel >= 0 && (g.day >= g.planEnd)) attestation(false);
    // a new plan
    if (g.planYear == d.y || g.day - g.postStart < 10 || !g.decisions.empty() || g.day <= g.busyUntil) return;
    int end = std::min(dayOf(d.y, 12, 20), g.postEnd - 5);
    int span = end - g.day;
    g.planYear = d.y;
    if (span < 100) return;                       // too little of the year is left: no plan until January
    float frac = span / 350.f;
    float years = std::max(1.f, (g.postEnd - g.postStart) / 365.f);
    g.planRec = std::max(1, (int)std::lround((1.4f + 0.7f * T().role) * frac));
    g.planSpot = std::max(3, (int)std::lround((9 + 5.f * T().role) * frac));
    g.planVal = std::max(40.f, std::round(T().valuePar / years * frac / 10) * 10);
    g.planStart = g.day; g.planEnd = end;
    auto line = [&](float f) {
        int rec = std::max(1, (int)std::lround(g.planRec * f)), spot = std::max(2, (int)std::lround(g.planSpot * f));
        if (f > 1 && rec <= g.planRec) rec = g.planRec + 1;
        return fstr("%d recruitment%s, %d points of evaluated material, %d new contacts", rec, rec == 1 ? "" : "s", (int)std::lround(g.planVal * f / 10) * 10, spot);
    };
    addDecision(DK_PLAN, d.y, fstr("The work plan for %d", d.y),
                fstr("The %s wants your personal work plan for %d on his desk by Friday, for inclusion in the plan of the rezidentura, which goes to Centre for inclusion in the plan of the department. ", T().role == 3 ? "department" : "Rezident", d.y)
                    + "What you undertake now is what you will be measured against at the attestation on 20 December.\n\n"
                      "A bold plan is noticed at once, and remembered in December. A modest one is noticed too.",
                {"A modest plan: " + line(0.6f), "A firm plan: " + line(1.0f), "An ambitious plan: " + line(1.5f)});
}

void Sim::planDecide(int option) {
    static const float F[] = {0.6f, 1.0f, 1.5f};
    option = std::max(0, std::min(2, option));
    float f = F[option];
    int firm = g.planRec;
    g.planRec = std::max(1, (int)std::lround(g.planRec * f));
    if (f > 1 && g.planRec <= firm) g.planRec = firm + 1;
    g.planSpot = std::max(2, (int)std::lround(g.planSpot * f));
    g.planVal = std::lround(g.planVal * f / 10) * 10.f;
    g.planLevel = option;
    g.baseRec = g.totRecruited; g.baseSpot = g.totSpotted; g.baseVal = g.totValue;
    g.standCentre = clampf(g.standCentre + (option == 0 ? -2.f : option == 2 ? 3.f : 0.f), 0, 100);
    if (T().role < 3) g.standRez = clampf(g.standRez + (option == 0 ? -1.f : option == 2 ? 2.f : 0.f), 0, 100);
    cable(T().chief, fstr("Work plan for %d", g.planYear),
          fstr("Your plan is approved and has gone forward: %d recruitment%s, %d points of evaluated material, %d new contacts, by 20 December.\n\n", g.planRec, g.planRec == 1 ? "" : "s", (int)g.planVal, g.planSpot)
              + (option == 0 ? "It is the plan of a careful man. Centre has more careful men than it needs."
                 : option == 2 ? "This is the kind of undertaking Centre likes to read. See that it likes reading the results as much."
                               : "A proper plan. Fulfil it.")
              + "\n\n" + upper(T().chief));
}

/** The December attestation: the plan against the result, and what is written in your file. */
void Sim::attestation(bool endOfTour) {
    if (g.planLevel < 0) return;
    int level = g.planLevel;
    g.planLevel = -1;
    float part = 1;
    if (endOfTour) {
        part = clampf((g.day - g.planStart) / (float)std::max(1, g.planEnd - g.planStart), 0, 1);
        if (part < 0.35f) return;                 // the year had hardly begun
    }
    int rec = g.totRecruited - g.baseRec, spot = g.totSpotted - g.baseSpot;
    float val = g.totValue - g.baseVal;
    float r1 = std::min(1.3f, rec / std::max(0.5f, g.planRec * part)), r2 = std::min(1.3f, val / std::max(10.f, g.planVal * part)), r3 = std::min(1.3f, spot / std::max(1.f, g.planSpot * part));
    float f = 0.4f * r1 + 0.4f * r2 + 0.2f * r3;
    float dc, dr;
    std::string verdict;
    if (f >= 1.0f) { dc = 2 + 3.f * level; dr = 2 + level; verdict = level == 2 ? "PLAN OVERFULFILLED. Named in the directorate's order for the results of the year." : "Plan fulfilled."; }
    else if (f >= 0.7f) { dc = level == 0 ? -1.f : level == 1 ? -2.f : -4.f; dr = -1; verdict = "Plan not fulfilled in full."; }
    else { dc = level == 0 ? -3.f : level == 1 ? -5.f : -8.f; dr = -3; verdict = level == 2 ? "PLAN NOT FULFILLED. The officer undertook what he could not deliver." : "PLAN NOT FULFILLED."; }
    g.standCentre = clampf(g.standCentre + dc, 0, 100);
    if (T().role < 3) g.standRez = clampf(g.standRez + dr, 0, 100);
    if (f >= 1.0f && level == 2) g.patron = clampf(g.patron + 6, 0, 100);
    // the character reference, in the form such things take
    bool stable = g.home >= 35 && g.wifeHere && g.dirt < 30;
    std::string ch = std::string("Politically literate") + (stable ? ", morally stable" : "") + ". "
                     + (g.you.tradecraft >= 60 ? "Operationally experienced; " : "Gaining operational experience; ")
                     + (g.overdueDays > 0 || g.paperwork > 8 ? "careless of his reporting. " : "punctual in his reporting. ")
                     + (g.cover < 40 ? "Neglects his cover duties. " : "")
                     + (g.padCaught ? "Inclined to present material from open sources as the product of his contacts. " : "")
                     + (!stable && g.wifeHere && g.home < 35 ? "The situation in the officer's family requires the attention of the Party organisation. " : "")
                     + (!g.wifeHere ? "The officer's wife has returned to the Union; the reasons have been noted. " : "")
                     + (f >= 1.0f ? "Fit for promotion in due course." : f >= 0.7f ? "Fit for the post he holds." : "Requires closer supervision.");
    g.padCaught = false;
    int y = g.planYear;
    std::string result = fstr("Recruitments %d of %d undertaken. Evaluated material %d points of %d. New contacts %d of %d.", rec, (int)std::lround(g.planRec * part), (int)val, (int)std::lround(g.planVal * part), spot, (int)std::lround(g.planSpot * part));
    g.attest.push_back(fstr("%d  %s (%s plan): %d%%. ", y, T().city, PLAN_LABEL[level], (int)std::lround(f * 100)) + verdict);
    Person* keep = subj; subj = nullptr;
    cable(T().chief, fstr("Attestation for %d", y),
          fstr("ATTESTATION of %s %s for %d%s\n\n", RANK[g.rank], g.officer.c_str(), y, endOfTour ? " (on completion of the tour)" : "") + result + "\n\n" + verdict + "\n\n" + ch
              + "\n\nThe attestation has been read to the officer and placed on his personal file.\n\n" + upper(T().chief));
    subj = keep;
}

std::string Sim::planText() {
    if (training()) return "";
    if (g.planLevel < 0) return "  No plan in force. The next is drawn up in January.\n";
    int rec = g.totRecruited - g.baseRec, spot = g.totSpotted - g.baseSpot;
    float val = g.totValue - g.baseVal;
    float t = clampf((g.day - g.planStart) / (float)std::max(1, g.planEnd - g.planStart), 0, 1);
    auto bar = [](float f) { int n = (int)std::lround(clampf(f, 0, 1) * 10); return "[" + std::string(n, '#') + std::string(10 - n, '.') + "]"; };
    std::string sb = fstr("  A %s plan, to be met by %s.\n", PLAN_LABEL[g.planLevel], fmtShort(g.planEnd).c_str());
    sb += "  " + pad("Recruitments", 22) + bar(rec / (float)std::max(1, g.planRec)) + fstr("  %d of %d\n", rec, g.planRec);
    sb += "  " + pad("Evaluated material", 22) + bar(val / std::max(1.f, g.planVal)) + fstr("  %d of %d points\n", (int)val, (int)g.planVal);
    sb += "  " + pad("New contacts", 22) + bar(spot / (float)std::max(1, g.planSpot)) + fstr("  %d of %d\n", spot, g.planSpot);
    sb += "  " + pad("The year gone", 22) + bar(t) + "\n";
    return sb;
}

// ============================================================================ home, the wife, and leave
bool Sim::onLeave(int day) const { return g.leaveFrom >= 0 && day >= g.leaveFrom && day <= g.leaveTo; }

/** Each night: what the day did to your household. */
void Sim::homeNight() {
    if (training()) return;
    bool leave = onLeave(g.day);
    if (leave) {
        g.home = clampf(g.home + 0.8f, 0, 100);
        g.fatigue = 0;
        g.you.heat = std::max(8.f, g.you.heat - 0.2f);
        return;
    }
    if (!g.wifeHere) return;
    float d = 0;
    if (g.day <= g.busyUntil) d -= 0.3f;
    else {
        int ev = order(g.day, 2).type;
        d += ev == O_REST ? 0.15f : ev == O_FUNCTION ? -0.15f : ev == O_COVER ? -0.3f : -0.6f;
        if (dayOff(g.day))
            for (int b = 0; b < 2; b++) d += order(g.day, b).type == O_REST ? 0.15f : -0.4f;
    }
    d += (50 - g.home) * 0.01f;
    g.home = clampf(g.home + d, 0, 100);
    // her shopping takes her past the signal site, if you have asked it of her
    if (g.wifeSignal >= 0 && !dayOff(g.day)) {
        Site* s = site(g.wifeSignal);
        if (!s || s->retired || s->type != ST_SIGNAL) g.wifeSignal = -1;
        else if (g.home < 40) {
            if (dateOf(g.day).dow == 0) say("Your wife did not go past the signal site. She says she has a life of her own, such as it is.");
        } else {
            s->markSeen = s->mark; s->markSeenDay = g.day;
            g.home = clampf(g.home - 0.12f, 0, 100);
            if (pct(g.you.heat / 30.f)) s->burned = true;            // the wives of interesting men are followed too
            if (s->mark == 4) { say("Your wife passed " + s->name + " on her way home: a man was standing there with a carrier bag. She took a bar of chocolate out of her handbag and ate it as she went by."); escapeSeen(*s); }
            else if (s->mark == 2) alert("Your wife passed " + s->name + " on her way to the shops: THE DANGER SIGNAL IS UP.");
            else if (s->mark == 1) say("Your wife passed " + s->name + " on her way to the shops: the 'drop loaded' mark is up.");
        }
    }
}

void Sim::officeMonthly() {
    if (training()) return;
    // your own pay, such of it as is left
    g.purse += 6 + 3 * g.rank;
    g.patron = clampf(g.patron - 0.5f, 0, 100);
    if (g.patron > 8 && pct(0.25f)) patronFalls("has been retired on grounds of health");
    // the household
    if (g.wifeHere && !onLeave(g.day)) {
        if (g.home < 12) {
            g.standCentre = clampf(g.standCentre - 6, 0, 100); g.standRez = clampf(g.standRez - 4, 0, 100);
            g.home = 30;
            alert("Your wife has been to see the secretary of the Party organisation about the state of her marriage. He has spoken to the Rezident, and the Rezident has spoken to you.");
        } else if (g.home < 22) {
            bool asked = false;
            for (auto& x : g.decisions) if (x.kind == DK_WIFE) asked = true;
            if (!asked) addDecision(DK_WIFE, 0, "At home",
                                    "Your wife said it last night without raising her voice. She did not come abroad to sit in a compound flat watching the door, with a husband who comes home after midnight and cannot say where he has been. "
                                    "The children hardly know you. She wants to go home.",
                                    {"Promise her that it will change, and see that for a while it does", "Let her go back to Moscow with the children", "Ask the Rezident's wife to have a word with her"});
        } else if (g.home < 35 && pct(60)) {
            static const char* const L[] = {"At home there is not much conversation. You have been out most evenings this month.",
                                            "Your wife asked whether you remembered what day Saturday was. You did not.",
                                            "Your wife has taken to going to bed before you come in."};
            alert(L[r().n(3)]);
        }
    }
    // the neighbours
    if (g.gru < 22 && pct(50)) {
        g.standRez = clampf(g.standRez - 1.5f, 0, 100);
        alert("The Ambassador has complained to both Rezidents about 'quarrels between the services under his roof'. Yours was named.");
    }
    g.gru = clampf(g.gru + (50 - g.gru) * 0.03f, 0, 100);
    g.padded = std::max(0.f, g.padded - 0.5f);
    // what you keep back is talked about sooner or later
    if (g.skimmed > P(40)) {
        for (auto& o : g.officers)
            if (o.mole && !o.gone && o.moleDay <= g.day && pct(12)) { g.dirt = std::min(100.f, g.dirt + 30); break; }
    }
    // a year without leave
    Date d = dateOf(g.day);
    if (d.m == 1 && g.wifeHere && g.leaveYear != d.y - 1 && g.day - g.postStart > 420) {
        g.home = clampf(g.home - 10, 0, 100);
        alert("You took no leave last year. Your wife has not seen her mother in two years, and says so.");
    }
}

void Sim::wifeDecide(int option) {
    if (option == 0) g.home = clampf(g.home + 14, 0, 100);
    else if (option == 2) { g.home = clampf(g.home + 7, 0, 100); g.standRez = clampf(g.standRez - 1.5f, 0, 100); }
    else {
        g.wifeHere = false; g.wifeSignal = -1;
        g.cover = std::max(0.f, g.cover - 8);
        g.standCentre = clampf(g.standCentre - 2, 0, 100);
        g.home = 45;
        cable("Line KR", "Your family", "It is noted that your family has returned to the Union. An officer living alone abroad is of interest to the adversary in ways that a married one is not. "
              "You will report any new acquaintance, of either sex, on the day you make it.\n\nLINE KR");
    }
}

std::string Sim::bookLeave(int startDay) {
    if (training()) return "There is no leave on the course.";
    if (g.leaveFrom >= 0 && g.day <= g.leaveTo) return g.day >= g.leaveFrom ? "You are on leave." : "Leave is already booked from " + fmtDay(g.leaveFrom) + ". Cancel it first.";
    Date d = dateOf(startDay);
    if (g.day - g.postStart < 240) return "Leave is not granted in the first eight months of a tour.";
    if (g.leaveYear == d.y) return fstr("You have had your leave for %d.", d.y);
    if (startDay < g.day + 14) return "The Embassy needs a fortnight's notice: tickets, the hand-over, the Rezident's signature.";
    if (startDay > g.day + 150) return "Not more than five months ahead.";
    if (startDay + 30 > g.postEnd - 30) return "Too near the end of your tour.";
    if (g.day <= g.standDownUntil) return "Not while the station is stood down.";
    if (T().role < 3 && g.standRez < 28) return "The Rezident will not sign it. 'When there are results there will be leave.'";
    g.leaveFrom = startDay; g.leaveTo = startDay + 29;
    cable(T().chief, "Leave", "Leave is granted from " + fmtDay(g.leaveFrom) + " to " + fmtDay(g.leaveTo) + ": thirty days in the Union, with your family.\n\n"
          "Before you go: every agent you run yourself is to be told when he will next be met, or handed to another officer, or put on ice. Agreed meetings that fall in your leave will be moved to the week of your return. "
          "Nobody empties your dead drops while you are away unless you arrange it.\n\n" + upper(T().chief));
    return "Leave booked: " + fmtDay(g.leaveFrom) + " to " + fmtDay(g.leaveTo) + ". Meetings agreed for those weeks will be moved to your return; think about what will be lying in the dead drops.";
}

std::string Sim::cancelLeave() {
    if (g.leaveFrom < 0 || g.day >= g.leaveFrom) return "There is no booked leave to cancel.";
    g.leaveFrom = g.leaveTo = -1;
    if (g.wifeHere) g.home = clampf(g.home - 6, 0, 100);
    return "Leave cancelled. The tickets go back. It will be remembered at home.";
}

void Sim::leaveDawn() {
    if (g.leaveFrom < 0) return;
    if (g.day == g.leaveFrom) {
        g.overdueDays = 0;
        g.busyUntil = std::max(g.busyUntil, g.leaveTo);
        g.busyWhy = "on leave in the Union";
        g.leaveYear = dateOf(g.day).y;
        int k = 0;
        for (auto& p : g.persons)
            if (p.stage == AGENT && p.handler == 0 && p.nextMeet >= g.leaveFrom && p.nextMeet <= g.leaveTo) { p.nextMeet = g.leaveTo + 2 + 2 * k++; schedule(p); }
        addDecision(DK_LEAVE, 0, "Leave",
                    fstr("The aeroplane to Moscow, with %d suitcases. Thirty days. ", 4 + g.goods)
                        + (g.goods > 0 ? fstr("In the suitcases are %d things that cannot be bought at home for any money. ", g.goods) : std::string("You are bringing nothing much back with you. "))
                        + "\n\nHow will you spend the month?",
                    {"With the family: the dacha, her mother, a fortnight on the Black Sea" + std::string(g.goods > 0 ? " (the presents are for them)" : ""),
                     "Calling on the people who matter at Centre" + std::string(g.goods > 0 ? ", and not empty-handed" : ""),
                     g.goods > 0 ? "With the family - and what is in the suitcases goes to the commission shops" : "Quietly, at home in the flat"});
    }
    if (g.day == g.leaveTo + 1) {
        g.fatigue = 0;
        Person* keep = subj; subj = nullptr;
        cable("Rezidentura duty officer", "Back from leave", fstr("You are back at your desk. %d cables came in while you were away. ", unread())
              + "See what is in your dead drops, and see your agents.");
        subj = keep;
    }
}

void Sim::leaveDecide(int option) {
    int used = std::min(g.goods, 4);
    if (option == 0) {
        if (g.wifeHere) g.home = clampf(g.home + 10 + 2.f * used, 0, 100);
        g.goods -= used;
    } else if (option == 1) {
        g.patron = clampf(g.patron + 6 + 5.f * used, 0, 100);
        g.standCentre = clampf(g.standCentre + 2, 0, 100);
        g.goods -= used;
        if (g.wifeHere) g.home = clampf(g.home - 4, 0, 100);
        cable("Centre", "A conversation", "General " + g.patronName + " received you for forty minutes" + (used > 0 ? ", and was good enough to accept a small remembrance from abroad" : "")
              + ". He asked about the station, about the Rezident, and about your plans. " + word(g.patron, W_PATRON, 5) + ".");
    } else if (g.goods > 0) {
        int got = g.goods * 110;
        g.purse += got;
        if (g.wifeHere) g.home = clampf(g.home + 8, 0, 100);
        g.goods = 0;
        if (pct(12)) {
            g.standCentre = clampf(g.standCentre - 5, 0, 100);
            cable("Centre", "Party commission", "The Party commission has had occasion to speak to you about the sale of articles brought from abroad. An officer of the service is not a tradesman. The matter is noted on your Party record.");
        }
    } else if (g.wifeHere) g.home = clampf(g.home + 6, 0, 100);
}

std::string Sim::setWifeSignal(int siteId) {
    if (siteId < 0) { g.wifeSignal = -1; return "Your wife's shopping is her own again."; }
    Site* s = site(siteId);
    if (!s || s->retired || s->type != ST_SIGNAL) return "Choose a signal site.";
    if (!g.wifeHere) return "Your wife is in Moscow.";
    if (eastSide(s->district)) return "On our own side of the frontier there is no need.";
    if (g.home < 40) return "She will not hear of it. Things at home are not good enough to ask.";
    g.wifeSignal = siteId;
    return "Each weekday your wife's shopping will take her past " + s->name + ", and in the evening she will tell you what she saw there. "
           "She has had the wives' course and no more. It costs her something every day, and if they are interested in you they are interested in her.";
}

// ============================================================================ the private purse
std::string Sim::buyHomeGift() {
    const int cost = 15;
    if (!g.wifeHere) return "There is nobody at home to bring it to.";
    if (g.purse < cost) return fstr("It costs %d %s, and you have %d of your own.", P(cost), T().currency, P(g.purse));
    if (g.day - g.lastHomeGift < 10) return "Twice in a fortnight would look like a bad conscience.";
    g.purse -= cost; g.lastHomeGift = g.day;
    g.home = clampf(g.home + 6, 0, 100);
    static const char* const W[] = {"A dress from a shop where the assistants call her 'madam'.", "Dinner for two at a restaurant with tablecloths, and no talk of work.", "Shoes for the children, and a coat for her.", "A day out of the city, the whole family."};
    return std::string(W[r().n(4)]) + fstr(" (%d %s of your own money.)", P(cost), T().currency);
}

std::string Sim::buyGoods() {
    const int cost = 40;
    if (g.goods >= 6) return "Six suitcases is as much as the customs at Sheremetyevo will overlook, even for one of ours.";
    if (g.purse < cost) return fstr("It costs %d %s, and you have %d of your own.", P(cost), T().currency, P(g.purse));
    g.purse -= cost; g.goods++;
    static const char* const W[] = {"A Japanese cassette recorder", "A sheepskin coat", "Two pairs of American jeans and a carton of cigarettes", "French scent and a silk scarf", "A portable television", "A set of tyres for a Zhiguli"};
    return std::string(W[r().n(6)]) + fstr(", put by for Moscow (%d %s). At home it is worth three times that - as a present to the right man, or in a commission shop.", P(cost), T().currency);
}

std::string Sim::giftChief() {
    const int cost = 30;
    if (g.purse < cost) return fstr("It costs %d %s, and you have %d of your own.", P(cost), T().currency, P(g.purse));
    if (g.day - g.lastChiefGift < 90) return "Too soon after the last one. There is a line between respect and something else.";
    g.purse -= cost; g.lastChiefGift = g.day;
    if (T().role < 3) { g.standRez = clampf(g.standRez + 3, 0, 100); return "A case of whisky for the Rezident's name-day. He said you should not have, and put it in his cupboard."; }
    g.standCentre = clampf(g.standCentre + 1.5f, 0, 100);
    return "A parcel in the pouch for the head of the department, with your respects. It was acknowledged.";
}

std::string Sim::buyAsset(int k) {
    if (k < 0 || k > 2) return "";
    if (g.assets & (1 << k)) return "You have one.";
    if (g.purse < ASSET_COST[k]) return fstr("It takes %d %s in hard currency, and you have %d of your own.", P(ASSET_COST[k]), T().currency, P(g.purse));
    g.purse -= ASSET_COST[k];
    g.assets |= 1 << k;
    if (g.wifeHere) g.home = clampf(g.home + (k == 1 ? 15.f : 10.f), 0, 100);
    static const char* const W[] = {"A Zhiguli, paid for in cheques and waiting in a garage in Moscow. At home the queue for one is seven years.",
                                    "The first payment on a cooperative flat: three rooms, on the right side of the city. Your wife has already chosen the wallpaper.",
                                    "A dacha: a wooden house, six hundredths of a hectare, and an apple tree. It is what people serve abroad for."};
    return W[k];
}

std::string Sim::setSkim(int tenths) {
    g.skim = std::max(0, std::min(3, tenths));
    if (g.skim == 0) return "Every agent will get what he signs for.";
    return fstr("From now on you keep back %d0%% of whatever you hand an agent in person. He signs for the whole sum. ", g.skim)
           + "The books will balance; the agent will know; and anybody he tells will know.";
}

/** You hand an agent money. Returns what actually reaches him. */
int Sim::payAgent(Person& p, int amt) {
    if (!me() || g.skim <= 0 || amt <= 0 || training()) return amt;
    int kept = amt * g.skim / 10;
    if (kept <= 0) return amt;
    p.shortPaid += kept; g.skimmed += kept;
    g.purse += std::max(1, (int)std::lround(kept / T().price));
    if (controlled(p)) g.dirt = std::min(100.f, g.dirt + kept / T().price * 0.6f);       // the other side counts the money too
    else {
        float sting = clampf(kept * 10.f / (p.salary + P(10)), 0, 8) * (0.4f + p.tru[A_MONEY] / 100.f);
        p.rapport = clampf(p.rapport - sting, 0, 100); p.commitment = clampf(p.commitment - sting * 0.6f, 0, 100);
        if (p.shortPaid > P(20) && pct(12.f * g.skim)) p.warn.push_back("counted the money in front of you and said, without looking up, that it was short again.");
    }
    return amt - kept;
}

void Sim::skimExposed(const std::string& how) {
    if (g.skimmed <= 0 || g.phase != PH_PLAY) return;
    int sum = g.skimmed;
    bool big = sum >= P(150);
    g.purse = std::max(0, g.purse - (int)std::lround(sum / T().price));
    g.skimmed = 0; g.skim = 0;
    for (auto& p : g.persons) p.shortPaid = 0;
    Person* keep = subj; subj = nullptr;
    if (big && g.patron < 55) {
        endPosting(END_RECALLED, "RECALLED TO MOSCOW",
                   how + fstr(" The sum is %d %s.\n\nMisappropriation of operational funds is a matter for the Party commission and the personnel directorate. You are recalled, and what you kept back has been recovered from your pay.", sum, T().currency));
        subj = keep;
        return;
    }
    if (big) {
        g.patron = clampf(g.patron - 40, 0, 100);
        g.standCentre = clampf(g.standCentre - 8, 0, 100); g.standRez = clampf(g.standRez - 10, 0, 100);
        cable("Centre", "Operational funds", how + fstr(" The sum is %d %s.\n\nThe matter has been closed at the request of General %s, on the understanding that the money is repaid and that it does not happen twice. ", sum, T().currency, g.patronName.c_str())
              + "You owe him a good deal more than the money.\n\nCENTRE");
    } else {
        g.standCentre = clampf(g.standCentre - 6, 0, 100); g.standRez = clampf(g.standRez - 8, 0, 100);
        cable(T().chief, "Operational funds", how + fstr(" The sum is %d %s.\n\nIt is not large, and I have chosen to treat it as carelessness. It has been repaid from your pay. There will not be a second conversation.\n\n", sum, T().currency) + upper(T().chief));
    }
    subj = keep;
}

void Sim::patronFalls(const std::string& how) {
    float was = g.patron;
    std::string old = g.patronName;
    g.patron = 4;
    g.patronName = RU_LAST[r().n((int)RU_LAST.size())];
    if (was > 50) g.standCentre = clampf(g.standCentre - 4, 0, 100);
    Person* keep = subj; subj = nullptr;
    cable("Centre", "Appointments", "General " + old + " " + how + ". His duties pass to General " + g.patronName + "."
          + (was > 50 ? "\n\nYou were known as his man. Those who waited behind him have waited a long time." : was > 25 ? "\n\nHe knew your name. His successor does not." : ""));
    subj = keep;
}

/** When the General Secretary changes, so do a good many other people. */
void Sim::officeHistory(const Date& d) {
    static const int WHEN[][3] = {{1982, 11, 22}, {1984, 2, 20}, {1985, 3, 25}, {1988, 10, 4}};
    for (auto& w : WHEN)
        if (d.y == w[0] && d.m == w[1] && d.d == w[2] && g.patron > 6 && pct(30)) patronFalls("has been relieved of his post in the reorganisation and transferred to the reserve");
    // the climate of opinion: what events do to people who work for us out of conviction
    struct Shock { int y, m, d; float ideo; const char* what; };
    static const Shock SH[] = {
        {1970, 5, 5, 5, "American troops have shot dead four students at a university in Ohio"},
        {1972, 12, 19, 5, "The Americans are bombing Hanoi with B-52s"},
        {1973, 9, 12, 6, "The government of Chile has been overthrown by its army, with American help"},
        {1974, 8, 9, 3, "The American President has resigned in disgrace"},
        {1979, 12, 29, -8, "Soviet troops are in Kabul"},
        {1980, 9, 1, -3, "The Polish government has had to sign an agreement with striking workers"},
        {1981, 12, 14, -5, "Martial law has been declared in Poland"},
        {1983, 9, 3, -6, "A Korean airliner with 269 people aboard has been shot down by a Soviet fighter"},
        {1983, 10, 23, 6, "Millions have marched in the Western capitals against the new American missiles"},
        {1986, 4, 30, -5, "A reactor at Chernobyl has burned for four days before anything was said"},
        {1988, 6, 29, 4, "The Party conference in Moscow has been shown on television, arguing in the open"},
        {1989, 6, 5, -3, "The Chinese army has cleared Tiananmen Square with tanks"}};
    for (auto& s : SH) {
        if (d.y != s.y || d.m != s.m || d.d != s.d) continue;
        int moved = 0;
        for (auto& p : g.persons) {
            if (!p.live()) continue;
            p.tru[A_IDEO] = clampf(p.tru[A_IDEO] + s.ideo, 0, 100); p.app[A_IDEO] = clampf(p.app[A_IDEO] + s.ideo, 0, 100);
            if (p.stage == AGENT && !controlled(p) && p.tru[A_IDEO] > p.tru[A_MONEY]) {
                p.commitment = clampf(p.commitment + s.ideo * 1.3f, 0, 100);
                if (s.ideo < 0) p.stress = clampf(p.stress - s.ideo, 0, 100);
                moved++;
            }
        }
        Person* keep = subj; subj = nullptr;
        cable("Line PR", "The climate of opinion", std::string(s.what) + ". "
              + (s.ideo < 0 ? "Those who work with us out of conviction will be asking themselves what they are convinced of; those who were coming towards us will hesitate."
                            : "It will bring people towards us who would not have listened a month ago, and it will steady those who work with us out of conviction.")
              + (moved > 0 ? fstr("\n\nBy the files, %d of your agents work%s for the idea rather than the money. See %s.", moved, moved == 1 ? "s" : "", moved == 1 ? "him" : "them") : std::string("")));
        subj = keep;
    }
}

// ============================================================================ compromise operations
int Sim::kompCost(int kind) const { return P(kind == 0 ? 60 : kind == 1 ? 90 : 15); }

std::string Sim::prepareCompromise(Person& p, int kind) {
    if (training()) return "Not on the course.";
    if (kind < 0 || kind > 2) return "Choose a method.";
    if (p.stage != DEVELOPING && p.stage != SPOTTED) return "Compromising material is prepared against a contact who has not yet been recruited.";
    if (p.handler != 0) return "Only against a contact you are developing yourself.";
    if (p.kompState == 1 || p.kompState == 2) return "An operation against " + p.name + " is already in hand.";
    if (p.kompState == 3) return "You already hold material against " + p.name + ".";
    if (g.day - p.kompDay < 60) return "Not so soon after the last attempt. He would have to be a fool.";
    if (p.meetings < 2) return "You do not know him well enough to arrange anything. Meet him at least twice.";
    if (kind == 2 && p.meetings < 3) return "For photographs to mean anything there must be a pattern of meetings: at least three.";
    p.kompState = 1; p.kompKind = kind;
    queue(C_KOMP, p.id, 90, "Compromise operation: " + p.name);
    g.paperwork += 2;
    file(p, std::string("Sanction asked for a compromise operation (") + KOMP_LABEL[kind] + ").");
    return gx(&p, std::string("Proposal drafted (90 groups): ") + KOMP_LABEL[kind] + fstr(". If Centre sanctions it, it is staged at your next dinner with him (an overt social meeting in the evening) and costs %d %s. ", kompCost(kind), T().currency)
                      + "What it yields depends on what he really is, not on what you think he is. If he sees what was tried, he may go straight to his security officer.");
}

/** The evening that was arranged. Called at a long overt meeting when Centre has sanctioned the operation. */
void Sim::stageCompromise(Person& p) {
    if (p.kompState != 2) return;
    int kind = p.kompKind, cost = std::min(kompCost(kind), g.funds);
    money(-cost, "Compromise operation against " + p.name);
    p.kompDay = g.day;
    paper(2);
    float chance = kind == 0 ? 20 + p.tru[A_MONEY] * 0.8f : kind == 1 ? 10 + p.tru[A_VULN] * 0.9f - p.tru[A_DISC] * 0.1f : 75;
    bool ok = controlled(p) ? true : pct(clampf(chance, 5, 95));
    if (ok) {
        p.kompState = 3;
        p.komp = kind == 0 ? 35 + p.tru[A_MONEY] * 0.3f + rnd(0, 15) : kind == 1 ? 45 + p.tru[A_VULN] * 0.4f + rnd(0, 15) : 22 + p.empSec * 0.25f + rnd(0, 15);
        if (controlled(p)) p.komp = rnd(60, 90);                    // they are glad to let you think so
        p.komp = clampf(p.komp, 10, 100);
        static const char* const DONE[] = {"He took the envelope 'as a consultant's fee' and signed for it in his own name. There are now three such receipts.",
                                           "The acquaintance was made, and went as far as such things go. The technical officer has the photographs.",
                                           "The technical officer photographed him from a parked car, taking an envelope from the hand of a Soviet diplomat. The pictures are sharp."};
        say(std::string("    THE COMPROMISE OPERATION: ") + DONE[kind]);
        file(p, std::string("COMPROMISING MATERIAL HELD: ") + (kind == 0 ? "signed receipts for money." : kind == 1 ? "photographs of a private nature." : "photographs of him accepting an envelope from a Soviet officer.")
                    + " Assessed effect: " + word(p.komp + rnd(-12, 12), W_KOMP, 5) + ".");
        p.tru[A_VULN] = clampf(p.tru[A_VULN] + 10, 0, 100); p.app[A_VULN] = clampf(p.app[A_VULN] + 10, 0, 100);
        alert("Compromising material is now held against " + p.name + ". A recruitment approach by coercion will bite harder - if he is the man to be coerced.");
        return;
    }
    p.kompState = 4;
    static const char* const MISS[] = {"He would not take the money. 'I think you misunderstand our friendship.'", "The introduction was made. He was polite to the lady and went home at ten.", "He never touched the envelope; the photographs show two men at a table."};
    say(std::string("    THE COMPROMISE OPERATION failed. ") + MISS[kind]);
    if (pct(30)) {              // he understood what was being tried
        bool reports = pct((100 - p.tru[A_DISC]) * 0.6f + p.empSec * 0.2f);
        clearAppointments(p);
        p.stage = BROKEN;
        if (reports) {
            heat(8); g.standRez = clampf(g.standRez - 4, 0, 100);
            p.knownAdv = true;
            file(p, "The compromise operation FAILED. He understood what was being attempted and is believed to have reported it.");
            alert(p.name + " understood what was being tried and has broken off. The Embassy has received a protest about 'an attempt to suborn a public servant'.");
        } else {
            file(p, "The compromise operation FAILED. He understood what was being attempted and has broken off contact.");
            alert(p.name + " understood what was being tried. He will not meet you again.");
        }
    } else {
        p.rapport = clampf(p.rapport - 6, 0, 100);
        file(p, "The compromise operation failed. He does not appear to have understood what was intended.");
    }
}

/** Centre's answer to a proposal to compromise a contact. */
void Sim::kompSanction(Person& p) {
    if (p.kompState != 1) return;
    if (!p.live() || p.stage == AGENT) { p.kompState = 0; return; }
    if (!pct(88)) {
        p.kompState = 0;
        p.kompDay = g.day - 30;
        file(p, "Centre refused sanction for a compromise operation.");
        cable("Centre", "Your proposal concerning " + p.name, "NOT sanctioned. The contact's position makes a scandal more costly to us than to him. Continue development by ordinary means.\n\nCENTRE");
        return;
    }
    p.kompState = 2;
    file(p, std::string("Centre sanctioned the compromise operation (") + KOMP_LABEL[p.kompKind] + "). To be staged at the next dinner with him.");
    cable("Centre", "Your proposal concerning " + p.name, std::string("SANCTIONED: ") + KOMP_LABEL[p.kompKind] + ".\n\n"
          + (p.kompKind == 1 ? "The friends will provide the acquaintance; the technical officer will see to the rest. " : p.kompKind == 0 ? "The receipts are to be in his own hand and for sums he cannot explain. " : "The technical officer will be in position. ")
          + "It will be staged at your next dinner with him (an overt social meeting in the evening). You will not be present when it matters.\n\nCENTRE");
}

// ============================================================================ the text of the Private Life screen
std::string Sim::privateText() {
    if (training()) return "PRIVATE LIFE\n\nOn the course you have none.\n";
    std::string sb = "AT HOME\n";
    if (g.wifeHere) {
        sb += "  Your wife and children are with you at the post.\n";
        sb += "  How things are at home: " + word(g.home, W_HOME, 5) + ".\n";
        Site* s = site(g.wifeSignal);
        sb += std::string("  Her shopping takes her past: ") + (s ? shortSite(s) + (s->markSeenDay >= 0 ? " (last passed " + fmtShort(s->markSeenDay) + ")" : "") : std::string("nowhere in particular")) + ".\n";
    } else sb += "  Your wife and children are in Moscow. You live alone in the compound.\n";
    sb += "\nLEAVE\n";
    if (onLeave(g.day)) sb += "  You are on leave in the Union until " + fmtDay(g.leaveTo) + ".\n";
    else if (g.leaveFrom > g.day) sb += "  Booked: " + fmtDay(g.leaveFrom) + " to " + fmtDay(g.leaveTo) + ".\n";
    else if (g.leaveYear == dateOf(g.day).y) sb += fstr("  Taken for %d.\n", g.leaveYear);
    else if (g.day - g.postStart < 240) sb += "  Not in the first eight months of a tour (from " + fmtShort(g.postStart + 240) + ").\n";
    else sb += "  Thirty days in the Union are due to you this year. Not taken.\n";
    sb += "\nYOUR OWN MONEY\n";
    sb += fstr("  Put by .................... %d %s\n", P(g.purse), T().currency);
    sb += fstr("  Things for Moscow ......... %d (of 6 that will pass the customs)\n", g.goods);
    {
        std::string own;
        for (int k = 0; k < 3; k++) if (g.assets & (1 << k)) own += (own.empty() ? "" : "; ") + std::string(k == 0 ? "a Zhiguli" : k == 1 ? "a cooperative flat" : "a dacha");
        sb += "  At home you own ........... " + (own.empty() ? std::string("what you left with") : own) + "\n";
    }
    sb += fstr("  Kept back from agents ..... %s", g.skim == 0 ? "nothing" : fstr("%d0%% of what you hand them", g.skim).c_str());
    if (g.skimmed > 0) sb += fstr("  (so far, at this station: %d %s)", g.skimmed, T().currency);
    sb += "\n";
    sb += "\nAT CENTRE\n";
    sb += "  General " + g.patronName + ", of the directorate's leadership. " + word(g.patron, W_PATRON, 5) + ".\n";
    sb += "\nTHE NEIGHBOURS\n";
    sb += "  Relations with the military intelligence rezidentura: " + word(g.gru, W_GRU, 5) + ".\n";
    if (!g.attest.empty()) {
        sb += "\nATTESTATIONS\n";
        for (auto& a : g.attest) sb += "  " + a + "\n";
    }
    return sb;
}

// ============================================================================ when they hold something against you
void Sim::dirtDecide(int option) {
    if (option == 0) {              // tell Line KR everything
        g.dirt = 0;
        g.standRez = clampf(g.standRez - 8, 0, 100); g.standCentre = clampf(g.standCentre - 8, 0, 100);
        g.paperwork += 6;
        cable("Line KR", "Your statement", "Your statement has been taken, all eleven pages of it. That you came to us before they came back to you is the one thing in it that is to your credit. "
              "It has gone to Centre with the rest.\n\nLINE KR");
        if (g.skimmed > 0) skimExposed("In his statement to Line KR the officer admitted that he had kept back part of his agents' remuneration.");
        else if (g.patron < 35 && pct(35))
            endPosting(END_RECALLED, "RECALLED TO MOSCOW", "An officer against whom the adversary holds material cannot remain at his post, however he came to tell us of it. You are recalled. "
                       "The confession is on your file, beside what was confessed.");
    } else if (option == 1) {       // say nothing, do nothing
        if (pct(50)) {
            g.dirt = 0;
            if (g.patron >= 55) {
                g.patron = clampf(g.patron - 40, 0, 100);
                g.standCentre = clampf(g.standCentre - 10, 0, 100); g.standRez = clampf(g.standRez - 12, 0, 100);
                cable("Centre", "A letter to the Ambassador", "An envelope addressed to the Ambassador arrived by the ordinary post, with photographs and a typed sheet. The Ambassador sent it to the Rezident, and the Rezident to Centre.\n\n"
                      "General " + g.patronName + " has taken the view that it is a provocation, and the file is closed. He did not take that view for nothing.\n\nCENTRE");
            } else
                endPosting(END_RECALLED, "RECALLED TO MOSCOW", "An envelope addressed to the Ambassador arrived by the ordinary post, with photographs and a typed sheet. The Ambassador sent it to the Rezident, and the Rezident to Centre.\n\n"
                           "You said nothing when they showed it to you, and nothing afterwards. That is what the commission could not forgive.");
        } else g.lastApproach = g.day - 150;      // they are patient, and they will be back
    } else {                        // give him something small
        g.dirt = std::min(100.f, g.dirt + 25);
        g.you.heat = std::max(8.f, g.you.heat - 12);
        std::vector<Person*> mine;
        for (auto& p : g.persons) if (p.stage == AGENT && !controlled(p)) mine.push_back(&p);
        if (!mine.empty()) { Person* p = mine[r().n((int)mine.size())]; p->suspicion += 30; p->knownAdv = true; }
        if (pct(45)) pend(g.day + 120 + r().n(500), P_SECRET, 0, 5);
        g.lastApproach = g.day - 120;
    }
}
