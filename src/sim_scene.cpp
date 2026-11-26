// sim_scene.cpp - meetings played as scenes. When you meet one of your agents for a long meeting, or make a
// recruitment approach, the evening stops at the table and you choose how to play it. The choice is kept in
// g.sceneForce and the day is replayed from the morning with it (see endDay), exactly like a decision in the street.
// The scene shows only what an officer at the table could see or already knows.
#include "sim.h"

static const char* const CONFW[] = {"none", "poor", "fair", "good", "high"};

void Sim::scene(Order& o, Person& p, int b) {
    bool agent = p.stage == AGENT;
    bool pitching = !agent && o.pitch >= 0;
    if (!agent && !pitching) return;
    int f = g.sceneForce[b];
    Site* st = site(o.site);
    if (f < 0) {
        StreetHalt h;
        h.block = b; h.kind = DK_SCENE;
        auto est = [&](int i) { return p.estC[i] < 5 ? std::string("not assessed") : fstr("%d, %s confidence", (int)std::lround(p.estV[i]), CONFW[(int)(clampf(p.estC[i], 0, 99.9f) / 20)]); };
        std::string where = st ? shortSite(st) : std::string("the meeting place");
        if (agent) {
            h.title = gx(&p, p.codename + " - " + where);
            std::string sb = gx(&p, p.codename + " is at the table. Relations " + word(p.rapport, W_RAPPORT, 5) + "; commitment " + word(p.commitment, W_COMMIT, 5)
                                  + fstr("; he has been meeting you for %d months. ", std::max(1, (g.day - p.firstDay) / 30)));
            if (!p.warn.empty()) {
                sb += gx(&p, "Before you can begin, he tells you:\n");
                for (auto& w : p.warn) sb += gx(&p, "  - he " + w + "\n");
            } else sb += gx(&p, "He has nothing to report beyond his work.\n");
            Requirement* q = req(p.reqId);
            sb += std::string("\nTempo: ") + TEMPO[std::max(0, std::min(3, p.tempo))] + ".  Tasked: " + (q && !q->closed ? q->title : std::string("general collection")) + ".";
            sb += gx(&p, std::string("\n\nHow do you handle him tonight?"));
            h.text = sb;
            h.opts = {gx(&p, "Reassure him. Talk about his family, take the pressure off, tell him the service looks after its own"),
                      gx(&p, "Press him for more. The requirement is urgent, and he can do better"),
                      gx(&p, fstr("Praise his work, and give him a bonus from your own funds (%d %s)", P(10), T().currency)),
                      gx(&p, "Test him. Go back over the details of his last report, as if casually")};
        } else {
            h.title = gx(&p, "The approach - " + p.name);
            std::string sb = gx(&p, "Two hours, and the moment has come. " + p.name + ", " + p.job + ", " + p.employer + ". Relations " + word(p.rapport, W_RAPPORT, 5)
                                  + "; commitment " + word(p.commitment, W_COMMIT, 5) + ". Centre's sanction: " + (p.sanction == 2 ? "GRANTED." : "NOT GIVEN - you would be acting on your own authority."));
            sb += "\n\nWhat your file says of him:\n";
            sb += "  Money motive ......... " + est(A_MONEY) + "\n  Ideological sympathy . " + est(A_IDEO) + "\n  Ego / resentment ..... " + est(A_EGO) + "\n  Vulnerability ........ " + est(A_VULN) + "\n  Nerve ................ " + est(A_NERVE) + "\n";
            sb += gx(&p, "\nA refusal may be reported to his security officer. On what do you make the approach - or not tonight at all?");
            h.text = sb;
            h.opts = {"Money: he is worth more than they pay him", "Ideology: the cause of peace, and what his own side is doing", "Ego: he is undervalued, and we know his worth",
                      "Coercion: what we know about him", gx(&p, "Not tonight. Keep developing him")};
        }
        throw h;
    }
    // ---- the scene, as you chose to play it
    if (!agent) {
        if (f >= 0 && f <= 3) { o.pitch = f; say(std::string("    You made the approach on: ") + PITCH[f] + "."); }
        else { o.pitch = -1; say("    You judged the moment was not right, and talked of other things."); file(p, "An approach was considered at a meeting, and put off."); }
        return;
    }
    switch (f) {
        case 0:                                   // reassure
            p.stress = clampf(p.stress - 10 - p.rapport / 20, 0, 100);
            p.rapport = clampf(p.rapport + 3, 0, 100);
            say(gx(&p, "    You let him talk about his family, and told him the service does not forget those who serve it. He went home calmer than he came."));
            break;
        case 1: {                                 // press for more
            bool balk = p.stress > 55 || p.tru[A_NERVE] < 35;
            if (balk && !controlled(p)) {
                p.rapport = clampf(p.rapport - 5, 0, 100); p.stress = clampf(p.stress + 8, 0, 100);
                say(gx(&p, "    You pressed him. He went white and said he was doing all he could, and more than was safe. He left angry."));
                file(p, "Pressed for more at a meeting; resisted, frightened.");
            } else {
                if (p.tempo < 3) { o.tempo = p.tempo + 1; }
                p.rapport = clampf(p.rapport - 2, 0, 100); p.stress = clampf(p.stress + 6, 0, 100);
                say(gx(&p, "    You pressed him. He took it, and promised more."));
            }
            break;
        }
        case 2: {                                 // praise and a bonus
            int amt = std::min(g.funds, P(10));
            if (amt > 0) { money(-amt, "Bonus to agent " + p.codename + " (receipt held)"); p.paidTotal += amt; p.paid90 += amt; }
            p.rapport = clampf(p.rapport + 6, 0, 100); p.commitment = clampf(p.commitment + 4, 0, 100); p.stress = clampf(p.stress - 4, 0, 100);
            say(gx(&p, fstr("    You told him his work is read in Moscow, and gave him %d %s on top of his money. He was pleased, and showed it.", amt, T().currency)));
            break;
        }
        case 3: {                                 // test his story
            learn(p, A_HONEST, 40); learn(p, A_DISC, 25);
            p.rapport = clampf(p.rapport - 3, 0, 100);
            bool slip = controlled(p) ? pct(35 + noseNow() * 0.3f) : (p.fabricator && pct(30 + noseNow() * 0.2f));
            if (slip) {
                std::string t = controlled(p) ? "Note: asked again, his account of how he came by the documents was smoother than the first time - as if rehearsed since."
                                              : "Note: asked again, he could not say which file two of the pages came from.";
                say("    " + t);
                file(p, t);
            } else say(gx(&p, "    You went back over his last report, as if casually. His answers were the same as before. He noticed what you were doing, and did not like it."));
            break;
        }
        default: break;
    }
}
