// sim_trade.cpp - the older tricks of the trade:
//   chicken feed (true material that costs its owners nothing) and playing a controlled agent back;
//   confidential contacts, who are not agents and never will be, but talk;
//   the letter through the door, which is a provocation or the best agent the station will ever have;
//   the standing escape plan - a man with a carrier bag at a street corner on a Tuesday evening;
//   the newspapers written up as a source; and the cable that summons an officer home.
#include "sim.h"
#include <algorithm>
#include <cmath>

// ============================================================================ chicken feed and playback
std::string Sim::proposePlayback(Person& p) {
    if (training()) return "Not on the course.";
    if (p.stage != AGENT) return "Only an agent can be played back.";
    if (p.handler != 0) return "Take the case back yourself first. Nobody plays a double through a third person.";
    if (p.play == 1) return "The proposal is with Centre.";
    if (p.play == 2) return gx(&p, p.codename + " is already being played back. To end it, terminate him.");
    p.play = 1;
    queue(C_PLAY, p.id, 120, "Playback: " + p.codename);
    g.paperwork += 2;
    file(p, "Proposed to Centre: that he is under the adversary's control and should be played back.");
    return gx(&p, "Proposal drafted (120 groups): that " + p.codename + " is the adversary's, and that instead of dropping him you go on meeting him - taking what he brings as worthless, and handing him chicken feed prepared by Centre. "
                  "Centre will want grounds: a failed verification, or what Line KR has reported.");
}

void Sim::playSanction(Person& p) {
    if (p.play != 1) return;
    if (p.stage != AGENT) { p.play = 0; return; }
    bool grounds = (p.verify == 3 && p.verifyResult != "CONSISTENT") || p.ciBy != 0 || p.traceBad;
    if (!pct(grounds ? 90 : 35)) {
        p.play = 0;
        file(p, "Centre refused to sanction a playback.");
        cable("Centre", "Your proposal concerning " + p.codename, std::string("NOT sanctioned. ") + (grounds ? "Centre prefers that the case be closed." : "You have shown no grounds for thinking the source controlled. Ask for his verification first.") + "\n\nCENTRE");
        return;
    }
    p.play = 2; p.playMeets = 0;
    file(p, "PLAYBACK sanctioned. His material is to be disregarded; he is to be given only what Centre prepares.");
    cable("Centre", "Your proposal concerning " + p.codename,
          "SANCTIONED. " + p.codename + " is to be treated as an instrument of the adversary and played back.\n\n"
          "1. Nothing he brings is to be credited. It will be read for what it shows of the adversary's intentions.\n"
          "2. At each meeting you will give him the material Centre sends for the purpose: true, and of no value. Task him as if you believed in him.\n"
          "3. They can end it whenever they choose, and in whatever way they choose. Never go to him uncovered.\n\n"
          "If he is not theirs after all, you are throwing away an agent. Be sure.\n\nCENTRE");
}

