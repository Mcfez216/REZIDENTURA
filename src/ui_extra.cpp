// ui_extra.cpp - the conveniences:
//   screens lifted off the desk into windows of their own (which can be docked together, or dragged to another monitor);
//   the settings; the keys; the checklist and the notebook; the handbook with its contents and a search; small popups.
#include "ui.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <cfloat>
#include <cmath>
#include <filesystem>
#include <fstream>

using namespace gfx;

/** A heading in a typed document: a line in capitals. */
static bool heading(const std::string& l) {
    if (l.size() < 3 || l[0] == ' ') return false;
    bool letter = false;
    for (char c : l) { if (islower((unsigned char)c)) return false; if (isupper((unsigned char)c)) letter = true; }
    return letter;
}

extern const char* const TAB_RU[App::N_TAB];
extern const char* const TAB_EN[App::N_TAB];

// ============================================================================ settings, kept beside the saved games
void App::loadSettings() {
    std::ifstream f(std::filesystem::u8path(saveDir + "/settings.ini"));
    std::string line;
    while (f && std::getline(f, line)) {
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string k = line.substr(0, eq), v = line.substr(eq + 1);
        while (!v.empty() && (v.back() == '\r' || v.back() == ' ')) v.pop_back();
        auto b = [&]() { return v == "1"; };
        if (k == "multiMonitor") cfg.multiMonitor = b();
        else if (k == "rememberWindows") cfg.rememberWindows = b();
        else if (k == "uiScale") cfg.uiScale = std::max(0.7f, std::min(1.6f, (float)atof(v.c_str())));
        else if (k == "fullscreen") cfg.fullscreen = b();
        else if (k == "monthlyBackups") cfg.monthlyBackups = b();
        else if (k == "runStopMeeting") cfg.runStopMeeting = b();
        else if (k == "runStopCable") cfg.runStopCable = b();
        else if (k == "runStopAlert") cfg.runStopAlert = b();
        else if (k == "runMaxDays") cfg.runMaxDays = std::max(2, std::min(90, atoi(v.c_str())));
        else if (k == "jumpAfterDay") cfg.jumpAfterDay = b();
        else if (k == "tooltips") cfg.tooltips = b();
        else if (k == "confirmEndDay") cfg.confirmEndDay = b();
        else if (k == "toasts") cfg.toasts = b();
        else if (k == "popped" && cfg.rememberWindows) for (int i = 0; i < N_TAB && i < (int)v.size(); i++) popped[i] = v[(size_t)i] == '1';
    }
}

void App::saveSettings() {
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::u8path(saveDir), ec);
    std::ofstream f(std::filesystem::u8path(saveDir + "/settings.ini"));
    if (!f) return;
    std::string pop;
    for (int i = 0; i < N_TAB; i++) pop += popped[i] ? '1' : '0';
    f << "multiMonitor=" << cfg.multiMonitor << "\nrememberWindows=" << cfg.rememberWindows << "\nuiScale=" << cfg.uiScale << "\nfullscreen=" << cfg.fullscreen
      << "\nmonthlyBackups=" << cfg.monthlyBackups << "\nrunStopMeeting=" << cfg.runStopMeeting << "\nrunStopCable=" << cfg.runStopCable << "\nrunStopAlert=" << cfg.runStopAlert
      << "\nrunMaxDays=" << cfg.runMaxDays << "\njumpAfterDay=" << cfg.jumpAfterDay << "\ntooltips=" << cfg.tooltips << "\nconfirmEndDay=" << cfg.confirmEndDay << "\ntoasts=" << cfg.toasts << "\npopped=" << pop << "\n";
}

/** Keep the autosaves of the last three months: autosave-1 is the newest. */
void App::rotateBackups() {
    if (sim.g.ironman) return;                // one career, one file
    std::error_code ec;
    auto path = [&](int n) { return std::filesystem::u8path(saveDir + fstr("/autosave-month-%d.rez", n)); };
    std::filesystem::remove(path(3), ec);
    std::filesystem::rename(path(2), path(3), ec);
    std::filesystem::rename(path(1), path(2), ec);
    std::filesystem::copy_file(std::filesystem::u8path(saveDir + "/autosave.rez"), path(1), std::filesystem::copy_options::overwrite_existing, ec);
}

