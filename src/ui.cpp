// ui.cpp - every screen of the game, drawn with Dear ImGui.
// The screens read the game through sim.g and the text builders in sim_text.cpp, and change it
// only by calling Sim's public functions.
#include "ui.h"
#include "imgui.h"
#include <filesystem>
#include <fstream>

bool App::recordRects = false;
std::map<std::string, std::array<float, 4>> App::rects;

// ============================================================================ helpers
static ImVec4 hex(unsigned rgb, float a = 1) { return ImVec4(((rgb >> 16) & 255) / 255.f, ((rgb >> 8) & 255) / 255.f, (rgb & 255) / 255.f, a); }
static const ImVec4 PAPER = hex(0xF4EEDD), DESK = hex(0xDCD6C4), INK = hex(0x1A1A1A), RED = hex(0x8C1E1E), DIM = hex(0x757063), BAR = hex(0x2E2A24);

static void rec(const std::string& key) {
    if (!App::recordRects) return;
    ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
    App::rects[key] = {a.x, a.y, b.x, b.y};
}
static bool Btn(const char* label) { bool r = ImGui::Button(label); rec(label); return r; }
static bool Check(const char* label, bool* v) { bool r = ImGui::Checkbox(label, v); rec(label); return r; }

struct Opt { std::string label; int value; };
/** A drop-down of (text, number) pairs. Returns true when the choice changed. */
static bool combo(const char* label, int& value, const std::vector<Opt>& opts, float width = 0) {
    const char* preview = "";
    for (auto& o : opts) if (o.value == value) preview = o.label.c_str();
    if (width != 0) ImGui::SetNextItemWidth(width);
    bool changed = false;
    bool open = ImGui::BeginCombo(label, preview, ImGuiComboFlags_HeightLarge);
    rec(std::string("combo:") + label);
    if (open) {
        for (size_t i = 0; i < opts.size(); i++) {
            ImGui::PushID((int)i);
            bool sel = opts[i].value == value;
            if (ImGui::Selectable(opts[i].label.c_str(), sel)) { value = opts[i].value; changed = true; }
            rec("opt:" + opts[i].label);
            if (sel) ImGui::SetItemDefaultFocus();
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
    return changed;
}
/** A bordered sheet of paper with wrapped typewriter text on it. */
static void pane(const char* id, const std::string& text, ImVec2 size = ImVec2(0, 0)) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, PAPER);
    ImGui::BeginChild(id, size, ImGuiChildFlags_Borders);
    ImGui::PushTextWrapPos(0);
    ImGui::TextUnformatted(text.c_str(), text.c_str() + text.size());
    ImGui::PopTextWrapPos();
    ImGui::EndChild();
    ImGui::PopStyleColor();
}
static const ImGuiTableFlags TBL = ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp;
static void cell(const std::string& s) { ImGui::TableNextColumn(); ImGui::TextUnformatted(s.c_str()); }
static float lineH() { return ImGui::GetFrameHeightWithSpacing(); }

void App::theme() {
    ImGuiStyle& st = ImGui::GetStyle();
    ImGui::StyleColorsLight();
    st.WindowRounding = 0; st.ChildRounding = 0; st.FrameRounding = 2; st.PopupRounding = 2; st.TabRounding = 2; st.GrabRounding = 2;
    st.WindowBorderSize = 0; st.FrameBorderSize = 1; st.WindowPadding = ImVec2(8, 8); st.FramePadding = ImVec2(8, 4);
    st.ItemSpacing = ImVec2(8, 5); st.CellPadding = ImVec2(6, 3); st.ScrollbarSize = 14;
    ImVec4* c = st.Colors;
    c[ImGuiCol_Text] = INK; c[ImGuiCol_TextDisabled] = DIM;
    c[ImGuiCol_WindowBg] = DESK; c[ImGuiCol_ChildBg] = DESK; c[ImGuiCol_PopupBg] = hex(0xF7F2E4);
    c[ImGuiCol_Border] = hex(0xA09880); c[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_FrameBg] = hex(0xF7F2E4); c[ImGuiCol_FrameBgHovered] = hex(0xFFFAEC); c[ImGuiCol_FrameBgActive] = hex(0xE9E1C8);
    c[ImGuiCol_TitleBg] = hex(0xC9C2AE); c[ImGuiCol_TitleBgActive] = hex(0xC9C2AE); c[ImGuiCol_TitleBgCollapsed] = hex(0xC9C2AE);
    c[ImGuiCol_Button] = hex(0xC9C2AE); c[ImGuiCol_ButtonHovered] = hex(0xB9B198); c[ImGuiCol_ButtonActive] = hex(0xA39B80);
    c[ImGuiCol_Header] = hex(0x8C1E1E, 0.26f); c[ImGuiCol_HeaderHovered] = hex(0x8C1E1E, 0.14f); c[ImGuiCol_HeaderActive] = hex(0x8C1E1E, 0.36f);
    c[ImGuiCol_Tab] = hex(0xC9C2AE); c[ImGuiCol_TabHovered] = hex(0xFFFAEC); c[ImGuiCol_TabSelected] = PAPER;
    c[ImGuiCol_TabSelectedOverline] = RED; c[ImGuiCol_TabDimmed] = hex(0xC9C2AE); c[ImGuiCol_TabDimmedSelected] = PAPER;
    c[ImGuiCol_TableHeaderBg] = hex(0xC9C2AE); c[ImGuiCol_TableBorderStrong] = hex(0xA09880); c[ImGuiCol_TableBorderLight] = hex(0xCFC7AF);
    c[ImGuiCol_TableRowBg] = PAPER; c[ImGuiCol_TableRowBgAlt] = hex(0xEEE7D4);
    c[ImGuiCol_ScrollbarBg] = hex(0xDCD6C4); c[ImGuiCol_ScrollbarGrab] = hex(0xB0A890); c[ImGuiCol_ScrollbarGrabHovered] = hex(0x9A927A); c[ImGuiCol_ScrollbarGrabActive] = hex(0x857D66);
    c[ImGuiCol_CheckMark] = RED; c[ImGuiCol_SliderGrab] = RED; c[ImGuiCol_SliderGrabActive] = RED;
    c[ImGuiCol_Separator] = hex(0xA09880); c[ImGuiCol_ModalWindowDimBg] = ImVec4(0, 0, 0, 0.40f);
    c[ImGuiCol_NavCursor] = RED; c[ImGuiCol_TextSelectedBg] = hex(0x8C1E1E, 0.25f);
    st.ScaleAllSizes(S);
}