/** A meeting with an agent who is being played back. */
void Sim::playbackMeet(Person& p, Site& s) {
    if (p.play != 2) return;
    p.playMeets++;
    paper(1);
    if (!controlled(p)) {
        // he was honest all along: he is handed rubbish, and what he brings goes in the waste-paper basket
        p.rapport = clampf(p.rapport - 5, 0, 100); p.commitment = clampf(p.commitment - 4, 0, 100);
        say("    You gave him Centre's prepared questions and took his film without looking at it.");
        if (p.playMeets == 3 || (p.playMeets > 3 && pct(30))) {
            say("    He asked why you no longer seemed interested in what he brings. He seemed to mean it.");
            file(p, "Under playback he appears puzzled and hurt rather than watchful. Is it certain that he is theirs?");
        }
        return;
    }
    say("    You gave him Centre's chicken feed as if it had cost you something, and tasked him as if you believed in him.");
    heat(-1.5f);                                             // they think they can read your diary
    g.standCentre = clampf(g.standCentre + 0.8f, 0, 100);
    credit(SECURITY, rnd(5, 11), -1);
    if (pct(22)) {
        std::vector<Site*> known;
        for (auto& x : g.sites) if (x.burned && !x.retired && x.id != s.id) known.push_back(&x);
        std::vector<Person*> theirs;
        for (auto& q : g.persons) if (&q != &p && q.stage == AGENT && controlled(q) && q.ciBy == 0) theirs.push_back(&q);
        int k = r().n(3);
        if (k == 0 && !known.empty()) {
            Site* x = known[r().n((int)known.size())];
            say("    From the questions he was given to ask you: they know of " + shortSite(x) + ".");
            file(p, "PLAYBACK: his briefing shows the adversary knows " + x->name + ".");
        } else if (k == 1 && !theirs.empty()) {
            Person* q = theirs[r().n((int)theirs.size())];
            q->ciBy = -1; q->ciDay = g.day;
            say("    He let fall something about " + q->employer + " that he could only have from somebody who reports to the same people.");
            file(*q, "PLAYBACK of " + p.codename + ": indications that the adversary has a second source close to us at " + q->employer + ".");
        } else {
            int d = 0;
            for (int i = 1; i < 8; i++) if (g.distMod[i] * T().density[i] > g.distMod[d] * T().density[d]) d = i;
            say(std::string("    He was told to ask where you like to walk. From what interested them: their effort is heaviest in ") + T().district[d] + ".");
        }
    }
    // sooner or later they see that the channel carries nothing
    if (pct(4 + p.playMeets * 1.5f - A->tradecraft / 25.f)) {
        p.play = 3;
        clearAppointments(p);
        p.stage = TERMINATED; p.nextMeet = -1; p.nextDrop = -1;
        heat(6);
        file(p, "PLAYBACK ended by the adversary: he did not appear again.");
        alert(p.codename + " will not be seen again. The other side has understood that the channel was carrying nothing, and has closed it. Expect them to be bad-tempered about it.");
        g.standCentre = clampf(g.standCentre + 2 + std::min(6, p.playMeets) * 0.5f, 0, 100);
    }
}

// ============================================================================ confidential contacts
std::string Sim::registerContact(Person& p) {
    if (training()) return "Not on the course.";
    if (p.stage != DEVELOPING) return "A confidential contact is somebody you are developing openly - not an agent, and not a stranger.";
    if (p.confid == 1) return "The registration is with Centre.";
    if (p.confid == 2) return gx(&p, p.name + " is already registered as a confidential contact.");
    if (p.meetings < 4 || p.rapport < 45) return gx(&p, "Not yet. Centre registers a confidential contact when the relationship is established: at least four meetings, and friendly terms.");
    int have = 0, cap = 3 + T().role;
    for (auto& q : g.persons) if (q.live() && q.stage == DEVELOPING && q.confid >= 1) have++;
    if (have >= cap) return fstr("Centre carries %d confidential contacts on the books of an officer in your post, and you have them. Close one, or recruit one properly.", cap);
    p.confid = 1;
    queue(C_CONFID, p.id, 40, "Confidential contact: " + p.name);
    g.paperwork += 1;
    return gx(&p, "Registration drafted (40 groups). A confidential contact is not recruited and signs nothing: he knows he is talking to a Soviet diplomat, and talks. "
                  "Once he is registered, each lunch or dinner with him yields a report for the pouch - opinion, gossip, who is rising and who is finished - worth what his position is worth.");
}

void Sim::confidRegistered(Person& p) {
    if (p.confid != 1) return;
    if (!p.live() || p.stage == AGENT) { p.confid = 0; return; }
    p.confid = 2;
    file(p, "Registered by Centre as a CONFIDENTIAL CONTACT.");
    cable("Centre", "Registration of " + p.name, p.name + ", " + p.job + ", " + p.employer + ", is registered as a confidential contact of the rezidentura. "
          "Report each conversation. He is not to be tasked, paid a salary, or met clandestinely unless and until he is recruited in the ordinary way.\n\nCENTRE");
}

