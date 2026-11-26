// sim_tutorial.cpp - the training course: Moscow, June 1968.
//
// Before a new officer is sent abroad he spends some weeks on practical exercises in the city. The
// training brigade of the Seventh Directorate plays the adversary's watchers; retired officers play the
// contacts. Nothing here counts against you, and - unlike anywhere else in the game - after each exercise
// the instructor tells you what was REALLY going on. That is the point of the course.
//
// The course is a list of lessons. Each morning the instructor sets one (tutorialIssue), each night he
// marks it (tutorialNight). A lesson not done is simply set again.
#include "sim.h"

enum { L_NONE, L_SCOUT, L_CASE, L_SITES, L_ROUTE, L_CONTACT, L_PREPARE, L_MEETING, L_DROP, L_EXAM, L_DONE };
static const char* LESSON[] = {"", "Finding a place", "Casing", "A dead drop and a signal", "The route", "A contact", "Before a meeting",
                               "The meeting", "The dead drop", "Passing-out test", ""};

static std::string up(std::string s) { for (auto& c : s) c = (char)toupper((unsigned char)c); return s; }

void Sim::tutorialSetup() {
    g.tut = L_SCOUT; g.tutTries = 0; g.tutMarks = 0; g.tutMax = 0;
    g.tutA = g.tutB = g.tutMeet = g.tutDrop = g.tutSignal = g.tutRoll = -1;
    g.tutLog.clear(); g.tutText.clear();
    g.funds = 0;
    money(60, "Exercise allowance drawn from the bursar");
    g.film = 0; g.cameras = 0; g.containers = 2; g.pads = 0;
    Officer in;
    in.id = g.nextId++; in.role = ROLE_REZIDENT; in.rank = 3; in.name = officerName();
    g.officers.push_back(in);
    for (int i = 0; i < 2; i++) {
        Officer o;
        o.id = g.nextId++; o.role = ROLE_COLLEAGUE; o.rank = 0; o.name = officerName();
        o.a.tradecraft = rnd(28, 40); o.a.street = rnd(20, 40);
        g.officers.push_back(o);
    }
    cable("Instructor", "Course orders",
          "Trainee " + g.officer + ",\n\nFor the next three weeks you will work in the city as if it were a foreign capital. You are a 'Third Secretary'; "
          "the School is your 'Embassy'; lectures are your 'cover duties'. The training brigade of the Seventh Directorate will follow you when it chooses, "
          "and will not tell you when. The people you meet are retired officers playing their parts, and they have played them many times.\n\n"
          "Each morning I set one exercise. It is on the card pinned to your desk, and in the morning brief on the Planner. Give your orders for the day, "
          "press End day, and read the Daily report: I shall have written under it what you did - and what you did not see.\n\n"
          "That last is a courtesy of the School. Abroad, nobody will ever tell you.\n\n"
          "If you already know your trade, the card has a button to end the course and take the aeroplane.\n\n" + up(g.officers[0].title()));
    attention = false;
    tutorialIssue();
    dawn();
}

static std::string blockHint() {
    return "HOW TO GIVE AN ORDER: open the Planner. Each day is a row; each row has a Morning, an Afternoon and an Evening. Double-click a block (today is the top row), "
           "choose the Activity, fill in what the form asks for, press OK. Then press End day at the top right.";
}