// ============================================================================ files
bool App::saveTo(const std::string& path) {
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);
    std::vector<uint8_t> buf = sim.save();
    std::ofstream f(std::filesystem::u8path(path), std::ios::binary);
    if (!f) return false;
    f.write((const char*)buf.data(), (std::streamsize)buf.size());
    return (bool)f;
}
bool App::loadFrom(const std::string& path) {
    std::ifstream f(std::filesystem::u8path(path), std::ios::binary);
    if (!f) return false;
    std::vector<uint8_t> buf((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    Sim t;
    if (!t.load(buf)) return false;
    sim = t;
    inGame = true;
    selPerson = selSite = selRoll = selReq = selCable = selOfficer = -1;
    reportDay = -1; selRow = 0; selCol = 1;
    return true;
}
void App::autosave() { saveTo(saveDir + "/autosave.rez"); }

// ============================================================================ the frame
void App::frame() {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::Begin("##main", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);
    if (!inGame) titleScreen();
    else if (sim.g.phase == PH_INTERLUDE) interludeScreen();
    else if (sim.g.phase == PH_OVER) overScreen();
    else gameScreen();
    popups();
    ImGui::End();
}

void App::titleScreen() {
    static const char* TXT =
        "\n"
        "    R E Z I D E N T U R A\n"
        "    ---------------------\n\n"
        "    First Chief Directorate, Committee for State Security\n"
        "    1968 - 1991\n\n\n"
        "    You are an operational officer of the KGB, twenty-nine years old, about to take up your first\n"
        "    post abroad: London, September 1968. If you last, there will be Vienna, Washington and Bonn,\n"
        "    and a country that does not outlive your career.\n\n"
        "    Nobody shoots at anybody. The work is finding people with access, studying them, recruiting\n"
        "    them, and keeping them alive and producing - one morning, afternoon and evening at a time.\n"
        "    Later it is making other officers do the same, and wondering which of them is reporting to\n"
        "    the other side.\n\n"
        "    You are never shown the truth about anyone. You see what they show you, what Centre's\n"
        "    evaluators say of their material, and what you notice in the street behind you.\n"
        "    The rest is judgement.\n\n"
        "    Read the Manual tab (section 14, 'Your first week') before your first meeting.\n";
    float bh = lineH() * 1.6f;
    pane("title", TXT, ImVec2(0, -bh));
    ImGui::Spacing();
    if (Btn("Begin a new career")) openNew = true;
    ImGui::SameLine();
    std::error_code ec;
    bool hasAuto = std::filesystem::exists(std::filesystem::u8path(saveDir + "/autosave.rez"), ec);
    ImGui::BeginDisabled(!hasAuto);
    if (Btn("Continue")) { if (!loadFrom(saveDir + "/autosave.rez")) note = "The autosave could not be read."; }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (Btn("Load a saved game...")) openLoad = true;
    ImGui::SameLine();
    if (Btn("Quit")) wantQuit = true;
}

void App::interludeScreen() {
    GameState& g = sim.g;
    ImGui::PushStyleColor(ImGuiCol_Text, RED);
    ImGui::TextUnformatted(g.interludeTitle.c_str());
    ImGui::PopStyleColor();
    float bh = lineH() * ((float)g.interludeOpts.size() + 0.6f);
    pane("interlude", g.interludeText, ImVec2(0, -bh));
    for (size_t i = 0; i < g.interludeOpts.size(); i++) {
        if (Btn(g.interludeOpts[i].c_str())) { sim.chooseInterlude((int)i); autosave(); wantTab = 4; selPerson = selSite = selRoll = selReq = selCable = selOfficer = -1; reportDay = -1; break; }
    }
}

void App::overScreen() {
    GameState& g = sim.g;
    ImGui::PushStyleColor(ImGuiCol_Text, RED);
    ImGui::TextUnformatted(g.overTitle.c_str());
    ImGui::PopStyleColor();
    pane("over", g.overText, ImVec2(0, -lineH() * 1.4f));
    if (Btn("Return to the title screen")) inGame = false;
    ImGui::SameLine();
    if (Btn("Quit")) wantQuit = true;
}

bool App::tab(const char* label, int idx) {
    bool open = ImGui::BeginTabItem(label, nullptr, wantTab == idx ? ImGuiTabItemFlags_SetSelected : 0);
    std::string key = label;
    size_t h = key.find("###");
    rec("tab:" + (h == std::string::npos ? key : key.substr(h + 3)));
    if (wantTab == idx) wantTab = -1;
    if (open) curTab = idx;
    return open;
}

void App::doEndDay(bool run) {
    if (!sim.g.decisions.empty()) { note = "A decision is waiting for you."; return; }
    for (int n = 0; n < (run ? 30 : 1); n++) {
        if (n > 0) {
            bool mine = false;
            for (auto& p : sim.g.persons) if (p.stage == AGENT && p.handler == 0 && p.nextMeet == sim.g.day) mine = true;
            if (mine) break;
        }
        sim.attention = false;
        sim.endDay();
        if (sim.g.phase == PH_PLAY || sim.g.phase == PH_INTERLUDE) autosave();
        if (sim.attention || sim.g.phase != PH_PLAY || !sim.g.decisions.empty()) break;
    }
    reportDay = sim.g.day - 1;
    selRow = 0;
    if (sim.g.phase != PH_PLAY) return;
    if (sim.unread() > 0) wantTab = 4;
    else {
        auto it = sim.g.reports.find(sim.g.day - 1);
        if (sim.attention || (it != sim.g.reports.end() && it->second.size() > 5)) wantTab = 5;
    }
}

void App::gameScreen() {
    GameState& g = sim.g;
    // ---- top bar
    ImGui::PushStyleColor(ImGuiCol_ChildBg, BAR);
    ImGui::BeginChild("bar", ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 2 + 14 * S), 0, ImGuiWindowFlags_NoScrollbar);
    ImGui::SetCursorPos(ImVec2(10 * S, 6 * S));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
    ImGui::TextUnformatted(sim.status1().c_str());
    ImGui::PopStyleColor();
    ImGui::SetCursorPosX(10 * S);
    ImGui::PushStyleColor(ImGuiCol_Text, hex(0xDDD5BF));
    ImGui::TextUnformatted(sim.status2().c_str());
    ImGui::PopStyleColor();
    const char* bl[] = {"Title", "Save", "Run to next event >>", "END DAY >"};
    float w = 0;
    for (auto l : bl) w += ImGui::CalcTextSize(l).x + ImGui::GetStyle().FramePadding.x * 2 + ImGui::GetStyle().ItemSpacing.x;
    ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - w - 6 * S, 10 * S));
    if (Btn("Title")) { autosave(); inGame = false; }
    ImGui::SameLine();
    if (Btn("Save")) openSave = true;
    ImGui::SameLine();
    if (Btn("Run to next event >>")) doEndDay(true);
    ImGui::SameLine();
    if (Btn("END DAY >")) doEndDay(false);
    ImGui::EndChild();
    ImGui::PopStyleColor();
    if (!inGame || g.phase != PH_PLAY) return;

    // ---- tabs
    if (ImGui::BeginTabBar("tabs")) {
        std::string centre = sim.unread() ? fstr("Centre (%d)###Centre", sim.unread()) : std::string("Centre###Centre");
        if (tab("Day Planner", 0)) { tabPlanner(); ImGui::EndTabItem(); }
        if (tab("Contacts & Agents", 1)) { tabContacts(); ImGui::EndTabItem(); }
        if (tab("Sites & Routes", 2)) { tabSites(); ImGui::EndTabItem(); }
        if (tab("Material & Stores", 3)) { tabMaterial(); ImGui::EndTabItem(); }
        if (tab(centre.c_str(), 4)) { tabCentre(); ImGui::EndTabItem(); }
        if (tab("Operations Report", 5)) { tabReport(); ImGui::EndTabItem(); }
        if (tab("Station", 6)) { tabStation(); ImGui::EndTabItem(); }
        if (tab("Surveillance Log", 7)) { tabSurveillance(); ImGui::EndTabItem(); }
        if (tab("Accounts", 8)) { tabAccounts(); ImGui::EndTabItem(); }
        if (tab("Career", 9)) { textTab("career", sim.careerText()); ImGui::EndTabItem(); }
        if (tab("Manual", 10)) { textTab("manual", manual); ImGui::EndTabItem(); }
        ImGui::EndTabBar();
    }
}

void App::textTab(const char* id, const std::string& text) { pane(id, text); }

// ============================================================================ DAY PLANNER
void App::beginEdit(int day, int block) {
    edDay = day; edBlock = block;
    ed = sim.order(day, block);
    edError.clear();
    openEditor = true;
}

/** Put an order into a block if the rules allow it. Returns the reason if they do not. */
std::string App::place(Order o, int day, int block) {
    if (sim.isSet(day, block) && sim.order(day, block).appointment) return "that block holds an agreed meeting.";
    o.cs = false;                              // the team is arranged day by day; ask again
    o.appointment = false;
    std::string err = sim.validate(o, day, block);
    if (!err.empty()) return err;
    sim.setOrder(day, block, o);
    return "";
}