/** Lunch with a registered confidential contact: something for the pouch. */
void Sim::confidTalk(Person& p) {
    if (p.confid != 2 || !me() || g.day - p.lastConfid < 14) return;
    p.lastConfid = g.day;
    Roll& x = newRoll(p);
    x.oral = true;
    x.value = x.genuine ? clampf(p.tru[A_ACCESS] * rnd(0.18f, 0.38f) + p.tru[A_INFL] * 0.12f, 3, 48) : rnd(18, 40);
    x.state = R_YOU;
    paper(1.5f);
    say(fstr("    As a confidential contact he talked freely, and knew he was doing it. Written up as item No. %d, for the pouch.", x.id));
}

// ============================================================================ the letter through the door
namespace {
const char* const PIN_PLACE[] = {"the top of the right-hand banister on the stairs of the underground station nearest the Embassy", "the wooden post at the entrance to the public gardens",
                                 "the frame of the notice-board outside the public library", "the back of the third bench along the river walk"};
const char* const ACK_PLACE[] = {"a piece of blue tape round the telephone flex of the middle call-box at the main station", "a chewing-gum wrapper pressed into the railings by the bus stop opposite",
                                 "a chalk cross on the lamp-post at the corner", "an empty cigarette packet on the window-ledge of the tobacconist"};
}

/** An officer of a foreign service offers himself, without a name. ref: bit 0 = he is genuine; bits 1.. = the service. */
void Sim::walkInLetter() {
    bool genuine = pct(55);
    int svc = 0;
    const char* who = "one of their secret departments";
    std::string head = T().role == 3 ? "your own letter-box" : "the Rezident's letter-box";
    addDecision(DK_WALKIN, (svc << 1) | (genuine ? 1 : 0), "A letter through the door",
                "In the night an envelope was pushed through " + head + ". Inside: two pages that look like a genuine internal paper of " + who + " - a list of our own people whom they have identified, with most of them right - and a typed letter. "
                "The writer gives no name. He says he can give us everything, and that he will not come near the Embassy.\n\n"
                "If we are interested we are to press a drawing-pin into " + PIN_PLACE[r().n(4)] + ". He will answer with " + ACK_PLACE[r().n(4)] + ", and then fill a dead drop he describes: the cistern of a cinema lavatory.\n\n"
                + (T().role == 3 ? "Nobody offers a service's crown jewels through a letter-box. Unless he does."
                                 : "The Rezident has read it twice and says it is a provocation: nobody offers a service's crown jewels through a letter-box, and the list is there to make us believe the rest. He may be right. He was not sent abroad for his imagination."),
                {T().role == 3 ? "Press the drawing-pin in: take it up" : "Press the drawing-pin in yourself, against the Rezident's judgment", "Ask Centre what to do (two or three weeks)", "A provocation. Do nothing"});
}