// ============================================================================ windows lifted off the desk
void App::popOut(int t, bool out) {
    if (t < 0 || t >= N_TAB) return;
    popped[t] = out;
    if (out) focusTab = t;
    else if (!popped[tab]) {}
    saveSettings();
}

/** Every screen back on the desk; the windows keep their places for next time. */
void App::closeWindows() {
    for (bool& p : popped) p = false;
    saveSettings();
}

/** Every screen back on the desk, and every remembered position forgotten. */
void App::resetWindows() {
    for (int i = 0; i < N_TAB; i++) { popped[i] = false; resetPending[i] = true; }
    ImGui::ClearIniSettings();
    tab = T_PLAN;
    saveSettings();
}

void App::poppedWindows() {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    int shown = 0;
    for (int t = 0; t < N_TAB; t++) {
        if (!popped[t]) continue;
        ImVec2 size(std::min(1180 * S, vp->WorkSize.x * 0.86f), std::min(800 * S, vp->WorkSize.y * 0.84f));
        ImVec2 pos = vp->WorkPos + ImVec2((230 + 34 * shown) * S, (118 + 30 * shown) * S);
        ImGuiCond when = resetPending[t] ? ImGuiCond_Always : ImGuiCond_FirstUseEver;
        ImGui::SetNextWindowPos(pos, when);
        ImGui::SetNextWindowSize(size, when);
        if (resetPending[t]) ImGui::SetNextWindowDockID(0, ImGuiCond_Always);
        resetPending[t] = false;
        ImGui::SetNextWindowSizeConstraints(ImVec2(640 * S, 420 * S), ImVec2(FLT_MAX, FLT_MAX));
        // (a focus request would shut whatever dialogue is open - the settings, for one)
        if (focusTab == t) { if (!ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel)) ImGui::SetNextWindowFocus(); focusTab = -1; }
        ImGui::PushStyleColor(ImGuiCol_WindowBg, rgb4(skin::K_DESK));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10 * S, 8 * S));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
        bool open = true;
        std::string title = std::string(TAB_EN[t]) + "###win:" + TAB_EN[t];
        ImGui::PushFont(F.bar);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(14 * S, std::floor((skin::titleBarH(S) - F.bar->FontSize) / 2)));
        bool vis = ImGui::Begin(title.c_str(), &open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::PopStyleVar();
        float fs = ImGui::GetFontSize(), padY = std::floor((skin::titleBarH(S) - F.bar->FontSize) / 2);
        ImGui::PopFont();
        if (vis) {
            ImGuiWindow* w = ImGui::GetCurrentWindow();
            skin::desk(w->DrawList, w->InnerRect.Min, w->InnerRect.Max, S);
            // the game's own title bar over ImGui's (a window docked with others shows tabs instead)
            if (!w->DockIsActive) {
                ImRect tr = w->TitleBarRect();
                ImDrawList* dl = w->DrawList;
                dl->PushClipRect(w->Pos, w->Pos + w->Size, false);
                dl->AddRectFilled(tr.Min, tr.Max, rgb(skin::K_BAR));
                skin::titleBar(dl, tr.Min, tr.GetWidth(), S, TAB_EN[t], "a window of its own - close it to put it back on the desk");
                ImVec2 cc(tr.Max.x - 14 * S - fs * 0.5f, tr.Min.y + padY + fs * 0.5f);
                bool ch = ImGui::IsMouseHoveringRect(cc - ImVec2(fs * 0.5f, fs * 0.5f), cc + ImVec2(fs * 0.5f, fs * 0.5f));
                skin::icon(dl, skin::IC_CLOSE, cc, 18 * S, rgb(ch ? skin::K_BRASS_HI : skin::K_BRASS));
                skin::trim(dl, w->Pos, w->Pos + w->Size, S);
                dl->PopClipRect();
            }
            ImGui::BeginChild("##screen", ImVec2(0, 0), 0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            drawTab(t);
            ImGui::EndChild();
        }
        ImGui::End();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
        if (!open) popOut(t, false);
        shown++;
    }
    if (focusTab >= 0 && !popped[focusTab]) focusTab = -1;
    // A question on the table stays on top: a window (or a frame of windows) that has only just appeared goes behind it.
    ImGuiContext& g = *GImGui;
    ImGuiWindow* modal = nullptr;
    for (int i = 0; i < g.OpenPopupStack.Size && !modal; i++)
        if (ImGuiWindow* w = g.OpenPopupStack[i].Window) if (w->Flags & ImGuiWindowFlags_Modal) modal = w;
    if (modal && modal->WasActive)
        for (int t = 0; t < N_TAB; t++) {
            if (!popped[t]) continue;
            ImGuiWindow* w = ImGui::FindWindowByName((std::string("###win:") + TAB_EN[t]).c_str());
            if (!w) continue;
            w = w->RootWindowDockTree ? w->RootWindowDockTree : w;
            if (ImGui::FindWindowDisplayIndex(w) > ImGui::FindWindowDisplayIndex(modal)) ImGui::BringWindowToDisplayBehind(w, modal);
        }
}