void App::tabPlanner() {
    GameState& g = sim.g;
    float leftW = ImGui::GetContentRegionAvail().x * 0.66f;
    ImGui::BeginChild("planL", ImVec2(leftW, 0));
    if (ImGui::BeginTable("plan", 5, TBL, ImVec2(0, -lineH() * 1.2f))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Date", ImGuiTableColumnFlags_WidthFixed, 120 * S);
        ImGui::TableSetupColumn("Morning", 0, 3);
        ImGui::TableSetupColumn("Afternoon", 0, 3);
        ImGui::TableSetupColumn("Evening", 0, 3);
        ImGui::TableSetupColumn("Diary", 0, 2.4f);
        ImGui::TableHeadersRow();
        for (int r = 0; r < 35; r++) {
            int day = g.day + r;
            if (day > g.postEnd) break;
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::PushStyleColor(ImGuiCol_Text, weekend(day) ? DIM : INK);
            ImGui::TextUnformatted(fmtDay(day).c_str());
            ImGui::PopStyleColor();
            for (int b = 0; b < 3; b++) {
                ImGui::TableNextColumn();
                Order o = sim.order(day, b);
                bool set = sim.isSet(day, b);
                ImGui::PushStyleColor(ImGuiCol_Text, o.appointment ? RED : (set ? INK : DIM));
                std::string label = sim.describe(o) + fstr("##p%d_%d", r, b);
                if (ImGui::Selectable(label.c_str(), selRow == r && selCol == b + 1, ImGuiSelectableFlags_AllowDoubleClick)) {
                    selRow = r; selCol = b + 1;
                    if (ImGui::IsMouseDoubleClicked(0)) beginEdit(day, b);
                }
                rec(fstr("plan:%d:%d", r, b));
                ImGui::PopStyleColor();
            }
            cell(sim.diary(day));
        }
        ImGui::EndTable();
    }
    int day = g.day + selRow, b = selCol - 1;
    if (Btn("Edit order...")) beginEdit(day, b);
    ImGui::SameLine();
    if (Btn("Back to routine")) {
        if (sim.isSet(day, b) && sim.order(day, b).appointment) note = "This is an agreed meeting. The agent will be there whether you are or not. If you must miss it, leave it and accept the consequences - or abort on the day.";
        else sim.clearOrder(day, b);
    }
    ImGui::SameLine();
    if (Btn("Copy")) {
        Order o = sim.order(day, b);
        if (o.appointment) note = "An agreed meeting cannot be copied: its date and place belong to the agent's communications plan.";
        else { copied = o; hasClip = true; }
    }
    ImGui::SameLine();
    if (Btn("Paste")) {
        if (!hasClip) note = "Copy an order first.";
        else { std::string e = place(copied, day, b); if (!e.empty()) note = e; }
    }
    ImGui::SameLine();
    if (Btn("Repeat weekly x3")) {
        Order o = sim.order(day, b);
        if (o.appointment) note = "An agreed meeting repeats by itself, under the agent's communications plan.";
        else {
            std::string skipped;
            for (int k = 1; k <= 3; k++) { std::string e = place(o, day + 7 * k, b); if (!e.empty()) skipped += fmtDay(day + 7 * k) + ": " + e + "\n"; }
            if (!skipped.empty()) note = "Not placed:\n" + skipped;
        }
    }
    ImGui::SameLine();
    if (Btn("Arrange countersurveillance team for that day")) note = sim.requestCS(day);
    ImGui::EndChild();
    ImGui::SameLine();
    pane("brief", g.brief);
}

// ---------------------------------------------------------------------------- the order form
static const char* HINT[N_OTYPE] = {
    "Press work at the Embassy. Keeps your cover credible; about five blocks a week holds it steady. If the watchers are on you they see a diplomat doing his job.",
    "Write up contact reports and accounts. Clears up to 4.5 hours of outstanding paperwork.",
    "Recovers fatigue. A tired officer misses surveillance.",
    "A reception, lecture or society meeting. The usual way to meet new people. Afternoon or evening.",
    "Go over one of your sites (or walk one of your routes) again: raises its casing level (max 3), sharpens your estimates, and is the cheapest way to notice men waiting at a dead drop.",
    "Look for a new meeting place, dead drop, signal site or detection route in a district.",
    "A surveillance detection route. It only helps the operational act in the block IMMEDIATELY after it. A prepared route beats an improvised one - until you have walked it too often.",
    "Coffee (morning), lunch (afternoon) or dinner (evening) in the open. Builds the relationship and your assessment. For contacts only - never for a recruited agent.",
    "A meeting at one of your meeting sites. Needed for a recruitment approach. Agents can only be met on the agreed date at the agreed site.",
    "Leave money, film or written instructions in the agent's dead drop. He collects them at his next servicing.",
    "Empty a dead drop. Read the signal site first: the 'drop loaded' mark tells you there is something to fetch; a danger signal tells you to stay away.",
    "Walk past a signal site and see what mark is up. Little time, little risk.",
    "Put up your call-out mark. If the agent sees it he comes to the next meeting site in his rotation three evenings later.",
    "Place the sealed package for an illegal in a dead drop that has never been used, in the district Centre named. You will never meet the man who collects it.",
    "A marked card. Tell exactly ONE colleague that you will service this drop at this hour, and go and see whether anybody is waiting there. Use a drop nobody else has ever heard of."};