/** The man behind the letter comes into the card index - as an agent offering himself, or as the other side's instrument. */
void Sim::letterTakenUp(bool genuine, int svc) {
    (void)svc;
    Person np = makeSpy(genuine ? 0.f : 1.f);
    np.walkIn = true;
    np.rapport = 32; np.commitment = genuine ? 55.f : 60.f; np.stage = DEVELOPING;
    np.meetings = 1;
    if (!np.dangle) np.tru[A_MONEY] = np.app[A_MONEY] = std::max(np.tru[A_MONEY], rnd(55, 85));
    file(np, "WALK-IN BY LETTER. Offered himself anonymously; contact established by signal and dead drop. " + np.job + ", " + np.employer + ".");
    int id = np.id;
    std::string who = np.name + ", " + np.job + ", " + np.employer;
    g.totSpotted++; g.mSpotted++;
    g.persons.push_back(np);
    Person* p = person(id);
    Roll& x = newRoll(*p);
    x.frames = FRAMES; x.genuine = true;                     // his sample is true either way: that is what a sample is for
    x.value = genuine ? rnd(70, 92) : rnd(48, 64);
    x.state = R_YOU;
    Person* keep = subj; subj = nullptr;
    cable("Line KR", "The letter", "The drawing-pin was answered within the day, and the cistern was filled the next night: a cassette of film (item No. " + fstr("%d", x.id) + ", in your safe) and a second letter, this time with a name. "
          "He is " + who + ", if he is anybody. He is in the card index. Trace him, ask Centre for sanction, and meet him only as you would meet a man who may be bait.\n\nLINE KR");
    subj = keep;
    if (!genuine) { heat(4); return; }
    // a traitor in the station tells them, as traitors do
    for (auto& o : g.officers)
        if (o.mole && !o.gone && o.moleDay <= g.day) { pend(g.day + 25 + r().n(50), P_KOBA, id, 0); break; }
}

void Sim::walkInDecide(int ref, int option) {
    bool genuine = ref & 1;
    int svc = ref >> 1;
    if (option == 0) {
        if (T().role < 3) g.standRez = clampf(g.standRez - 3, 0, 100);
        letterTakenUp(genuine, svc);
    } else if (option == 1) {
        g.paperwork += 1;
        pend(g.day + 14 + r().n(8), P_LETTER, ref, 0);
    } else if (genuine) {
        if (pct(50)) pend(g.day + 90 + r().n(200), P_LOSTOFFER, svc, 0);
    } else if (T().role < 3) g.standRez = clampf(g.standRez + 1, 0, 100);
}

/** Centre's answer about the letter: by now he may have lost patience - or been given away. */
void Sim::letterAnswer(int ref) {
    bool genuine = ref & 1;
    int svc = ref >> 1;
    bool mole = false;
    for (auto& o : g.officers) if (o.mole && !o.gone && o.moleDay <= g.day) mole = true;
    if (genuine && mole && pct(60)) {
        g.standCentre = clampf(g.standCentre - 3, 0, 100);
        cable("Centre", "The letter", "Centre's analysts judge the document GENUINE and the offer probably sincere. You were to take it up.\n\n"
              "It is too late. The newspapers report this morning that an officer of the service concerned has been arrested and charged with attempting to communicate with a foreign power. "
              "Nobody outside your station knew that the letter existed. Consider what that means, and who read it.\n\nCENTRE");
        return;
    }
    if (genuine && pct(25)) {
        cable("Centre", "The letter", "Centre's analysts judge the document GENUINE. Take it up at once.\n\nLINE KR adds: the drawing-pin was placed this morning. There has been no answer. A man who writes such letters does not wait three weeks.");
        g.standCentre = clampf(g.standCentre - 2, 0, 100);
        return;
    }
    bool says = pct(75) ? genuine : !genuine;
    addDecision(DK_WALKIN, (svc << 1) | (genuine ? 1 : 0), "The letter: Centre's answer",
                std::string("Centre's analysts have compared the two pages with what is held. They judge the document ") + (says ? "GENUINE: the names are right, and two of them could not have been known to a forger." : "a FABRICATION built round true names, of the kind prepared to draw an officer into the open.")
                    + "\n\nCentre leaves the decision to the station, 'which is on the spot'.",
                {"Press the drawing-pin in: take it up", "Ask Centre again (they will say the same)", "Leave it alone"});
}

