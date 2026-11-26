// ui_launch.cpp - the launcher: the small window the game opens in. Continue, a new career (where to begin,
// background, rules), saved games, the service records of past careers, settings, the handbook.
// When a career is begun or loaded the window grows into the desk (main_win32.cpp does the resizing).
#include "records.h"
#include "ui.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <cfloat>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>

using namespace gfx;

namespace {
const char* const DIFF_LABEL[] = {"Forgiving", "Standard", "Unforgiving"};
const char* const DIFF_TEXT[] = {
    "The watchers are fewer and slower, and agents forgive a missed meeting.",
    "The game as it is meant to be played.",
    "More watchers, sharper counter-intelligence, and less patience at Centre."};
const int STARTS[] = {TH_MOSCOW, TH_LONDON, TH_VIENNA, TH_DELHI, TH_WASHINGTON, TH_BERLIN1, TH_BONN};
const char* const ROLE_TITLE[] = {"case officer", "senior case officer", "head of line", "Rezident"};
const char* const MONTHS[] = {"", "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

std::string startLabel(int th) {
    const Theatre& t = THEATRES[th];
    if (t.training) return "Moscow, June 1968 - the training course";
    return std::string(th == TH_BERLIN1 ? "East Berlin" : t.city) + ", " + MONTHS[t.sm] + " " + std::to_string(t.sy) + " - " + ROLE_TITLE[t.role];
}
std::string startText(int th) {
    const Theatre& t = THEATRES[th];
    if (t.training) return "Three weeks of field exercises with an instructor who tells you afterwards what was really going on - the only time anyone will. Then London. Recommended for a first career.";
    std::string s = std::string("You arrive as ") + RANK[Sim::startRank(th)] + ", " + t.cover + ". ";
    if (t.slot == 0) s += "The whole career lies ahead: four postings, each decided by how the last one went.";
    else if (t.slot == 3) s += "The last posting of a career, and the end of the state you serve. Experienced, and expected to deliver.";
    else s += "The earlier postings are behind you; you start with the experience they would have given you. Later postings follow from how this one goes.";
    if (t.role >= 2) s += fstr(" You command %d officers of your own.", t.subs);
    else if (t.role == 1) s += " One junior officer works under you.";
    return s;
}

/** A row of a menu: an icon in a brass ring, a title in capitals and a line under it, the whole row clickable. */
bool menuRow(const char* id, const char* title, const char* sub, float w, float S, bool enabled = true, int ic = -1) {
    using namespace skin;
    ImVec2 p = ImGui::GetCursorScreenPos();
    float h = (sub && *sub ? 52 : 42) * S;
    ImGui::BeginDisabled(!enabled);
    bool r = ImGui::InvisibleButton(id, ImVec2(w, h));
    ImGui::EndDisabled();
    uiRec(id);
    bool hov = enabled && ImGui::IsItemHovered();
    bool held = hov && ImGui::IsItemActive();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float al = enabled ? 1 : 0.45f;
    if (hov) {
        dl->AddRectFilledMultiColor(p, p + ImVec2(w, h), rgb(K_RED_LO, 0.75f), rgb(K_RED_LO, 0.0f), rgb(K_RED_LO, 0.0f), rgb(K_RED_LO, 0.75f));
        dl->AddRectFilled(p, p + ImVec2(3 * S, h), rgb(K_BRASS_HI));
    }
    dl->AddRectFilled(ImVec2(p.x, p.y + h - 1), ImVec2(p.x + w, p.y + h), rgb(K_BRASS, 0.12f));
    float x = p.x + 16 * S;
    if (ic >= 0) {
        ImVec2 c(p.x + 32 * S, p.y + h / 2 + (held ? 1 : 0));
        dl->AddCircleFilled(c, 17 * S, rgb(K_INSET, al), 32);
        dl->AddCircle(c, 17 * S, rgb(hov ? K_BRASS : K_BRASS_LO, al), 32, 1.2f * S);
        icon(dl, ic, c, 18 * S, rgb(hov ? K_BRASS_HI : K_BRASS, al));
        x = p.x + 62 * S;
    }
    std::string t = upper(title);
    float ty = p.y + (sub && *sub ? 7 : (h - F.bar->FontSize) / 2 / S) * S;
    tracked(dl, F.bar, F.bar->FontSize, ImVec2(x, ty), rgb(hov ? K_BRASS_HI : K_TEXT, al), t.c_str(), 1.4f * S);
    if (sub && *sub) dl->AddText(F.uiSmall, F.uiSmall->FontSize, ImVec2(x, ty + F.bar->FontSize + 4 * S), rgb(K_DIM, al), sub);
    // a chevron on the right
    ImVec2 c(p.x + w - 20 * S + (hov ? 3 * S : 0), p.y + h / 2);
    dl->AddLine(c + ImVec2(-4 * S, -6 * S), c + ImVec2(2 * S, 0), rgb(hov ? K_BRASS_HI : K_BRASS_LO, al), 1.8f * S);
    dl->AddLine(c + ImVec2(2 * S, 0), c + ImVec2(-4 * S, 6 * S), rgb(hov ? K_BRASS_HI : K_BRASS_LO, al), 1.8f * S);
    return r && enabled;
}

/** A heading in brass capitals with a rule to the right. */
void caption(const char* t, float S) { (void)S; uiSection(t); }
void hint(const std::string& t) {
    ImGui::PushFont(F.uiSmall);
    ImGui::PushStyleColor(ImGuiCol_Text, rgb4(skin::K_DIM));
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x);
    ImGui::TextUnformatted(t.c_str());
    ImGui::PopTextWrapPos();
    ImGui::PopStyleColor();
    ImGui::PopFont();
}

std::vector<std::string> split(const std::string& s, char sep) {
    std::vector<std::string> v;
    std::string cur;
    for (char c : s) { if (c == sep) { v.push_back(cur); cur.clear(); } else cur += c; }
    v.push_back(cur);
    return v;
}
}  // namespace