void App::orderEditor() {
    GameState& g = sim.g;
    if (openEditor) { ImGui::OpenPopup("Order"); openEditor = false; }
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (!ImGui::BeginPopupModal("Order", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) return;
    const float W = 520 * S, LW = 250 * S;           // field width, label column
    Order& o = ed;
    bool appt = o.appointment;
    Person* p = sim.person(o.person);
    bool agent = p && p->stage == AGENT;
    ImGui::PushStyleColor(ImGuiCol_Text, RED);
    ImGui::TextUnformatted((fmtDay(edDay) + " - " + BLOCK_LABEL[edBlock] + (appt ? "   (AGREED MEETING: date, hour and place are fixed)" : "")).c_str());
    ImGui::PopStyleColor();
    ImGui::PushTextWrapPos(LW + W + 20 * S);
    ImGui::TextUnformatted(HINT[o.type]);
    ImGui::PopTextWrapPos();
    ImGui::Separator();
    auto label = [&](const char* s) { ImGui::AlignTextToFramePadding(); ImGui::TextUnformatted(s); ImGui::SameLine(LW); };

    // ---- what
    std::vector<Opt> types;
    bool lineN = false;
    for (auto& q : g.reqs) if (!q.closed && q.kind == RK_LINEN) lineN = true;
    for (int t = 0; t < N_OTYPE; t++) { if (t == O_LINE_N && !lineN && o.type != O_LINE_N) continue; types.push_back({OTYPE_LABEL[t], t}); }
    label("Activity");
    ImGui::BeginDisabled(appt);
    if (combo("##type", o.type, types, W)) { o.person = -1; o.site = -1; o.pitch = -1; o.am = -1; edError.clear(); p = nullptr; agent = false; }
    ImGui::EndDisabled();
    int t = o.type;
    bool usesPerson = t == O_MEET_OVERT || t == O_MEET_CLAN || t == O_LOAD_DROP || t == O_PLACE_SIGNAL;
    if (usesPerson) {
        std::vector<Opt> ps;
        for (auto& x : g.persons) {
            if (!x.live()) continue;
            if (!appt && x.handler != 0) continue;
            if (t == O_MEET_OVERT && x.stage == AGENT) continue;
            if ((t == O_LOAD_DROP || t == O_PLACE_SIGNAL) && x.stage != AGENT) continue;
            ps.push_back({x.full() + " - " + STAGE_LABEL[x.stage], x.id});
        }
        if (o.person < 0 && !ps.empty()) o.person = ps[0].value;
        label("Contact / agent");
        ImGui::BeginDisabled(appt);
        combo("##person", o.person, ps, W);
        ImGui::EndDisabled();
        p = sim.person(o.person);
        agent = p && p->stage == AGENT;
    }
    auto siteOpts = [&](int type, int only, int district) {
        std::vector<Opt> v;
        for (auto& s : g.sites) {
            if (s.retired || (type >= 0 && s.type != type) || (only >= 0 && s.id != only) || (district >= 0 && s.district != district)) continue;
            std::string l = clip(s.name, 60);
            if (s.type == ST_ROUTE) l += "  (quality " + word(s.estSight, W_LEVEL, 5) + fstr(", known %d/3", s.casing);
            else l += "  (cover " + word(s.estCover, W_LEVEL, 5) + fstr(", cased %d/3", s.casing);
            l += std::string(s.exposure > 55 ? ", OVER-USED)" : ")");
            v.push_back({l, s.id});
        }
        return v;
    };
    bool usesSite = t == O_CASE || t == O_MEET_CLAN || t == O_LOAD_DROP || t == O_UNLOAD_DROP || t == O_READ_SIGNAL || t == O_LINE_N || t == O_CONTROL;
    if (t == O_LINE_N) {
        std::vector<Opt> qs;
        for (auto& q : g.reqs) if (!q.closed && q.kind == RK_LINEN) qs.push_back({fstr("No. %d  ", q.id) + q.title, q.id});
        if (o.lineN < 0 && !qs.empty()) o.lineN = qs[0].value;
        label("Line N task");
        combo("##linen", o.lineN, qs, W);
    }
    if (usesSite) {
        Requirement* lq = sim.req(o.lineN);
        std::vector<Opt> ss = t == O_CASE ? siteOpts(-1, -1, -1) : t == O_MEET_CLAN ? siteOpts(ST_MEET, appt ? o.site : -1, -1)
                              : t == O_LOAD_DROP ? siteOpts(ST_DROP, p ? p->dropSite : -9, -1) : t == O_READ_SIGNAL ? siteOpts(ST_SIGNAL, -1, -1)
                              : t == O_LINE_N ? siteOpts(ST_DROP, -1, lq ? lq->district : -1) : siteOpts(ST_DROP, -1, -1);
        bool found = false;
        for (auto& x : ss) if (x.value == o.site) found = true;
        if (!found) o.site = ss.empty() ? -1 : ss[0].value;
        label(t == O_CASE ? "Site or route" : "Site");
        ImGui::BeginDisabled(appt);
        if (ss.empty()) ImGui::TextDisabled("(none available)"); else combo("##site", o.site, ss, W);
        ImGui::EndDisabled();
    }
    if (t == O_SDR) {
        std::vector<Opt> rs = {{"Improvise as you go", -1}};
        for (auto& x : siteOpts(ST_ROUTE, -1, -1)) rs.push_back(x);
        label("Route");
        combo("##route", o.route, rs, W);
    }
    if (t == O_SCOUT) {
        std::vector<Opt> ds, ks;
        for (int i = 0; i < 8; i++) ds.push_back({std::string(sim.T().district[i]) + (sim.T().dist[i] == 0 ? "  (on the doorstep)" : sim.T().dist[i] == 1 ? "  (near)" : sim.T().dist[i] == 2 ? "  (moderate)" : "  (far)"), i});
        for (int i = 0; i < N_SITETYPE; i++) ks.push_back({SITETYPE_LABEL[i], i});
        label("District");
        combo("##district", o.district, ds, W);
        label("Looking for");
        combo("##kind", o.siteType, ks, W);
    }
    if (t == O_FUNCTION) {
        std::vector<Opt> sc;
        for (int i = 0; i < N_SECTOR; i++) sc.push_back({SECTOR_LABEL[i], i});
        label("Circle");
        combo("##sector", o.sector, sc, W);
    }
    if (t == O_CONTROL) {
        std::vector<Opt> os;
        for (auto& x : g.officers) if (!x.gone && (x.role == ROLE_COLLEAGUE || x.role == ROLE_SUB || x.role == ROLE_TECH)) os.push_back({x.title(), x.id});
        if (o.officer < 0 && !os.empty()) o.officer = os[0].value;
        label("The one officer told");
        combo("##officer", o.officer, os, W);
    }
    // ---- security
    bool approach = t == O_CASE || t == O_SCOUT || t == O_MEET_CLAN || t == O_LOAD_DROP || t == O_UNLOAD_DROP || t == O_LINE_N || t == O_CONTROL;
    if (approach) {
        ImGui::SeparatorText("Security");
        std::vector<Opt> pol;
        for (int i = 0; i < 3; i++) pol.push_back({POLICY[i], i});
        label("If surveillance is seen");
        combo("##policy", o.policy, pol, W);
        auto cs = g.csDays.find(edDay);
        bool arranged = cs != g.csDays.end() && cs->second.state == 2;
        if (arranged) {
            Officer* a = sim.officer(cs->second.o1); Officer* b = sim.officer(cs->second.o2);
            Check(("Countersurveillance team covers this act: " + (a ? a->title() : std::string("?")) + ", " + (b ? b->title() : std::string("?"))).c_str(), &o.cs);
        } else { o.cs = false; ImGui::TextDisabled("No countersurveillance team is arranged for this day (Day Planner, below the table)."); }
    }
    // ---- agenda
    bool meet = t == O_MEET_CLAN, load = t == O_LOAD_DROP, overt = t == O_MEET_OVERT;
    bool pitching = meet && !agent && o.pitch >= 0;
    if (meet || load || overt) {
        ImGui::SeparatorText("Agenda");
        if (meet) Check("Long meeting, about two hours (untick for a ten-minute contact)", &o.longMeet);
        if (meet && agent) Check(gx(p, "Take delivery of his exposed film (untick for a talk-only meeting: nothing incriminating changes hands)").c_str(), &o.collect);
        label(agent || load ? "Money to hand over" : "Money / gift to offer");
        ImGui::SetNextItemWidth(140 * S);
        ImGui::InputInt("##pay", &o.pay, sim.P(5), sim.P(20));
        o.pay = std::max(0, std::min(o.pay, 99999));
        ImGui::SameLine();
        if (agent || load) ImGui::TextDisabled("%s", gx(p, fstr("(he is owed %d; you hold %d %s)", p ? std::max(0, p->owed) : 0, g.funds, sim.T().currency)).c_str());
        else ImGui::TextDisabled("(you hold %d %s)", g.funds, sim.T().currency);
        if ((meet && (agent || pitching)) || load) {
            label("Fresh cassettes to give");
            ImGui::SetNextItemWidth(140 * S);
            ImGui::InputInt("##film", &o.film, 1, 2);
            o.film = std::max(0, std::min(o.film, 6));
            ImGui::SameLine(); ImGui::TextDisabled("(you hold %d)", g.film);
            Check(fstr("Issue a Minox camera (you hold %d)", g.cameras).c_str(), &o.camera);
        }
        if (meet && (agent || pitching)) {
            label("One-time pad sheets to give");
            ImGui::SetNextItemWidth(140 * S);
            ImGui::InputInt("##pads", &o.pads, 1, 2);
            o.pads = std::max(0, std::min(o.pads, 6));
            ImGui::SameLine(); ImGui::TextDisabled("%s", gx(p, fstr("(you hold %d; with them he can receive broadcasts)", g.pads)).c_str());
        }
        if (load) Check(fstr("Use a concealment container (you hold %d)", g.containers).c_str(), &o.container);
        if (meet && agent) Check("Give security instruction (long meetings only)", &o.train);
        if ((meet && !agent) || overt) Check("Ask a small favour (builds commitment; may be refused)", &o.favour);
        if ((meet && (agent || pitching)) || load) {
            std::vector<Opt> tp = {{"(no change)", -1}};
            for (int i = 0; i < 4; i++) tp.push_back({TEMPO[i], i});
            label("Tempo of collection");
            combo("##tempo", o.tempo, tp, W);
            std::vector<Opt> rq = {{"(no change)", -2}, {"General collection (no requirement)", -1}};
            for (auto& q : g.reqs) if (!q.closed && q.kind == RK_COLLECT) rq.push_back({fstr("No. %d  ", q.id) + q.title + " [" + SECTOR_SHORT[q.sector] + "]", q.id});
            label("Task against requirement");
            combo("##task", o.reqId, rq, W);
        }
        if (meet || overt) {
            std::vector<Opt> am = {{"(none)", -1}};
            for (auto& q : g.reqs) if (!q.closed && q.kind == RK_ACTIVE) am.push_back({fstr("No. %d  ", q.id) + q.title, q.id});
            if (am.size() > 1) { label("Pass Service A material"); combo("##am", o.am, am, W); }
        }
        if (meet && !agent) {
            std::vector<Opt> pi = {{"(no approach at this meeting)", -1}};
            for (int i = 0; i < 4; i++) pi.push_back({std::string("Pitch on: ") + PITCH[i], i});
            label("Recruitment approach");
            combo("##pitch", o.pitch, pi, W);
            if (o.pitch >= 0 && p && p->sanction != 2) { ImGui::PushStyleColor(ImGuiCol_Text, RED); ImGui::TextUnformatted("Centre has NOT sanctioned this recruitment."); ImGui::PopStyleColor(); }
        }
    }
    // ---- communications plan
    if (meet && (agent || pitching)) {
        ImGui::SeparatorText("Communications plan - briefed at this meeting");
        std::vector<Opt> me = {{"(no change)", -1}, {"every 14 days", 14}, {"every 21 days", 21}, {"every 28 days", 28}, {"every 42 days", 42}, {"every 60 days", 60}, {"none - drops and signals only", 0}};
        label("Personal meetings");
        combo("##meetEvery", o.meetEvery, me, W);
        Check(gx(p, "Change the meeting sites in his rotation").c_str(), &o.setMeetSites);
        if (o.setMeetSites) {
            ImGui::BeginChild("rot", ImVec2(LW + W, 5.2f * ImGui::GetFrameHeightWithSpacing()), ImGuiChildFlags_Borders);
            for (auto& s : siteOpts(ST_MEET, -1, -1)) {
                bool in = std::find(o.meetSites.begin(), o.meetSites.end(), s.value) != o.meetSites.end(), was = in;
                ImGui::PushID(s.value);
                ImGui::Checkbox(s.label.c_str(), &in);
                ImGui::PopID();
                if (in && !was) o.meetSites.push_back(s.value);
                if (!in && was) o.meetSites.erase(std::find(o.meetSites.begin(), o.meetSites.end(), s.value));
            }
            ImGui::EndChild();
        }
        std::vector<Opt> de = {{"(no change)", -1}, {"every 7 days", 7}, {"every 14 days", 14}, {"every 21 days", 21}, {"every 28 days", 28}, {"no dead drop", 0}};
        label(gx(p, "He services his drop").c_str());
        combo("##dropEvery", o.dropEvery, de, W);
        std::vector<Opt> dsites = {{"(no change)", -2}, {"(none)", -1}}, ssites = dsites;
        for (auto& s : siteOpts(ST_DROP, -1, -1)) dsites.push_back(s);
        for (auto& s : siteOpts(ST_SIGNAL, -1, -1)) ssites.push_back(s);
        label("Dead drop");
        combo("##dropSite", o.dropSite, dsites, W);
        label("Signal site");
        combo("##signalSite", o.signalSite, ssites, W);
    }
    ImGui::Separator();
    if (!edError.empty()) { ImGui::PushStyleColor(ImGuiCol_Text, RED); ImGui::PushTextWrapPos(LW + W); ImGui::TextUnformatted(edError.c_str()); ImGui::PopTextWrapPos(); ImGui::PopStyleColor(); }
    if (Btn("OK")) {
        Order out = o;
        if (!usesPerson) out.person = -1;
        if (!usesSite) out.site = -1;
        if (!o.setMeetSites) out.meetSites.clear();
        std::string err = sim.validate(out, edDay, edBlock);
        if (err.empty()) { sim.setOrder(edDay, edBlock, out); ImGui::CloseCurrentPopup(); }
        else edError = err;
    }
    ImGui::SameLine();
    if (Btn("Cancel")) ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
}

// ============================================================================ CONTACTS & AGENTS
void App::tabContacts() {
    GameState& g = sim.g;
    float leftW = ImGui::GetContentRegionAvail().x * 0.56f;
    ImGui::BeginChild("perL", ImVec2(leftW, 0));
    Person* sel = sim.person(selPerson);
    if (ImGui::BeginTable("per", 7, TBL, ImVec2(0, -lineH() * 2.4f))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Name / codename", 0, 3.0f);
        ImGui::TableSetupColumn("Status", 0, 1.3f);
        ImGui::TableSetupColumn("Line", 0, 1.1f);
        ImGui::TableSetupColumn("Position", 0, 3.6f);
        ImGui::TableSetupColumn("Relations", 0, 1.3f);
        ImGui::TableSetupColumn("Run by", 0, 1.9f);
        ImGui::TableSetupColumn("Next meeting", 0, 1.6f);
        ImGui::TableHeadersRow();
        for (int pass = 0; pass < 3; pass++)
            for (auto& p : g.persons) {
                int k = p.stage == AGENT ? 0 : (p.live() ? 1 : 2);
                if (k != pass || (k == 2 && !showClosed)) continue;
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::PushID(p.id);
                if (ImGui::Selectable(p.full().c_str(), selPerson == p.id, ImGuiSelectableFlags_SpanAllColumns)) selPerson = p.id;
                rec("person:" + p.name);
                ImGui::PopID();
                cell(STAGE_LABEL[p.stage]); cell(SECTOR_SHORT[p.sector]); cell(p.job + ", " + p.employer);
                cell(word(p.rapport, W_RAPPORT, 5)); cell(sim.handlerName(p));
                cell(p.stage == AGENT && p.nextMeet >= 0 ? fmtShort(p.nextMeet) : std::string(""));
            }
        ImGui::EndTable();
    }
    if (sel) {
        if (Btn("Request trace")) note = sim.requestTrace(*sel);
        ImGui::SameLine();
        if (Btn("Propose recruitment (sanction)")) note = sim.requestSanction(*sel);
        ImGui::SameLine();
        if (Btn("Terminate contact")) {
            int id = sel->id;
            confirmText = "Terminate all contact with " + sel->full() + "? The file is closed for good.";
            confirmAction = [this, id]() { if (Person* q = sim.person(id)) note = sim.terminate(*q); };
        }
        if (sel->stage == AGENT) {
            ImGui::SameLine();
            if (Btn(sel->handler ? "Instruct..." : "Radio instruction...")) { instrPerson = sel->id; instrTempo = -1; instrReq = -2; instrCallout = false; openInstr = true; }
        }
        // who runs the case
        std::vector<Opt> hs = {{"you personally", 0}};
        for (auto& o : g.officers) if (o.role == ROLE_SUB && !o.gone) hs.push_back({o.title(), o.id});
        if (hs.size() > 1 && sel->live()) {
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Case officer:");
            ImGui::SameLine();
            int h = sel->handler;
            if (combo("##handler", h, hs, 220 * S)) note = sim.setHandler(*sel, h);
            if (sel->handler != 0 && sel->stage != AGENT) {
                ImGui::SameLine();
                Check("authorise him to make the approach once Centre sanctions it", &sel->pitchAuth);
            }
        }
    } else ImGui::TextDisabled("Select a file.");
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 170 * S);
    Check("Show closed files", &showClosed);
    ImGui::EndChild();
    ImGui::SameLine();
    pane("dossier", sel ? sim.dossier(*sel) : std::string("Select a file."));
}