// ============================================================================ keys
void App::shortcuts() {
    ImGuiIO& io = ImGui::GetIO();
    if (ImGui::IsKeyPressed(ImGuiKey_F10, false)) openSettings = true;
    if (io.WantTextInput || ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel)) return;
    if (sim.g.phase != PH_PLAY) return;
    bool enter = ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false);
    if (io.KeyCtrl && enter) { doEndDay(io.KeyShift); return; }
    bool iron = sim.g.ironman;
    if ((io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false)) || (iron && (ImGui::IsKeyPressed(ImGuiKey_F5, false) || ImGui::IsKeyPressed(ImGuiKey_F9, false)))) {
        if (iron) { note = "An ironman career keeps one file, saved every night. There are no copies and no going back."; return; }
        openSave = true;
        return;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_F5, false)) {
        lastQuick = fmtDay(sim.g.day);
        note = saveTo(saveDir + "/quicksave.rez") ? "Quick save written (" + lastQuick + "). F9 returns to it." : std::string("The quick save could not be written.");
    }
    if (ImGui::IsKeyPressed(ImGuiKey_F9, false)) {
        std::error_code ec;
        if (!std::filesystem::exists(std::filesystem::u8path(saveDir + "/quicksave.rez"), ec)) note = "There is no quick save yet. F5 makes one.";
        else {
            confirmText = "Go back to the quick save? Everything since is lost.";
            confirmAction = [this]() { if (!loadFrom(saveDir + "/quicksave.rez")) note = "The quick save could not be read."; };
        }
    }
    if (ImGui::IsKeyPressed(ImGuiKey_F1, false)) wantTab = T_MANUAL;
    if (ImGui::IsKeyPressed(ImGuiKey_F2, false)) wantTab = T_NOTES;
    if (!io.KeyCtrl && !io.KeyAlt && !io.KeyShift) {
        static const ImGuiKey DIG[] = {ImGuiKey_1, ImGuiKey_2, ImGuiKey_3, ImGuiKey_4, ImGuiKey_5, ImGuiKey_6, ImGuiKey_7, ImGuiKey_8, ImGuiKey_9, ImGuiKey_0};
        for (int i = 0; i < 10 && i < N_TAB; i++) if (ImGui::IsKeyPressed(DIG[i], false)) wantTab = i;
    }
}