// ---------------------------------------------------------------------------- the service records
void App::readArchive() {
    archive.clear();
    std::ifstream f(std::filesystem::u8path(saveDir + "/service-records.txt"));
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        auto v = split(line, '\t');
        if (v.size() < 14 || v[0] != "1") continue;
        Record r;
        r.officer = v[1]; r.rank = v[2]; r.background = atoi(v[3].c_str()); r.start = atoi(v[4].c_str());
        r.difficulty = atoi(v[5].c_str()); r.ironman = v[6] == "1"; r.seed = v[7]; r.ended = v[8]; r.ending = v[9];
        r.postings = v[10]; r.recruited = atoi(v[11].c_str()); r.awards = atoi(v[13].c_str());
        if (!v[12].empty()) for (auto& c : split(v[12], ',')) { int k = atoi(c.c_str()); if (k >= 0 && k < N_CITATION) r.cites.push_back(k); }
        if (r.start < 0 || r.start >= (int)THEATRES.size()) r.start = TH_LONDON;
        archive.push_back(r);
    }
    archiveRead = true;
}

void App::recordCareer() {
    GameState& g = sim.g;
    if (g.archived || g.phase != PH_OVER) return;
    g.archived = true;
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::u8path(saveDir), ec);
    std::string path = saveDir + "/service-records.txt";
    bool fresh = !std::filesystem::exists(std::filesystem::u8path(path), ec);
    std::ofstream f(std::filesystem::u8path(path), std::ios::app);
    if (fresh) f << "# Rezidentura - service records. One line per finished career; the game reads this file. Delete it to start the records afresh.\n";
    f << sim.archiveLine() << "\n";
    archiveRead = false;
    autosave();
}

/** Citations earned across all careers (and "Every road", which only the records as a whole can earn). */
std::vector<bool> App::citationsEarned() {
    if (!archiveRead) readArchive();
    std::vector<bool> got(N_CITATION, false);
    bool bg[4] = {};
    for (auto& r : archive) {
        for (int c : r.cites) got[c] = true;
        if (r.background >= 0 && r.background < 4) bg[r.background] = true;
    }
    got[CI_ALLBACK] = bg[0] && bg[1] && bg[2] && bg[3];
    return got;
}

// ---------------------------------------------------------------------------- the launcher
ImVec2 App::launcherSize() const { return ImVec2(std::floor(1040 * S), std::floor(650 * S)); }

bool App::launcherDragAt(float x, float y) const {
    ImVec2 sz = launcherSize();
    return !inGame && y >= 0 && y < 46 * S && x < sz.x - 96 * S && !ImGui::IsAnyItemHovered();
}