// ============================================================================ SITES & ROUTES
void App::tabSites() {
    GameState& g = sim.g;
    if (ImGui::BeginTable("sites", 9, TBL, ImVec2(0, -lineH() * 1.2f))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Site", 0, 5);
        ImGui::TableSetupColumn("Type", 0, 1.4f);
        ImGui::TableSetupColumn("District", 0, 1.4f);
        ImGui::TableSetupColumn("Natural cover", 0, 1.2f);
        ImGui::TableSetupColumn("View / quality", 0, 1.2f);
        ImGui::TableSetupColumn("Cased", 0, 0.7f);
        ImGui::TableSetupColumn("Uses", 0, 0.6f);
        ImGui::TableSetupColumn("Last used", 0, 1);
        ImGui::TableSetupColumn("Notes", 0, 4.5f);
        ImGui::TableHeadersRow();
        for (int type = 0; type < N_SITETYPE; type++)
            for (auto& s : g.sites) {
                if (s.type != type || (s.retired && !showRetired)) continue;
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::PushID(s.id);
                if (ImGui::Selectable(s.name.c_str(), selSite == s.id, ImGuiSelectableFlags_SpanAllColumns)) selSite = s.id;
                ImGui::PopID();
                cell(SITETYPE_LABEL[s.type]); cell(sim.T().district[s.district]);
                cell(s.type == ST_ROUTE ? std::string("-") : word(s.estCover, W_LEVEL, 5)); cell(word(s.estSight, W_LEVEL, 5));
                cell(fstr("%d/3", s.casing)); cell(fstr("%d", s.uses)); cell(s.lastUsed < 0 ? std::string("-") : fmtShort(s.lastUsed));
                cell(sim.siteNotes(s));
            }
        ImGui::EndTable();
    }
    if (Btn("Strike site off the list")) {
        Site* s = sim.site(selSite);
        if (!s) note = "Select a site first.";
        else {
            std::string used;
            for (auto& q : g.persons) if (q.stage == AGENT && (std::find(q.meetSites.begin(), q.meetSites.end(), s->id) != q.meetSites.end() || q.dropSite == s->id || q.signalSite == s->id)) used = gx(&q, q.label() + " still has this site in his communications plan. Brief him on a new plan first.");
            if (!used.empty()) note = used;
            else { int id = s->id; confirmText = "Strike off " + s->name + "? It will not be offered again."; confirmAction = [this, id]() { if (Site* t = sim.site(id)) sim.retireSite(*t); }; }
        }
    }
    ImGui::SameLine();
    Check("Show struck-off sites", &showRetired);
    ImGui::SameLine();
    ImGui::TextDisabled("  New sites and routes are found with the 'Scout district' order. Estimates sharpen with each casing.");
}