// ============================================================================ the standing escape plan
/** At a long meeting: rehearse the plan and leave it standing. He stays in place, with a way out. */
void Sim::rehearseEscape(Person& p) {
    if (p.exfil != 2) { say("    You had no travel documents prepared for him."); return; }
    if (p.signalSite < 0) { say("    He has no signal site. A standing escape plan needs a place where he can be seen without a word passing."); return; }
    p.exfil = 0; p.escape = 2;
    Site* sg = site(p.signalSite);
    say("    You rehearsed the escape plan until he could say it backwards. If he believes he is about to be arrested he stands at " + (sg ? shortSite(sg) : std::string("the signal site"))
        + " at half past seven in the evening with a carrier bag from a shop you named. When somebody walks past him eating a bar of chocolate, he knows he has been seen, and three nights later he is at the pick-up.");
    file(p, "ESCAPE PLAN standing. Signal: a man with a carrier bag at " + (sg ? sg->name : std::string("his signal site")) + ". The documents are held in the safe.");
    if (!controlled(p)) p.stress = clampf(p.stress - 12, 0, 100);       // a man works better who knows there is a door
}

/** He believes he is finished, and goes to stand at the corner with the carrier bag. */
void Sim::escapeSignal(Person& p) {
    Site* sg = site(p.signalSite);
    if (p.escape != 2 || !sg || controlled(p)) return;
    p.escape = 3; p.escapeDay = g.day;
    sg->mark = 4;
    p.tempo = 0;
    clearAppointments(p);
    p.nextMeet = -1;
}

/** Somebody of ours has walked past the signal site and seen the man with the carrier bag. */
void Sim::escapeSeen(Site& s) {
    for (auto& p : g.persons) {
        if (p.stage != AGENT || p.signalSite != s.id || p.escape != 3) continue;
        p.escape = 4;
        pend(g.day + 2, P_ESCAPE, p.id, 0);
        Person* keep = subj; subj = &p;
        alert("THE ESCAPE SIGNAL. " + p.codename + " was standing at " + s.name + " with the carrier bag. He has been acknowledged. The pick-up is in three nights; it is out of your hands now.");
        file(p, "Gave the escape signal; acknowledged.");
        subj = keep;
    }
}

void Sim::escapePickup(Person& p) {
    if (p.stage != AGENT || p.escape != 4) return;
    p.escape = 0;
    if (Site* sg = site(p.signalSite)) if (sg->mark == 4) sg->mark = 0;
    float ok = 82 - std::max(0.f, p.suspicion - 78) * 1.5f - (g.you.heat > 55 ? 8 : 0) + p.skComms / 10.f - (p.tru[A_NERVE] < 40 ? 8 : 0);
    if (pct(clampf(ok, 20, 96))) {
        p.stage = EXFILTRATED;
        clearAppointments(p);
        p.nextMeet = -1; p.nextDrop = -1;
        g.totExfil++;
        for (int id : p.held) if (Roll* x = roll(id)) x->state = R_LOST;
        p.held.clear();
        g.standCentre = clampf(g.standCentre + (p.service != SVC_NONE ? 9.f : 5.f), 0, 100); g.standRez = clampf(g.standRez + 3, 0, 100);
        file(p, "EXFILTRATED on the standing plan. Collected at the pick-up and taken across the frontier.");
        alert(p.label() + " IS OUT. He was at the pick-up on the third night, and crossed the frontier before it was light. They were perhaps a day behind him.");
    } else arrest(p, "ARRESTED on his way to the pick-up. They had been closer than he knew.");
}