void App::peekAutosave() {
    peekDone = true;
    peekOk = false;
    std::ifstream f(std::filesystem::u8path(saveDir + "/autosave.rez"), std::ios::binary);
    if (!f) return;
    std::vector<uint8_t> buf((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    Sim t;
    if (!t.load(buf)) { peekLine1 = "The last game could not be read"; peekLine2 = "It may be from an older version of the game."; return; }
    const GameState& g = t.g;
    const Theatre& th = THEATRES[g.theatre];
    peekOk = true;
    if (g.phase == PH_OVER) {
        peekLine1 = std::string(RANK[g.rank]) + " " + g.officer + " - the career is over";
        peekLine2 = "Open it to read the final record.";
        return;
    }
    peekLine1 = (th.training ? std::string("Senior Lieutenant ") : std::string(RANK[g.rank]) + " ") + g.officer + "  -  " + th.city;
    peekLine2 = fmtDay(g.day) + "   -   " + DIFF_LABEL[std::max(0, std::min(2, g.difficulty))] + "   -   world " + std::to_string(g.seed) + (g.ironman ? "   -   ironman" : "");
}

void App::titleScreen() {
    using namespace skin;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 a = ImGui::GetWindowPos(), sz = ImGui::GetWindowSize(), b = a + sz;
    if (!peekDone) peekAutosave();
    pushInk(true);
    float M = std::floor(14 * S);
    // ---- the poster on the left, in a brass frame: the banner field, rays, and the emblem in gold
    float PWD = std::floor(std::min(372 * S, sz.x * 0.36f));
    ImVec2 pa(a.x + M, a.y + M), pb(a.x + M + PWD, b.y - M);
    dl->AddRectFilled(pa + ImVec2(4 * S, 6 * S), pb + ImVec2(4 * S, 6 * S), rgb(0, 0.45f));
    dl->AddRectFilledMultiColor(pa, pb, rgb(0x9A120E), rgb(0x7E0D0A), rgb(0x3A0505), rgb(0x540808));
    dl->PushClipRect(pa, pb, true);
    ImVec2 em = (pa + pb) * 0.5f + ImVec2(0, -sz.y * 0.03f);
    float E = std::min(PWD * 0.6f, sz.y * 0.4f);
    for (int i = 0; i < 40; i++) {
        float a0 = i * 6.2831853f / 40, a1 = a0 + 6.2831853f / 80;
        dl->AddTriangleFilled(em, em + ImVec2(std::cos(a0), std::sin(a0)) * (sz.y * 1.2f), em + ImVec2(std::cos(a1), std::sin(a1)) * (sz.y * 1.2f), rgb(0xFFD27A, 0.03f));
    }
    dl->AddCircleFilled(em, E * 0.66f, rgb(0xFFD27A, 0.04f), 64);
    hammerSickle(dl, em + ImVec2(5 * S, 7 * S), E, rgb(0x200000, 0.45f));
    hammerSickle(dl, em, E, rgb(0xE9B73A));
    ImVec2 st = em + ImVec2(E * 0.06f, -E * 0.64f);
    star(dl, st + ImVec2(3 * S, 4 * S), E * 0.10f, rgb(0, 0), 0, rgb(0x200000, 0.45f), 3 * S);
    star(dl, st, E * 0.10f, rgb(0, 0), 0, rgb(0xE9B73A), 3 * S);
    // a vignette, so the poster sits back in its frame
    dl->AddRectFilledMultiColor(pa, ImVec2(pb.x, pa.y + 90 * S), rgb(0, 0.35f), rgb(0, 0.35f), rgb(0, 0), rgb(0, 0));
    dl->AddRectFilledMultiColor(ImVec2(pa.x, pb.y - 120 * S), pb, rgb(0, 0), rgb(0, 0), rgb(0, 0.5f), rgb(0, 0.5f));
    {
        const char* yrs = "1968  \xC2\xB7  1991";
        float tw = trackedWidth(F.bar, F.bar->FontSize, yrs, 3 * S);
        float yy = pb.y - 50 * S;
        tracked(dl, F.bar, F.bar->FontSize, ImVec2((pa.x + pb.x - tw) / 2, yy), rgb(0xF2D58A, 0.9f), yrs, 3 * S);
        divider(dl, ImVec2(pa.x + 40 * S, yy - 16 * S), PWD - 80 * S, S, 0.8f);
    }
    dl->PopClipRect();
    trim(dl, pa, pb, S);
    // ---- the panel on the right
    ImVec2 ra(pb.x + M, a.y + M), rb(b.x - M, b.y - M);
    skin::frame(dl, ra, rb, S);
    // window controls (the window has no frame of its own)
    {
        auto ctl = [&](const char* id, ImVec2 c, bool close) {
            ImGui::SetCursorScreenPos(c - ImVec2(17 * S, 14 * S));
            bool hit = ImGui::InvisibleButton(id, ImVec2(34 * S, 28 * S));
            uiRec(id);
            bool hov = ImGui::IsItemHovered();
            if (hov) dl->AddRectFilled(c - ImVec2(17 * S, 14 * S), c + ImVec2(17 * S, 14 * S), rgb(close ? K_RED : K_ROW));
            icon(dl, close ? IC_CLOSE : IC_MIN, c, 16 * S, rgb(hov ? K_TEXT : K_DIM));
            return hit;
        };
        if (ctl("window:close", ImVec2(rb.x - 26 * S, ra.y + 24 * S), true)) wantQuit = true;
        if (ctl("window:minimise", ImVec2(rb.x - 62 * S, ra.y + 24 * S), false)) wantMinimise = true;
    }
    float L = ra.x + 36 * S, R = rb.x - 36 * S;
    // the title
    float tsize = std::floor(F.title->FontSize * 0.64f);
    float ty = ra.y + 40 * S;
    dl->AddText(F.title, tsize, ImVec2(L + 2 * S, ty + 3 * S), rgb(0, 0.6f), "РЕЗИДЕНТУРА");
    dl->AddText(F.title, tsize, ImVec2(L, ty), rgb(K_BRASS_HI), "РЕЗИДЕНТУРА");
    tracked(dl, F.uiCaps, F.uiCaps->FontSize, ImVec2(L + 2 * S, ty + tsize * 1.22f), rgb(K_DIM), "A KGB OFFICER'S CAREER ABROAD", 2.2f * S);
    float top = ty + tsize * 1.22f + F.uiCaps->FontSize + 18 * S;
    divider(dl, ImVec2(L, top), R - L, S, 0.9f);
    top += 18 * S;
    ImGui::SetCursorScreenPos(ImVec2(L - 4 * S, top));
    ImGui::BeginChild("##launch", ImVec2(R - L + 8 * S, rb.y - top - 40 * S), 0, ImGuiWindowFlags_NoBackground);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8 * S, 6 * S));
    if (page == 1) launchNew();
    else if (page == 2) launchLoad();
    else if (page == 3) launchRecords();
    else launchHome();
    ImGui::PopStyleVar();
    ImGui::EndChild();
    // the footer
    std::string foot = "Version " REZ_VERSION "   \xC2\xB7   The people are invented. The places, the dates and the history are real.";
    dl->AddLine(ImVec2(L, rb.y - 34 * S), ImVec2(R, rb.y - 34 * S), rgb(K_BRASS, 0.12f), 1);
    dl->AddText(F.uiSmall, F.uiSmall->FontSize, ImVec2(L, rb.y - 26 * S), rgb(K_FAINT), foot.c_str());
    popInk();
}