// ============================================================================ MATERIAL & STORES
void App::tabMaterial() {
    GameState& g = sim.g;
    float leftW = ImGui::GetContentRegionAvail().x * 0.52f;
    ImGui::BeginChild("matL", ImVec2(leftW, 0));
    if (ImGui::BeginTable("rolls", 7, TBL, ImVec2(0, -lineH() * 1.2f))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("No.", 0, 0.7f);
        ImGui::TableSetupColumn("Source", 0, 1.3f);
        ImGui::TableSetupColumn("Type", 0, 0.8f);
        ImGui::TableSetupColumn("Shot", 0, 1.1f);
        ImGui::TableSetupColumn("State", 0, 3);
        ImGui::TableSetupColumn("Since", 0, 1.1f);
        ImGui::TableSetupColumn("Centre's grade", 0, 3);
        ImGui::TableHeadersRow();
        for (int i = (int)g.rolls.size() - 1; i >= 0; i--) {
            Roll& x = g.rolls[i];
            if (x.state == R_AGENT || x.state == R_DROP) continue;      // you do not know what he is holding
            Person* p = sim.person(x.agentId);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::PushID(x.id);
            if (ImGui::Selectable(fstr("%d", x.id).c_str(), selRoll == x.id, ImGuiSelectableFlags_SpanAllColumns)) selRoll = x.id;
            ImGui::PopID();
            cell(p ? p->label() : std::string("?")); cell(x.oral ? "oral" : "film"); cell(fmtShort(x.shotDay));
            cell(std::string(RSTATE[x.state]) + (x.cabled && x.state == R_READY ? " (summary queued)" : "")); cell(fmtShort(x.stateDay)); cell(x.grade);
        }
        ImGui::EndTable();
    }
    if (Btn("Cable a summary of the selected item to Centre")) { Roll* x = sim.roll(selRoll); note = x ? sim.cableSummary(*x) : std::string("Select an item of material first."); }
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("matR", ImVec2(0, 0));
    pane("stores", sim.storesText(), ImVec2(0, -lineH() * 2.4f));
    if (Btn("Requisition film (5)")) note = sim.requestFilm();
    ImGui::SameLine();
    if (Btn("Requisition camera")) note = sim.requestCamera();
    ImGui::SameLine();
    if (Btn("Order containers (3)")) note = sim.requestContainers();
    if (Btn("Draw one-time pads")) note = sim.requestPads();
    ImGui::SameLine();
    if (Btn("Ask Centre for supplementary funds")) note = sim.requestFunds();
    ImGui::EndChild();
}

// ============================================================================ CENTRE
void App::tabCentre() {
    GameState& g = sim.g;
    float leftW = ImGui::GetContentRegionAvail().x * 0.5f, h = ImGui::GetContentRegionAvail().y;
    ImGui::BeginChild("cenL", ImVec2(leftW, 0));
    if (ImGui::BeginTable("reqs", 6, TBL, ImVec2(0, h * 0.58f))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("No.", 0, 0.6f);
        ImGui::TableSetupColumn("Line", 0, 1.3f);
        ImGui::TableSetupColumn("Requirement", 0, 4.6f);
        ImGui::TableSetupColumn("Issued", 0, 1.2f);
        ImGui::TableSetupColumn("Due", 0, 1.2f);
        ImGui::TableSetupColumn("State", 0, 1.6f);
        ImGui::TableHeadersRow();
        for (int pass = 0; pass < 2; pass++)
            for (int i = (int)g.reqs.size() - 1; i >= 0; i--) {
                Requirement& q = g.reqs[i];
                if ((q.closed ? 1 : 0) != pass) continue;
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::PushID(q.id);
                ImGui::PushStyleColor(ImGuiCol_Text, q.closed ? DIM : INK);
                if (ImGui::Selectable(fstr("%d", q.id).c_str(), selReq == q.id, ImGuiSelectableFlags_SpanAllColumns)) selReq = q.id;
                ImGui::PopID();
                cell(q.kind == RK_ACTIVE ? "Service A" : q.kind == RK_LINEN ? "Line N" : SECTOR_SHORT[q.sector]); cell(q.title); cell(fmtShort(q.issued));
                cell(q.due < 0 ? std::string("standing") : fmtShort(q.due));
                cell(q.closed ? (q.failed ? "LAPSED" : (q.kind == RK_LINEN ? "carried out" : "satisfied"))
                     : q.kind == RK_ACTIVE ? fstr("%d of %d placed", (int)q.progress, (int)q.need) : q.kind == RK_LINEN ? std::string("to do") : fstr("%d%%", (int)std::min(99.f, q.progress * 100 / q.need)));
                ImGui::PopStyleColor();
            }
        ImGui::EndTable();
    }
    Requirement* q = sim.req(selReq);
    pane("reqtext", q ? fstr("REQUIREMENT No. %d - ", q->id) + (q->kind == RK_ACTIVE ? "Service A (active measures)" : q->kind == RK_LINEN ? "Line N (illegals support)" : SECTOR_LABEL[q->sector])
                          + "\n" + q->title + "\n\n" + q->text + "\n\n" + (q->due >= 0 ? "Required by " + fmtDay(q->due) + "." : std::string("Standing requirement."))
                        : std::string("Select a requirement to read it.\n\nCollection requirements are met by graded material from agents in that line who are tasked against them. "
                                      "Service A requirements are met by placing material through people with influence. Line N tasks are carried out by you in person."));
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("cenR", ImVec2(0, 0));
    if (ImGui::BeginTable("cables", 3, TBL, ImVec2(0, h * 0.40f))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Date", 0, 1.1f);
        ImGui::TableSetupColumn("From", 0, 1.5f);
        ImGui::TableSetupColumn("Subject", 0, 5);
        ImGui::TableHeadersRow();
        ImGuiListClipper cl;
        cl.Begin((int)g.cables.size());
        while (cl.Step())
            for (int r = cl.DisplayStart; r < cl.DisplayEnd; r++) {
                int i = (int)g.cables.size() - 1 - r;
                Cable& c = g.cables[i];
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::PushID(i);
                ImGui::PushStyleColor(ImGuiCol_Text, c.read ? DIM : INK);
                if (ImGui::Selectable(((c.read ? "  " : "* ") + fmtShort(c.day)).c_str(), selCable == i, ImGuiSelectableFlags_SpanAllColumns)) { selCable = i; c.read = true; }
                if (r == 0) rec("cable:0");
                ImGui::PopID();
                cell(c.from); cell(c.subject);
                ImGui::PopStyleColor();
            }
        ImGui::EndTable();
    }
    if (Btn("Mark all as read")) for (auto& c : g.cables) c.read = true;
    std::string body = "Select a cable to read it. Unread cables are marked *.";
    if (selCable >= 0 && selCable < (int)g.cables.size()) { Cable& c = g.cables[selCable]; body = "FROM: " + c.from + "\nDATE: " + fmtDay(c.day) + "\nSUBJECT: " + c.subject + "\n\n" + c.body; }
    pane("cabletext", body);
    ImGui::EndChild();
}