/** Set today's exercise: the card on the desk, and a cable. */
void Sim::tutorialIssue() {
    const Theatre& t = T();
    std::string s;
    Site* meet = site(g.tutMeet);
    switch (g.tut) {
        case L_SCOUT:
            s = "Find yourself a meeting place.\n\n"
                "Today's order: THIS AFTERNOON - 'Scout district for a new site or route'. District: " + std::string(t.district[4]) + ". Looking for: Meeting site.\n\n" + blockHint();
            break;
        case L_CASE:
            s = "Go back and look at it properly.\n\n"
                "Today's order: THIS AFTERNOON - 'Case a site or walk a route', and choose the place you found yesterday.\n\n"
                "Then look at the Sites screen. 'Natural cover' and 'View' are YOUR ESTIMATES. They are words, not numbers, because that is all an officer has.";
            break;
        case L_SITES:
            s = "An agent needs three places: somewhere to meet, somewhere to leave things, and somewhere to leave a mark that says he has left them.\n\n"
                "Today's orders: THIS MORNING - scout " + std::string(t.district[6]) + " for a Dead drop. THIS AFTERNOON - scout " + std::string(t.district[1]) + " for a Signal site.\n\n"
                "Two acts in one day will tire you. Watch 'Fatigue' in the bar at the top.";
            break;
        case L_ROUTE:
            s = "Today you may be followed. I shall not say.\n\n"
                "Today's orders: THIS AFTERNOON - 'Surveillance detection route' (improvise). THIS EVENING - 'Case a site or walk a route': your dead drop.\n\n"
                "A route only protects the act in the block IMMEDIATELY after it. On the evening order, leave 'If surveillance is seen' at Normal: you break off if you are sure, and go on if you are not.";
            break;
        case L_CONTACT: {
            Person* a = person(g.tutA);
            s = "You have 'met at a reception' " + (a ? a->name : std::string("a contact")) + ". He is in your files (Contacts).\n\n"
                "Today's order: THIS AFTERNOON - 'Overt social meeting' with him. Lunch costs four roubles of your allowance.\n\n"
                "Afterwards read his file. The bars under ASSESSMENT are your estimate of what he SHOWS you, with the width of your ignorance beside each.";
            s = gx(a, s);
            break;
        }
        case L_PREPARE: {
            Person* b = person(g.tutB);
            s = "You have 'taken over' an agent, codename " + (b ? b->codename : std::string("?")) + ". Read his file. He photographs papers with a Minox and is owed his month's money.\n\n"
                "His agreed meeting is TOMORROW EVENING at " + (meet ? meet->name : std::string("your meeting site")) + ". It is already in your planner, in red, marked MEET*. An agent is met on the agreed evening or not at all.\n\n"
                "Today: (1) Material - press 'Requisition film'. (2) Planner - double-click TOMORROW EVENING and set the agenda: Money = what he is owed; tick 'Give security instruction'. "
                "(3) Put a 'Surveillance detection route' in TOMORROW AFTERNOON. (4) THIS AFTERNOON - 'Rezidentura: paperwork'. Then End day.";
            s = gx(b, s);
            break;
        }
        case L_MEETING: {
            Person* b = person(g.tutB);
            s = "Tonight you meet " + (b ? b->codename : std::string("your agent")) + ".\n\n"
                "Check the planner's top row: Afternoon should read 'SDR', Evening 'MEET*'. If the afternoon is still empty, put the route in now. Then End day and read the report.";
            break;
        }
        case L_DROP: {
            Person* b = person(g.tutB);
            Site* d = site(g.tutDrop); Site* sg = site(g.tutSignal);
            s = (b ? b->codename : std::string("Your agent")) + " was told at the meeting to use the dead drop. Last night he filled it and put up his mark.\n\n"
                "Today's orders: THIS MORNING - 'Read signal site': " + (sg ? sg->name : std::string("your signal site")) + ". THIS AFTERNOON - 'Surveillance detection route'. "
                "THIS EVENING - 'Unload dead drop': " + (d ? d->name : std::string("your dead drop")) + ".\n\n"
                "The mark tells you there is something to fetch before you go near the place. A danger mark tells you to stay away.";
            break;
        }
        case L_EXAM: {
            Person* a = person(g.tutA); Person* b = person(g.tutB);
            s = "The passing-out test. One of your two - " + (a ? a->name : std::string("the contact")) + " or " + (b ? b->full() : std::string("the agent")) +
                " - was told to play a man who is controlled by the adversary. The other played it straight.\n\n"
                "Read both files again, slowly: the lines in the log that begin 'Note:', the manner each showed you, how each behaved. A provocation is built to look like exactly what a case officer hopes to find, and gives itself away only in small things.\n\n"
                "On Contacts, select the one you believe is controlled and press 'Terminate contact'. Leave the other alone. Then End day.";
            break;
        }
        default: break;
    }
    g.tutText = fstr("EXERCISE %d - ", g.tut) + up(LESSON[g.tut]) + "\n\n" + s;
    cable("Instructor", fstr("Exercise %d: ", g.tut) + LESSON[g.tut], s);
    attention = true;
}