void App::launchHome() {
    using namespace skin;
    float w = ImGui::GetContentRegionAvail().x;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    // continue: the last game, described
    if (peekOk || !peekLine1.empty()) {
        ImVec2 p = ImGui::GetCursorScreenPos() + ImVec2(4 * S, 0);
        float cw = w - 8 * S, h = 78 * S;
        ImGui::SetCursorScreenPos(p);
        ImGui::BeginDisabled(!peekOk);
        bool hit = ImGui::InvisibleButton("Continue", ImVec2(cw, h));
        ImGui::EndDisabled();
        uiRec("Continue");
        bool hov = peekOk && ImGui::IsItemHovered();
        ImVec2 q = p + ImVec2(cw, h);
        dl->AddRectFilled(p + ImVec2(3 * S, 4 * S), q + ImVec2(3 * S, 4 * S), rgb(0, 0.4f));
        dl->AddRectFilledMultiColor(p, q, rgb(hov ? 0x3C2F24 : 0x32281F), rgb(hov ? 0x3C2F24 : 0x32281F), rgb(0x1F1914), rgb(0x1F1914));
        dl->AddRect(p, q, rgb(hov ? K_BRASS : K_BRASS_LO), 0, 0, 1.2f * S);
        dl->AddRect(p + ImVec2(4 * S, 4 * S), q - ImVec2(4 * S, 4 * S), rgb(K_BRASS, hov ? 0.3f : 0.15f), 0, 0, 1);
        dl->AddRectFilled(p, ImVec2(p.x + 4 * S, q.y), rgb(K_RED));
        tracked(dl, F.uiCaps, F.uiCaps->FontSize, p + ImVec2(22 * S, 10 * S), rgb(K_BRASS), "CONTINUE", 2.2f * S);
        dl->AddText(F.uiTitle, F.uiTitle->FontSize, p + ImVec2(22 * S, 27 * S), rgb(K_TEXT), peekLine1.c_str());
        dl->AddText(F.uiSmall, F.uiSmall->FontSize, p + ImVec2(22 * S, 27 * S + F.uiTitle->FontSize + 3 * S), rgb(K_DIM), peekLine2.c_str());
        if (peekOk) {
            ImVec2 c(q.x - 42 * S, p.y + h / 2);
            dl->AddCircleFilled(c + ImVec2(0, 2 * S), 20 * S, rgb(0, 0.5f), 32);
            dl->AddCircleFilled(c, 20 * S, rgb(hov ? 0xC8402A : K_RED), 32);
            dl->AddCircle(c, 20 * S, rgb(hov ? K_BRASS_HI : K_BRASS), 32, 1.5f * S);
            dl->AddCircle(c, 16 * S, rgb(0, 0.25f), 32, 1);
            dl->AddTriangleFilled(c + ImVec2(-5 * S, -8 * S), c + ImVec2(-5 * S, 8 * S), c + ImVec2(9 * S, 0), rgb(0xFBF3E2));
        }
        if (hit && peekOk && !loadFrom(saveDir + "/autosave.rez")) note = "The last game could not be read (it may be from an older version of the game).";
        ImGui::Dummy(ImVec2(1, 12 * S));
    }
    if (!archiveRead) readArchive();
    auto got = citationsEarned();
    int n = 0;
    for (bool x : got) n += x;
    std::string rec = archive.empty() ? std::string("No career has ended yet") : fstr("%d career%s on file  \xC2\xB7  %d of %d citations", (int)archive.size(), archive.size() == 1 ? "" : "s", n, N_CITATION);
    if (menuRow("New career", "New career", "Choose where to begin, your background and the rules", w, S, true, IC_NEW)) { page = 1; }
    if (menuRow("Load a saved game", "Load a saved game", "Named saves, quick saves and the monthly backups", w, S, true, IC_LOAD)) { page = 2; loadList.clear(); }
    if (menuRow("Service records", "Service records", rec.c_str(), w, S, true, IC_RECORDS)) { page = 3; readArchive(); }
    if (menuRow("Settings", "Settings", "Display, windows and keys", w, S, true, IC_GEAR)) openSettings = true;
    if (!pdfPath.empty() && openFile && menuRow("Handbook", "Officer's Handbook", "The manual, as a PDF", w, S, true, IC_BOOK)) openFile(pdfPath);
    if (menuRow("Quit", "Quit", nullptr, w, S, true, IC_QUIT)) wantQuit = true;
}