// ============================================================================ REPORT
void App::tabReport() {
    GameState& g = sim.g;
    if (reportDay < 0 || !g.reports.count(reportDay)) reportDay = g.reports.empty() ? -1 : g.reports.rbegin()->first;
    if (Btn("< Earlier")) { auto it = g.reports.find(reportDay); if (it != g.reports.end() && it != g.reports.begin()) reportDay = std::prev(it)->first; }
    ImGui::SameLine();
    if (Btn("Later >")) { auto it = g.reports.find(reportDay); if (it != g.reports.end() && std::next(it) != g.reports.end()) reportDay = std::next(it)->first; }
    ImGui::SameLine();
    ImGui::TextUnformatted(reportDay < 0 ? "No reports yet. Plan your day and press END DAY." : ("Report for " + fmtDay(reportDay)).c_str());
    std::string text;
    auto it = g.reports.find(reportDay);
    if (it != g.reports.end()) for (auto& l : it->second) text += l + "\n";
    pane("report", text);
}

// ============================================================================ STATION
void App::tabStation() {
    GameState& g = sim.g;
    float leftW = ImGui::GetContentRegionAvail().x * 0.42f, h = ImGui::GetContentRegionAvail().y;
    pane("station", sim.stationText(), ImVec2(leftW, 0));
    ImGui::SameLine();
    ImGui::BeginChild("staR", ImVec2(0, 0));
    if (ImGui::BeginTable("officers", 4, TBL, ImVec2(0, h * 0.36f))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Officer", 0, 2.2f);
        ImGui::TableSetupColumn("Post", 0, 2.4f);
        ImGui::TableSetupColumn("Assignment", 0, 3.2f);
        ImGui::TableSetupColumn("Cases", 0, 0.7f);
        ImGui::TableHeadersRow();
        static const char* OROLE[] = {"Rezident", "Security (Line KR)", "Technical officer", "Case officer, another line", "Case officer, UNDER YOU"};
        for (int pass = 0; pass < 2; pass++)
            for (auto& o : g.officers) {
                if ((o.role == ROLE_SUB ? 0 : 1) != pass) continue;
                int n = 0;
                for (auto& p : g.persons) if (p.live() && p.handler == o.id) n++;
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::PushID(o.id);
                ImGui::PushStyleColor(ImGuiCol_Text, o.gone ? DIM : INK);
                if (ImGui::Selectable(o.title().c_str(), selOfficer == o.id, ImGuiSelectableFlags_SpanAllColumns)) selOfficer = o.id;
                rec("officer:" + o.name);
                ImGui::PopID();
                cell(OROLE[o.role]);
                cell(o.gone ? "no longer at the station" : o.role == ROLE_SUB ? std::string(ASSIGN_LABEL[o.assign]) + (o.assign == AS_SPOT ? std::string(" - ") + SECTOR_SHORT[o.sector] : std::string("")) : std::string("-"));
                cell(o.role == ROLE_SUB ? fstr("%d", n) : std::string(""));
                ImGui::PopStyleColor();
            }
        ImGui::EndTable();
    }
    Officer* o = sim.officer(selOfficer);
    if (o && !o->gone && o->role == ROLE_SUB) {
        std::vector<Opt> as, sc, pol;
        for (int i = 0; i < 4; i++) as.push_back({ASSIGN_LABEL[i], i});
        for (int i = 0; i < N_SECTOR; i++) sc.push_back({SECTOR_LABEL[i], i});
        for (int i = 0; i < 3; i++) pol.push_back({POLICY[i], i});
        ImGui::AlignTextToFramePadding(); ImGui::TextUnformatted("Assignment"); ImGui::SameLine(110 * S);
        combo("##assign", o->assign, as, 330 * S);
        if (o->assign == AS_SPOT) { ImGui::SameLine(); combo("##osector", o->sector, sc, 250 * S); }
        ImGui::AlignTextToFramePadding(); ImGui::TextUnformatted("His orders"); ImGui::SameLine(110 * S);
        combo("##opolicy", o->policy, pol, 330 * S);
    }
    if (o && !o->gone && o->role != ROLE_REZIDENT && o->role != ROLE_KR) {
        if (o->role == ROLE_SUB) ImGui::SameLine();
        if (Btn("Report him to Directorate K")) {
            int id = o->id;
            confirmText = "Report your suspicions of " + o->title() + " to Directorate K, over the head of the station?\n\nIf you are right, a traitor is removed. If you are wrong, you have accused a comrade and everybody will know it.";
            confirmAction = [this, id]() { if (Officer* x = sim.officer(id)) note = sim.denounce(*x); };
        }
    }
    pane("ofile", o ? sim.officerFile(*o) : std::string("Select an officer.\n\nOfficers under you work on their own judgement within the assignment you give them: an officer on Spotting brings in contacts and develops them; "
                                                         "an officer Handling meets the agents and contacts you have given him (Contacts & Agents, 'Case officer'); an officer on Support forms your countersurveillance team.\n\n"
                                                         "Every officer who watches your back sees where you go and whom you meet. Each file shows what its officer knows of your work."));
    ImGui::EndChild();
}

// ============================================================================ SURVEILLANCE LOG, ACCOUNTS
void App::tabSurveillance() {
    GameState& g = sim.g;
    float leftW = ImGui::GetContentRegionAvail().x * 0.62f;
    if (ImGui::BeginTable("surv", 8, TBL, ImVec2(leftW, 0))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Date", 0, 1.1f);
        ImGui::TableSetupColumn("Time", 0, 1);
        ImGui::TableSetupColumn("District", 0, 1.3f);
        ImGui::TableSetupColumn("Act", 0, 2.4f);
        ImGui::TableSetupColumn("Check made", 0, 3);
        ImGui::TableSetupColumn("Seen", 0, 1.1f);
        ImGui::TableSetupColumn("Outcome", 0, 1.1f);
        ImGui::TableSetupColumn("Zenit that morning", 0, 3.4f);
        ImGui::TableHeadersRow();
        ImGuiListClipper cl;
        cl.Begin((int)g.sdrLog.size());
        while (cl.Step())
            for (int r = cl.DisplayStart; r < cl.DisplayEnd; r++) {
                SdrLog& l = g.sdrLog[g.sdrLog.size() - 1 - r];
                auto z = g.zenit.find(l.day);
                ImGui::TableNextRow();
                cell(fmtShort(l.day)); cell(BLOCK_LABEL[l.block]); cell(sim.T().district[l.district]); cell(l.act); cell(l.route);
                ImGui::TableNextColumn();
                if (l.signs) ImGui::PushStyleColor(ImGuiCol_Text, RED);
                ImGui::TextUnformatted(l.signs == 0 ? "nothing" : l.signs == 1 ? "POSSIBLE" : "CONFIRMED");
                if (l.signs) ImGui::PopStyleColor();
                cell(l.abort ? "ABORTED" : "went ahead"); cell(z == g.zenit.end() ? std::string("") : z->second);
            }
        ImGui::EndTable();
    }
    ImGui::SameLine();
    pane("survtext", sim.survSummary());
}

void App::tabAccounts() {
    GameState& g = sim.g;
    float leftW = ImGui::GetContentRegionAvail().x * 0.55f;
    if (ImGui::BeginTable("acc", 5, TBL, ImVec2(leftW, 0))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Date", 0, 1.2f);
        ImGui::TableSetupColumn("Entry", 0, 6);
        ImGui::TableSetupColumn("Out", 0, 0.9f);
        ImGui::TableSetupColumn("In", 0, 0.9f);
        ImGui::TableSetupColumn("Balance", 0, 1);
        ImGui::TableHeadersRow();
        ImGuiListClipper cl;
        cl.Begin((int)g.ledger.size());
        while (cl.Step())
            for (int r = cl.DisplayStart; r < cl.DisplayEnd; r++) {
                Ledger& l = g.ledger[g.ledger.size() - 1 - r];
                ImGui::TableNextRow();
                cell(fmtShort(l.day)); cell(l.what); cell(l.amt < 0 ? fstr("%d", -l.amt) : std::string("")); cell(l.amt > 0 ? fstr("%d", l.amt) : std::string("")); cell(fstr("%d", l.bal));
            }
        ImGui::EndTable();
    }
    ImGui::SameLine();
    pane("acctext", sim.accSummary());
}