/** Each morning of the course: the brigade follows you only when the exercise calls for it. */
void Sim::tutorialDawn() {
    for (int b = 0; b < 3; b++) { g.you.covered[b] = false; g.you.teamQ[b] = 45; }
    for (int i = 0; i < 8; i++) g.hot[i] = false;
    if (g.tut == L_ROUTE) g.you.covered[1] = g.you.covered[2] = true;
    g.brief = "MORNING BRIEF - " + fmtDay(g.day) + "\n\nTHE INSTRUCTOR'S CARD:\n\n" + g.tutText
              + "\n\n(In the field this sheet carries the Zenit intercept of the watchers' radio and Line KR's weekly opinion of your position. The School has neither.)\n";
}

/** Each night of the course: the instructor marks the exercise and says what really happened. */
void Sim::tutorialNight() {
    if (g.tut == L_NONE || g.tut >= L_DONE) return;
    const Theatre& t = T();
    std::string d;                 // the debrief
    bool pass = false;
    int lesson = g.tut;
    auto words = [&](const Site& s) { return "cover '" + word(s.estCover, W_LEVEL, 5) + "', view of the approaches '" + word(s.estSight, W_LEVEL, 5) + "'"; };
    auto truth = [&](const Site& s) { return fstr("cover %d out of 100, view %d out of 100", (int)std::lround(s.cover), (int)std::lround(s.sight)); };
    switch (g.tut) {
        case L_SCOUT: {
            Site* found = nullptr;
            for (auto& s : g.sites) if (s.type == ST_MEET && !s.retired) found = &s;
            if (found) {
                pass = true; g.tutMeet = found->id;
                d = "You found " + found->name + ". Your first impression: " + words(*found) + ".\n\nTHE TRUTH, which in the field you are never shown: " + truth(*found)
                    + ". One look gives a rough idea and no more.";
            } else d = "You did not bring back a meeting site. Planner: double-click this afternoon; Activity 'Scout district for a new site or route'; Looking for 'Meeting site'; OK; then END DAY.";
            break;
        }
        case L_CASE: {
            Site* s = site(g.tutMeet);
            if (s && s->casing >= 2) {
                pass = true;
                d = "A second visit to " + s->name + ". Your estimate is now: " + words(*s) + ". The truth is still " + truth(*s)
                    + ".\n\nEach casing narrows the error; three is as much as a place repays. A well-cased place is also one where you will notice men waiting before you walk into them.";
            } else d = "The place has not been cased. Planner: this afternoon, 'Case a site or walk a route', and choose it from the list.";
            break;
        }
        case L_SITES: {
            Site* dr = nullptr; Site* sg = nullptr;
            for (auto& s : g.sites) { if (s.type == ST_DROP && !s.retired) dr = &s; if (s.type == ST_SIGNAL && !s.retired) sg = &s; }
            if (dr) g.tutDrop = dr->id;
            if (sg) g.tutSignal = sg->id;
            if (dr && sg) {
                pass = true;
                d = "Dead drop: " + dr->name + ". Signal site: " + sg->name + fstr(".\n\nYour fatigue tonight stands at %d. Above 50 an officer begins to miss things in the street; above 70 he misses most of them; at 92 he is ill and loses the day. "
                    "A block of Rest takes off 22, and every night takes off 10.", (int)std::lround(g.fatigue));
            } else d = std::string("Still wanted: ") + (dr ? "" : "a Dead drop (scout " + std::string(t.district[6]) + ")") + (!dr && !sg ? " and " : "") + (sg ? "" : "a Signal site (scout " + std::string(t.district[1]) + ")") + ".";
            break;
        }
        case L_ROUTE: {
            const SdrLog* log = nullptr;
            for (auto& l : g.sdrLog) if (l.day == g.day && l.block == 2) log = &l;
            bool full = log && log->route.find("route") != std::string::npos;
            if (log && full) {
                pass = true;
                float det = clampf(66 - 5 + (g.you.tradecraft - 50) * 0.5f + g.you.street * 0.25f - 45 * 0.3f, 4, 95);
                float quick = clampf(36 + (g.you.tradecraft - 50) * 0.5f + g.you.street * 0.25f - 45 * 0.3f, 4, 95);
                d = fstr("THE TRUTH: a team of four and a grey Volga were behind you from two o'clock until you went home. With a full route your chance of confirming them was about %d in 100. "
                         "With only the short check an officer makes on his way, it would have been about %d.\n\n", (int)det, (int)quick);
                if (log->signs == 2 && log->abort) d += "You confirmed them and broke off. That is the whole art. An aborted act costs an evening; an observed one costs an agent.";
                else if (log->signs == 2) d += "You confirmed them and went on regardless. Abroad, the place you visited would tonight be on the adversary's list.";
                else if (log->signs == 1) d += std::string("You had a doubt - no more - and ") + (log->abort ? "broke off." : "went on, as your orders said. Abroad, the place you visited would now be known to the adversary, and nothing on your desk would say so.");
                else d += "You saw nothing, and went on. Abroad, the place you visited would now be known to the adversary - and nothing on your desk would ever tell you. 'No surveillance detected' is not 'nobody there'.";
                d += "\n\nExperience improves the odds. So does a prepared route, and so does a countersurveillance team; you will have both abroad.";
                for (auto& s : g.sites) { s.burned = false; s.staked = false; }
                g.you.heat = g.you.peakHeat = 10;
            } else if (log) d = "You went to the drop without a route before it. The route goes in the block BEFORE the act: afternoon 'Surveillance detection route', evening 'Case a site'. Again tomorrow.";
            else d = "No approach was made this evening. Afternoon: 'Surveillance detection route'. Evening: 'Case a site or walk a route' - your dead drop. Again tomorrow.";
            break;
        }
        case L_CONTACT: {
            Person* a = person(g.tutA);
            if (a && a->lastContact == g.day) {
                pass = true;
                d = "You have lunched with " + a->name + ". Compare your file with the truth:\n\n";
                int shown = 0;
                for (int i = 0; i < A_INFL && shown < 3; i++) if (a->estC[i] >= 5) {
                    d += "  " + pad(ATTR[i], 22) + fstr("your estimate %d (+/-%d)     what he shows %d     what is true %d\n", (int)std::lround(a->estV[i]), (int)std::lround((100 - a->estC[i]) * 0.3f),
                                                    (int)std::lround(a->app[i]), (int)std::lround(a->tru[i]));
                    shown++;
                }
                d += "\nThree numbers, and you see only the first. Your estimate closes on what he SHOWS. Whether what he shows is true is another question - keep it in mind for the end of the course.";
            } else d = "The lunch did not take place. Planner: this afternoon, 'Overt social meeting', and choose him.";
            break;
        }
        case L_PREPARE: {
            pass = true;
            bool film = g.film > 0;
            for (auto& x : g.pending) if (x.kind == P_FILM) film = true;
            bool route = order(g.day + 1, 1).type == O_SDR;
            Order m = order(g.day + 1, 2);
            Person* b = person(g.tutB);
            bool paid = b && m.appointment && m.pay >= b->owed;
            d = std::string("Preparations.\n\n  Film requisitioned ............ ") + (film ? "yes" : "NO - the stores take days; an agent without film cannot photograph")
                + "\n  Route planned for tomorrow .... " + (route ? "yes" : "NO - put it in tomorrow AFTERNOON before you end the day")
                + "\n  Money on the agenda ........... " + (paid ? "yes" : "no - an agent who is not paid on time grumbles, then sulks")
                + "\n\nOutstanding paperwork over 8 hours is 'in arrears' and is held against you. Every act you carry out adds to the pile.";
            if (film && route && paid) g.tutMarks++;
            g.tutMax++;
            break;
        }
        case L_MEETING: {
            Person* b = person(g.tutB);
            if (b && b->lastContact == g.day) {
                pass = true;
                int inHand = 0;
                for (auto& x : g.rolls) if (x.agentId == b->id && (x.state == R_LAB || x.state == R_READY || x.state == R_YOU)) inHand++;
                d = fstr("Meeting held. %d item(s) of his are now in the chain - look at Material. Film goes to the lab, then by Thursday's pouch to Centre, and is graded there three to five weeks after it was taken. "
                         "The grade is the only thing Centre ever tells you about your agent.\n\n", inHand)
                    + gx(b, "His next meeting was agreed before you parted and is already in your planner. What he tells you - new rules in his office, questions from his security officer - he tells you at meetings and nowhere else.");
            } else {
                d = "The meeting did not take place. For the exercise, he will be at the same place TOMORROW evening. Afternoon: route. Evening: the red MEET*.";
                if (b) { b->misses = 0; b->nextMeet = g.day + 1; b->meetIdx = 0; schedule(*b); }
            }
            break;
        }
        case L_DROP: {
            Roll* x = roll(g.tutRoll);
            Site* sg = site(g.tutSignal);
            if (x && x->state != R_DROP) {
                pass = true;
                bool read = sg && sg->markSeenDay >= g.day - 1;
                bool route = false;
                for (auto& l : g.sdrLog) if (l.day == g.day && l.act.find("Unload") != std::string::npos && l.route.find("route") != std::string::npos) route = true;
                d = std::string("The drop is emptied.\n\n  Signal read first ....... ") + (read ? "yes" : "no - you went to the place without knowing whether anything was there, or whether he had warned you off")
                    + "\n  Route before the act .... " + (route ? "yes" : "no")
                    + "\n\nA drop is safer than a meeting and tells you nothing: not that he has been promoted, or moved, or is frightened. And what lies in a drop can be found by a gardener or a dog. You are not told. You find it empty.";
                if (read && route) g.tutMarks++;
                g.tutMax++;
            } else d = "His film is still in the drop. Morning: 'Read signal site'. Afternoon: 'Surveillance detection route'. Evening: 'Unload dead drop'.";
            break;
        }
        case L_EXAM: {
            Person* a = person(g.tutA); Person* b = person(g.tutB);
            if (!a || !b) { pass = true; break; }
            bool ga = !a->live(), gb = !b->live();
            Person* bad = a->dangle ? a : b; Person* good = a->dangle ? b : a;
            g.tutMax += 3;
            if (ga != gb) {
                pass = true;
                bool right = !bad->live();
                if (right) g.tutMarks += 3;
                d = std::string(right ? "CORRECT. " : "WRONG. ") + bad->label() + " was the provocation. " + good->label() + " was genuine"
                    + (right ? "." : ", and you have thrown away a good source to keep a bad one.") + "\n\n"
                    + gx(bad, "The sign was in " + bad->label() + "'s file: the line beginning 'Note:'." + (bad == b ? " He was also calmer than a man in his position has any right to be." : "")) + "\n\n"
                    "Abroad there is no instructor. There are the notes in the file, the manner at meetings, a grade from Centre marked 'queried', a source who is too regular and too good - and your judgement.";
            } else if (g.tutTries >= 1) {
                pass = true;
                d = std::string(ga ? "You closed both files. " : "You closed neither. ") + "I asked for one. " + bad->label() + " was the provocation; the 'Note:' line in the file was the sign.";
            } else d = std::string(ga ? "You closed both files; I asked for one. " : "You have not closed a file. ") + "Contacts: select the one you believe is controlled, press 'Terminate contact', END DAY."
                       + (ga ? " (For the exercise both are restored.)" : "");
            if (!pass && ga) { a->stage = DEVELOPING; b->stage = AGENT; }
            break;
        }
        default: break;
    }
    if (pass) {
        if (lesson != L_PREPARE && lesson != L_DROP && lesson != L_EXAM) { g.tutMax++; if (g.tutTries == 0) g.tutMarks++; }
        g.tut++; g.tutTries = 0;
    } else g.tutTries++;
    say("");
    say("THE INSTRUCTOR - " + up(LESSON[lesson]) + (pass ? "" : "  (to be done again)"));
    say(d);
    g.tutLog.push_back(fmtShort(g.day) + "  " + LESSON[lesson] + (pass ? ": done." : ": not done."));
    attention = true;

    // set the scene for the next exercise
    if (pass) switch (g.tut) {
        case L_CONTACT: {
            Person a = makePerson(POLITICAL, 0);
            Person b = makePerson(MILITARY, 0);
            bool aBad = r().coin();
            a.fabricator = b.fabricator = false;
            for (int i = 0; i < N_ATTR; i++) { a.app[i] = a.tru[i]; b.app[i] = b.tru[i]; }
            if (aBad) makeDangle(a); else makeDangle(b);
            a.stage = SPOTTED; a.rapport = 22; a.firstDay = g.day;
            a.file.push_back(pad(fmtShort(g.day), 11) + "Met 'at a reception' (exercise). A retired officer playing a ministry clerk.");
            g.tutA = a.id; g.tutB = b.id;
            g.persons.push_back(a);
            // the agent is prepared now but only handed to you two lessons on
            b.stage = BROKEN;
            g.persons.push_back(b);
            break;
        }
        case L_PREPARE: {
            Person* b = person(g.tutB);
            if (!b) break;
            b->stage = AGENT;
            b->codename = CODENAMES[g.codenameIdx++ % CODENAMES.size()];
            b->rapport = 36; b->commitment = 80; b->stress = b->dangle ? 12.f : 38.f; b->craft = 30;
            b->camera = true; b->film = 1; b->tempo = 1;
            b->salary = 20; b->owed = 20;
            b->meetEvery = 28; b->meetSites = {g.tutMeet}; b->meetIdx = 0;
            b->dropEvery = 0; b->dropSite = g.tutDrop; b->signalSite = g.tutSignal;
            b->nextMeet = g.day + 2;                 // "tomorrow evening" as seen from tomorrow morning's card
            b->sanction = 2; b->traceState = 2; b->firstDay = g.day - 200; b->meetings = 6; b->lastContact = g.day - 20;
            for (int i = 0; i < N_ATTR; i++) { b->estC[i] = 40; b->estV[i] = clampf(b->app[i] + r().gauss() * 11, 0, 100); }
            b->file.clear();
            b->file.push_back(gx(b, pad("1967", 11) + "'Recruited' (exercise) on a material basis. Photographs papers at his office with a Minox."));
            b->file.push_back(pad(fmtShort(g.day + 1), 11) + "Handed over to you for the course.");
            Roll& x = newRoll(*b);
            x.frames = FRAMES; x.value = b->dangle ? 38.f : b->tru[A_ACCESS] * 0.8f; x.state = R_AGENT;
            b->held.push_back(x.id);
            schedule(*b);
            break;
        }
        case L_DROP: {
            Person* b = person(g.tutB);
            Site* dsite = site(g.tutDrop); Site* sg = site(g.tutSignal);
            if (!b || !dsite) break;
            Roll& x = newRoll(*b);
            x.frames = FRAMES; x.value = b->dangle ? 35.f : b->tru[A_ACCESS] * 0.8f; x.state = R_DROP;
            g.tutRoll = x.id;
            dsite->rolls.push_back(x.id); dsite->loadedDay = g.day;
            if (sg) sg->mark = 1;
            break;
        }
        case L_EXAM: {                              // both must be there to choose between
            Person* a = person(g.tutA); Person* b = person(g.tutB);
            if (a && !a->live()) a->stage = DEVELOPING;
            if (b && !b->live()) b->stage = AGENT;
            break;
        }
        default: break;
    }
    if (g.tut >= L_DONE) { endTraining(true); return; }
    tutorialIssue();
}