static bool backLink(float S) {
    using namespace skin;
    const char* id = "\xE2\x80\xB9 Back";
    ImVec2 p = ImGui::GetCursorScreenPos();
    float tw = trackedWidth(F.uiCaps, F.uiCaps->FontSize, "BACK", 2 * S);
    ImVec2 sz(tw + 30 * S, 24 * S);
    bool r = ImGui::InvisibleButton(id, sz);
    uiRec(id);
    bool hov = ImGui::IsItemHovered();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 c = rgb(hov ? K_BRASS_HI : K_BRASS);
    ImVec2 m(p.x + 7 * S, p.y + sz.y / 2);
    dl->AddLine(m + ImVec2(4 * S, -5 * S), m + ImVec2(-1 * S, 0), c, 1.6f * S);
    dl->AddLine(m + ImVec2(-1 * S, 0), m + ImVec2(4 * S, 5 * S), c, 1.6f * S);
    tracked(dl, F.uiCaps, F.uiCaps->FontSize, ImVec2(p.x + 20 * S, p.y + (sz.y - F.uiCaps->FontSize) / 2), c, "BACK", 2 * S);
    return r;
}

/** The heading of a page of the launcher. */
static void pageTitle(const char* t) {
    using namespace skin;
    ImVec2 p = ImGui::GetCursorScreenPos();
    std::string u = upper(t);
    tracked(ImGui::GetWindowDrawList(), F.uiTitle, F.uiTitle->FontSize, p + ImVec2(2 * scale, 0), rgb(K_BRASS_HI), u.c_str(), 1.5f * scale);
    ImGui::Dummy(ImVec2(1, F.uiTitle->FontSize + 6 * scale));
}