// ============================================================================ the settings
void App::settingsPopup(ImVec2 centre) {
    if (openSettings) { ImGui::OpenPopup("Settings"); openSettings = false; }
    ImGui::SetNextWindowPos(centre, ImGuiCond_Always, ImVec2(0.5f, 0.5f));      // (its size settles over the first frames: keep it centred)

    bool keep = true;
    if (!uiModal("Settings", &keep, ImGuiWindowFlags_AlwaysAutoResize)) return;
    bool changed = false;
    auto head = [&](const char* ru, const char* en) { (void)ru; uiSection(en); };
    auto hint = [&](const char* s) { ImGui::PushFont(F.uiSmall); ImGui::PushStyleColor(ImGuiCol_Text, rgb4(C_INK_FADED)); ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 760 * S); ImGui::TextUnformatted(s); ImGui::PopTextWrapPos(); ImGui::PopStyleColor(); ImGui::PopFont(); };

    // never taller than the window it opens in (the launcher is small): the body scrolls, the buttons stay in view
    {
        float vh = inGame ? ImGui::GetMainViewport()->WorkSize.y : launcherSize().y;
        ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, std::max(160.f, vh - 150 * S)));
        ImGui::BeginChild("##setbody", ImVec2(890 * S, 0), ImGuiChildFlags_AutoResizeY);
    }
    head("ОКНА", "Windows");
    hint("Any screen can be opened in a window of its own: right-click its name in the list on the left, or click the small mark at the right of the name. "
         "Drag a window by its title bar - to another monitor if you have one. Drop one window on another to make them share a frame, side by side or as tabs. "
         "Close a window and the screen goes back on the desk.");
    int out = 0;
    for (bool p : popped) if (p) out++;
    if (uiBtn("Reset windows", ImVec2(190 * S, 0))) { resetWindows(); note = "Every screen is back on the desk, and the remembered positions are forgotten."; }
    if (ImGui::IsItemHovered() && cfg.tooltips) ImGui::SetTooltip("Every screen back on the desk, and every remembered window position forgotten.\nUse it if a window has got lost off the edge of a screen.");
    ImGui::SameLine();
    if (uiBtn("Close all windows", ImVec2(190 * S, 0))) closeWindows();
    if (ImGui::IsItemHovered() && cfg.tooltips) ImGui::SetTooltip("Every screen back on the desk. Their places are remembered for next time.");
    ImGui::SameLine();
    ImGui::TextDisabled("%s", out == 0 ? "No screen is in a window of its own." : fstr("%d screen%s in windows of their own.", out, out == 1 ? " is" : "s are").c_str());
    if (inGame && sim.g.phase == PH_PLAY) {
        ImGui::PushFont(F.ui);
        for (int i = 0; i < N_TAB; i++) {
            bool v = popped[i];
            if (ImGui::Checkbox((std::string(TAB_EN[i]) + "##pw").c_str(), &v)) popOut(i, v);
            uiRec(std::string("setpop:") + TAB_EN[i]);
            if (i % 5 != 4 && i + 1 < N_TAB) ImGui::SameLine((float)((i % 5) + 1) * 172 * S);
        }
        ImGui::PopFont();
    }
    ImGui::BeginDisabled(!viewportsOk);
    changed |= uiCheck("Windows may leave the main window (for a second monitor)", &cfg.multiMonitor);
    ImGui::EndDisabled();
    changed |= uiCheck("Remember where the windows were, between sessions", &cfg.rememberWindows);
    changed |= uiCheck("Full screen  (F11)", &cfg.fullscreen);

    head("ВИД", "Display");
    ImGui::SetNextItemWidth(260 * S);
    int pc = (int)std::lround(cfg.uiScale * 100);
    if (ImGui::SliderInt("Size of everything", &pc, 70, 160, "%d%%")) { cfg.uiScale = pc / 100.f; changed = true; }
    ImGui::SameLine(); ImGui::TextDisabled("(from the next start)");
    changed |= uiCheck("Explanations when the mouse rests on things", &cfg.tooltips);

    head("ХОД ИГРЫ", "The game");
    ImGui::TextUnformatted("'Skip ahead' stops:");
    ImGui::BeginDisabled(); bool yes = true; ImGui::Checkbox("when a decision is asked of you", &yes); ImGui::EndDisabled();
    changed |= uiCheck("on the morning of a meeting with one of your own agents", &cfg.runStopMeeting);
    changed |= uiCheck("when something important happens (marked *** in the report)", &cfg.runStopAlert);
    changed |= uiCheck("when a new cable arrives", &cfg.runStopCable);
    ImGui::SetNextItemWidth(260 * S);
    changed |= ImGui::SliderInt("and in any case after (days)", &cfg.runMaxDays, 2, 90);
    changed |= uiCheck("After End Day, turn to the cables or the report by itself", &cfg.jumpAfterDay);
    changed |= uiCheck("Ask before ending a day while something urgent is undone (see Checklist)", &cfg.confirmEndDay);
    changed |= uiCheck("Show a short notice of what happened after each day", &cfg.toasts);
    changed |= uiCheck("Keep the autosaves of the last three months as well (autosave-month-1 ... 3)", &cfg.monthlyBackups);

    head("КЛАВИШИ", "Keys");
    ImGui::PushFont(F.ui);
    static const char* const KEYS[][2] = {{"Ctrl+Enter", "End the day"}, {"Ctrl+Shift+Enter", "Skip ahead"}, {"1 ... 9, 0", "The first ten screens, in the order of the list"}, {"F1", "The handbook"}, {"F2", "Checklist and notebook"},
                                          {"F5 / F9", "Quick save / back to the quick save"}, {"Ctrl+S", "Save a named copy"}, {"F10", "These settings"}, {"F11", "Full screen"},
                                          {"Right-click a name", "Open that screen in its own window, or put it back"}};
    if (ImGui::BeginTable("keys", 4, ImGuiTableFlags_SizingFixedFit)) {
        for (size_t i = 0; i < sizeof KEYS / sizeof KEYS[0]; i++) {
            ImGui::TableNextColumn(); ImGui::TextColored(rgb4(C_RED), "%s", KEYS[i][0]);
            ImGui::TableNextColumn(); ImGui::Text("%s      ", KEYS[i][1]);
        }
        ImGui::EndTable();
    }
    ImGui::PopFont();
    ImGui::EndChild();

    ImGui::Dummy(ImVec2(1, 6 * S));
    ImGui::Separator();
    if (uiBtn("Close##settings", ImVec2(120 * S, 0))) ImGui::CloseCurrentPopup();
    if (inGame) {
        ImGui::SameLine();
        if (uiBtn("Save and go to the title screen", ImVec2(300 * S, 0))) { if (sim.g.phase == PH_PLAY) autosave(); inGame = false; ImGui::CloseCurrentPopup(); }
    }
    if (!pdfPath.empty() && openFile) {
        ImGui::SameLine();
        if (uiBtn("Open the handbook (PDF)", ImVec2(250 * S, 0))) openFile(pdfPath);
    }
    if (changed) saveSettings();
    uiEndModal();
}