// ============================================================================ POPUPS
void App::popups() {
    GameState& g = sim.g;
    ImVec2 centre = ImGui::GetMainViewport()->GetCenter();
    const float PW = 640 * S;
    orderEditor();

    // ---- radio / instruction to an agent
    if (openInstr) { ImGui::OpenPopup("Instruction to an agent"); openInstr = false; }
    ImGui::SetNextWindowPos(centre, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Instruction to an agent", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        Person* p = sim.person(instrPerson);
        if (!p) ImGui::CloseCurrentPopup();
        else {
            bool viaOfficer = p->handler != 0;
            ImGui::PushTextWrapPos(PW);
            ImGui::TextUnformatted(gx(p, viaOfficer
                ? p->label() + " is run by " + sim.handlerName(*p) + ". He will pass your instruction on at the next meeting."
                : p->label() + (p->radio ? fstr(" has a broadcast schedule and %d pad sheet(s). Centre will transmit the groups within a few days of your cable (60 groups). You will not learn whether he heard them.", p->pads)
                                         : std::string(" has no broadcast schedule. Issue him pad sheets at a meeting first."))).c_str());
            ImGui::PopTextWrapPos();
            std::vector<Opt> tp = {{"(no change)", -1}};
            for (int i = 0; i < 4; i++) tp.push_back({TEMPO[i], i});
            std::vector<Opt> rq = {{"(no change)", -2}, {"General collection (no requirement)", -1}};
            for (auto& q : g.reqs) if (!q.closed && q.kind == RK_COLLECT) rq.push_back({fstr("No. %d  ", q.id) + q.title, q.id});
            combo("Tempo", instrTempo, tp, 380 * S);
            combo("Tasking", instrReq, rq, 380 * S);
            if (!viaOfficer) Check("Summon him to his next meeting site, three evenings after the broadcast", &instrCallout);
            if (Btn("Send")) { note = viaOfficer ? sim.instruct(*p, instrTempo, instrReq) : sim.radioMsg(*p, instrTempo, instrReq, instrCallout); ImGui::CloseCurrentPopup(); }
            ImGui::SameLine();
            if (Btn("Cancel##instr")) ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // ---- new career
    if (openNew) { ImGui::OpenPopup("A new career"); openNew = false; }
    ImGui::SetNextWindowPos(centre, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("A new career", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("Your surname:");
        ImGui::SetNextItemWidth(260 * S);
        ImGui::InputText("##name", nameBuf, sizeof nameBuf);
        ImGui::TextUnformatted("World number (blank = a random world; the same number gives the same people):");
        ImGui::SetNextItemWidth(260 * S);
        ImGui::InputText("##seed", seedBuf, sizeof seedBuf, ImGuiInputTextFlags_CharsDecimal);
        if (Btn("Begin - London, 2 September 1968")) {
            uint64_t seed = (uint64_t)strtoull(seedBuf, nullptr, 10);
            if (seed == 0) seed = (uint64_t)(ImGui::GetTime() * 1000003.0) % 1000000 + (uint64_t)ImGui::GetFrameCount() % 997 + 1;
            sim.newCareer(seed, nameBuf);
            inGame = true;
            selPerson = selSite = selRoll = selReq = selCable = selOfficer = -1; reportDay = -1; selRow = 0; selCol = 1;
            wantTab = 4;                        // two cables are waiting
            autosave();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (Btn("Cancel##new")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    // ---- save
    if (openSave) { ImGui::OpenPopup("Save"); openSave = false; }
    ImGui::SetNextWindowPos(centre, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Save", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("The game saves itself every night (autosave). To keep a named copy as well:");
        ImGui::SetNextItemWidth(300 * S);
        ImGui::InputText("##save", saveBuf, sizeof saveBuf);
        if (Btn("Save##do")) {
            std::string n;
            for (char ch : std::string(saveBuf)) if (isalnum((unsigned char)ch) || ch == '-' || ch == '_' || ch == ' ') n += ch;
            if (n.empty()) n = "posting";
            note = saveTo(saveDir + "/" + n + ".rez") ? "Saved as " + n + ".rez in the saves folder." : std::string("The file could not be written.");
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (Btn("Cancel##save")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    // ---- load
    if (openLoad) {
        loadList.clear();
        std::error_code ec;
        for (auto& e : std::filesystem::directory_iterator(std::filesystem::u8path(saveDir), ec))
            if (e.path().extension() == ".rez") loadList.push_back(e.path().filename().u8string());
        std::sort(loadList.begin(), loadList.end());
        ImGui::OpenPopup("Load");
        openLoad = false;
    }
    ImGui::SetNextWindowPos(centre, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Load", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (loadList.empty()) ImGui::TextUnformatted("There are no saved games in the saves folder.");
        for (auto& f : loadList)
            if (ImGui::Selectable(f.c_str(), false, ImGuiSelectableFlags_DontClosePopups, ImVec2(360 * S, 0))) {
                if (!loadFrom(saveDir + "/" + f)) note = f + " could not be read (it may be from an older version).";
                ImGui::CloseCurrentPopup();
            }
        ImGui::Spacing();
        if (Btn("Cancel##load")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    // ---- confirm
    if (!confirmText.empty() && !ImGui::IsPopupOpen("Confirm")) ImGui::OpenPopup("Confirm");
    ImGui::SetNextWindowPos(centre, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Confirm", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::PushTextWrapPos(PW);
        ImGui::TextUnformatted(confirmText.c_str());
        ImGui::PopTextWrapPos();
        ImGui::Spacing();
        if (Btn("Yes")) { auto f = confirmAction; confirmText.clear(); confirmAction = nullptr; ImGui::CloseCurrentPopup(); if (f) f(); }
        ImGui::SameLine();
        if (Btn("No")) { confirmText.clear(); confirmAction = nullptr; ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }

    // ---- a decision that cannot be put off
    bool otherOpen = ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopup);
    if (inGame && g.phase == PH_PLAY && !g.decisions.empty() && !otherOpen && note.empty()) ImGui::OpenPopup("Decision");
    ImGui::SetNextWindowPos(centre, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Decision", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (g.decisions.empty()) ImGui::CloseCurrentPopup();
        else {
            Decision d = g.decisions[0];
            ImGui::PushStyleColor(ImGuiCol_Text, RED);
            ImGui::TextUnformatted(d.title.c_str());
            ImGui::PopStyleColor();
            ImGui::PushTextWrapPos(PW);
            ImGui::TextUnformatted(d.text.c_str());
            ImGui::PopTextWrapPos();
            ImGui::Spacing();
            for (size_t i = 0; i < d.opts.size(); i++) {
                ImGui::PushID((int)i);
                ImGui::PushTextWrapPos(PW);
                if (ImGui::Button(fstr("%d", (int)i + 1).c_str())) { sim.decide(d.id, (int)i); autosave(); ImGui::CloseCurrentPopup(); }
                rec(fstr("decision:%d", (int)i));
                ImGui::SameLine();
                ImGui::TextUnformatted(d.opts[i].c_str());
                ImGui::PopTextWrapPos();
                ImGui::PopID();
            }
        }
        ImGui::EndPopup();
    }

    // ---- a note
    if (!note.empty() && !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopup)) ImGui::OpenPopup("Rezidentura");
    ImGui::SetNextWindowPos(centre, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Rezidentura", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::PushTextWrapPos(PW);
        ImGui::TextUnformatted(note.c_str());
        ImGui::PopTextWrapPos();
        ImGui::Spacing();
        if (Btn("OK##note") || note.empty()) { note.clear(); ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }
}