// ============================================================================ the newspapers, written up
std::string Sim::padReport() {
    if (training()) return "Not on the course.";
    if (g.day - g.lastPad < 14) return "Not twice in a fortnight. Even Centre reads the newspapers sometimes.";
    Person* src = nullptr;
    for (auto& p : g.persons)
        if (p.live() && p.handler == 0 && (p.stage == DEVELOPING || p.stage == AGENT) && p.meetings >= 2 && (!src || p.rapport > src->rapport)) src = &p;
    if (!src) return "A report needs a source to hang it on: a contact you have met at least twice.";
    g.lastPad = g.day;
    g.paperwork += 1.5f;
    float pts = rnd(12, 24);
    bool noticed = pct(10 + g.padded * 8);
    g.padded += 1;
    if (noticed) {
        g.padCaught = true;
        g.standCentre = clampf(g.standCentre - 6, 0, 100);
        if (T().role < 3) g.standRez = clampf(g.standRez - 2, 0, 100);
        Person* keep = subj; subj = nullptr;
        cable("Centre", "Your report of a conversation with a source", "The 'assessment by a well-placed source' in your last report reproduces, in places word for word, a leading article that appeared in the press four days before your report was written. "
              "Centre subscribes to the same newspapers. This is noted on your file.\n\nCENTRE");
        subj = keep;
        return "You wrote up three days of newspapers as the considered view of " + src->name + ", and sent it. Somebody at Centre had read the same newspapers.";
    }
    g.totValue += pts; g.mValue += pts;
    credit(POLITICAL, pts, -1);
    return "You wrote up three days of newspapers as the considered view of " + src->name + fstr(", and sent it. It will be counted (%d points). ", (int)pts)
           + "Half the station does it. The other half is caught now and then.";
}

// ============================================================================ the summons
void Sim::summons() {
    addDecision(DK_SUMMONS, 0, "A cable, personal",
                "PERSONAL FOR " + g.officer + ". You will fly to Moscow on Saturday for consultations concerning your next appointment, and to be received by the leadership of the directorate. Confirm.\n\n"
                "The Rezident handed it to you himself, and smiled. Nobody is summoned for consultations at two days' notice. Your predecessor in this post was, once: he is teaching at the institute now, they say, though nobody has seen him.\n\n"
                "The man who lunches on Thursdays gave you a telephone number once, for an evening like this.",
                {"Go. Whatever they think they know, they cannot prove it", "Ring the number"});
}

void Sim::summonsDecide(int option) {
    if (option == 1) {
        if (pct(55)) {
            finishCareer("DEFECTOR",
                         "You rang the number from a call-box and said the sentence you had been given. Two days later, on the road to the airport, your car stopped at a lay-by you had been told to look for. "
                         "You crossed a frontier in the boot of another car, under a blanket that was meant to hide the heat of a body, with a sedative in your pocket that you did not take.\n\n"
                         "In Moscow you were tried in your absence and sentenced to death. The debriefing took two years. What you told them was worth, they said, more than they had paid for anything since the war.");
        } else {
            finishCareer("LEFORTOVO",
                         "You rang the number from a call-box and said the sentence you had been given. The men who met you at the lay-by on the airport road were not the ones you expected. They had been listening to that call-box for a month.\n\n"
                         "The investigation took four months and the trial one day, in closed session.");
        }
        return;
    }
    // the dacha, the brandy, the questions
    float confess = 45 - (g.patron >= 55 ? 20 : 0) + (g.fatigue > 60 ? 10 : 0) - g.you.tradecraft / 10.f + (g.home < 30 ? 8 : 0);
    if (pct(clampf(confess, 10, 80))) {
        finishCareer("LEFORTOVO",
                     "They took you to a dacha outside Moscow, 'to wait for the General'. There was a housekeeper, and sandwiches, and Armenian brandy. After the second glass the room began to move. "
                     "Two men you had never seen asked you questions for five hours. You remember the questions. You do not remember what you answered.\n\n"
                     "In the morning they told you what you had answered. The trial took one day, in closed session.");
        return;
    }
    g.dirt = 0;
    endPosting(END_RECALLED, "RECALLED TO MOSCOW",
               "They took you to a dacha outside Moscow, 'to wait for the General'. There was a housekeeper, and sandwiches, and Armenian brandy. After the second glass the room began to move. "
               "Two men you had never seen asked you questions for five hours. You remember denying everything, and you remember that it was difficult.\n\n"
               "In the morning they said: we know very well that you have been deceiving us for a long time, but we have decided that you may stay in the service. Not abroad. "
               "You were given a desk in a department that reads other people's files, and a car that followed you home.");
}