// ============================================================================ small popups: a codename, a pencilled note
void App::extraPopups(ImVec2 centre) {
    settingsPopup(centre);
    if (renamePerson >= 0 && !ImGui::IsPopupOpen("A codename")) ImGui::OpenPopup("A codename");
    ImGui::SetNextWindowPos(centre, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (uiModal("A codename", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        Person* p = sim.person(renamePerson);
        ImGui::TextUnformatted(p ? ("A new working name for " + p->full() + ":").c_str() : "?");
        if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
        uiText("##codename", renameBuf, ImVec2(260 * S, 0));
        ImGui::TextDisabled("One word, two to fourteen letters. It is how he appears everywhere from now on.");
        if (uiBtn("Rename", ImVec2(110 * S, 0))) { if (p) note = sim.renameAgent(*p, renameBuf); renamePerson = -1; ImGui::CloseCurrentPopup(); }
        ImGui::SameLine();
        if (uiBtn("Cancel##rn", ImVec2(110 * S, 0))) { renamePerson = -1; ImGui::CloseCurrentPopup(); }
        uiEndModal();
    }
    if (notePerson >= 0 && !ImGui::IsPopupOpen("A note on the file")) ImGui::OpenPopup("A note on the file");
    ImGui::SetNextWindowPos(centre, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (uiModal("A note on the file", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        Person* p = sim.person(notePerson);
        ImGui::TextUnformatted(p ? ("Your own note on " + p->full() + " (nobody reads it but you):").c_str() : "?");
        if (p) {
            if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
            uiText("##pnote", p->note, ImVec2(560 * S, 150 * S), true);
        }
        if (uiBtn("Done", ImVec2(110 * S, 0))) { notePerson = -1; ImGui::CloseCurrentPopup(); }
        uiEndModal();
    }
}

// ============================================================================ checklist and notebook
void App::tabNotes() {
    GameState& g = sim.g;
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float leftW = std::floor(avail.x * 0.56f);
    static const int TO_TAB[] = {T_PLAN, T_PEOPLE, T_SITES, T_MAT, T_CENTRE, T_STATION, T_ACC, T_HOME};
    if (docPanel("todoDoc", ImVec2(leftW, avail.y), C_CARD, 18, "Checklist", fmtDay(g.day).c_str())) {
        std::vector<Sim::Todo> v = sim.todo();
        if (v.empty()) uiTyped("Nothing presses. Which is when a careful officer cases a site, or walks a route, or goes home.", rgb(C_INK_FADED), F.body);
        int last = 3, n = 0;
        for (auto& t : v) {
            if (t.level != last) {
                last = t.level;
                ImGui::Dummy(ImVec2(1, 5 * S));
                ImGui::PushFont(F.uiCaps);
                ImGui::PushStyleColor(ImGuiCol_Text, rgb4(t.level == 2 ? C_RED : C_INK));
                ImGui::TextUnformatted(t.level == 2 ? "TODAY" : t.level == 1 ? "SOON" : "WORTH KNOWING");
                ImGui::PopStyleColor();
                ImGui::PopFont();
            }
            ImGui::PushID(n++);
            ImGui::PushStyleColor(ImGuiCol_Text, rgb4(t.level == 2 ? C_RED : C_INK));
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - 6 * S);
            ImVec2 p0 = ImGui::GetCursorScreenPos();
            ImVec2 ts = ImGui::CalcTextSize(t.text.c_str(), nullptr, false, ImGui::GetContentRegionAvail().x - 28 * S);
            if (ImGui::Selectable("##go", false, 0, ImVec2(0, ts.y))) wantTab = TO_TAB[std::max(0, std::min(7, t.tab))];
            uiRec(fstr("todo:%d", n - 1));
            if (ImGui::IsItemHovered() && cfg.tooltips) ImGui::SetTooltip("Go to %s", TAB_EN[TO_TAB[std::max(0, std::min(7, t.tab))]]);
            ImDrawList* dl = ImGui::GetWindowDrawList();
            dl->AddRect(p0 + ImVec2(2 * S, 3 * S), p0 + ImVec2(13 * S, 14 * S), rgb(t.level == 2 ? C_RED : C_INK_FADED), 0, 0, 1.2f * S);
            dl->AddText(nullptr, 0, p0 + ImVec2(22 * S, 0), rgb(t.level == 2 ? C_RED : C_INK), t.text.c_str(), nullptr, ImGui::GetContentRegionAvail().x - 28 * S);
            ImGui::PopTextWrapPos();
            ImGui::PopStyleColor();
            ImGui::PopID();
        }
    }
    docEnd();
    ImGui::SameLine(0, 16 * S);
    if (docPanel("noteDoc", ImVec2(ImGui::GetContentRegionAvail().x, avail.y), 0xF7F2DF, 18, "Notebook", "Kept with the saved game")) {
        ImVec2 sz = ImGui::GetContentRegionAvail();
        ImVec2 p0 = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float lh = ImGui::GetTextLineHeight();
        for (float y = p0.y + lh + 4 * S; y < p0.y + sz.y - 4 * S; y += lh) dl->AddLine(ImVec2(p0.x, y), ImVec2(p0.x + sz.x, y), rgb(0x7A9CC6, 0.28f), 1);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, rgb4(0, 0));
        ImGui::PushStyleColor(ImGuiCol_Text, rgb4(C_PENCIL_BLUE));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
        uiText("##notebook", g.notes, sz, true);
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);
        if (g.notes.empty() && !ImGui::IsItemActive())
            dl->AddText(p0 + ImVec2(10 * S, 5 * S), rgb(C_INK_FADED, 0.7f), "Click here and write: who is due what, what you suspect,\nwhich site you have used too often...");
    }
    docEnd();
}

// ============================================================================ the handbook: contents, a search, and the PDF
namespace {
struct Section { std::string title; size_t a, b; };
std::vector<Section> sections;
size_t sectionsFor = 0;
bool rule(const std::string& l) { return l.size() >= 20 && l.find_first_not_of('=') == std::string::npos; }
void parseSections(const std::string& m) {
    sections.clear();
    sectionsFor = m.size();
    std::vector<std::pair<size_t, size_t>> lines;
    for (size_t a = 0; a < m.size();) { size_t b = m.find('\n', a); if (b == std::string::npos) b = m.size(); lines.push_back({a, b}); a = b + 1; }
    auto text = [&](size_t i) { std::string l = m.substr(lines[i].first, lines[i].second - lines[i].first); while (!l.empty() && l.back() == '\r') l.pop_back(); return l; };
    size_t start = 0;
    std::string title = "Title page";
    for (size_t i = 0; i + 2 < lines.size(); i++) {
        if (!rule(text(i)) || !rule(text(i + 2)) || text(i + 1).empty()) continue;
        if (lines[i].first > start) sections.push_back({title, start, lines[i].first});
        title = text(i + 1);
        start = lines[i + 2].second + 1 <= m.size() ? lines[i + 2].second + 1 : m.size();
        i += 2;
    }
    if (start < m.size()) sections.push_back({title, start, m.size()});
}
std::string lower(std::string s) { for (auto& c : s) c = (char)tolower((unsigned char)c); return s; }
}

void App::tabManual() {
    if (sectionsFor != manual.size()) parseSections(manual);
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float sideW = std::min(330 * S, avail.x * 0.34f);
    std::string find = lower(manualFind);
    if (docPanel("manualIdx", ImVec2(sideW, avail.y), C_CARD, 12, "Contents", nullptr)) {
        uiText("##find", manualFind, ImVec2(ImGui::GetContentRegionAvail().x, 0), false, "find a word...");
        float foot = (!pdfPath.empty() && openFile) ? ImGui::GetFrameHeightWithSpacing() + 4 * S : 0;
        ImGui::BeginChild("##sections", ImVec2(0, ImGui::GetContentRegionAvail().y - foot), 0, 0);
        ImGui::PushFont(F.ui);
        if (find.empty() && ImGui::Selectable("The whole handbook", manualSection < 0)) manualSection = -1;
        for (int i = 0; i < (int)sections.size(); i++) {
            const Section& sc = sections[(size_t)i];
            if (!find.empty() && lower(manual.substr(sc.a, sc.b - sc.a)).find(find) == std::string::npos && lower(sc.title).find(find) == std::string::npos) continue;
            if (ImGui::Selectable((sc.title + "##s" + std::to_string(i)).c_str(), manualSection == i)) manualSection = i;
            uiRec(fstr("manual:%d", i));
        }
        ImGui::PopFont();
        ImGui::EndChild();
        if (foot > 0 && uiBtn("Open the handbook as a PDF", ImVec2(ImGui::GetContentRegionAvail().x, 0))) openFile(pdfPath);
    }
    docEnd();
    ImGui::SameLine(0, 16 * S);
    float w = std::min(ImGui::GetContentRegionAvail().x, 1000 * S);
    if (docPanel("manual", ImVec2(w, avail.y), C_PAPER, 26, "Officer's Handbook")) {
        bool whole = manualSection < 0 || manualSection >= (int)sections.size();
        docTitle("НАСТАВЛЕНИЕ", whole ? "The Officer's Handbook" : sections[(size_t)manualSection].title.c_str());
        std::string text = whole ? manual : manual.substr(sections[(size_t)manualSection].a, sections[(size_t)manualSection].b - sections[(size_t)manualSection].a);
        uiTyped(text, rgb(C_INK), F.small, [find](const std::string& l, ImU32& c, ImFont*& f) {
            if (heading(l)) { f = F.bold; c = rgb(C_RED); }
            if (!find.empty() && lower(l).find(find) != std::string::npos) c = rgb(C_PENCIL_BLUE);
        }, 61);
    }
    docEnd();
}

// ============================================================================ notices after a day
void App::pushToasts(int fromDay, size_t cablesBefore) {
    if (!cfg.toasts) return;
    std::vector<Toast> add;
    double now = ImGui::GetTime();
    for (int d = fromDay; d < sim.g.day; d++) {
        auto it = sim.g.reports.find(d);
        if (it == sim.g.reports.end()) continue;
        for (auto& l : it->second) {
            size_t k = l.find("*** ");
            if (k == std::string::npos) continue;
            std::string t = l.substr(k + 4);
            if (sim.g.day - fromDay > 1) t = fmtShort(d) + "  " + t;
            add.push_back({t, T_REPORT, now, false});
        }
    }
    for (size_t i = cablesBefore; i < sim.g.cables.size(); i++) {
        const Cable& c = sim.g.cables[i];
        if (c.routine()) continue;
        add.push_back({"Cable from " + c.from + ": " + c.subject, T_CENTRE, now, true});
    }
    if (add.empty()) return;
    toastList.clear();
    int shown = 0, more = 0;
    for (auto& t : add) { if (shown < 4) { toastList.push_back(t); shown++; } else more++; }
    if (more) toastList.push_back({fstr("... and %d more in the report and the cables", more), T_REPORT, now, false});
    for (size_t i = 0; i < toastList.size(); i++) toastList[i].t0 = now + 0.08 * (double)i;      // they come in one after another
}

void App::drawToasts() {
    if (toastList.empty()) return;
    double now = ImGui::GetTime();
    const double LIFE = 9.0, FADE = 0.8;
    float W = 380 * S, gap = 8 * S;
    std::vector<float> hs;
    float total = 0;
    for (auto& t : toastList) {
        float h = std::max(46 * S, F.ui->CalcTextSizeA(F.ui->FontSize, FLT_MAX, W - 66 * S, t.text.c_str()).y + 24 * S);
        hs.push_back(h);
        total += h + gap;
    }
    // a window of its own, so that a click on a notice goes to the notice and not to the desk under it
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + vp->WorkSize.x - 18 * S, vp->WorkPos.y + vp->WorkSize.y - 14 * S), ImGuiCond_Always, ImVec2(1, 1));
    ImGui::SetNextWindowSize(ImVec2(W + 48 * S, total));
    ImGui::SetNextWindowViewport(vp->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::Begin("##toasts", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav
                 | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMove);
    ImGui::PopStyleVar(2);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 o = ImGui::GetWindowPos();
    bool anyHover = ImGui::IsWindowHovered();
    float y = o.y + total;
    int go = -1;
    for (size_t i = 0; i < toastList.size(); i++) {
        Toast& t = toastList[i];
        if (anyHover) t.t0 = std::max(t.t0, now - (LIFE - FADE - 2.0));        // a notice being read stays
        double age = now - t.t0;
        float h = hs[i];
        y -= h + gap;
        float alpha = (float)std::max(0.0, std::min(1.0, std::min(std::max(0.0, age) / 0.25, (LIFE - age) / FADE)));
        float slide = (float)std::max(0.0, 1.0 - std::max(0.0, age) / 0.25) * 40 * S;
        ImVec2 a(o.x + 48 * S + slide, y), b(a.x + W, y + h);
        ImGui::SetCursorScreenPos(a);
        ImGui::PushID((int)i);
        if (ImGui::InvisibleButton("toast", b - a)) go = t.tab;
        uiRec(fstr("toast:%d", (int)i));
        bool hov = ImGui::IsItemHovered();
        ImGui::PopID();
        if (hov) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        // a small dark slip with a brass edge: an icon for where it leads, and the line
        dl->AddRectFilled(a + ImVec2(3 * S, 4 * S), b + ImVec2(3 * S, 4 * S), rgb(0, 0.45f * alpha));
        dl->AddRectFilledMultiColor(a, b, rgb(hov ? 0x3A3026 : 0x2F271F, alpha), rgb(hov ? 0x3A3026 : 0x2F271F, alpha), rgb(0x221C17, alpha), rgb(0x221C17, alpha));
        dl->AddRect(a, b, rgb(skin::K_BRASS_LO, alpha), 0, 0, 1);
        dl->AddRect(a + ImVec2(3 * S, 3 * S), b - ImVec2(3 * S, 3 * S), rgb(skin::K_BRASS, 0.18f * alpha), 0, 0, 1);
        dl->AddRectFilled(a, ImVec2(a.x + 3 * S, b.y), rgb(t.cable ? 0x7A6ED0 : 0xC23A26, alpha));
        ImVec2 ic(a.x + 26 * S, (a.y + b.y) / 2);
        dl->AddCircleFilled(ic, 13 * S, rgb(skin::K_INSET, alpha), 24);
        dl->AddCircle(ic, 13 * S, rgb(skin::K_BRASS_LO, alpha), 24, 1);
        skin::icon(dl, t.cable ? skin::IC_CABLE : skin::IC_REPORT, ic, 15 * S, rgb(skin::K_BRASS_HI, alpha));
        float th = F.ui->CalcTextSizeA(F.ui->FontSize, FLT_MAX, W - 66 * S, t.text.c_str()).y;
        dl->AddText(F.ui, F.ui->FontSize, ImVec2(a.x + 50 * S, (a.y + b.y - th) / 2), rgb(skin::K_TEXT, alpha), t.text.c_str(), nullptr, W - 66 * S);
    }
    ImGui::End();
    if (go >= 0) { wantTab = go; toastList.clear(); return; }
    toastList.erase(std::remove_if(toastList.begin(), toastList.end(), [&](const Toast& t) { return now - t.t0 > LIFE; }), toastList.end());
}