void App::launchNew() {
    if (backLink(S)) { page = 0; return; }
    pageTitle("New career");
    float w = ImGui::GetContentRegionAvail().x, LW = 150 * S, FW = w - LW - 10 * S;
    ImGui::BeginChild("##newform", ImVec2(w, ImGui::GetContentRegionAvail().y - 48 * S), 0, ImGuiWindowFlags_NoBackground);
    auto row = [&](const char* label) {
        ImGui::Dummy(ImVec2(1, 4 * S));
        ImGui::AlignTextToFramePadding();
        ImGui::PushFont(F.label);
        ImGui::TextColored(rgb4(skin::K_BRASS), "%s", label);
        ImGui::PopFont();
        ImGui::SameLine(LW);
    };
    row("Surname");
    uiText("##name", newName, ImVec2(220 * S, 0));
    ImGui::SameLine();
    if (uiBtn("Another")) newName = RU_LAST[(size_t)(ImGui::GetTime() * 1000) % RU_LAST.size()];
    row("Play");
    {
        std::vector<Opt> o;
        for (int i = 0; i < N_SCENARIO; i++) o.push_back({i == 0 ? std::string("A full career, 1968-1991") : std::string("Scenario: ") + SCENARIOS[i].name, i});
        uiCombo("##scenario", newScenario, o, FW);
        if (newScenario > 0) {
            ImGui::SetCursorPosX(LW);
            ImGui::BeginGroup(); ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + FW);
            hint(std::string(SCENARIOS[newScenario].blurb) + "  A few months, one operation, a score out of 100.");
            ImGui::PopTextWrapPos(); ImGui::EndGroup();
        }
    }
    if (newScenario == 0) {
        row("Begin in");
        std::vector<Opt> o;
        for (int th : STARTS) o.push_back({startLabel(th), th});
        uiCombo("##start", newStart, o, FW);
        ImGui::SetCursorPosX(LW);
        ImGui::BeginGroup(); ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + FW); hint(startText(newStart)); ImGui::PopTextWrapPos(); ImGui::EndGroup();
    }
    row("Background");
    {
        std::vector<Opt> o;
        for (int i = 0; i < 4; i++) o.push_back({BACKGROUND_LABEL[i], i});
        uiCombo("##background", newBackground, o, FW);
        ImGui::SetCursorPosX(LW);
        ImGui::BeginGroup(); hint(BACKGROUND_TEXT[newBackground]); ImGui::EndGroup();
    }
    row("Opposition");
    for (int i = 0; i < 3; i++) {
        if (i) ImGui::SameLine();
        ImGui::RadioButton(DIFF_LABEL[i], &newDifficulty, i);
        uiRec(DIFF_LABEL[i]);
    }
    ImGui::SetCursorPosX(LW);
    ImGui::BeginGroup(); hint(DIFF_TEXT[newDifficulty]); ImGui::EndGroup();
    row("World number");
    uiText("##seed", newSeed, ImVec2(120 * S, 0), false, "random");
    {
        std::string digits;
        for (char c : newSeed) if (isdigit((unsigned char)c)) digits += c;
        if (digits.size() > 9) digits.resize(9);
        newSeed = digits;
    }
    ImGui::SameLine();
    hint("The same number gives the same people, places and Rezidents.");
    row("Rules");
    uiCheck("Ironman", &newIronman);
    ImGui::SameLine();
    hint("One career, one file: no named saves, no quick load.");
    ImGui::EndChild();
    // begin
    float bw = 200 * S;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + w - bw);
    if (uiPrimary(newScenario ? "Begin scenario" : "Begin career", ImVec2(bw, 38 * S))) {
        uint64_t seed = (uint64_t)strtoull(newSeed.c_str(), nullptr, 10);
        if (seed == 0) seed = (uint64_t)(ImGui::GetTime() * 1000003.0) % 1000000 + (uint64_t)ImGui::GetFrameCount() % 997 + 1;
        Sim::CareerOpts o;
        o.start = newStart; o.background = newBackground; o.difficulty = newDifficulty; o.ironman = newIronman; o.scenario = newScenario;
        sim.newCareer(seed, newName.empty() ? std::string("Sokolov") : newName, o);
        inGame = true;
        page = 0;
        resetSelection();
        forgetTextures("p");
        wantTab = newScenario ? T_PLAN : newStart == TH_MOSCOW ? T_PLAN : T_CENTRE;
        autosave();
    }
}