void Sim::endTraining(bool completed) {
    int pc = g.tutMax > 0 ? g.tutMarks * 100 / g.tutMax : 0;
    std::string verdict = !completed ? "Course not completed" : pc >= 85 ? "Excellent" : pc >= 60 ? "Good" : "Satisfactory";
    std::string rep_ = "RED BANNER INSTITUTE - PASSING-OUT REPORT\nTrainee " + g.officer + "\n\n";
    for (auto& l : g.tutLog) rep_ += "  " + l + "\n";
    rep_ += fstr("\n  Marks: %d of %d.   Assessment: ", g.tutMarks, g.tutMax) + verdict + ".\n";
    g.courseReport = rep_;
    g.decisions.clear();
    g.tut = L_NONE; g.tutText.clear();
    g.nextTheatre = TH_LONDON;
    g.phase = PH_INTERLUDE;
    g.interludeTitle = completed ? "END OF THE COURSE" : "THE COURSE CUT SHORT";
    g.interludeText = rep_ + "\n------------------------------------------------------------\n"
        "By order of the head of the First Chief Directorate you are commissioned Captain and posted to the London rezidentura, under cover of Third Secretary (Press), with effect from 2 September 1968.\n\n"
        "In London nobody will tell you what was behind you in the street, or what the man across the table really is. You will have your files, your eyes, and what Centre says of the product.\n\n"
        "The Handbook (the last screen in the list, or F1) has the full rules; section 14 is a plan for your first week.\n\nBefore you leave, the Institute allows you one specialisation:";
    g.interludeOpts = {"Advanced operational course (tradecraft)", "Faculty of agent psychology (assessment of people)", "Language immersion: English"};
    attention = true;
}

void Sim::skipTraining() {
    if (!training() || g.phase != PH_PLAY) return;
    endTraining(false);
}