void App::launchLoad() {
    if (backLink(S)) { page = 0; return; }
    pageTitle("Load a saved game");
    if (loadList.empty()) {
        std::error_code ec;
        std::vector<std::pair<std::filesystem::file_time_type, std::string>> files;
        for (auto& e : std::filesystem::directory_iterator(std::filesystem::u8path(saveDir), ec))
            if (e.path().extension() == ".rez") files.push_back({e.last_write_time(ec), e.path().filename().u8string()});
        std::sort(files.begin(), files.end(), [](auto& x, auto& y) { return x.first > y.first; });      // newest first
        loadInfo.clear();
        for (auto& f : files) {
            loadList.push_back(f.second);
            std::ifstream in(std::filesystem::u8path(saveDir + "/" + f.second), std::ios::binary);
            std::vector<uint8_t> buf((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            Sim t;
            if (!t.load(buf)) loadInfo.push_back("cannot be read by this version");
            else if (t.g.phase == PH_OVER) loadInfo.push_back(t.g.officer + "  -  the career is over");
            else loadInfo.push_back(std::string(THEATRES[t.g.theatre].training ? "Senior Lieutenant" : RANK[t.g.rank]) + " " + t.g.officer + "  -  " + THEATRES[t.g.theatre].city + "  -  " + fmtDay(t.g.day));
        }
        if (loadList.empty()) loadList.push_back("");
    }
    if (loadList.size() == 1 && loadList[0].empty()) { hint("There are no saved games yet. The game saves itself every night; named copies are made with Save, or F5."); return; }
    ImGui::BeginChild("##files", ImVec2(0, 0), 0, ImGuiWindowFlags_NoBackground);
    float w = ImGui::GetContentRegionAvail().x;
    for (size_t i = 0; i < loadList.size(); i++) {
        std::string name = loadList[i].substr(0, loadList[i].size() - 4);
        if (name == "autosave") name = "Autosave (last night)";
        else if (name == "quicksave") name = "Quick save";
        else if (name.rfind("autosave-month-", 0) == 0) name = "Backup from the end of a month (" + name.substr(15) + ")";
        if (menuRow(("file:" + loadList[i]).c_str(), name.c_str(), i < loadInfo.size() ? loadInfo[i].c_str() : "", w, S, true, skin::IC_SAVE)) {
            if (loadFrom(saveDir + "/" + loadList[i])) page = 0;
            else note = loadList[i] + " could not be read (it may be from an older version of the game).";
        }
    }
    ImGui::EndChild();
}

void App::launchRecords() {
    if (backLink(S)) { page = 0; return; }
    pageTitle("Service records");
    if (!archiveRead) readArchive();
    ImGui::BeginChild("##records", ImVec2(0, 0), 0, ImGuiWindowFlags_NoBackground);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float w = ImGui::GetContentRegionAvail().x;
    auto got = citationsEarned();
    // endings found
    std::vector<bool> seen(N_ENDING, false);
    for (auto& r : archive) for (int i = 0; i < N_ENDING; i++) if (r.ending == ENDINGS[i].key) seen[i] = true;
    int nEnd = 0, nCit = 0;
    for (bool x : seen) nEnd += x;
    for (bool x : got) nCit += x;
    caption(fstr("CITATIONS  -  %d OF %d", nCit, N_CITATION).c_str(), S);
    {
        float cw = (w - 8 * S) / 2, ch = 44 * S;
        for (int i = 0; i < N_CITATION; i++) {
            if (i % 2) ImGui::SameLine(0, 8 * S);
            ImVec2 p = ImGui::GetCursorScreenPos();
            ImGui::Dummy(ImVec2(cw, ch));
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", CITATIONS[i].text);
            ImVec2 q = p + ImVec2(cw, ch - 4 * S);
            dl->AddRectFilledMultiColor(p, q, rgb(got[i] ? 0x352A1F : 0x221D18), rgb(got[i] ? 0x352A1F : 0x221D18), rgb(got[i] ? 0x271F18 : 0x1A1612), rgb(got[i] ? 0x271F18 : 0x1A1612));
            dl->AddRect(p, q, rgb(got[i] ? skin::K_BRASS : skin::K_BRASS_LO, got[i] ? 0.9f : 0.5f), 0, 0, 1);
            ImVec2 c = p + ImVec2(20 * S, (ch - 4 * S) / 2);
            if (got[i]) { star(dl, c, 10 * S, rgb(0xD8452C), 0, 0, 0); dl->AddCircle(c, 13 * S, rgb(skin::K_BRASS), 24, 1.2f * S); }
            else dl->AddCircle(c, 9 * S, rgb(skin::K_FAINT), 24, 1.2f * S);
            dl->AddText(F.uiBold, F.uiBold->FontSize * 0.92f, p + ImVec2(40 * S, 4 * S), rgb(got[i] ? skin::K_BRASS_HI : skin::K_DIM), CITATIONS[i].title);
            dl->AddText(F.uiSmall, F.uiSmall->FontSize * 0.9f, p + ImVec2(40 * S, 22 * S), rgb(got[i] ? skin::K_TEXT : skin::K_FAINT), CITATIONS[i].text);
        }
    }
    ImGui::Dummy(ImVec2(1, 8 * S));
    caption(fstr("ENDINGS FOUND  -  %d OF %d", nEnd, N_ENDING).c_str(), S);
    for (int i = 0; i < N_ENDING; i++) {
        ImGui::PushFont(seen[i] ? F.uiMed : F.ui);
        ImVec2 sp = ImGui::GetCursorScreenPos() + ImVec2(8 * S, ImGui::GetTextLineHeight() / 2);
        if (seen[i]) star(dl, sp, 7 * S, rgb(0xD8452C), 0, 0, 0);
        else dl->AddCircle(sp, 4 * S, rgb(skin::K_FAINT), 12, 1);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 24 * S);
        ImGui::TextColored(seen[i] ? rgb4(skin::K_TEXT) : rgb4(skin::K_FAINT), "%s", seen[i] ? ENDINGS[i].title : "(not yet found)");
        ImGui::PopFont();
    }
    ImGui::Dummy(ImVec2(1, 8 * S));
    caption(fstr("CAREERS  -  %d", (int)archive.size()).c_str(), S);
    if (archive.empty()) hint("When a career ends - well or badly - its record is entered here.");
    for (int i = (int)archive.size() - 1; i >= 0; i--) {
        const Record& r = archive[i];
        std::string end;
        for (int k = 0; k < N_ENDING; k++) if (r.ending == ENDINGS[k].key) end = ENDINGS[k].title;
        ImGui::PushFont(F.label);
        ImGui::TextColored(rgb4(skin::K_BRASS_HI), "%s", (r.rank + " " + r.officer + "  \xC2\xB7  " + end).c_str());
        ImGui::PopFont();
        hint(std::string(BACKGROUND_LABEL[std::max(0, std::min(3, r.background))]) + ", from " + startLabel(r.start) + ". " + DIFF_LABEL[std::max(0, std::min(2, r.difficulty))]
             + (r.ironman ? ", ironman" : "") + ", world " + r.seed + ". Ended " + r.ended + ".");
        hint("Postings: " + (r.postings.empty() ? std::string("none completed") : r.postings) + fstr(". Agents recruited: %d. Decorations: %d.", r.recruited, r.awards));
        ImGui::Dummy(ImVec2(1, 6 * S));
    }
    ImGui::EndChild();
}
