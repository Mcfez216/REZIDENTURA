// ui_tabs.cpp - the desk: the twelve screens behind the folder tabs, and the documents and
// photographs they are made of. Everything is read from sim.g and the text builders in
// sim_text.cpp; the game is changed only through Sim's public functions.
#include "ui.h"
#include "imgui.h"
#include <filesystem>
#include <fstream>
#include "imgui_internal.h"
#include <cfloat>
#include <cmath>

using namespace gfx;

// ============================================================================ small helpers
namespace {

uint32_t hsh(uint32_t a, uint32_t b) {
    uint32_t h = a * 2654435761u ^ (b + 0x9E3779B9u + (a << 6) + (a >> 2));
    h ^= h >> 15; h *= 2246822519u; h ^= h >> 13; h *= 3266489917u; h ^= h >> 16;
    return h;
}
float frac(uint32_t h) { return (float)(h & 0xFFFFFF) / 16777216.f; }

const ImGuiTableFlags TBL = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersInnerH
                            | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp;

void cell(const std::string& s) { ImGui::TableNextColumn(); ImGui::TextUnformatted(s.c_str()); }
void cellCol(const std::string& s, ImU32 c) {
    ImGui::TableNextColumn();
    ImGui::PushStyleColor(ImGuiCol_Text, c);
    ImGui::TextUnformatted(s.c_str());
    ImGui::PopStyleColor();
}
float lineH() { return ImGui::GetFrameHeightWithSpacing(); }
std::string upper(std::string s) {
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = (unsigned char)s[i];
        if (c == 0xC3 && i + 1 < s.size() && (unsigned char)s[i + 1] >= 0xA0 && (unsigned char)s[i + 1] <= 0xBE) { s[i + 1] = (char)((unsigned char)s[i + 1] - 0x20); i++; }  // ä ö ü é ...
        else if (c < 0x80) s[i] = (char)toupper(c);
    }
    return s;
}
bool has(const std::string& s, const char* w) { return s.find(w) != std::string::npos; }
const char* curSym(const char* cur) {
    std::string c = cur;
    return c == "pounds" ? "£" : c == "dollars" ? "$" : c == "marks" ? "DM" : c == "rupees" ? "Rs" : c == "roubles" ? "руб" : "S";
}
/** A line typed all in capitals is a heading. */
bool heading(const std::string& l) {
    if (l.size() < 3 || l[0] == ' ') return false;
    int letters = 0;
    for (char c : l) { if (islower((unsigned char)c)) return false; if (isupper((unsigned char)c)) letters++; }
    return letters >= 3;
}

/** Buttons laid out left to right, wrapping onto a new line when the row is full. */
struct Flow {
    bool first = true;
    bool btn(const char* label) {
        float w = ImGui::CalcTextSize(label, nullptr, true).x + ImGui::GetStyle().FramePadding.x * 2;
        if (!first) {
            ImGui::SameLine();
            if (w > ImGui::GetContentRegionAvail().x) ImGui::NewLine();
        }
        first = false;
        return uiBtn(label);
    }
    void item(float w) {
        if (!first) {
            ImGui::SameLine();
            if (w > ImGui::GetContentRegionAvail().x) ImGui::NewLine();
        }
        first = false;
    }
};

/** Smooth noise, for streets that are not ruled with a ruler. */
float vnoise(float x, float y, uint32_t seed) {
    int xi = (int)std::floor(x), yi = (int)std::floor(y);
    float fx = x - xi, fy = y - yi;
    auto h = [&](int i, int j) { return frac(hsh(seed, (uint32_t)(i * 73856093) ^ (uint32_t)(j * 19349663))); };
    float sx = fx * fx * (3 - 2 * fx), sy = fy * fy * (3 - 2 * fy);
    float a = h(xi, yi) + (h(xi + 1, yi) - h(xi, yi)) * sx;
    float b = h(xi, yi + 1) + (h(xi + 1, yi + 1) - h(xi, yi + 1)) * sx;
    return a + (b - a) * sy;
}

/** A wobbly pencil ring. */
void pencilRing(ImDrawList* dl, ImVec2 c, float rx, float ry, ImU32 col, float th, uint32_t seed) {
    ImVec2 pts[40];
    for (int i = 0; i < 40; i++) {
        float a = i / 40.f * 6.6f + frac(seed) * 6.28f;         // a little more than a full turn: the pencil overshoots
        float k = 1 + (frac(hsh(seed, (uint32_t)i)) - 0.5f) * 0.10f;
        pts[i] = c + ImVec2(std::cos(a) * rx * k, std::sin(a) * ry * k);
    }
    dl->AddPolyline(pts, 40, col, 0, th);
}

/** A ruled or plain index card, drawn in the parent window behind a child. */
void lined(ImDrawList* dl, ImVec2 a, ImVec2 b, float S, float first) {
    for (float y = a.y + first; y < b.y - 6 * S; y += 18 * S) dl->AddLine(ImVec2(a.x + 4 * S, y), ImVec2(b.x - 4 * S, y), rgb(0x7A9CC6, 0.30f), 1);
    dl->AddLine(ImVec2(a.x + 4 * S, a.y + first - 18 * S + 2 * S), ImVec2(b.x - 4 * S, a.y + first - 18 * S + 2 * S), rgb(C_PENCIL_RED, 0.45f), 1);
}

}  // namespace

// ============================================================================ documents
bool App::docPanel(const char* id, ImVec2 size, unsigned colour, float pad, const char* title, const char* hint, ImGuiWindowFlags flags) {
    ImVec2 avail = ImGui::GetContentRegionAvail();
    if (size.x <= 0) size.x = std::max(20.f, avail.x + size.x);
    if (size.y <= 0) size.y = std::max(20.f, avail.y + size.y);
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    skin::frame(dl, p, p + size, S);
    float top = title ? skin::titleBar(dl, p, size.x, S, title, hint) + 1 : std::floor(6 * S);
    float in = std::floor(6 * S);
    ImVec2 ca = p + ImVec2(in, top), cb = p + size - ImVec2(in, in);
    bool paper = isPaper(colour);
    int n = 0;
    if (paper) {
        // a document: a sheet of paper laid in the frame, with ink colours
        ca = ca + ImVec2(4 * S, 4 * S);
        cb = cb - ImVec2(4 * S, 4 * S);
        sheet(dl, ca, cb, rgb(colour), S, true);
        ImGui::PushStyleColor(ImGuiCol_Text, rgb4(C_INK)); n++;
        ImGui::PushStyleColor(ImGuiCol_TextDisabled, rgb4(C_INK_FADED)); n++;
        ImGui::PushStyleColor(ImGuiCol_FrameBg, rgb4(0xFBF8EF, 0.9f)); n++;
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, rgb4(0xFFFDF6)); n++;
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, rgb4(0xF1E9D4)); n++;
        ImGui::PushStyleColor(ImGuiCol_Border, rgb4(0x8E8062, 0.55f)); n++;
        ImGui::PushStyleColor(ImGuiCol_CheckMark, rgb4(C_RED)); n++;
        ImGui::PushStyleColor(ImGuiCol_Header, rgb4(0xE9D9A6, 0.9f)); n++;
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, rgb4(0xEADFBF, 0.7f)); n++;
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, rgb4(0xE3CF92)); n++;
        ImGui::PushStyleColor(ImGuiCol_Separator, rgb4(0x8E8062, 0.45f)); n++;
        ImGui::PushStyleColor(ImGuiCol_TableBorderLight, rgb4(0x8E8062, 0.22f)); n++;
        ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, rgb4(0x8E8062, 0.55f)); n++;
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, rgb4(0x8E8062, 0.45f)); n++;
        ImGui::PushStyleColor(ImGuiCol_Button, rgb4(0xEFE5CB)); n++;
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, rgb4(C_HIGHLIGHT, 0.6f)); n++;
    }
    pushInk(!paper);
    docStack.push_back({p, size, n});
    ImGui::SetCursorScreenPos(ca);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(pad * S, pad * S));
    bool r = ImGui::BeginChild(id, cb - ca, ImGuiChildFlags_AlwaysUseWindowPadding, flags | ImGuiWindowFlags_NoBackground);
    ImGui::PopStyleVar();
    return r;
}
void App::docEnd() {
    ImGui::EndChild();
    if (docStack.empty()) return;
    DocFrame f = docStack.back();
    docStack.pop_back();
    popInk();
    ImGui::PopStyleColor(f.colours);
    // the layout continues from the frame, not from the child inside it
    ImGui::SetCursorScreenPos(f.a);
    ImGui::Dummy(f.size);
}
void App::docTitle(const char* /*ru*/, const char* title, const char* hint) {
    float y0 = ImGui::GetCursorPosY();
    ImGui::PushFont(F.uiTitle);
    ImGui::TextColored(darkInk() ? rgb4(skin::K_BRASS_HI) : rgb4(0x2A2018), "%s", title);
    ImGui::PopFont();
    if (hint && *hint) {
        ImGui::SameLine(0, 12 * S);
        ImGui::SetCursorPosY(std::floor(y0 + F.uiTitle->Ascent - F.uiSmall->Ascent + 0.5f));
        ImGui::PushFont(F.uiSmall);
        ImGui::TextColored(rgb4(C_INK_FADED), "%s", hint);
        ImGui::PopFont();
    }
    ImGui::SetCursorPosY(std::max(ImGui::GetCursorPosY(), y0 + F.uiTitle->FontSize + ImGui::GetStyle().ItemSpacing.y));
    ImVec2 a = ImGui::GetCursorScreenPos();
    float w = ImGui::GetContentRegionAvail().x;
    ImGui::GetWindowDrawList()->AddLine(a + ImVec2(0, 1 * S), a + ImVec2(w, 1 * S), rgb(C_INK, 0.28f), 1.0f * S);
    ImGui::Dummy(ImVec2(w, 6 * S));
}

// ============================================================================ photographs
int App::era() const {
    int y = dateOf(sim.g.postStart).y;
    return y < 1971 ? art::ERA_60S : y < 1981 ? art::ERA_70S : art::ERA_80S;
}
ImTextureID App::facePhoto(const Person& p) {
    art::Face f;
    f.seed = hsh((uint32_t)sim.g.seed, (uint32_t)p.id);
    f.female = p.female; f.age = p.age; f.era = era();
    if (sim.g.theatre == TH_DELHI) {                       // Indian names: a darker print; Western diplomats keep the default
        std::string last = p.name.substr(p.name.rfind(' ') + 1);
        for (auto n : sim.T().last) if (last == n) f.skin = 0.55f;
    }
    return tex(fstr("pf:%llu:%d", (unsigned long long)sim.g.seed, p.id), [f] { return art::portrait(f, 240, 320); });
}
ImTextureID App::candidPhoto(const Person& p) {
    uint32_t s = hsh((uint32_t)sim.g.seed, (uint32_t)p.id + 7777);
    int e = era();
    return tex(fstr("pc:%llu:%d", (unsigned long long)sim.g.seed, p.id), [s, e] { return art::street(s, art::SC_CANDID, e, 352, 264); });
}
ImTextureID App::officerPhoto(const Officer& o) {
    art::Face f;
    f.seed = hsh((uint32_t)sim.g.seed, (uint32_t)o.id + 100000);
    f.age = 30 + o.rank * 6 + (int)(hsh(f.seed, 3) % 6); f.uniform = true; f.era = era();
    return tex(fstr("po:%llu:%d", (unsigned long long)sim.g.seed, o.id), [f] { return art::portrait(f, 240, 320); });
}
ImTextureID App::myPhoto() {
    art::Face f;
    f.seed = hsh((uint32_t)sim.g.seed, 424242);
    f.age = 29 + std::max(0, dateOf(sim.g.postStart).y - 1968); f.uniform = true; f.era = era();
    return tex(fstr("pm:%llu:%d:%d", (unsigned long long)sim.g.seed, sim.g.rank, f.era), [f] { return art::portrait(f, 240, 320); });
}
ImTextureID App::sitePhoto(const Site& s) {
    static const int SCENE[N_SITETYPE] = {art::SC_MEET, art::SC_DROP, art::SC_SIGNAL, art::SC_ROUTE};
    uint32_t seed = hsh((uint32_t)sim.g.seed, (uint32_t)s.id + 50000);
    int sc = s.flat ? art::SC_FLAT : SCENE[s.type], e = era();
    return tex(fstr("ps:%llu:%d", (unsigned long long)sim.g.seed, s.id), [seed, sc, e] { return art::street(seed, sc, e, 352, 264); });
}
ImTextureID App::scenePhoto(int key, int scene) {
    uint32_t seed = hsh((uint32_t)sim.g.seed, (uint32_t)key * 31 + (uint32_t)scene);
    int e = era();
    return tex(fstr("pk:%llu:%d:%d", (unsigned long long)sim.g.seed, key, scene), [seed, scene, e] { return art::street(seed, scene, e, 352, 264); });
}

// ============================================================================ DAY PLANNER
void App::instructorCard(float width) {
    GameState& g = sim.g;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 fa = ImGui::GetCursorScreenPos();
    ImVec2 fb = fa + ImVec2(width, ImGui::GetContentRegionAvail().y);
    // the card, pinned inside a frame of its own
    skin::frame(dl, fa, fb, S);
    float bh = skin::titleBar(dl, fa, width, S, "Field exercise", "the instructor's card");
    ImVec2 a = fa + ImVec2(10 * S, bh + 10 * S), b = fb - ImVec2(10 * S, 10 * S);
    width = b.x - a.x;
    sheet(dl, a, b, rgb(0xF8F4E4), S, true);
    lined(dl, a, b, S, 70 * S);
    pushpin(dl, ImVec2(a.x + width / 2, a.y + 10 * S), S, rgb(C_RED));
    dl->AddText(F.stamp, F.stamp->FontSize, a + ImVec2(16 * S, 18 * S), rgb(C_INK), "ВЫСШАЯ ШКОЛА КГБ  -  ПРАКТИЧЕСКИЕ ЗАНЯТИЯ");
    dl->AddText(F.tiny, F.tiny->FontSize, a + ImVec2(16 * S, 18 * S + F.stamp->FontSize), rgb(C_INK_FADED), "The instructor's card: today's field exercise");
    ImGui::SetCursorScreenPos(a + ImVec2(0, 56 * S));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(18 * S, 8 * S));
    ImGui::BeginChild("card", ImVec2(width, b.y - a.y - 56 * S), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoBackground);
    ImGui::PopStyleVar();
    uiTyped(g.tutText.empty() ? std::string("The course is over.") : g.tutText, rgb(C_INK), F.small, [](const std::string& l, ImU32& c, ImFont*& f) {
        if (has(l, "EXERCISE")) { f = F.bold; c = rgb(C_PENCIL_BLUE); }
        else if (has(l, "Today's order") || has(l, "TODAY") || has(l, "THIS ")) c = rgb(C_RED);
    }, 21);
    if (!g.tutLog.empty()) {
        ImGui::Dummy(ImVec2(1, 8 * S));
        ImGui::PushFont(F.hand);
        ImGui::TextColored(rgb4(C_PENCIL_BLUE), "Marks so far:");
        for (auto& l : g.tutLog) ImGui::TextColored(rgb4(C_PENCIL_BLUE), "%s", l.c_str());
        ImGui::PopFont();
    }
    ImGui::EndChild();
}

void App::tabPlanner() {
    GameState& g = sim.g;
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float leftW = std::floor(avail.x * 0.63f);
    if (docPanel("planDoc", ImVec2(leftW, avail.y), C_LEDGER, 12, "Day planner", "Double-click a block to give an order")) {
        float buttons = lineH() * 2 + 6 * S;
        ImGui::PushFont(F.ui);
        if (ImGui::BeginTable("plan", 5, TBL, ImVec2(0, ImGui::GetContentRegionAvail().y - buttons))) {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Date", ImGuiTableColumnFlags_WidthFixed, 132 * S);
            ImGui::TableSetupColumn("Morning", 0, 3);
            ImGui::TableSetupColumn("Afternoon", 0, 3);
            ImGui::TableSetupColumn("Evening", 0, 3);
            ImGui::TableSetupColumn("Diary", 0, 2.3f);
            uiHeaders();
            for (int r = 0; r < 35; r++) {
                int day = g.day + r;
                if (day > g.postEnd) break;
                ImGui::TableNextRow();
                if (r == 0) ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, rgb(C_HIGHLIGHT, 0.22f));
                Date d = dateOf(day);
                bool hol = !sim.holiday(day).empty();
                if (hol || day <= g.busyUntil || day <= g.standDownUntil || sim.onLeave(day)) ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, hol ? rgb(0xB01E1E, 0.08f) : rgb(C_VIOLET, 0.08f));
                cellCol(fmtDay(day), d.dow == 6 || hol ? rgb(0xB01E1E) : d.dow == 5 ? rgb(C_INK_FADED) : rgb(C_INK));
                for (int b = 0; b < 3; b++) {
                    ImGui::TableNextColumn();
                    Order o = sim.order(day, b);
                    bool set = sim.isSet(day, b);
                    // what kind of work it is shows in the ink: street work in red pencil, detection routes in blue
                    bool street = Sim::isOp(o.type) || o.type == O_READ_SIGNAL || o.type == O_PLACE_SIGNAL;
                    bool hot = o.type == O_MEET_CLAN || o.type == O_LOAD_DROP || o.type == O_UNLOAD_DROP || o.type == O_LINE_N;
                    bool bare = set && hot && !o.cs && (b == 0 || sim.order(day, b - 1).type != O_SDR);
                    ImU32 ink = o.appointment ? rgb(C_RED) : !set ? rgb(C_INK_FADED, 0.75f) : street ? rgb(C_PENCIL_RED) : o.type == O_SDR ? rgb(C_PENCIL_BLUE) : rgb(C_INK);
                    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(ink));
                    std::string label = std::string(bare ? "! " : "") + sim.describe(o) + fstr("##p%d_%d", r, b);
                    if (ImGui::Selectable(label.c_str(), selRow == r && selCol == b + 1, ImGuiSelectableFlags_AllowDoubleClick)) {
                        selRow = r; selCol = b + 1;
                        if (ImGui::IsMouseDoubleClicked(0)) beginEdit(day, b);
                    }
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && cfg.tooltips && set) {
                        std::string tip = sim.describe(o) + "\n\n" + ORDER_HINT[o.type];
                        if (bare) tip += "\n\n! No detection route in the block before: only the short check en route will show whether you are followed.";
                        if (o.cs) tip += "\n\nA countersurveillance team covers this act.";
                        ImGui::BeginTooltip();
                        ImGui::PushTextWrapPos(420 * S);
                        ImGui::TextUnformatted(tip.c_str());
                        ImGui::PopTextWrapPos();
                        ImGui::EndTooltip();
                    }
                    uiRec(fstr("plan:%d:%d", r, b));
                    ImGui::PopStyleColor();
                }
                cellCol(sim.diary(day), rgb(C_INK_FADED));
            }
            ImGui::EndTable();
        }
        ImGui::PopFont();
        int day = g.day + selRow, b = selCol - 1;
        Flow fl;
        if (fl.btn("Edit order...")) beginEdit(day, b);
        if (fl.btn("Back to routine")) {
            if (sim.isSet(day, b) && sim.order(day, b).appointment) note = "This is an agreed meeting. The agent will be there whether you are or not. If you must miss it, leave it and accept the consequences - or abort on the day.";
            else sim.clearOrder(day, b);
        }
        if (fl.btn("Copy")) {
            Order o = sim.order(day, b);
            if (o.appointment) note = "An agreed meeting cannot be copied: its date and place belong to the agent's communications plan.";
            else { copied = o; hasClip = true; }
        }
        if (fl.btn("Paste")) {
            if (!hasClip) note = "Copy an order first.";
            else { std::string e = place(copied, day, b); if (!e.empty()) note = e; }
        }
        if (fl.btn("Repeat for 3 weeks")) {
            Order o = sim.order(day, b);
            if (o.appointment) note = "An agreed meeting repeats by itself, under the agent's communications plan.";
            else {
                std::string skipped;
                for (int k = 1; k <= 3; k++) { std::string e = place(o, day + 7 * k, b); if (!e.empty()) skipped += fmtDay(day + 7 * k) + ": " + e + "\n"; }
                if (!skipped.empty()) note = "Not placed:\n" + skipped;
            }
        }
        if (!sim.training() && fl.btn("Book a countersurveillance team")) note = sim.requestCS(day);
        if (!sim.training()) {
            if (fl.btn(g.deputy ? "Deputy: ON###deputy" : "Deputy: OFF###deputy")) {
                g.deputy = !g.deputy;
                if (g.deputy) { g.deputyDay = -1; sim.deputyPlan(); note = sim.deputyLog.empty() ? "Your deputy will put the routine in order each morning." : sim.deputyLog; }
            }
            if (ImGui::IsItemHovered() && cfg.tooltips)
                ImGui::SetTooltip("%s", "Your deputy puts the routine in order each morning, in blocks you have left to routine:\nstores on order, the agenda of agreed meetings, a detection route before every act,\nreports written up, rest when you are tired, signal sites read. He never changes an order you gave.\nWhat he did is in the morning brief and the daily report.");
        }
        std::string selTxt = "  " + fmtDay(day) + ", " + BLOCK_LABEL[std::max(0, b)];
        ImGui::SameLine();
        if (ImGui::CalcTextSize(selTxt.c_str()).x > ImGui::GetContentRegionAvail().x) ImGui::NewLine();
        ImGui::TextDisabled("%s", selTxt.c_str());
    }
    docEnd();
    ImGui::SameLine(0, 16 * S);
    ImGui::BeginGroup();
    float rw = ImGui::GetContentRegionAvail().x;
    if (sim.training()) instructorCard(rw);
    else {
        float opH = 0;
        if (Operation* op = sim.activeOp()) {     // the operation under way: a folder pinned above the brief
            auto st = sim.opStages(*op);
            opH = std::floor((112 + 25 * st.size() + (op->kind == OP_LEAK ? 40 : 0)) * S);
            std::string hint = fstr("%d days left", std::max(0, op->due - g.day));
            if (docPanel("opCard", ImVec2(rw, opH), C_MANILA, 14, ("Operation " + op->name).c_str(), hint.c_str(), ImGuiWindowFlags_NoScrollbar)) {
                ImDrawList* odl = ImGui::GetWindowDrawList();
                ImGui::PushFont(F.uiBold);
                ImGui::PushTextWrapPos(0);
                ImGui::TextUnformatted(op->title.c_str());
                ImGui::PopTextWrapPos();
                ImGui::PopFont();
                ImGui::Dummy(ImVec2(1, 2 * S));
                bool next = true;
                for (auto& s2 : st) {
                    ImVec2 q = ImGui::GetCursorScreenPos();
                    odl->AddRect(q + ImVec2(2 * S, 4 * S), q + ImVec2(14 * S, 16 * S), rgb(C_INK), 0, 0, 1.4f * S);
                    if (s2.second) {         // a tick in red pencil
                        odl->AddLine(q + ImVec2(3 * S, 9 * S), q + ImVec2(7 * S, 14 * S), rgb(C_PENCIL_RED), 2.2f * S);
                        odl->AddLine(q + ImVec2(7 * S, 14 * S), q + ImVec2(16 * S, 1 * S), rgb(C_PENCIL_RED), 2.2f * S);
                    }
                    ImU32 col = s2.second ? rgb(C_INK_FADED) : next ? rgb(C_RED) : rgb(C_INK);
                    odl->AddText(F.ui, F.ui->FontSize, q + ImVec2(22 * S, 1 * S), col, s2.first.c_str());
                    if (!s2.second) next = false;
                    ImGui::Dummy(ImVec2(1, 22 * S));
                }
                if (op->kind == OP_LEAK && op->stage < 9) {
                    if (uiBtn("Name the traitor...")) { std::string r = sim.nameTraitor(); if (!r.empty()) note = r; }
                }
                stamp(odl, ImGui::GetWindowPos() + ImVec2(ImGui::GetWindowWidth() - 70 * S, 52 * S), "СЕКРЕТНО", rgb(C_RED, 0.55f), 8, F.stamp, 15 * S, 1);
                if (ImGui::IsWindowHovered() && cfg.tooltips) ImGui::SetTooltip("%s", (op->brief + "\n\n" + op->stakes).c_str());
            }
            docEnd();
            opH += 10 * S;
        }
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos();
        if (docPanel("brief", ImVec2(rw, avail.y - opH), C_TELETYPE, 16, "Morning brief", nullptr)) {
            uiTyped(g.brief, rgb(C_INK), F.body, [](const std::string& l, ImU32& c, ImFont*& f) {
                if (heading(l) || has(l, "ZENIT") || has(l, "LINE KR") || l.rfind("TODAY", 0) == 0) f = F.bold;
                if (has(l, "HEAVY") || has(l, "priority target") || has(l, "arrears") || has(l, "over-tired") || has(l, "stand him down")) c = rgb(C_RED);
            }, (uint32_t)g.day);
        }
        docEnd();
        stamp(dl, p + ImVec2(rw - 100 * S, 84 * S), "ЗЕНИТ", rgb(C_VIOLET, 0.7f), 7, F.stamp, 20 * S, 0, "INTERCEPT");
    }
    ImGui::EndGroup();
}

// ============================================================================ CONTACTS & AGENTS
void App::dossierView(const Person& p) {
    GameState& g = sim.g;
    ImDrawList* pdl = ImGui::GetWindowDrawList();
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImVec2 a = ImGui::GetCursorScreenPos(), b = a + avail;
    // the folder, with its tab
    std::string tabText = p.codename.empty() ? fstr("ДЕЛО № %d", 4000 + p.id) : "«" + ru(p.codename) + "»";
    float tw = F.stamp->CalcTextSizeA(F.stamp->FontSize, FLT_MAX, 0, tabText.c_str()).x + 30 * S;
    pdl->AddRectFilled(a + ImVec2(26 * S, 2 * S), a + ImVec2(26 * S + tw, 26 * S), rgb(C_MANILA_DK), 6 * S, ImDrawFlags_RoundCornersTop);
    pdl->AddText(F.stamp, F.stamp->FontSize, a + ImVec2(41 * S, 4 * S), rgb(C_INK), tabText.c_str());
    sheet(pdl, a + ImVec2(0, 24 * S), b, rgb(C_MANILA_DK), S, true);
    ImGui::SetCursorScreenPos(a + ImVec2(12 * S, 34 * S));
    if (docBegin("dossier", ImVec2(avail.x - 22 * S, avail.y - 42 * S), C_PAPER, 18)) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 o = ImGui::GetCursorScreenPos();
        float w = ImGui::GetContentRegionAvail().x;
        ImU32 ink = rgb(C_INK), faded = rgb(C_INK_FADED);
        float hs = 14 * S;
        hammerSickle(dl, o + ImVec2(15 * S, 15 * S), 30 * S, rgb(C_INK, 0.85f));
        dl->AddText(F.stamp, hs, o + ImVec2(36 * S, 0), ink, "КОМИТЕТ ГОСУДАРСТВЕННОЙ БЕЗОПАСНОСТИ СССР");
        dl->AddText(F.stamp, hs, o + ImVec2(36 * S, hs + 1 * S), ink, "ПЕРВОЕ ГЛАВНОЕ УПРАВЛЕНИЕ");
        auto right = [&](const char* t, float y, ImU32 c) {
            float tx = F.stamp->CalcTextSizeA(hs, FLT_MAX, 0, t).x;
            dl->AddText(F.stamp, hs, ImVec2(o.x + w - tx, y), c, t);
        };
        right("СОВЕРШЕННО СЕКРЕТНО", o.y, rgb(C_RED));
        right("Экз. единственный", o.y + hs + 1 * S, faded);
        bool agentFile = !p.codename.empty();
        const char* kind = agentFile ? "ЛИЧНОЕ ДЕЛО АГЕНТА" : p.live() ? "ДЕЛО ОПЕРАТИВНОЙ РАЗРАБОТКИ" : "ДЕЛО ОПЕРАТИВНОЙ РАЗРАБОТКИ (ЗАКРЫТО)";
        std::string big = agentFile ? "«" + ru(p.codename) + "»" : fstr("№ %d", 4000 + p.id);
        float ky = o.y + 2 * hs + 12 * S;
        float kx = F.stamp->CalcTextSizeA(F.stamp->FontSize, FLT_MAX, 0, kind).x;
        dl->AddText(F.stamp, F.stamp->FontSize, ImVec2(o.x + (w - kx) / 2, ky), ink, kind);
        float bx = F.stampBig->CalcTextSizeA(F.stampBig->FontSize, FLT_MAX, 0, big.c_str()).x;
        dl->AddText(F.stampBig, F.stampBig->FontSize, ImVec2(o.x + (w - bx) / 2, ky + F.stamp->FontSize), ink, big.c_str());
        float y = ky + F.stamp->FontSize + F.stampBig->FontSize + 14 * S;
        // the photograph
        bool face = agentFile || p.stage == ARRESTED || p.meetings >= 3;
        ImVec2 ps = face ? ImVec2(120 * S, 160 * S) : ImVec2(168 * S, 126 * S);
        ImVec2 pp(o.x + 10 * S, y + 22 * S);
        photo(dl, face ? facePhoto(p) : candidPhoto(p), pp, ps, face ? -2.f : 1.5f, S, true);
        if (!face) pencil(dl, pp + ImVec2(4 * S, ps.y + 12 * S), "снимок наружки, " + fmtShort(std::max(p.firstDay, g.postStart)), rgb(C_PENCIL_BLUE, 0.85f), -1.5f, F.hand, 15 * S);
        // the particulars
        float fx = o.x + ps.x + 44 * S, fy = y + 2 * S, lw = 140 * S, rowH = 29 * S;
        Date today = dateOf(g.day);
        struct Field { const char* ruL; const char* en; std::string v; };
        std::vector<Field> fields = {
            {"ФАМИЛИЯ, ИМЯ", "Name", p.name},
            {"ДАТА РОЖДЕНИЯ", "Born", [&] { Date bd = dateOf(sim.birthday(p, sim.birthYear(p) + 40)); int age = today.y - sim.birthYear(p) - (today.m * 40 + today.d < bd.m * 40 + bd.d ? 1 : 0);
                                             return fstr("%d %s %d  (age %d)", bd.d, MON[bd.m], sim.birthYear(p), age); }()},
            {"ДОЛЖНОСТЬ", "Post", p.job},
            {"МЕСТО РАБОТЫ", "Employer", p.employer},
            {"ЛИНИЯ", "Line", SECTOR_LABEL[p.sector]},
            {"ПРОЖИВАЕТ", "Lives", sim.T().district[p.district]},
            {"ПСЕВДОНИМ", "Codename", agentFile ? p.codename + "  (" + ru(p.codename) + ")" : std::string("-")},
            {"НА СВЯЗИ У", "Run by", sim.handlerName(p)}};
        dl->PushClipRect(ImVec2(fx, fy - 4 * S), ImVec2(o.x + w, fy + rowH * fields.size() + 4 * S), true);
        for (auto& f : fields) {
            float line = std::floor(fy + rowH - 3 * S);                                  // the ruled line the entry is typed on
            dl->AddText(F.stamp, 14 * S, ImVec2(fx, line - 14 * S - F.tiny->FontSize), faded, f.ruL);
            dl->AddText(F.tiny, F.tiny->FontSize, ImVec2(fx, line - F.tiny->FontSize - 1 * S), faded, f.en);
            typeLine(dl, F.body, F.body->FontSize, ImVec2(fx + lw, line - F.body->Ascent - 3 * S), ink, f.v.c_str(), f.v.c_str() + f.v.size(), hsh((uint32_t)p.id, (uint32_t)fy));
            for (float x = fx + lw; x < o.x + w; x += 4 * S) dl->AddLine(ImVec2(x, line), ImVec2(x + 1.5f * S, line), rgb(C_INK, 0.35f), 1);
            fy += rowH;
        }
        dl->PopClipRect();
        // stamps and pencil
        int n = 0;
        auto st = [&](const char* t, const char* sub, unsigned col) {
            // the first two either side of the heading, any more in the margin under the photograph
            ImVec2 at = n == 0 ? ImVec2(o.x + w - 120 * S, ky + 34 * S) : n == 1 ? ImVec2(o.x + 120 * S, ky + 40 * S) : ImVec2(o.x + w - 140 * S - (n - 2) * 40 * S, y + 236 * S + (n - 2) * 50 * S);
            stamp(dl, at, t, rgb(col, 0.78f), n % 2 ? 6.f : -8.f, F.stamp, 24 * S, n % 2 ? 0 : 1, sub);
            n++;
        };
        if (p.stage == AGENT) st("ЗАВЕРБОВАН", "RECRUITED", C_RED);
        if (p.stage == ARRESTED) st("АРЕСТОВАН", "ARRESTED", C_RED);
        if (p.stage == TERMINATED || p.stage == BROKEN) st("СВЯЗЬ ПРЕКРАЩЕНА", p.stage == BROKEN ? "CONTACT LOST" : "TERMINATED", C_INK);
        if (p.stage == EXFILTRATED) st("ВЫВЕДЕН", "EXFILTRATED", C_VIOLET);
        if (p.walkIn) st("ИНИЦИАТИВНИК", "WALK-IN", C_VIOLET);
        if (p.service != SVC_NONE) st("СОТРУДНИК СПЕЦСЛУЖБЫ", "OFFICER OF A FOREIGN SERVICE", C_VIOLET);
        if (p.kompState == 3) st("КОМПРОМАТ", "MATERIAL HELD", C_RED);
        if (p.play == 2) st("ОПЕРАТИВНАЯ ИГРА", "PLAYED BACK", C_RED);
        if (p.confid == 2 && p.stage != AGENT) st("ДОВЕРИТЕЛЬНАЯ СВЯЗЬ", "CONFIDENTIAL CONTACT", C_VIOLET);
        if (p.verify == 3) st(p.verifyResult == "CONSISTENT" ? "ПРОВЕРЕН" : "ДЕЗИНФОРМАЦИЯ", p.verifyResult == "CONSISTENT" ? "VERIFIED" : p.verifyResult == "INVENTED" ? "INVENTED MATERIAL" : "FALSE ANSWERS",
                             p.verifyResult == "CONSISTENT" ? C_VIOLET : C_RED);
        if (p.traceBad || p.ciBy >= 0) {
            st("ВНИМАНИЕ", p.traceBad ? "ADVERSE TRACE" : "NAMED BY A SOURCE", C_RED);
            pencil(dl, ImVec2(o.x + w - 170 * S, y + 4 * S), "проверить!", rgb(C_PENCIL_RED), -5, F.handBig);
        }
        float headH = std::max(ps.y + 50 * S, rowH * fields.size() + 16 * S) + (y - o.y);
        ImGui::Dummy(ImVec2(w, headH));
        ImVec2 r0 = ImGui::GetCursorScreenPos();
        dl->AddLine(r0, r0 + ImVec2(w, 0), rgb(C_INK, 0.5f), 1);
        ImGui::Dummy(ImVec2(w, 6 * S));
        uiTyped(sim.dossier(p, false), rgb(C_INK), F.body, [](const std::string& l, ImU32& c, ImFont*& f) {
            if (heading(l)) f = F.bold;
            if (has(l, "ADVERSE") || has(l, "NOTE:") || has(l, "SILENT") || has(l, "CANNOT") || has(l, "NO CAMERA") || has(l, "PASSPORT") || has(l, "Failed meetings")
                || has(l, "FALSE") || has(l, "INVENTED")) c = rgb(C_RED);
        }, (uint32_t)p.id);
    }
    docEnd();
}

void App::tabContacts() {
    GameState& g = sim.g;
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float leftW = std::floor(avail.x * 0.50f);
    Person* sel = sim.person(selPerson);
    if (docPanel("perDoc", ImVec2(leftW, avail.y), C_CARD, 12, "Contacts and agents", nullptr)) {
        float controls = lineH() * (sel ? 4.3f : 1.3f) + 8 * S;
        uiText("##filter", filter, ImVec2(std::min(360 * S, ImGui::GetContentRegionAvail().x), 0), false, "find: a name, a codename, an employer...");
        std::string want;
        for (char ch : filter) want += (char)tolower((unsigned char)ch);
        auto matches = [&](const Person& q) {
            if (want.empty()) return true;
            std::string hay = q.name + " " + q.codename + " " + q.job + " " + q.employer + " " + SECTOR_SHORT[q.sector] + " " + STAGE_LABEL[q.stage];
            for (auto& ch : hay) ch = (char)tolower((unsigned char)ch);
            return hay.find(want) != std::string::npos;
        };
        ImGui::PushFont(F.ui);
        if (ImGui::BeginTable("per", 6, TBL, ImVec2(0, ImGui::GetContentRegionAvail().y - controls))) {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Name / codename", 0, 3.0f);
            ImGui::TableSetupColumn("Status", 0, 1.4f);
            ImGui::TableSetupColumn("Line", 0, 1.1f);
            ImGui::TableSetupColumn("Relations", 0, 1.3f);
            ImGui::TableSetupColumn("Run by", 0, 1.7f);
            ImGui::TableSetupColumn("Next meeting", 0, 1.4f);
            uiHeaders();
            for (int pass = 0; pass < 3; pass++)
                for (auto& p : g.persons) {
                    int k = p.stage == AGENT ? 0 : (p.live() ? 1 : 2);
                    if (k != pass || (k == 2 && !showClosed) || !matches(p)) continue;
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::PushID(p.id);
                    ImGui::PushStyleColor(ImGuiCol_Text, k == 2 ? rgb4(C_INK_FADED) : (k == 0 ? rgb4(C_RED) : rgb4(C_INK)));
                    if (ImGui::Selectable(p.full().c_str(), selPerson == p.id, ImGuiSelectableFlags_SpanAllColumns)) selPerson = p.id;
                    ImGui::PopStyleColor();
                    uiRec("person:" + p.name);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s, %s", p.job.c_str(), p.employer.c_str());
                    ImGui::PopID();
                    cell(STAGE_LABEL[p.stage]);
                    cell(SECTOR_SHORT[p.sector]);
                    cell(p.live() ? word(p.rapport, W_RAPPORT, 5) : std::string("-"));
                    cell(sim.handlerName(p));
                    cell(p.stage == AGENT && p.nextMeet >= 0 ? fmtShort(p.nextMeet) : std::string(""));
                }
            ImGui::EndTable();
        }
        ImGui::PopFont();
        if (sel) {
            // what can be done with this file: the usual things as buttons, the rest under "More actions"
            struct Act { std::string label; bool main; std::function<void()> go; };
            std::vector<Act> acts;
            int id = sel->id;
            if (sel->stage == AGENT) acts.push_back({sel->handler ? "Instruct..." : "Radio instruction...", true, [this, id] { instrPerson = id; instrTempo = -1; instrReq = -2; instrCallout = false; openInstr = true; }});
            if (sel->live()) {
                acts.push_back({"Request trace", sel->stage != AGENT, [this, id] { if (Person* q = sim.person(id)) note = sim.requestTrace(*q); }});
                if (sel->stage != AGENT) acts.push_back({"Propose recruitment (sanction)", true, [this, id] { if (Person* q = sim.person(id)) note = sim.requestSanction(*q); }});
                acts.push_back({sel->note.empty() ? "Add a note" : "Edit note", true, [this, id] { notePerson = id; }});
                if (sel->stage == AGENT) acts.push_back({"Rename", false, [this, id] { if (Person* q = sim.person(id)) { renamePerson = id; renameBuf = q->codename; } }});
                if (!sim.training() && sel->stage == DEVELOPING && sel->handler == 0 && sel->confid != 2)
                    acts.push_back({"Register as a confidential contact", false, [this, id] { if (Person* q = sim.person(id)) note = sim.registerContact(*q); }});
                if (!sim.training() && (sel->stage == DEVELOPING || sel->stage == SPOTTED) && sel->handler == 0 && sel->kompState != 3)
                    acts.push_back({"Prepare a compromise...", false, [this, id] {
                        Person* q = sim.person(id);
                        if (!q) return;
                        if (q->kompState == 1 || q->kompState == 2) note = sim.prepareCompromise(*q, q->kompKind);
                        else { kompPerson = id; kompKind = 0; openKomp = true; }
                    }});
            }
            if (sel->stage == AGENT && !sim.training()) {
                if (sel->verify == 0 || sel->verify == 3)
                    acts.push_back({"Ask Centre to verify him", false, [this, id] {
                        Person* q = sim.person(id);
                        if (!q) return;
                        confirmText = gx(q, "Ask Centre to verify " + q->label() + "?\n\nCentre will send control questions - things it already knows the answers to - and check his next two deliveries against them. "
                                            "It finds most fabricators and most men who are working for the other side. It is also wrong about one honest agent in fifteen.");
                        confirmAction = [this, id]() { if (Person* r = sim.person(id)) note = sim.requestVerify(*r); };
                    }});
                if (sel->handler == 0 && sel->play != 2)
                    acts.push_back({"Propose to play him back", false, [this, id] {
                        Person* q = sim.person(id);
                        if (!q) return;
                        if (q->play == 1) { note = sim.proposePlayback(*q); return; }
                        confirmText = gx(q, "Propose to Centre that " + q->label() + " is the adversary's, and should be played back?\n\n"
                                            "You go on meeting him. Nothing he brings is credited; at each meeting you hand him chicken feed prepared by Centre - true, and worthless - and task him as if you believed in him. "
                                            "While it lasts they think they can read your diary, and what they brief him to ask tells you what they know. They can end it when they like, and how they like.\n\n"
                                            "If he is honest after all, you will be throwing away an agent without knowing it.");
                        confirmAction = [this, id]() { if (Person* r = sim.person(id)) note = sim.proposePlayback(*r); };
                    }});
                acts.push_back({sel->exfil == 2 ? "Travel documents..." : "Request exfiltration", sel->exfil == 2, [this, id] {
                    Person* q = sim.person(id);
                    if (!q) return;
                    if (q->exfil != 0) { note = sim.requestExfil(*q); return; }
                    confirmText = gx(q, "Ask Centre to bring " + q->label() + " out of the country for good?\n\nIf Centre agrees, a passport in another name and a route arrive in one to two weeks; "
                                        "you hand them over at a meeting and he leaves the same night. His production ends with it. Centre will weigh what he has been worth.");
                    confirmAction = [this, id]() { if (Person* r = sim.person(id)) note = sim.requestExfil(*r); };
                }});
            }
            if (sel->live())
                acts.push_back({"Terminate contact", false, [this, id] {
                    Person* q = sim.person(id);
                    if (!q) return;
                    confirmText = "Terminate all contact with " + q->full() + "? The file is closed for good.";
                    confirmAction = [this, id]() { if (Person* r = sim.person(id)) note = sim.terminate(*r); };
                }});
            Flow fl;
            std::function<void()> run;
            bool more = false;
            for (auto& a : acts) { if (a.main) { if (fl.btn(a.label.c_str())) run = a.go; } else more = true; }
            if (more) {
                if (fl.btn("More actions \xE2\x96\xBE")) ImGui::OpenPopup("##moreacts");
                uiRec("More actions");
                if (ImGui::BeginPopup("##moreacts")) {
                    for (auto& a : acts) if (!a.main) {
                        if (a.label == "Terminate contact") ImGui::Separator();
                        if (ImGui::Selectable(a.label.c_str(), false, 0, ImVec2(300 * S, 0))) run = a.go;
                        uiRec(a.label);
                    }
                    ImGui::EndPopup();
                }
            }
            if (run) run();
            std::vector<Opt> hs = {{"you personally", 0}};
            for (auto& o : g.officers) if (o.role == ROLE_SUB && !o.gone) hs.push_back({o.title(), o.id});
            if (hs.size() > 1 && sel->live()) {
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Case officer:");
                ImGui::SameLine();
                int h = sel->handler;
                if (uiCombo("##handler", h, hs, 230 * S)) note = sim.setHandler(*sel, h);
                if (sel->handler != 0 && sel->stage != AGENT) { ImGui::SameLine(); uiCheck("authorise the approach once sanctioned", &sel->pitchAuth); }
            }
        }
        uiCheck("Show closed files", &showClosed);
    }
    docEnd();
    ImGui::SameLine(0, 16 * S);
    ImGui::BeginGroup();
    if (sel) dossierView(*sel);
    else {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 a = ImGui::GetCursorScreenPos(), b = a + ImGui::GetContentRegionAvail();
        sheet(dl, a + ImVec2(0, 24 * S), b, rgb(C_MANILA_DK), S, true);
        dl->AddRectFilled(a + ImVec2(26 * S, 2 * S), a + ImVec2(200 * S, 26 * S), rgb(C_MANILA_DK), 6 * S, ImDrawFlags_RoundCornersTop);
        stamp(dl, (a + b) * 0.5f, "СОВЕРШЕННО СЕКРЕТНО", rgb(C_RED, 0.7f), -6, F.stampBig, 34 * S, 1, "Select a file from the card index");
        ImGui::Dummy(b - a);
    }
    ImGui::EndGroup();
}

// ============================================================================ THE MAP
void App::drawMap(ImVec2 a, ImVec2 b) {
    GameState& g = sim.g;
    const Theatre& t = sim.T();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float W = b.x - a.x, H = b.y - a.y;
    auto P = [&](float x, float y) { return ImVec2(a.x + x * W, a.y + y * H); };
    uint32_t seed = 977u + (uint32_t)g.theatre * 1013u;
    auto warp = [&](float u, float v) {
        float du = (vnoise(u * 3.1f, v * 3.1f, seed) - 0.5f) * 0.06f + (vnoise(u * 9.f, v * 9.f, seed + 5) - 0.5f) * 0.012f;
        float dv = (vnoise(u * 3.1f + 17, v * 3.1f, seed + 1) - 0.5f) * 0.06f + (vnoise(u * 9.f, v * 9.f + 3, seed + 6) - 0.5f) * 0.012f;
        return P(u + du, v + dv);
    };
    sheet(dl, a, b, rgb(C_MAP), S, true);
    dl->PushClipRect(a + ImVec2(2, 2), b - ImVec2(2, 2), true);
    // the centre of the city: where the districts crowd together
    ImVec2 cen(0, 0);
    for (int i = 0; i < 8; i++) cen = cen + ImVec2(t.at[i].x, t.at[i].y);
    cen = cen * (1.f / 8);
    // built-up area
    for (int k = 6; k >= 1; k--) dl->AddCircleFilled(P(cen.x, cen.y), W * 0.08f * k, rgb(0xCDBF9E, 0.07f), 64);
    // our side of a frontier
    if (!t.wall.empty()) {
        std::vector<ImVec2> poly;
        for (auto& p : t.wall) poly.push_back(P(p.x, p.y));
        poly.push_back(P(1.02f, 1.02f));
        poly.push_back(P(1.02f, -0.02f));
        std::reverse(poly.begin(), poly.end());
        dl->AddConcavePolyFilled(poly.data(), (int)poly.size(), g.wallDown ? rgb(0xB89A6A, 0.10f) : rgb(0xB0563C, 0.13f));
    }
    // the street grid
    const int NX = 34, NY = 24;
    ImU32 street = rgb(0x8C7E62, 0.55f);
    for (int i = 0; i <= NX; i++)
        for (int j = 0; j < NY; j++) {
            if (frac(hsh(seed + 11, (uint32_t)(i * 97 + j))) < 0.17f) continue;
            float u = (float)i / NX, v0 = (float)j / NY, v1 = (float)(j + 1) / NY;
            ImVec2 p3[3] = {warp(u, v0), warp(u, (v0 + v1) / 2), warp(u, v1)};
            dl->AddPolyline(p3, 3, street, 0, 1.0f * S);
        }
    for (int j = 0; j <= NY; j++)
        for (int i = 0; i < NX; i++) {
            if (frac(hsh(seed + 12, (uint32_t)(j * 97 + i))) < 0.17f) continue;
            float v = (float)j / NY, u0 = (float)i / NX, u1 = (float)(i + 1) / NX;
            ImVec2 p3[3] = {warp(u0, v), warp((u0 + u1) / 2, v), warp(u1, v)};
            dl->AddPolyline(p3, 3, street, 0, 1.0f * S);
        }
    // main roads: radials from the centre and a ring
    auto road = [&](const std::vector<ImVec2>& pts) {
        dl->AddPolyline(pts.data(), (int)pts.size(), rgb(0x6E6048, 0.85f), 0, 5.0f * S);
        dl->AddPolyline(pts.data(), (int)pts.size(), rgb(0xF2E3AE), 0, 3.0f * S);
    };
    for (int k = 0; k < 7; k++) {
        float ang = k * 6.2831853f / 7 + frac(hsh(seed, 400)) * 1.5f;
        std::vector<ImVec2> pts;
        for (int s = 0; s <= 16; s++) {
            float r = s / 16.f * 0.75f;
            pts.push_back(warp(cen.x + std::cos(ang + std::sin(r * 4 + k) * 0.08f) * r, cen.y + std::sin(ang + std::sin(r * 4 + k) * 0.08f) * r * 1.1f));
        }
        road(pts);
    }
    {
        std::vector<ImVec2> pts;
        for (int s = 0; s <= 64; s++) {
            float ang = s / 64.f * 6.2831853f;
            pts.push_back(warp(cen.x + std::cos(ang) * 0.30f, cen.y + std::sin(ang) * 0.27f));
        }
        road(pts);
    }
    // parks
    for (int i = 0; i < 8; i++) {
        uint32_t h = hsh(seed + 21, (uint32_t)i);
        float ang = frac(h) * 6.28f, rr = 0.05f + frac(h >> 3) * 0.03f;
        ImVec2 c = P(t.at[i].x + std::cos(ang) * rr, t.at[i].y + std::sin(ang) * rr);
        float R = W * (0.022f + frac(h >> 7) * 0.02f);
        ImVec2 pts[18];
        for (int k = 0; k < 18; k++) {
            float an = k / 18.f * 6.2831853f;
            float kk = 0.75f + frac(hsh(h, (uint32_t)k)) * 0.4f;
            pts[k] = c + ImVec2(std::cos(an) * R * kk, std::sin(an) * R * kk * 0.8f);
        }
        dl->AddConvexPolyFilled(pts, 18, rgb(0xB4C49A));
        dl->AddPolyline(pts, 18, rgb(0x7E9468), ImDrawFlags_Closed, 1);
        for (int k = 0; k < 9; k++) {
            uint32_t q = hsh(h, 100 + (uint32_t)k);
            ImVec2 tp = c + ImVec2((frac(q) - 0.5f) * R * 1.1f, (frac(q >> 9) - 0.5f) * R * 0.9f);
            dl->AddCircleFilled(tp, 2.2f * S, rgb(0x6F8A58, 0.8f));
        }
    }
    // the river
    if (!t.river.empty()) {
        std::vector<ImVec2> pts;
        for (size_t i = 0; i + 1 < t.river.size(); i++)
            for (int s = 0; s < 6; s++) {
                float k = s / 6.f;
                float x = t.river[i].x + (t.river[i + 1].x - t.river[i].x) * k, y = t.river[i].y + (t.river[i + 1].y - t.river[i].y) * k;
                pts.push_back(P(x + (vnoise(x * 6, y * 6, seed + 9) - 0.5f) * 0.012f, y + (vnoise(x * 6 + 3, y * 6, seed + 9) - 0.5f) * 0.012f));
            }
        pts.push_back(P(t.river.back().x, t.river.back().y));
        dl->AddPolyline(pts.data(), (int)pts.size(), rgb(0x5E7A8C), 0, 15 * S);
        dl->AddPolyline(pts.data(), (int)pts.size(), rgb(0xA9C1CF), 0, 12 * S);
        dl->AddPolyline(pts.data(), (int)pts.size(), rgb(0xBFD2DC), 0, 4 * S);
    }
    // the frontier
    if (!t.wall.empty()) {
        std::vector<ImVec2> pts;
        for (auto& p : t.wall) pts.push_back(P(p.x, p.y));
        if (!g.wallDown) {
            dl->AddPolyline(pts.data(), (int)pts.size(), rgb(0x3A3634), 0, 4 * S);
            for (size_t i = 0; i + 1 < pts.size(); i++) {
                ImVec2 d = pts[i + 1] - pts[i];
                float l = std::sqrt(d.x * d.x + d.y * d.y);
                ImVec2 u = d * (1 / l), n(u.y, -u.x);                      // ticks on the western side
                for (float s = 4 * S; s < l; s += 9 * S) dl->AddLine(pts[i] + u * s, pts[i] + u * s + n * (6 * S), rgb(0x3A3634), 1.5f * S);
            }
        } else
            for (size_t i = 0; i + 1 < pts.size(); i++) {
                ImVec2 d = pts[i + 1] - pts[i];
                float l = std::sqrt(d.x * d.x + d.y * d.y);
                for (float s = 0; s < l; s += 12 * S) dl->AddLine(pts[i] + d * (s / l), pts[i] + d * (std::min(l, s + 6 * S) / l), rgb(0x3A3634, 0.6f), 2 * S);
            }
        ImVec2 lp = (pts[0] + pts[1]) * 0.5f + ImVec2(12 * S, 0);
        const char* wl = g.wallDown ? "граница открыта / frontier open" : "ГОСГРАНИЦА / SECTOR BOUNDARY";
        ImVec2 ws = F.tiny->CalcTextSizeA(F.tiny->FontSize, FLT_MAX, 0, wl);
        dl->AddRectFilled(lp - ImVec2(2 * S, 1 * S), lp + ws + ImVec2(2 * S, 1 * S), rgb(C_MAP, 0.85f));
        dl->AddText(F.tiny, F.tiny->FontSize, lp, rgb(0x3A3634), wl);
    }
    // the grid of squares, lettered round the edge
    static const char* COLS[] = {"А", "Б", "В", "Г", "Д", "Е", "Ж", "З"};
    for (int i = 1; i < 8; i++) dl->AddLine(P(i / 8.f, 0), P(i / 8.f, 1), rgb(0x5E7A8C, 0.18f), 1);
    for (int j = 1; j < 6; j++) dl->AddLine(P(0, j / 6.f), P(1, j / 6.f), rgb(0x5E7A8C, 0.18f), 1);
    for (int i = 0; i < 8; i++) dl->AddText(F.tiny, F.tiny->FontSize, P((i + 0.5f) / 8, 0) + ImVec2(-4 * S, 3 * S), rgb(0x5E7A8C), COLS[i]);
    for (int j = 0; j < 6; j++) dl->AddText(F.tiny, F.tiny->FontSize, P(0, (j + 0.5f) / 6) + ImVec2(4 * S, -6 * S), rgb(0x5E7A8C), fstr("%d", j + 1).c_str());
    // Zenit: districts named in this morning's intercept, ringed in red pencil
    auto z = g.zenit.find(g.day);
    for (int i = 0; i < 8; i++) {
        if (z == g.zenit.end() || z->second.find(t.district[i]) == std::string::npos) continue;
        pencilRing(dl, P(t.at[i].x, t.at[i].y), W * 0.075f, H * 0.065f, rgb(C_PENCIL_RED, 0.75f), 2.2f * S, hsh(seed, (uint32_t)(i + g.day)));
        pencil(dl, P(t.at[i].x, t.at[i].y) + ImVec2(W * 0.05f, -H * 0.075f), "Z!", rgb(C_PENCIL_RED, 0.85f), 8);
    }
    // district names
    int home = 0;
    for (int i = 0; i < 8; i++) if (t.dist[i] == 0) { home = i; break; }
    for (int i = 0; i < 8; i++) {
        std::string n = upper(t.district[i]);
        float fs = F.stamp->FontSize * 0.85f;
        ImVec2 ts = F.stamp->CalcTextSizeA(fs, FLT_MAX, 0, n.c_str());
        ImVec2 c = P(t.at[i].x, t.at[i].y) - ImVec2(ts.x / 2, ts.y / 2 + 16 * S);
        ImU32 col = selDistrict == i ? rgb(C_RED) : rgb(0x4A4236, 0.85f);
        for (int dx = -1; dx <= 1; dx++)
            for (int dy = -1; dy <= 1; dy++) dl->AddText(F.stamp, fs, c + ImVec2((float)dx, (float)dy), rgb(C_MAP, 0.8f), n.c_str());
        dl->AddText(F.stamp, fs, c, col, n.c_str());
        if (sim.eastSide(i)) dl->AddText(F.tiny, F.tiny->FontSize, c + ImVec2(0, ts.y), rgb(0x8A3A2A, 0.8f), "наша сторона");
    }
    // the Embassy
    ImVec2 emb = P(t.at[home].x, t.at[home].y) + ImVec2(0, 6 * S);
    star(dl, emb, 11 * S, rgb(C_RED), 0, rgb(0x5A0E0E), 1.2f * S);
    const char* embName = sim.training() ? "ШКОЛА" : t.friends ? "ПРЕДСТАВИТЕЛЬСТВО" : "ПОСОЛЬСТВО";
    dl->AddText(F.tiny, F.tiny->FontSize, emb + ImVec2(13 * S, -4 * S), rgb(C_RED), embName);
    // sites
    auto sitePos = [&](const Site& s) {
        uint32_t h = hsh(seed + 31, (uint32_t)s.id);
        float ang = frac(h) * 6.2831853f, rr = 0.025f + frac(h >> 5) * 0.055f;
        return P(t.at[s.district].x + std::cos(ang) * rr, t.at[s.district].y + std::sin(ang) * rr * 1.1f);
    };
    for (auto& s : g.sites) {
        if (s.retired) continue;
        ImVec2 c = sitePos(s);
        bool sel = s.id == selSite;
        float r = (sel ? 7.5f : 5.5f) * S;
        switch (s.type) {
            case ST_MEET:
                if (s.flat) {          // a safe flat: a little house
                    ImVec2 h[5] = {c + ImVec2(-r, r), c + ImVec2(r, r), c + ImVec2(r, -r * 0.2f), c + ImVec2(0, -r * 1.25f), c + ImVec2(-r, -r * 0.2f)};
                    ImVec2 sh[5];
                    for (int k = 0; k < 5; k++) sh[k] = h[k] + ImVec2(1, 2);
                    dl->AddConvexPolyFilled(sh, 5, rgb(0, 0.3f));
                    dl->AddConvexPolyFilled(h, 5, rgb(0x7A2E14));
                    dl->AddRectFilled(c + ImVec2(-r * 0.25f, r * 0.15f), c + ImVec2(r * 0.25f, r), rgb(0xFFFFFF, 0.9f));
                    break;
                }
                dl->AddCircleFilled(c + ImVec2(1, 2), r, rgb(0, 0.3f));
                dl->AddCircleFilled(c, r, rgb(0xB3261E));
                dl->AddCircleFilled(c, r * 0.4f, rgb(0xFFFFFF, 0.9f));
                break;
            case ST_DROP:
                dl->AddRectFilled(c - ImVec2(r, r) + ImVec2(1, 2), c + ImVec2(r, r) + ImVec2(1, 2), rgb(0, 0.3f));
                dl->AddRectFilled(c - ImVec2(r, r), c + ImVec2(r, r), rgb(0x2D4C8A));
                dl->AddLine(c - ImVec2(r * 0.6f, 0), c + ImVec2(r * 0.6f, 0), rgb(0xFFFFFF, 0.9f), 1.5f * S);
                break;
            case ST_SIGNAL:
                dl->AddTriangleFilled(c + ImVec2(0, -r * 1.2f), c + ImVec2(r * 1.1f, r * 0.8f), c + ImVec2(-r * 1.1f, r * 0.8f), rgb(0x2F6B3A));
                break;
            default: {
                ImVec2 pts[7];
                uint32_t h = hsh(seed + 41, (uint32_t)s.id);
                for (int k = 0; k < 7; k++) {
                    float an = k / 7.f * 6.2831853f;
                    float rr = W * (0.022f + frac(hsh(h, (uint32_t)k)) * 0.018f);
                    pts[k] = c + ImVec2(std::cos(an) * rr, std::sin(an) * rr * 0.85f);
                }
                for (int k = 0; k < 7; k++) {
                    ImVec2 p0 = pts[k], p1 = pts[(k + 1) % 7], d = p1 - p0;
                    float l = std::sqrt(d.x * d.x + d.y * d.y);
                    for (float s2 = 0; s2 < l; s2 += 7 * S) dl->AddLine(p0 + d * (s2 / l), p0 + d * (std::min(l, s2 + 3.5f * S) / l), rgb(C_VIOLET, sel ? 1.f : 0.65f), (sel ? 2.4f : 1.6f) * S);
                }
                dl->AddCircleFilled(c, 3 * S, rgb(C_VIOLET));
            }
        }
        if (sel) pencilRing(dl, c, 14 * S, 13 * S, rgb(C_PENCIL_BLUE), 2 * S, (uint32_t)s.id);
    }
    // today's plan, pencilled in
    for (int bl = 0; bl < 3; bl++) {
        Order o = sim.order(g.day, bl);
        Site* s = sim.site(o.site);
        ImVec2 to;
        if (o.type == O_SDR && sim.site(o.route)) to = sitePos(*sim.site(o.route));
        else if (s && Sim::isOp(o.type)) to = sitePos(*s);
        else if (o.type == O_SCOUT) to = P(t.at[o.district].x, t.at[o.district].y);
        else continue;
        ImVec2 from = emb, mid = (from + to) * 0.5f + ImVec2((to.y - from.y) * 0.15f, (from.x - to.x) * 0.15f);
        // the pencil line: solid for the car, dash-and-dot for a taxi, dashed on foot
        const int N = 40;
        ImVec2 prev = from;
        for (int k = 1; k <= N; k++) {
            float u = k / (float)N;
            ImVec2 q = from * ((1 - u) * (1 - u)) + mid * (2 * u * (1 - u)) + to * (u * u);
            bool draw = o.travel == 1 || (o.travel == 2 ? (k % 4 != 0) : (k % 2 == 0));
            if (draw) dl->AddLine(prev, q, rgb(C_PENCIL_BLUE, 0.85f), (o.travel == 1 ? 2.4f : 1.8f) * S);
            prev = q;
        }
        ImVec2 lab = (from + mid * 2 + to) * 0.25f;
        dl->AddCircleFilled(lab, 9 * S, rgb(C_MAP));
        dl->AddCircle(lab, 9 * S, rgb(C_PENCIL_BLUE), 0, 1.5f * S);
        std::string n = fstr("%d", bl + 1);
        ImVec2 ts = F.hand->CalcTextSizeA(F.hand->FontSize * 0.8f, FLT_MAX, 0, n.c_str());
        dl->AddText(F.hand, F.hand->FontSize * 0.8f, lab - ts * 0.5f, rgb(C_PENCIL_BLUE), n.c_str());
    }
    // cartouche, north arrow, the restriction stamp
    ImVec2 ca = P(0, 1) + ImVec2(14 * S, -96 * S), cb = ca + ImVec2(250 * S, 82 * S);
    dl->AddRectFilled(ca, cb, rgb(0xF4EEDA));
    dl->AddRect(ca, cb, rgb(C_INK), 0, 0, 1.5f * S);
    dl->AddRect(ca + ImVec2(3 * S, 3 * S), cb - ImVec2(3 * S, 3 * S), rgb(C_INK), 0, 0, 0.8f);
    dl->AddText(F.stampBig, F.stampBig->FontSize * 0.75f, ca + ImVec2(12 * S, 6 * S), rgb(C_INK), t.cityRu);
    dl->AddText(F.tiny, F.tiny->FontSize, ca + ImVec2(12 * S, 6 * S + F.stampBig->FontSize * 0.78f), rgb(C_INK_FADED), (std::string("СХЕМА ГОРОДА  -  plan of ") + t.city).c_str());
    float sy = cb.y - 16 * S;
    for (int k = 0; k < 5; k++) {
        ImVec2 s0 = ImVec2(ca.x + 12 * S + k * 30 * S, sy);
        if (k % 2 == 0) dl->AddRectFilled(s0, s0 + ImVec2(30 * S, 5 * S), rgb(C_INK));
        dl->AddRect(s0, s0 + ImVec2(30 * S, 5 * S), rgb(C_INK));
    }
    dl->AddText(F.tiny, F.tiny->FontSize * 0.9f, ImVec2(ca.x + 170 * S, sy - 5 * S), rgb(C_INK), "1 : 25 000");
    ImVec2 na = P(1, 0) + ImVec2(-34 * S, 58 * S);
    dl->AddTriangleFilled(na + ImVec2(0, -18 * S), na + ImVec2(7 * S, 10 * S), na + ImVec2(0, 4 * S), rgb(C_INK));
    dl->AddTriangle(na + ImVec2(0, -18 * S), na + ImVec2(-7 * S, 10 * S), na + ImVec2(0, 4 * S), rgb(C_INK), 1);
    dl->AddText(F.stamp, F.stamp->FontSize * 0.8f, na + ImVec2(-5 * S, -36 * S), rgb(C_INK), "С");
    stamp(dl, P(0, 0) + ImVec2(150 * S, 34 * S), "ДЛЯ СЛУЖЕБНОГО ПОЛЬЗОВАНИЯ", rgb(C_RED, 0.75f), -4, F.stamp, 18 * S, 0);
    dl->PopClipRect();
    // clicking: the nearest site, else the nearest district
    ImGui::SetCursorScreenPos(a);
    ImGui::InvisibleButton("map", b - a);
    uiRec("map");
    if (ImGui::IsItemHovered()) {
        ImVec2 m = ImGui::GetIO().MousePos;
        const Site* best = nullptr;
        float bd = 12 * S * 12 * S;
        for (auto& s : g.sites) {
            if (s.retired) continue;
            ImVec2 d = sitePos(s) - m;
            float dd = d.x * d.x + d.y * d.y;
            if (dd < bd) { bd = dd; best = &s; }
        }
        int bestD = -1;
        float dd2 = W * 0.09f * W * 0.09f;
        for (int i = 0; i < 8; i++) {
            ImVec2 d = P(t.at[i].x, t.at[i].y) - m;
            float dd = d.x * d.x + d.y * d.y;
            if (dd < dd2) { dd2 = dd; bestD = i; }
        }
        if (best) ImGui::SetTooltip("%s\n%s", best->flat ? "Safe flat" : SITETYPE_LABEL[best->type], best->name.c_str());
        else if (bestD >= 0) ImGui::SetTooltip("%s", t.district[bestD]);
        if (ImGui::IsItemClicked()) {
            if (best) { selSite = best->id; selDistrict = best->district; }
            else if (bestD >= 0) { selDistrict = bestD; selSite = -1; }
            else { selDistrict = -1; selSite = -1; }
        }
    }
}

void App::tabMap() {
    GameState& g = sim.g;
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float mapW = std::floor(std::min(avail.x * 0.70f, avail.y * 1.45f));
    ImVec2 a = ImGui::GetCursorScreenPos();
    {   // the plan, laid in a frame of its own
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 fb = a + ImVec2(mapW, avail.y);
        skin::frame(dl, a, fb, S);
        std::string hint = std::string(sim.T().city) + "  -  for official use only";
        float bh = skin::titleBar(dl, a, mapW, S, "City plan", hint.c_str());
        drawMap(a + ImVec2(10 * S, bh + 8 * S), fb - ImVec2(10 * S, 10 * S));
    }
    ImGui::SetCursorScreenPos(a + ImVec2(mapW + 16 * S, 0));
    if (docPanel("mapNotes", ImVec2(avail.x - mapW - 16 * S, avail.y), C_CARD, 14, "About the map", nullptr)) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        // legend
        ImVec2 p = ImGui::GetCursorScreenPos();
        float lh = 22 * S;
        const char* L[] = {"meeting site", "safe flat", "dead drop", "signal site", "detection route", "the Embassy", "your plan for today", "named by Zenit this morning"};
        for (int i = 0; i < 8; i++) {
            ImVec2 c = p + ImVec2(10 * S, i * lh + 10 * S);
            int k = i == 0 ? 0 : i == 1 ? -1 : i - 1;
            if (k < 0) {
                float r = 5.5f * S;
                ImVec2 h[5] = {c + ImVec2(-r, r), c + ImVec2(r, r), c + ImVec2(r, -r * 0.2f), c + ImVec2(0, -r * 1.25f), c + ImVec2(-r, -r * 0.2f)};
                dl->AddConvexPolyFilled(h, 5, rgb(0x7A2E14));
                dl->AddRectFilled(c + ImVec2(-r * 0.25f, r * 0.15f), c + ImVec2(r * 0.25f, r), rgb(0xFFFFFF, 0.9f));
            }
            switch (k) {
                case -1: break;
                case 0: dl->AddCircleFilled(c, 5.5f * S, rgb(0xB3261E)); dl->AddCircleFilled(c, 2.2f * S, rgb(0xFFFFFF)); break;
                case 1: dl->AddRectFilled(c - ImVec2(5 * S, 5 * S), c + ImVec2(5 * S, 5 * S), rgb(0x2D4C8A)); break;
                case 2: dl->AddTriangleFilled(c + ImVec2(0, -6 * S), c + ImVec2(6 * S, 4 * S), c + ImVec2(-6 * S, 4 * S), rgb(0x2F6B3A)); break;
                case 3: dl->AddCircle(c, 6 * S, rgb(C_VIOLET), 12, 1.6f * S); break;
                case 4: star(dl, c, 7 * S, rgb(C_RED)); break;
                case 5: dl->AddLine(c - ImVec2(8 * S, 0), c + ImVec2(8 * S, 0), rgb(C_PENCIL_BLUE), 2 * S); break;
                default: pencilRing(dl, c, 8 * S, 6 * S, rgb(C_PENCIL_RED), 1.6f * S, 3); break;
            }
            dl->AddText(F.small, F.small->FontSize, c + ImVec2(18 * S, -F.small->FontSize / 2), rgb(C_INK), L[i]);
        }
        ImGui::Dummy(ImVec2(10, 8 * lh + 10 * S));
        ImVec2 r0 = ImGui::GetCursorScreenPos();
        dl->AddLine(r0, r0 + ImVec2(ImGui::GetContentRegionAvail().x, 0), rgb(C_RULE), 1);
        ImGui::Dummy(ImVec2(10, 6 * S));
        Site* s = sim.site(selSite);
        if (s) {
            float pw = std::min(ImGui::GetContentRegionAvail().x - 20 * S, 300 * S);
            ImVec2 pp = ImGui::GetCursorScreenPos() + ImVec2(8 * S, 18 * S);
            photo(dl, sitePhoto(*s), pp, ImVec2(pw, pw * 0.75f), 1.2f, S, true);
            ImGui::Dummy(ImVec2(pw, pw * 0.75f + 34 * S));
            uiTyped(sim.siteReport(*s), rgb(C_INK), F.small, [](const std::string& l, ImU32& c, ImFont*&) { if (has(l, "OVER-USED") || has(l, "danger")) c = rgb(C_RED); }, (uint32_t)s->id);
        } else if (selDistrict >= 0) {
            uiTyped(sim.districtText(selDistrict), rgb(C_INK), F.small, [](const std::string& l, ImU32&, ImFont*& f) { if (heading(l)) f = F.bold; }, 4);
            ImGui::Spacing();
            ImGui::PushFont(F.ui);
            for (auto& x : g.sites) {
                if (x.retired || x.district != selDistrict) continue;
                ImGui::PushID(x.id);
                if (ImGui::Selectable((std::string(x.flat ? "Safe flat" : SITETYPE_LABEL[x.type]) + ": " + x.name).c_str(), false)) selSite = x.id;
                ImGui::PopID();
            }
            ImGui::PopFont();
        } else
            uiTyped("Click a district for what you know of it, or a mark for the site.\n\n"
                    "The plan is drawn from memory and from the Embassy's own street plans: the places of your sites are where you know them to be. "
                    "New sites are found with the order 'Scout district'.", rgb(C_INK_FADED), F.small, nullptr, 5);
    }
    docEnd();
}

// ============================================================================ SITES & ROUTES
void App::tabSites() {
    GameState& g = sim.g;
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float leftW = std::floor(avail.x * 0.62f);
    if (docPanel("sitesDoc", ImVec2(leftW, avail.y), C_CARD, 12, "Sites and routes", nullptr)) {
        ImGui::PushFont(F.ui);
        if (ImGui::BeginTable("sites", 8, TBL, ImVec2(0, ImGui::GetContentRegionAvail().y - lineH() * 2.3f))) {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Site", 0, 5);
            ImGui::TableSetupColumn("Type", 0, 1.5f);
            ImGui::TableSetupColumn("District", 0, 1.5f);
            ImGui::TableSetupColumn("Cover", 0, 1.1f);
            ImGui::TableSetupColumn("View / quality", 0, 1.3f);
            ImGui::TableSetupColumn("Cased", 0, 0.7f);
            ImGui::TableSetupColumn("Uses", 0, 0.6f);
            ImGui::TableSetupColumn("Last used", 0, 1.1f);
            uiHeaders();
            for (int type = 0; type < N_SITETYPE; type++)
                for (auto& s : g.sites) {
                    if (s.type != type || (s.retired && !showRetired)) continue;
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::PushID(s.id);
                    ImGui::PushStyleColor(ImGuiCol_Text, s.retired ? rgb4(C_INK_FADED) : (s.exposure > 55 ? rgb4(C_RED) : rgb4(C_INK)));
                    if (ImGui::Selectable(s.name.c_str(), selSite == s.id, ImGuiSelectableFlags_SpanAllColumns)) selSite = s.id;
                    ImGui::PopStyleColor();
                    uiRec("site:" + s.name);
                    ImGui::PopID();
                    cell(s.flat ? "Safe flat" : SITETYPE_LABEL[s.type]);
                    cell(sim.T().district[s.district]);
                    cell(s.type == ST_ROUTE ? std::string("-") : word(s.estCover, W_LEVEL, 5));
                    cell(word(s.estSight, W_LEVEL, 5));
                    cell(fstr("%d/3", s.casing));
                    cell(fstr("%d", s.uses));
                    cell(s.lastUsed < 0 ? std::string("-") : fmtShort(s.lastUsed));
                }
            ImGui::EndTable();
        }
        ImGui::PopFont();
        if (uiBtn("Strike site off the list")) {
            Site* s = sim.site(selSite);
            if (!s) note = "Select a site first.";
            else {
                std::string used;
                for (auto& q : g.persons)
                    if (q.stage == AGENT && (std::find(q.meetSites.begin(), q.meetSites.end(), s->id) != q.meetSites.end() || q.dropSite == s->id || q.signalSite == s->id))
                        used = gx(&q, q.label() + " still has this site in his communications plan. Brief him on a new plan first.");
                if (!used.empty()) note = used;
                else {
                    int id = s->id;
                    confirmText = "Strike off " + s->name + "? It will not be offered again.";
                    confirmAction = [this, id]() { if (Site* x = sim.site(id)) sim.retireSite(*x); };
                }
            }
        }
        ImGui::SameLine();
        uiCheck("Show struck-off sites", &showRetired);
        ImGui::SameLine();
        ImGui::TextDisabled("  New sites are found with 'Scout district'.");
    }
    docEnd();
    ImGui::SameLine(0, 16 * S);
    if (docPanel("siteCard", ImVec2(0, avail.y), C_PAPER, 16, "Site card")) {
        Site* s = sim.site(selSite);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (!s) {
            docTitle("КАРТОЧКА МЕСТА", "Site card");
            uiTyped("Select a site in the register.\n\nEach site has a card: what your casing found, how often it has been used, and who besides you knows it.\n\n"
                    "The Map shows where they all are.", rgb(C_INK_FADED), F.small, nullptr, 9);
        } else {
            static const char* RU_T[] = {"МЕСТО ВСТРЕЧ", "ТАЙНИК", "МЕСТО СИГНАЛА", "ПРОВЕРОЧНЫЙ МАРШРУТ"};
            if (s->flat) docTitle("КОНСПИРАТИВНАЯ КВАРТИРА", "Safe flat");
            else docTitle(RU_T[s->type], SITETYPE_LABEL[s->type]);
            float pw = std::min(ImGui::GetContentRegionAvail().x - 30 * S, 380 * S);
            ImVec2 pp = ImGui::GetCursorScreenPos() + ImVec2(10 * S, 22 * S);
            photo(dl, sitePhoto(*s), pp, ImVec2(pw, pw * 0.75f), -1.2f, S, true);
            pencil(dl, pp + ImVec2(pw * 0.55f, pw * 0.75f + 12 * S), fmtShort(std::max(g.postStart, s->lastUsed)), rgb(C_PENCIL_BLUE, 0.8f), -3, F.hand, 15 * S);
            if (s->exposure > 55) stamp(dl, pp + ImVec2(pw * 0.75f, 20 * S), "ИСПОЛЬЗОВАН", rgb(C_RED, 0.8f), 9, F.stamp, 20 * S, 1, "OVER-USED");
            ImGui::Dummy(ImVec2(pw, pw * 0.75f + 44 * S));
            uiTyped(sim.siteReport(*s), rgb(C_INK), F.small, [](const std::string& l, ImU32& c, ImFont*&) { if (has(l, "OVER-USED") || has(l, "DANGER")) c = rgb(C_RED); }, (uint32_t)s->id);
        }
    }
    docEnd();
}

// ============================================================================ MATERIAL & STORES
/** The safe, with what is in it. */
static void drawSafe(ImDrawList* dl, ImVec2 a, ImVec2 b, float S, const GameState& g, const Theatre& t, int passports) {
    // the box
    dl->AddRectFilled(a + ImVec2(4 * S, 6 * S), b + ImVec2(6 * S, 8 * S), rgb(0, 0.35f), 6 * S);
    dl->AddRectFilled(a, b, rgb(0x3E4743), 6 * S);
    dl->AddRectFilled(a + ImVec2(12 * S, 12 * S), b - ImVec2(12 * S, 12 * S), rgb(0x262C29));
    dl->AddRectFilledMultiColor(a + ImVec2(12 * S, 12 * S), ImVec2(b.x - 12 * S, a.y + 40 * S), rgb(0, 0.35f), rgb(0, 0.35f), rgb(0, 0), rgb(0, 0));
    dl->AddRect(a, b, rgb(0x5C6862), 6 * S, 0, 2 * S);
    for (int i = 0; i < 4; i++) dl->AddCircleFilled(ImVec2(a.x + 6 * S, a.y + (30 + i * ((b.y - a.y - 60 * S) / 3 / S)) * S), 2.5f * S, rgb(0x8A948E));
    float mid = (a.y + b.y) / 2;
    dl->AddRectFilled(ImVec2(a.x + 12 * S, mid - 2 * S), ImVec2(b.x - 12 * S, mid + 4 * S), rgb(0x56605B));
    auto label = [&](ImVec2 p, const std::string& s) { dl->AddText(F.tiny, F.tiny->FontSize, p, rgb(0xE6E0D0, 0.85f), s.c_str()); };
    // top shelf: money, film, cameras
    float x = a.x + 30 * S, y = mid - 30 * S;
    int bundles = std::min(4, std::max(g.funds > 0 ? 1 : 0, g.funds / std::max(1, (int)std::lround(60 * t.price))));
    for (int i = 0; i < bundles; i++) cash(dl, ImVec2(x + 30 * S, y - i * 7 * S), 50 * S, curSym(t.currency));
    label(ImVec2(x, mid - 16 * S), fstr("%d %s", g.funds, t.currency));
    x += 100 * S;
    int films = std::min(g.film, 10);
    for (int i = 0; i < films; i++) filmCassette(dl, ImVec2(x + 16 * S + (i % 5) * 34 * S, y - 16 * S + (i / 5) * 20 * S), 22 * S, false);
    label(ImVec2(x, mid - 16 * S), fstr("film: %d", g.film));
    x += 190 * S;
    for (int i = 0; i < std::min(g.cameras, 3); i++) minox(dl, ImVec2(x + 26 * S, y - 16 * S + i * 13 * S), 32 * S);
    label(ImVec2(x, mid - 16 * S), fstr("Minox: %d", g.cameras));
    // bottom shelf: pads, containers, devices, a package, passports
    x = a.x + 30 * S; y = b.y - 40 * S;
    for (int i = 0; i < std::min(g.pads, 6); i++) padBook(dl, ImVec2(x + 18 * S + i * 5 * S, y - i * 3 * S), 40 * S);
    label(ImVec2(x, b.y - 28 * S), fstr("pads: %d", g.pads));
    x += 90 * S;
    for (int i = 0; i < std::min(g.containers, 6); i++) bolt(dl, ImVec2(x + 10 * S + i * 16 * S, y - 12 * S), 30 * S);
    label(ImVec2(x, b.y - 28 * S), fstr("containers: %d", g.containers));
    x += 120 * S;
    for (int i = 0; i < std::min(g.bugs, 3); i++) bug(dl, ImVec2(x + 16 * S + i * 30 * S, y - 6 * S), 36 * S);
    if (g.bugs > 0) label(ImVec2(x, b.y - 28 * S), fstr("devices: %d", g.bugs));
    x += 100 * S;
    if (g.lineNPkgs > 0) { parcel(dl, ImVec2(x + 22 * S, y - 8 * S), 40 * S); label(ImVec2(x, b.y - 28 * S), "Line N"); x += 70 * S; }
    for (int i = 0; i < passports && x < b.x - 40 * S; i++) {
        ImVec2 pa(x + i * 8 * S, y - 26 * S), pb = pa + ImVec2(24 * S, 34 * S);
        dl->AddRectFilled(pa, pb, rgb(0x1E3A5F), 2 * S);
        dl->AddRect(pa, pb, rgb(0xC9A227, 0.8f), 2 * S);
        dl->AddCircle(pa + ImVec2(12 * S, 14 * S), 6 * S, rgb(0xC9A227, 0.8f));
    }
    if (passports > 0) label(ImVec2(x, b.y - 28 * S), "passport");
}

void App::tabMaterial() {
    GameState& g = sim.g;
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float leftW = std::floor(avail.x * 0.50f);
    if (docPanel("matDoc", ImVec2(leftW, avail.y), C_CARD, 12, "Material received", nullptr)) {
        Roll* x = sim.roll(selRoll);
        float below = lineH() * 1.3f + (x ? 96 * S : 0);
        ImGui::PushFont(F.ui);
        if (ImGui::BeginTable("rolls", 7, TBL, ImVec2(0, ImGui::GetContentRegionAvail().y - below))) {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("No.", 0, 0.6f);
            ImGui::TableSetupColumn("Source", 0, 1.3f);
            ImGui::TableSetupColumn("Kind", 0, 1.0f);
            ImGui::TableSetupColumn("Shot", 0, 1.1f);
            ImGui::TableSetupColumn("Where it is", 0, 2.6f);
            ImGui::TableSetupColumn("Since", 0, 1.1f);
            ImGui::TableSetupColumn("Centre's grade", 0, 2.6f);
            uiHeaders();
            for (int i = (int)g.rolls.size() - 1; i >= 0; i--) {
                Roll& r = g.rolls[i];
                if (r.state == R_AGENT || r.state == R_DROP) continue;          // you do not know what he is holding
                Person* p = sim.person(r.agentId);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::PushID(r.id);
                if (ImGui::Selectable(fstr("%d", r.id).c_str(), selRoll == r.id, ImGuiSelectableFlags_SpanAllColumns)) selRoll = r.id;
                uiRec(fstr("roll:%d", r.id));
                ImGui::PopID();
                cell(p ? p->label() : std::string("?"));
                ImGui::TableNextColumn();
                ImVec2 ip = ImGui::GetCursorScreenPos() + ImVec2(9 * S, ImGui::GetTextLineHeight() / 2);
                ImDrawList* dl = ImGui::GetWindowDrawList();
                if (r.tech) tapeReel(dl, ip, 15 * S); else if (r.oral) typedSheet(dl, ip, 15 * S); else filmCassette(dl, ip, 13 * S, true);
                ImGui::Dummy(ImVec2(20 * S, ImGui::GetTextLineHeight()));
                ImGui::SameLine();
                ImGui::TextUnformatted(r.tech ? "tape" : r.oral ? "oral" : "film");
                cell(fmtShort(r.shotDay));
                cellCol(std::string(RSTATE[r.state]) + (r.cabled && r.state == R_READY ? " (summary queued)" : ""), r.state == R_LOST ? rgb(C_RED) : rgb(C_INK));
                cell(fmtShort(r.stateDay));
                cell(r.grade);
            }
            ImGui::EndTable();
        }
        ImGui::PopFont();
        if (x) {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 p = ImGui::GetCursorScreenPos() + ImVec2(0, 6 * S);
            float w = ImGui::GetContentRegionAvail().x;
            // the item itself, with a luggage label tied to it
            ImVec2 c = p + ImVec2(50 * S, 40 * S);
            pushInk(false);                                     // a paper label, on the dark register
            if (x->tech) tapeReel(dl, c, 64 * S); else if (x->oral) typedSheet(dl, c, 72 * S); else filmCassette(dl, c, 60 * S, true);
            ImVec2 ta = p + ImVec2(110 * S, 8 * S), tb = ImVec2(p.x + w - 4 * S, p.y + 82 * S);
            dl->AddLine(c + ImVec2(20 * S, -10 * S), ta + ImVec2(0, 14 * S), rgb(0xA08A60), 1.2f);
            sheet(dl, ta, tb, rgb(0xEADBB4), S, true);
            dl->AddCircle(ta + ImVec2(10 * S, 14 * S), 4 * S, rgb(0x8A7146), 0, 1.5f);
            Person* src = sim.person(x->agentId);
            std::string l1 = fstr("No. %d  -  ", x->id) + (src ? src->label() : std::string("?")) + "  -  " + fmtShort(x->shotDay);
            std::string l2 = x->state == R_GRADED ? "Graded: " + x->grade : std::string(RSTATE[x->state]);
            typeLine(dl, F.bold, F.bold->FontSize, ta + ImVec2(22 * S, 10 * S), rgb(C_INK), l1.c_str(), l1.c_str() + l1.size(), 3);
            dl->PushClipRect(ta, tb, true);
            typeLine(dl, F.small, F.small->FontSize, ta + ImVec2(22 * S, 34 * S), x->state == R_LOST ? rgb(C_RED) : rgb(C_INK), l2.c_str(), l2.c_str() + l2.size(), 4);
            dl->PopClipRect();
            popInk();
            ImGui::Dummy(ImVec2(w, 92 * S));
        }
        if (uiBtn("Cable a summary of the selected item to Centre")) note = x ? sim.cableSummary(*x) : std::string("Select an item of material first.");
    }
    docEnd();
    ImGui::SameLine(0, 16 * S);
    ImGui::BeginGroup();
    float rw = ImGui::GetContentRegionAvail().x;
    ImVec2 sa = ImGui::GetCursorScreenPos();
    int passports = 0;
    for (auto& p : g.persons) if (p.exfil == 2) passports++;
    float safeH = 262 * S;
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        skin::frame(dl, sa, sa + ImVec2(rw, safeH), S);
        float bh = skin::titleBar(dl, sa, rw, S, "Safe No. 3", "what is in it tonight");
        drawSafe(dl, sa + ImVec2(14 * S, bh + 12 * S), sa + ImVec2(rw - 16 * S, safeH - 16 * S), S, g, sim.T(), passports);
    }
    ImGui::Dummy(ImVec2(rw, safeH + 12 * S));
    float reqH = lineH() * 3.2f;
    if (docPanel("stores", ImVec2(rw, ImGui::GetContentRegionAvail().y - reqH), C_TELETYPE, 14, "Your safe and the stores")) {
        uiTyped(sim.storesText(), rgb(C_INK), F.small, [](const std::string& l, ImU32& c, ImFont*& f) {
            if (heading(l) && l.find("....") == std::string::npos) f = F.bold;
            if (has(l, "NONE") || has(l, "in arrears") || has(l, "SEALED")) c = rgb(C_RED);
        }, 17);
    }
    docEnd();
    ImGui::Dummy(ImVec2(1, 4 * S));
    Flow fl;
    if (fl.btn("Requisition film (5)")) note = sim.requestFilm();
    if (fl.btn("Requisition camera")) note = sim.requestCamera();
    if (fl.btn("Order containers (3)")) note = sim.requestContainers();
    if (fl.btn("Draw one-time pads")) note = sim.requestPads();
    if (!sim.training()) {
        if (fl.btn("Ask Centre for supplementary funds")) note = sim.requestFunds();
        if (fl.btn("Order a listening device")) note = sim.requestBug();
        if (fl.btn("Rent a safe flat...")) { flatDistrict = -1; openFlat = true; }
        if (fl.btn("Pass off the newspapers as a source...")) {
            confirmText = "Write up three days of newspapers as the considered view of one of your contacts, and send it?\n\nIt counts towards the plan like any other report, and half the station does it. "
                          "Centre subscribes to the same newspapers: the more often you do it, the likelier somebody there notices, and then it is on your file and in your attestation.";
            confirmAction = [this]() { note = sim.padReport(); };
        }
    }
    ImGui::EndGroup();
}

// ============================================================================ CENTRE
static std::string fromRu(const std::string& from) {
    static const std::pair<const char*, const char*> M[] = {
        {"Centre", "ЦЕНТР"}, {"Rezident", "РЕЗИДЕНТ"}, {"Line KR", "ЛИНИЯ «КР»"}, {"Instructor", "РУКОВОДИТЕЛЬ ЗАНЯТИЙ"}, {"Technical officer", "ОПЕРАТИВНО-ТЕХНИЧЕСКАЯ ГРУППА"},
        {"Cipher clerk", "ШИФРОВАЛЬНАЯ ГРУППА"}, {"Directorate K", "УПРАВЛЕНИЕ «К»"}, {"Embassy bulletin", "БЮЛЛЕТЕНЬ ПОСОЛЬСТВА"}, {"Duty officer", "ДЕЖУРНЫЙ"},
        {"Rezidentura duty officer", "ДЕЖУРНЫЙ РЕЗИДЕНТУРЫ"}, {"Head of the Representation", "РУКОВОДИТЕЛЬ ПРЕДСТАВИТЕЛЬСТВА"},
        {"Support officer", "ГРУППА ОБЕСПЕЧЕНИЯ"}, {"Ambassador's office", "КАНЦЕЛЯРИЯ ПОСЛА"}, {"Head of the Fourth Department", "НАЧАЛЬНИК 4-ГО ОТДЕЛА"}};
    for (auto& m : M) if (from == m.first) return m.second;
    return "РЕЗИДЕНТУРА";
}

void App::cableView(const Cable& c, int index) {
    GameState& g = sim.g;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 o = ImGui::GetCursorScreenPos();
    float w = ImGui::GetContentRegionAvail().x;
    ImU32 ink = rgb(C_INK), faded = rgb(C_INK_FADED);
    float hs = 14 * S;
    bool councilOfMinisters = c.day < dayOf(1978, 7, 5);
    hammerSickle(dl, o + ImVec2(15 * S, 15 * S), 30 * S, rgb(C_INK, 0.85f));
    dl->AddText(F.stamp, hs, o + ImVec2(36 * S, 0), ink, "КОМИТЕТ ГОСУДАРСТВЕННОЙ БЕЗОПАСНОСТИ");
    dl->AddText(F.stamp, hs, o + ImVec2(36 * S, hs + 1 * S), ink, councilOfMinisters ? "ПРИ СОВЕТЕ МИНИСТРОВ СССР" : "СССР");
    const char* sec = "СОВ. СЕКРЕТНО";
    float sx = F.stamp->CalcTextSizeA(hs * 1.1f, FLT_MAX, 0, sec).x;
    dl->AddText(F.stamp, hs * 1.1f, ImVec2(o.x + w - sx, o.y), rgb(C_RED), sec);
    dl->AddText(F.stamp, hs, ImVec2(o.x + w - sx, o.y + hs + 2 * S), faded, "Экз. № 1");
    std::string head = fstr("ВХОДЯЩАЯ ШИФРТЕЛЕГРАММА № %d", 1000 + index * 7 + (int)(g.seed % 7));
    float y = o.y + 2 * hs + 14 * S;
    float tx = F.stamp->CalcTextSizeA(F.stamp->FontSize * 1.2f, FLT_MAX, 0, head.c_str()).x;
    dl->AddText(F.stamp, F.stamp->FontSize * 1.2f, ImVec2(o.x + (w - tx) / 2, y), ink, head.c_str());
    y += F.stamp->FontSize * 1.2f + 10 * S;
    Date d = dateOf(c.day);
    struct Row { const char* ruL; const char* en; std::string v; };
    Row rows[] = {{"ОТКУДА", "From", fromRu(c.from) + "  (" + c.from + ")"},
                  {"КОМУ", "To", ru(g.officer)},
                  {"ДАТА", "Date", fstr("%02d.%02d.%02d  ", d.d, d.m, d.y % 100) + fmtDay(c.day)},
                  {"ТЕМА", "Subject", c.subject}};
    float lw = 96 * S, rowH = 29 * S;
    dl->PushClipRect(ImVec2(o.x, y), ImVec2(o.x + w - 120 * S, y + 4 * rowH + 4 * S), true);
    for (auto& r : rows) {
        float line = std::floor(y + rowH - 3 * S);
        dl->AddText(F.stamp, 14 * S, ImVec2(o.x, line - 14 * S - F.tiny->FontSize), faded, r.ruL);
        dl->AddText(F.tiny, F.tiny->FontSize, ImVec2(o.x, line - F.tiny->FontSize - 1 * S), faded, r.en);
        typeLine(dl, F.bold, F.bold->FontSize, ImVec2(o.x + lw, line - F.bold->Ascent - 3 * S), ink, r.v.c_str(), r.v.c_str() + r.v.size(), (uint32_t)index * 13 + (uint32_t)y);
        for (float x = o.x + lw; x < o.x + w - 120 * S; x += 4 * S) dl->AddLine(ImVec2(x, line), ImVec2(x + 1.5f * S, line), rgb(C_INK, 0.3f), 1);
        y += rowH;
    }
    dl->PopClipRect();
    // the registration stamp of the station, in violet, and the reader's initials
    stamp(dl, ImVec2(o.x + w - 62 * S, y - 50 * S), fstr("Вх. № %d", 200 + index), rgb(C_VIOLET, 0.75f), -4, F.stamp, 21 * S, 0, fstr("%02d.%02d.%d", d.d, d.m, d.y));
    if (c.read) pencil(dl, ImVec2(o.x + w - 110 * S, y - 16 * S), "озн. " + ru(g.officer).substr(0, 2) + ".", rgb(C_PENCIL_BLUE, 0.85f), -6, F.hand, 17 * S);
    ImGui::Dummy(ImVec2(w, y - o.y + 6 * S));
    ImVec2 r0 = ImGui::GetCursorScreenPos();
    dl->AddLine(r0, r0 + ImVec2(w, 0), rgb(C_INK, 0.5f), 1);
    ImGui::Dummy(ImVec2(w, 6 * S));
    // a walk-in's letter, pinned to the cable
    if (c.subject == "A volunteer") {
        std::string who = c.body;
        size_t p0 = who.find("name as "), p1 = who.find(", and offers");
        who = p0 != std::string::npos && p1 != std::string::npos && p1 > p0 ? who.substr(p0 + 8, p1 - p0 - 8) : std::string("a man who did not give his name");
        std::string name = who.substr(0, who.find(','));
        std::string text = "Sirs,\n\nI am " + who + ".\nI have papers your government will want, and more where they come from.\n"
                           "I want to be paid for them, in cash. I will not come to your building again. Tell me where and when.\n\n- " + name;
        float lwid = std::min(w - 40 * S, 520 * S);
        ImVec2 la = ImGui::GetCursorScreenPos() + ImVec2(20 * S, 10 * S);
        int vs = dl->VtxBuffer.Size;
        float lhh = F.hand->FontSize * 1.15f;
        // wrap it by hand
        std::vector<std::string> lines;
        size_t s = 0;
        while (s <= text.size()) {
            size_t nl = text.find('\n', s);
            if (nl == std::string::npos) nl = text.size();
            const char* b = text.c_str() + s;
            const char* e = text.c_str() + nl;
            if (b == e) lines.push_back("");
            while (b < e) {
                const char* cut = F.hand->CalcWordWrapPositionA(1.0f, b, e, lwid - 40 * S);
                if (cut <= b) cut = b + 1;
                lines.push_back(std::string(b, cut));
                b = cut;
                while (b < e && *b == ' ') b++;
            }
            if (nl >= text.size()) break;
            s = nl + 1;
        }
        float lhgt = lines.size() * lhh + 40 * S;
        ImVec2 lb = la + ImVec2(lwid, lhgt);
        sheet(dl, la, lb, rgb(0xF7F6F0), S, true);
        for (float ly = la.y + 26 * S + lhh; ly < lb.y - 8 * S; ly += lhh) dl->AddLine(ImVec2(la.x + 6 * S, ly), ImVec2(lb.x - 6 * S, ly), rgb(0x7A9CC6, 0.35f), 1);
        float ly = la.y + 24 * S;
        for (auto& l : lines) { dl->AddText(F.hand, F.hand->FontSize, ImVec2(la.x + 20 * S, ly), rgb(0x1E2A55), l.c_str()); ly += lhh; }
        rotateSince(dl, vs, (la + lb) * 0.5f, -1.2f);
        paperclip(dl, la + ImVec2(lwid * 0.8f, -12 * S), S, 3);
        ImGui::Dummy(ImVec2(w, lhgt + 28 * S));
    }
    // a press cutting about an arrest
    if (c.subject.rfind("Arrest of", 0) == 0) {
        std::string who = c.body;
        size_t p0 = who.find("report that "), p1 = who.find(" has been arrested");
        who = p0 != std::string::npos && p1 != std::string::npos && p1 > p0 ? who.substr(p0 + 12, p1 - p0 - 12) : std::string("");
        float cw = std::min(w - 40 * S, 460 * S), chh = 250 * S;
        ImVec2 ca = ImGui::GetCursorScreenPos() + ImVec2(30 * S, 10 * S), cb = ca + ImVec2(cw, chh);
        int vs = dl->VtxBuffer.Size;
        sheet(dl, ca, cb, rgb(0xE9E3D2), S, true);
        float yy = ca.y + 10 * S;
        dl->AddText(F.tiny, F.tiny->FontSize, ImVec2(ca.x + 12 * S, yy), rgb(0x333333), (fmtDay(c.day) + "  -  page 1").c_str());
        yy += 16 * S;
        std::string h1 = "SPY CHARGE";
        dl->AddText(F.serif, F.serif->FontSize * 1.3f, ImVec2(ca.x + 12 * S, yy), rgb(0x161616), h1.c_str());
        yy += F.serif->FontSize * 1.35f;
        std::string h2 = upper(who.substr(0, who.find(',')));
        dl->AddText(F.serif, F.serif->FontSize * 0.75f, ImVec2(ca.x + 12 * S, yy), rgb(0x161616), (h2.empty() ? std::string("A CIVIL SERVANT") : h2).c_str());
        yy += F.serif->FontSize * 0.9f;
        ImVec2 pa(ca.x + 12 * S, yy + 4 * S), ps(140 * S, 105 * S);
        ImTextureID ph = scenePhoto(index, art::SC_ARREST);
        if (ph) dl->AddImage(ph, pa, pa + ps); else dl->AddRectFilled(pa, pa + ps, rgb(0x777777));
        // columns of small type, as grey lines
        for (int col = 0; col < 2; col++)
            for (float ly = (col == 0 ? pa.y + ps.y + 8 * S : yy + 6 * S); ly < cb.y - 10 * S; ly += 6 * S) {
                float x0 = col == 0 ? ca.x + 12 * S : pa.x + ps.x + 12 * S, x1 = col == 0 ? pa.x + ps.x : cb.x - 12 * S;
                float cut = frac(hsh((uint32_t)index, (uint32_t)(ly * 3 + col))) < 0.15f ? (x1 - x0) * 0.6f : 0;
                dl->AddLine(ImVec2(x0, ly), ImVec2(x1 - cut, ly), rgb(0x444444, 0.55f), 2.2f * S);
            }
        rotateSince(dl, vs, (ca + cb) * 0.5f, 2.0f);
        pencil(dl, cb + ImVec2(-150 * S, 6 * S), "вырезка из газеты", rgb(C_PENCIL_BLUE, 0.8f), -2, F.hand, 16 * S);
        ImGui::Dummy(ImVec2(w, chh + 40 * S));
    }
    // the news, as the bulletin's cutting from the local press
    if (c.from == "Embassy bulletin") {
        float cw = std::min(w - 40 * S, 520 * S);
        ImVec2 ca = ImGui::GetCursorScreenPos() + ImVec2(24 * S, 10 * S);
        std::vector<std::string> head;
        {   // the headline, wrapped in the serif face
            std::string hl = upper(c.subject);
            const char* b = hl.c_str(); const char* e = b + hl.size();
            while (b < e) { const char* cut = F.serif->CalcWordWrapPositionA(1.0f, b, e, cw - 28 * S); if (cut <= b) cut = b + 1; head.push_back(std::string(b, cut)); b = cut; while (b < e && *b == ' ') b++; }
        }
        float chh = 60 * S + head.size() * F.serif->FontSize * 1.1f + 100 * S;
        ImVec2 cb = ca + ImVec2(cw, chh);
        int vs = dl->VtxBuffer.Size;
        sheet(dl, ca, cb, rgb(0xE9E3D2), S, true);
        float yy = ca.y + 10 * S;
        dl->AddText(F.tiny, F.tiny->FontSize, ImVec2(ca.x + 14 * S, yy), rgb(0x333333), (std::string(sim.T().city) + "  -  " + fmtDay(c.day)).c_str());
        yy += 18 * S;
        dl->AddLine(ImVec2(ca.x + 14 * S, yy), ImVec2(cb.x - 14 * S, yy), rgb(0x222222), 1.5f);
        yy += 6 * S;
        for (auto& l : head) { dl->AddText(F.serif, F.serif->FontSize, ImVec2(ca.x + 14 * S, yy), rgb(0x161616), l.c_str()); yy += F.serif->FontSize * 1.1f; }
        yy += 8 * S;
        ImVec2 pa(ca.x + 14 * S, yy), ps(120 * S, 86 * S);
        ImTextureID ph = scenePhoto(index + 5000, art::SC_CANDID);
        if (ph) dl->AddImage(ph, pa, pa + ps); else dl->AddRectFilled(pa, pa + ps, rgb(0x777777));
        for (float ly = yy + 2 * S; ly < cb.y - 10 * S; ly += 6 * S) {
            float x1 = cb.x - 14 * S - (frac(hsh((uint32_t)index, (uint32_t)ly)) < 0.15f ? 80 * S : 0);
            dl->AddLine(ImVec2(pa.x + ps.x + 12 * S, ly), ImVec2(x1, ly), rgb(0x444444, 0.5f), 2.2f * S);
        }
        rotateSince(dl, vs, (ca + cb) * 0.5f, -1.5f);
        pencil(dl, ImVec2(cb.x - 160 * S, cb.y + 2 * S), "из местной печати", rgb(C_PENCIL_BLUE, 0.8f), -2, F.hand, 18 * S);
        ImGui::Dummy(ImVec2(w, chh + 34 * S));
    }
    uiTyped(c.body, rgb(C_INK), F.body, [](const std::string& l, ImU32& col, ImFont*& f) {
        if (heading(l)) f = F.bold;
        if (has(l, "NOT ") || has(l, "REFUSED") || has(l, "ADVERSE") || has(l, "FLASH")) col = rgb(C_RED);
    }, (uint32_t)index * 97 + 1);
}

void App::tabCentre() {
    GameState& g = sim.g;
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float leftW = std::floor(avail.x * 0.42f);
    ImGui::BeginGroup();
    // requirements
    float regH = std::floor(avail.y * 0.46f);
    if (docPanel("reqDoc", ImVec2(leftW, regH), C_CARD, 12, "Centre's requirements", nullptr)) {
        ImGui::PushFont(F.ui);
        if (ImGui::BeginTable("reqs", 5, TBL, ImVec2(0, ImGui::GetContentRegionAvail().y))) {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("No.", 0, 0.5f);
            ImGui::TableSetupColumn("Line", 0, 1.1f);
            ImGui::TableSetupColumn("Requirement", 0, 4.4f);
            ImGui::TableSetupColumn("Due", 0, 1.1f);
            ImGui::TableSetupColumn("State", 0, 1.3f);
            uiHeaders();
            for (int pass = 0; pass < 2; pass++)
                for (int i = (int)g.reqs.size() - 1; i >= 0; i--) {
                    Requirement& q = g.reqs[i];
                    if ((q.closed ? 1 : 0) != pass) continue;
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::PushID(q.id);
                    ImGui::PushStyleColor(ImGuiCol_Text, q.closed ? rgb4(C_INK_FADED) : rgb4(C_INK));
                    if (ImGui::Selectable(fstr("%d", q.id).c_str(), selReq == q.id, ImGuiSelectableFlags_SpanAllColumns)) selReq = q.id;
                    ImGui::PopID();
                    cell(q.kind == RK_ACTIVE ? "Service A" : q.kind == RK_LINEN ? "Line N" : SECTOR_SHORT[q.sector]);
                    cell(q.title);
                    cell(q.due < 0 ? std::string("standing") : fmtShort(q.due));
                    cell(q.closed ? (q.failed ? "LAPSED" : (q.kind == RK_LINEN ? "carried out" : "satisfied"))
                                  : q.kind == RK_ACTIVE ? fstr("%d of %d placed", (int)q.progress, (int)q.need) : q.kind == RK_LINEN ? std::string("to do")
                                                        : fstr("%d%%", (int)std::min(99.f, q.progress * 100 / q.need)));
                    ImGui::PopStyleColor();
                }
            ImGui::EndTable();
        }
        ImGui::PopFont();
    }
    docEnd();
    ImGui::Dummy(ImVec2(1, 8 * S));
    if (docPanel("reqText", ImVec2(leftW, 0), C_PAPER, 16, "Requirement")) {
        Requirement* q = sim.req(selReq);
        if (q) {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 p = ImGui::GetCursorScreenPos();
            docTitle(fstr("ЗАДАНИЕ № %d", q->id).c_str(), q->kind == RK_ACTIVE ? "Service A (active measures)" : q->kind == RK_LINEN ? "Line N (illegals support)" : SECTOR_LABEL[q->sector]);
            uiTyped(q->title + "\n\n" + q->text + "\n\n" + (q->due >= 0 ? "Required by " + fmtDay(q->due) + "." : std::string("Standing requirement.")), rgb(C_INK), F.body,
                    [](const std::string& l, ImU32&, ImFont*& f) { (void)l; (void)f; }, (uint32_t)q->id);
            if (q->closed) stamp(dl, p + ImVec2(ImGui::GetContentRegionAvail().x - 90 * S, 60 * S), q->failed ? "НЕ ВЫПОЛНЕНО" : "ВЫПОЛНЕНО", rgb(q->failed ? C_RED : C_VIOLET, 0.8f), -8, F.stamp, 24 * S, 1, q->failed ? "LAPSED" : "DONE");
        } else
            uiTyped("Select a requirement to read it.\n\nCollection requirements are met by graded material from agents in that line who are tasked against them. "
                    "Service A requirements are met by placing material through people with influence. Line N tasks are carried out by you in person.",
                    rgb(C_INK_FADED), F.small, nullptr, 2);
    }
    docEnd();
    ImGui::EndGroup();
    ImGui::SameLine(0, 16 * S);
    ImGui::BeginGroup();
    float rw = ImGui::GetContentRegionAvail().x;
    float cabH = std::floor(avail.y * 0.34f);
    if (docPanel("cabDoc", ImVec2(rw, cabH), C_CARD, 12, "Cables", nullptr)) {
        ImGui::PushFont(F.ui);
        if (ImGui::BeginTable("cables", 3, TBL, ImVec2(0, ImGui::GetContentRegionAvail().y - lineH() * 1.15f))) {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Date", 0, 1.2f);
            ImGui::TableSetupColumn("From", 0, 1.6f);
            ImGui::TableSetupColumn("Subject", 0, 5);
            uiHeaders();
            std::vector<int> rows;
            for (int i = (int)g.cables.size() - 1; i >= 0; i--) if (showRoutine || !g.cables[i].routine() || i == selCable) rows.push_back(i);
            ImGuiListClipper cl;
            cl.Begin((int)rows.size());
            while (cl.Step())
                for (int r = cl.DisplayStart; r < cl.DisplayEnd; r++) {
                    int i = rows[(size_t)r];
                    Cable& c = g.cables[i];
                    bool fresh = !c.read && !c.routine();
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::PushID(i);
                    ImGui::PushStyleColor(ImGuiCol_Text, fresh ? rgb4(C_INK) : c.routine() ? rgb4(0x7A7266) : rgb4(C_INK_FADED));
                    ImGui::PushFont(fresh ? F.uiBold : F.ui);
                    ImVec2 rp = ImGui::GetCursorScreenPos();
                    if (ImGui::Selectable(("     " + fmtShort(c.day)).c_str(), selCable == i, ImGuiSelectableFlags_SpanAllColumns)) { selCable = i; c.read = true; }
                    if (fresh) ImGui::GetWindowDrawList()->AddCircleFilled(rp + ImVec2(6 * S, ImGui::GetTextLineHeight() / 2), 3.5f * S, rgb(0xB3241B));
                    if (r == 0) uiRec("cable:0");
                    ImGui::PopID();
                    cell(c.from);
                    cell(c.routine() ? c.subject + "   (routine)" : c.subject);
                    ImGui::PopFont();
                    ImGui::PopStyleColor();
                }
            ImGui::EndTable();
        }
        ImGui::PopFont();
        if (uiBtn("Mark all as read")) for (auto& c : g.cables) c.read = true;
        ImGui::SameLine(0, 16 * S);
        uiCheck("Show routine traffic", &showRoutine);
        if (ImGui::IsItemHovered() && cfg.tooltips) ImGui::SetTooltip("Grades of material, stores issued and the car log are filed without being marked as new.");
    }
    docEnd();
    ImGui::Dummy(ImVec2(1, 8 * S));
    if (docPanel("cable", ImVec2(rw, 0), C_TELETYPE, 18, "Cable")) {
        if (selCable >= 0 && selCable < (int)g.cables.size()) cableView(g.cables[selCable], selCable);
        else uiTyped("Select a cable in the register to read it. Unread cables are in heavier type, with a red dot.", rgb(C_INK_FADED), F.small, nullptr, 1);
    }
    docEnd();
    ImGui::EndGroup();
}

// ============================================================================ OPERATIONS REPORT
void App::tabReport() {
    GameState& g = sim.g;
    if (reportDay < 0 || !g.reports.count(reportDay)) reportDay = g.reports.empty() ? -1 : g.reports.rbegin()->first;
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float w = std::min(avail.x, 1100 * S);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x - w) / 2);
    ImGui::BeginGroup();
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    std::string text;
    auto it = g.reports.find(reportDay);
    bool watched = false;
    if (it != g.reports.end())
        for (auto& l : it->second) {
            text += l + "\n";
            if (has(l, "POSSIBLE surveillance") || has(l, "SURVEILLANCE")) watched = true;
        }
    std::string hint = reportDay < 0 ? std::string("No reports yet - plan your day and press End day") : "for " + fmtDay(reportDay);
    if (docPanel("report", ImVec2(w, 0), C_PAPER, 22, "Daily report", hint.c_str())) {
        bool instructor = false;
        float tw = ImGui::GetContentRegionAvail().x - (watched ? 250 * S : 0);
        if (watched) {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 pp = ImGui::GetCursorScreenPos() + ImVec2(tw + 30 * S, 16 * S);
            photo(dl, scenePhoto(reportDay, art::SC_WATCHERS), pp, ImVec2(200 * S, 150 * S), 3.5f, S, true);
            pencil(dl, pp + ImVec2(40 * S, 166 * S), "они?", rgb(C_PENCIL_RED, 0.85f), -4, F.handBig);
        }
        ImGui::BeginChild("rtext", ImVec2(tw, 0), ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);
        uiTyped(text, rgb(C_INK), F.body, [&instructor](const std::string& l, ImU32& c, ImFont*& f) {
            if (l.rfind("THE INSTRUCTOR", 0) == 0) { instructor = true; f = F.bold; c = rgb(C_PENCIL_BLUE); return; }
            if (instructor) { f = F.hand; c = rgb(0x26407A); return; }      // his remarks, in his own hand
            if (has(l, "***")) c = rgb(C_RED);
            else if (heading(l)) f = F.bold;
        }, (uint32_t)reportDay);
        ImGui::EndChild();
    }
    docEnd();
    // earlier and later reports: two small buttons at the right end of the title bar
    {
        ImVec2 keep = ImGui::GetCursorScreenPos();
        float bw = 96 * S, bh = 24 * S;
        ImGui::SetCursorScreenPos(ImVec2(p0.x + w - 2 * bw - 6 * S - 14 * S, p0.y + std::floor((skin::titleBarH(S) - bh) / 2) + 1));
        if (uiBtn("\xE2\x80\xB9 Earlier", ImVec2(bw, bh))) { auto i2 = g.reports.find(reportDay); if (i2 != g.reports.end() && i2 != g.reports.begin()) reportDay = std::prev(i2)->first; }
        ImGui::SameLine(0, 6 * S);
        if (uiBtn("Later \xE2\x80\xBA", ImVec2(bw, bh))) { auto i2 = g.reports.find(reportDay); if (i2 != g.reports.end() && std::next(i2) != g.reports.end()) reportDay = std::next(i2)->first; }
        ImGui::SetCursorScreenPos(keep);
    }
    ImGui::EndGroup();
}

// ============================================================================ STATION
void App::officerView(const Officer& o) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float w = ImGui::GetContentRegionAvail().x;
    ImVec2 ps(105 * S, 140 * S);
    photo(dl, officerPhoto(o), p + ImVec2(w - ps.x - 10 * S, 18 * S), ps, 2.f, S, true);
    if (o.gone) stamp(dl, p + ImVec2(w - ps.x / 2 - 10 * S, 18 * S + ps.y / 2), "ВЫБЫЛ", rgb(C_RED, 0.8f), -12, F.stampBig, 30 * S, 1, "NO LONGER AT THE STATION");
    ImGui::BeginChild("otext", ImVec2(w - ps.x - 30 * S, 0), ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);
    uiTyped(sim.officerFile(o), rgb(C_INK), F.small, [](const std::string& l, ImU32& c, ImFont*& f) {
        if (heading(l)) f = F.bold;
        if (has(l, "NO LONGER")) c = rgb(C_RED);
    }, (uint32_t)o.id);
    ImGui::EndChild();
    ImGui::Dummy(ImVec2(w, std::max(0.f, p.y + ps.y + 40 * S - ImGui::GetCursorScreenPos().y)));
}

void App::tabStation() {
    GameState& g = sim.g;
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float leftW = std::floor(avail.x * 0.40f);
    if (docPanel("myFile", ImVec2(leftW, avail.y), C_PAPER, 18, "Your file")) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos();
        float w = ImGui::GetContentRegionAvail().x;
        photo(dl, myPhoto(), p + ImVec2(w - 118 * S, 6 * S), ImVec2(105 * S, 140 * S), -2.f, S, true);
        ImGui::BeginChild("myText", ImVec2(w - 130 * S, 190 * S), 0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);
        std::string st = sim.stationText();
        size_t cut = st.find("\n\n");
        uiTyped(st.substr(0, cut), rgb(C_INK), F.small, [](const std::string& l, ImU32&, ImFont*& f) { if (heading(l)) f = F.bold; }, 77);
        ImGui::EndChild();
        uiTyped(cut == std::string::npos ? std::string("") : st.substr(cut + 2), rgb(C_INK), F.small, [](const std::string& l, ImU32&, ImFont*& f) { if (heading(l)) f = F.bold; }, 78);
    }
    docEnd();
    ImGui::SameLine(0, 16 * S);
    ImGui::BeginGroup();
    float rw = ImGui::GetContentRegionAvail().x;
    float tabH = std::floor(avail.y * 0.36f);
    if (docPanel("officers", ImVec2(rw, tabH), C_CARD, 12, "Officers of the station", nullptr)) {
        ImGui::PushFont(F.ui);
        if (ImGui::BeginTable("officersT", 4, TBL, ImVec2(0, ImGui::GetContentRegionAvail().y))) {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Officer", 0, 2.2f);
            ImGui::TableSetupColumn("Post", 0, 2.4f);
            ImGui::TableSetupColumn("Assignment", 0, 3.2f);
            ImGui::TableSetupColumn("Cases", 0, 0.7f);
            uiHeaders();
            static const char* OROLE[] = {"Rezident", "Security (Line KR)", "Technical officer", "Case officer, another line", "Case officer, UNDER YOU"};
            static const char* OROLE_T[] = {"Instructor", "Security", "Technical officer", "Fellow trainee", "Fellow trainee"};
            for (int pass = 0; pass < 2; pass++)
                for (auto& o : g.officers) {
                    if ((o.role == ROLE_SUB ? 0 : 1) != pass) continue;
                    int n = 0;
                    for (auto& p : g.persons) if (p.live() && p.handler == o.id) n++;
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::PushID(o.id);
                    ImGui::PushStyleColor(ImGuiCol_Text, o.gone ? rgb4(C_INK_FADED) : rgb4(C_INK));
                    if (ImGui::Selectable(o.title().c_str(), selOfficer == o.id, ImGuiSelectableFlags_SpanAllColumns)) selOfficer = o.id;
                    uiRec("officer:" + o.name);
                    ImGui::PopID();
                    cell(sim.training() ? OROLE_T[o.role] : OROLE[o.role]);
                    cell(o.gone ? "no longer at the station" : o.role == ROLE_SUB ? std::string(ASSIGN_LABEL[o.assign]) + (o.assign == AS_SPOT ? std::string(" - ") + SECTOR_SHORT[o.sector] : std::string("")) : std::string("-"));
                    cell(o.role == ROLE_SUB ? fstr("%d", n) : std::string(""));
                    ImGui::PopStyleColor();
                }
            ImGui::EndTable();
        }
        ImGui::PopFont();
    }
    docEnd();
    ImGui::Dummy(ImVec2(1, 8 * S));
    Officer* o = sim.officer(selOfficer);
    if (docPanel("ofile", ImVec2(rw, 0), C_PAPER, 16, "Officer's file")) {
        bool controls = false;
        if (o && !o->gone && o->role == ROLE_SUB) {
            std::vector<Opt> as, sc, pol;
            for (int i = 0; i < 4; i++) as.push_back({ASSIGN_LABEL[i], i});
            for (int i = 0; i < N_SECTOR; i++) sc.push_back({SECTOR_LABEL[i], i});
            for (int i = 0; i < 3; i++) pol.push_back({POLICY[i], i});
            ImGui::AlignTextToFramePadding(); ImGui::TextUnformatted("Assignment"); ImGui::SameLine(120 * S);
            uiCombo("##assign", o->assign, as, 330 * S);
            if (o->assign == AS_SPOT) { ImGui::SameLine(); uiCombo("##osector", o->sector, sc, 250 * S); }
            ImGui::AlignTextToFramePadding(); ImGui::TextUnformatted("His orders"); ImGui::SameLine(120 * S);
            uiCombo("##opolicy", o->policy, pol, 330 * S);
            controls = true;
        }
        if (o && !o->gone && o->role != ROLE_REZIDENT && o->role != ROLE_KR && !sim.training()) {
            if (o->role == ROLE_SUB) ImGui::SameLine();
            if (uiBtn("Report him to Directorate K")) {
                int id = o->id;
                confirmText = "Report your suspicions of " + o->title() + " to Directorate K, over the head of the station?\n\nIf you are right, a traitor is removed. If you are wrong, you have accused a comrade and everybody will know it.";
                confirmAction = [this, id]() { if (Officer* x = sim.officer(id)) note = sim.denounce(*x); };
            }
            controls = true;
        }
        if (controls) {
            ImVec2 r0 = ImGui::GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddLine(r0 + ImVec2(0, 3 * S), r0 + ImVec2(ImGui::GetContentRegionAvail().x, 3 * S), rgb(C_RULE), 1);
            ImGui::Dummy(ImVec2(1, 8 * S));
        }
        if (o) officerView(*o);
        else uiTyped("Select an officer.\n\nOfficers under you work on their own judgement within the assignment you give them: an officer on Spotting brings in contacts and develops them; "
                     "an officer Handling meets the agents and contacts you have given him (Contacts, 'Case officer'); an officer on Support forms your countersurveillance team.\n\n"
                     "Every officer who watches your back sees where you go and whom you meet. Each file shows what its officer knows of your work.",
                     rgb(C_INK_FADED), F.small, nullptr, 6);
    }
    docEnd();
    ImGui::EndGroup();
}

// ============================================================================ SURVEILLANCE LOG
void App::tabSurveillance() {
    GameState& g = sim.g;
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float leftW = std::floor(avail.x * 0.62f);
    float carsH = std::floor(avail.y * 0.42f);
    ImGui::BeginGroup();
    if (docPanel("survDoc", ImVec2(leftW, avail.y - carsH - 10 * S), C_LEDGER, 12, "Surveillance checks", "Every approach: what you checked, and what you saw")) {
        ImGui::PushFont(F.ui);
        if (ImGui::BeginTable("surv", 8, TBL, ImVec2(0, ImGui::GetContentRegionAvail().y))) {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Date", 0, 1.1f);
            ImGui::TableSetupColumn("Time", 0, 1);
            ImGui::TableSetupColumn("District", 0, 1.3f);
            ImGui::TableSetupColumn("Act", 0, 2.4f);
            ImGui::TableSetupColumn("Check made", 0, 2.8f);
            ImGui::TableSetupColumn("Seen", 0, 1.1f);
            ImGui::TableSetupColumn("Outcome", 0, 1.1f);
            ImGui::TableSetupColumn("Zenit that morning", 0, 3.0f);
            uiHeaders();
            ImGuiListClipper cl;
            cl.Begin((int)g.sdrLog.size());
            while (cl.Step())
                for (int r = cl.DisplayStart; r < cl.DisplayEnd; r++) {
                    SdrLog& l = g.sdrLog[g.sdrLog.size() - 1 - r];
                    auto z = g.zenit.find(l.day);
                    ImGui::TableNextRow();
                    cell(fmtShort(l.day)); cell(BLOCK_LABEL[l.block]); cell(sim.T().district[l.district]); cell(l.act); cell(l.route);
                    cellCol(l.signs == 0 ? "nothing" : l.signs == 1 ? "POSSIBLE" : "CONFIRMED", l.signs ? rgb(C_RED) : rgb(C_INK));
                    cellCol(l.abort ? "ABORTED" : "went ahead", l.abort ? rgb(C_RED) : rgb(C_INK));
                    cell(z == g.zenit.end() ? std::string("") : z->second);
                }
            ImGui::EndTable();
        }
        ImGui::PopFont();
    }
    docEnd();
    ImGui::Dummy(ImVec2(1, 10 * S - ImGui::GetStyle().ItemSpacing.y));
    vehicleCards(leftW, carsH);
    ImGui::EndGroup();
    ImGui::SameLine(0, 16 * S);
    float rw = ImGui::GetContentRegionAvail().x;
    if (docPanel("survText", ImVec2(rw, avail.y), C_PAPER, 16, "The watchers", nullptr)) {
        {   // your own pattern over the last ninety days, as the log shows it - and as their analysts may be seeing it
            Sim::Pattern pt = sim.myPattern(90);
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 o = ImGui::GetCursorScreenPos();
            dl->AddText(F.bold, F.bold->FontSize, o, rgb(C_INK), "YOUR OWN PATTERN - LAST 90 DAYS");
            std::string pw = "Operational acts: " + std::to_string(pt.n) + ".  Predictability: " + sim.patternWord(pt) + ".";
            bool bad = pt.n >= 3 && sim.patternWord(pt).find("varied") == std::string::npos;
            dl->AddText(F.small, F.small->FontSize, o + ImVec2(0, F.bold->FontSize + 4 * S), rgb(bad ? C_RED : C_INK_FADED), pw.c_str());
            float gy = o.y + F.bold->FontSize + F.small->FontSize + 14 * S, cw = std::min(34 * S, (ImGui::GetContentRegionAvail().x - 90 * S) / 7), ch = 18 * S;
            int mx = 1;
            for (auto& row : pt.grid) for (int v : row) mx = std::max(mx, v);
            for (int d = 0; d < 7; d++) {
                const char* dn = DOW[d];
                dl->AddText(F.small, F.small->FontSize, ImVec2(o.x + 84 * S + d * cw + 4 * S, gy), rgb(C_INK_FADED), dn);
                for (int b = 0; b < 3; b++) {
                    ImVec2 a(o.x + 84 * S + d * cw, gy + F.small->FontSize + 4 * S + b * ch), z = a + ImVec2(cw - 3 * S, ch - 3 * S);
                    int v = pt.grid[d][b];
                    dl->AddRectFilled(a, z, v ? rgb(C_PENCIL_RED, 0.18f + 0.72f * v / mx) : rgb(C_RULE, 0.15f));
                    if (v) { std::string n = std::to_string(v); dl->AddText(F.small, F.small->FontSize, a + ImVec2(4 * S, 1 * S), rgb(v * 2 > mx ? 0xFFFFFFu : (unsigned)C_INK), n.c_str()); }
                }
            }
            for (int b = 0; b < 3; b++) dl->AddText(F.small, F.small->FontSize, ImVec2(o.x, gy + F.small->FontSize + 4 * S + b * ch + 1 * S), rgb(C_INK_FADED), BLOCK_LABEL[b]);
            float ty = gy + F.small->FontSize + 8 * S + 3 * ch;
            if (pt.topDist >= 0) {
                std::string dd = "Most used district: " + std::string(sim.T().district[pt.topDist]) + fstr(" (%d of %d)", pt.topDistN, pt.n);
                dl->AddText(F.small, F.small->FontSize, ImVec2(o.x, ty), rgb(C_INK_FADED), dd.c_str());
            }
            ImGui::Dummy(ImVec2(10, ty - o.y + F.small->FontSize + 10 * S));
            if (ImGui::IsItemHovered() && cfg.tooltips) ImGui::SetTooltip("Every operational act of yours over the last ninety days, by weekday and time of day.\nThe other side's analysts build the same table from what their teams see. A dark square is where they will be waiting.");
        }
        int key = 0;
        for (int i = (int)g.sdrLog.size() - 1; i >= 0; i--) if (g.sdrLog[i].signs > 0) { key = g.sdrLog[i].day; break; }
        float pw = std::min(ImGui::GetContentRegionAvail().x - 70 * S, 250 * S);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 pp = ImGui::GetCursorScreenPos() + ImVec2(10 * S, 20 * S);
        photo(dl, scenePhoto(key + 1, art::SC_WATCHERS), pp, ImVec2(pw, pw * 0.75f), -1.8f, S, true);
        pencil(dl, pp + ImVec2(pw + 14 * S, pw * 0.3f), key > 0 ? fmtShort(key) : std::string("?"), rgb(C_PENCIL_RED, 0.85f), -6, F.hand);
        ImGui::Dummy(ImVec2(pw, pw * 0.75f + 40 * S));
        uiTyped(sim.survSummary(), rgb(C_INK), F.small, [](const std::string& l, ImU32& c, ImFont*& f) {
            if (heading(l) && !has(l, "  ")) f = F.bold;
            if (has(l, "priority target") || has(l, "stand down")) c = rgb(C_RED);
        }, 31);
    }
    docEnd();
}

// ============================================================================ THE WATCHERS' CARS
static unsigned carColour(const std::string& c) {
    return c == "grey" ? 0x8A8D90 : c == "dark blue" ? 0x2C3E66 : c == "beige" ? 0xD2C29A : c == "dark green" ? 0x2F5135 : c == "black" ? 0x262626
         : c == "white" ? 0xECEBE4 : c == "maroon" ? 0x6B1E24 : c == "light blue" ? 0x8DB4D2 : 0x6A4A32;
}

void App::vehicleCards(float w, float height) {
    GameState& g = sim.g;
    if (!docPanel("cars", ImVec2(w, height), C_MANILA, 12, "The watchers' cars", "A car you know by its plate is a team you spot at once")) { docEnd(); return; }
    if (g.watchers.empty()) {
        uiTyped("Nothing logged yet. Each time you see surveillance - or think you do - the car goes into this file. Some will be theirs, some will be a dentist's. "
                "Line KR checks one entry a month. A car whose plate you know is recognised at once; the watchers change their plates every few months.",
                rgb(C_INK_FADED), F.small, nullptr, 3);
        docEnd();
        return;
    }
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float cw = 214 * S, ch = 150 * S, gap = 12 * S;
    int perRow = std::max(1, (int)((ImGui::GetContentRegionAvail().x + gap) / (cw + gap)));
    int shown = 0;
    // newest first; struck-off entries last
    std::vector<const Watcher*> list;
    for (int pass = 0; pass < 2; pass++)
        for (int i = (int)g.watchers.size() - 1; i >= 0; i--) if (g.watchers[i].struck == (pass == 1)) list.push_back(&g.watchers[i]);
    for (const Watcher* wp : list) {
        const Watcher& v = *wp;
        if (shown % perRow) ImGui::SameLine(0, gap);
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::PushID(v.id);
        ImGui::BeginGroup();
        sheet(dl, p, p + ImVec2(cw, ch), rgb(v.struck ? 0xE9E3D2 : 0xF8F4E4), S, true);
        for (float ly = p.y + 64 * S; ly < p.y + ch - 26 * S; ly += 16 * S) dl->AddLine(ImVec2(p.x + 4 * S, ly), ImVec2(p.x + cw - 4 * S, ly), rgb(0x7A9CC6, 0.25f), 1);
        carSketch(dl, p + ImVec2(cw * 0.36f, 30 * S), cw * 0.56f, rgb(carColour(v.colour), v.struck ? 0.35f : 0.9f), era(), (uint32_t)v.id);
        // the number plate, in the local style
        int th = g.theatre;
        bool dark = th == TH_LONDON || th == TH_MOSCOW || th == TH_DELHI || th == TH_VIENNA;
        ImVec2 pa = p + ImVec2(cw * 0.66f, 14 * S);
        ImVec2 ts = F.bold->CalcTextSizeA(F.small->FontSize, FLT_MAX, 0, v.plate.c_str());
        ImVec2 pb = pa + ImVec2(ts.x + 10 * S, ts.y + 4 * S);
        if (pb.x > p.x + cw - 6 * S) { pa.x -= pb.x - (p.x + cw - 6 * S); pb.x = p.x + cw - 6 * S; }
        dl->AddRectFilled(pa, pb, rgb(dark ? 0x1C1C1C : (th == TH_WASHINGTON ? 0xF4F1E6 : 0xF7F7F2)), 2 * S);
        dl->AddRect(pa, pb, rgb(0x1C1C1C), 2 * S, 0, 1.2f);
        dl->AddText(F.bold, F.small->FontSize, pa + ImVec2(5 * S, 2 * S), dark ? rgb(0xE8E8E0) : (th == TH_WASHINGTON ? rgb(0x1E3A6E) : rgb(0x111111)), v.plate.c_str());
        float y = p.y + 66 * S;
        std::string l1 = v.colour + " " + v.make;
        std::string l2 = fstr("seen %d time%s; last ", v.seen, v.seen == 1 ? "" : "s") + fmtShort(v.lastDay);
        std::string l3;
        for (int d : v.districts) l3 += (l3.empty() ? "" : ", ") + std::string(sim.T().district[d]);
        dl->PushClipRect(p, p + ImVec2(cw - 4 * S, ch), true);
        ImU32 ink = rgb(C_INK, v.struck ? 0.45f : 1);
        typeLine(dl, F.small, F.small->FontSize, ImVec2(p.x + 10 * S, y - 2 * S), ink, l1.c_str(), l1.c_str() + l1.size(), (uint32_t)v.id);
        typeLine(dl, F.tiny, F.tiny->FontSize, ImVec2(p.x + 10 * S, y + 16 * S), ink, l2.c_str(), l2.c_str() + l2.size(), (uint32_t)v.id + 1);
        typeLine(dl, F.tiny, F.tiny->FontSize, ImVec2(p.x + 10 * S, y + 32 * S), ink, l3.c_str(), l3.c_str() + l3.size(), (uint32_t)v.id + 2);
        dl->PopClipRect();
        ImVec2 sp = p + ImVec2(cw - 56 * S, ch - 22 * S);
        if (v.kr == 1) stamp(dl, sp, "ИХ", rgb(C_RED, 0.8f), -8, F.stamp, 17 * S, 1, "KR: THEIRS");
        else if (v.kr == 2) stamp(dl, sp, "ЧАСТНАЯ", rgb(C_VIOLET, 0.8f), 6, F.stamp, 15 * S, 0, "KR: PRIVATE");
        else if (v.kr == 3) stamp(dl, sp, "СНЯТА", rgb(C_INK, 0.7f), -5, F.stamp, 15 * S, 0, "KR: OUT OF USE");
        if (v.struck) {
            dl->AddLine(p + ImVec2(12 * S, 10 * S), p + ImVec2(cw - 12 * S, ch - 30 * S), rgb(C_PENCIL_RED, 0.6f), 2 * S);
            dl->AddLine(p + ImVec2(cw - 12 * S, 10 * S), p + ImVec2(12 * S, ch - 30 * S), rgb(C_PENCIL_RED, 0.6f), 2 * S);
        }
        ImGui::SetCursorScreenPos(p + ImVec2(8 * S, ch - 26 * S));
        ImGui::PushFont(F.uiSmall);
        if (ImGui::SmallButton(v.struck ? "Restore" : "Strike off")) sim.strikeWatcher(v.id, !v.struck);
        uiRec(fstr("car:%d", shown));
        ImGui::PopFont();
        ImGui::SetCursorScreenPos(p);
        ImGui::Dummy(ImVec2(cw, ch));
        ImGui::EndGroup();
        ImGui::PopID();
        shown++;
    }
    docEnd();
}

// ============================================================================ RETURNS
namespace {
/** One chart on graph paper. xs: month index -> x; the caller draws the series. */
struct Chart {
    ImVec2 a, b;           // the plotting area
    int n;                 // months
    float lo, hi;
    float X(int i) const { return n <= 1 ? a.x : a.x + (b.x - a.x) * i / (float)(n - 1); }
    float Y(float v) const { return b.y - (b.y - a.y) * clampf((v - lo) / (hi - lo), 0, 1); }
};
void graphPaper(ImDrawList* dl, ImVec2 a, ImVec2 b, float S) {
    dl->AddRectFilled(a, b, rgb(0xF4EFDF));
    float step = 5 * S;
    int k = 0;
    for (float x = a.x; x <= b.x; x += step, k++) dl->AddLine(ImVec2(x, a.y), ImVec2(x, b.y), rgb(0xD9845B, k % 5 ? 0.12f : 0.30f), 1);
    k = 0;
    for (float y = a.y; y <= b.y; y += step, k++) dl->AddLine(ImVec2(a.x, y), ImVec2(b.x, y), rgb(0xD9845B, k % 5 ? 0.12f : 0.30f), 1);
    dl->AddRect(a, b, rgb(0xB0603C, 0.6f));
}
}  // namespace

void App::tabReturns() {
    GameState& g = sim.g;
    ImVec2 avail = ImGui::GetContentRegionAvail();
    if (!docPanel("returns", ImVec2(avail.x, avail.y), C_PAPER, 18, "Results", "Monthly figures for the whole career - hover over a chart to read a month", ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) { docEnd(); return; }
    const auto& M = g.months;
    if (M.size() < 2) {
        uiTyped("The first returns are made at the end of your first month at a station. From then on every month is entered here: the material Centre graded, "
                "the people you found, the agents on your books, your standing with your chief and with Centre, and what Line KR thinks the other side thinks of you.",
                rgb(C_INK_FADED), F.body, nullptr, 2);
        docEnd();
        return;
    }
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 o = ImGui::GetCursorScreenPos();
    ImVec2 area = ImGui::GetContentRegionAvail();
    int n = (int)M.size();
    float gapX = 26 * S, gapY = 30 * S;
    float cw = (area.x - gapX) / 2, chh = (area.y - gapY - 8 * S) / 2;
    float mouseX = ImGui::GetIO().MousePos.x;
    int hover = -1;
    struct Panel { const char* ru; const char* en; };
    Panel P[4] = {{"Material", "graded (bars) and its worth (line)"}, {"Standing", "with your chief (ink) and with Centre (red)"},
                  {"Agents", "on your books (line), and new contacts (bars)"}, {"The watchers", "Line KR's view of their interest in you"}};
    float maxRolls = 1, maxValue = 1, maxAgents = 1, maxSpot = 1;
    for (auto& m : M) { maxRolls = std::max(maxRolls, (float)m.rolls); maxValue = std::max(maxValue, m.value); maxAgents = std::max(maxAgents, (float)m.agents); maxSpot = std::max(maxSpot, (float)m.spotted); }
    for (int k = 0; k < 4; k++) {
        ImVec2 pa = o + ImVec2((k % 2) * (cw + gapX), (k / 2) * (chh + gapY));
        ImVec2 pb = pa + ImVec2(cw, chh);
        dl->AddText(F.uiBold, F.uiBold->FontSize, pa, rgb(C_INK), P[k].ru);
        float tw = F.uiBold->CalcTextSizeA(F.uiBold->FontSize, FLT_MAX, 0, P[k].ru).x;
        dl->AddText(F.uiSmall, F.uiSmall->FontSize, ImVec2(pa.x + tw + 8 * S, pa.y + F.uiBold->Ascent - F.uiSmall->Ascent), rgb(0x6A6256), P[k].en);
        ImVec2 ga = pa + ImVec2(44 * S, F.stamp->FontSize + 6 * S), gb = pb - ImVec2(6 * S, 22 * S);
        graphPaper(dl, ga, gb, S);
        float bw = std::min(24 * S, std::max(1.5f, (gb.x - ga.x - 12 * S) / n * 0.7f));
        float inset = std::max(6 * S, bw / 2 + 3 * S);
        Chart c{ga + ImVec2(inset, 8 * S), gb - ImVec2(inset, 4 * S), n, 0, 1};
        // the postings, as bands with the city's name
        int start = 0;
        for (int i = 1; i <= n; i++) {
            if (i < n && M[i].theatre == M[start].theatre) continue;
            float x0 = c.X(start), x1 = c.X(i - 1);
            if (start > 0) for (float y = ga.y; y < gb.y; y += 6 * S) dl->AddLine(ImVec2(x0 - 1, y), ImVec2(x0 - 1, y + 3 * S), rgb(C_INK, 0.45f), 1);
            const char* city = THEATRES[M[start].theatre].city;
            if (x1 - x0 > 50 * S) dl->AddText(F.uiSmall, F.uiSmall->FontSize, ImVec2(x0 + 4 * S, gb.y + 3 * S), rgb(C_INK_FADED), city);
            start = i;
        }
        auto polyline = [&](const std::vector<ImVec2>& pts, ImU32 col, float th) { if (pts.size() > 1) dl->AddPolyline(pts.data(), (int)pts.size(), col, 0, th); };
        // a scale in round numbers: sets c.hi to a whole number of steps and labels them
        auto axis = [&](float maxV) {
            static const float STEPS[] = {1, 2, 5, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000};
            float step = STEPS[0];
            for (float s2 : STEPS) { step = s2; if (maxV / s2 <= 5) break; }
            c.hi = std::max(step, std::ceil(maxV / step) * step);
            for (float v = 0; v <= c.hi + 0.01f; v += step) {
                std::string s = fstr("%.0f", v);
                float sw = F.tiny->CalcTextSizeA(F.tiny->FontSize, FLT_MAX, 0, s.c_str()).x;
                dl->AddText(F.tiny, F.tiny->FontSize, ImVec2(ga.x - sw - 4 * S, c.Y(v) - F.tiny->FontSize / 2), rgb(C_INK_FADED), s.c_str());
            }
        };
        if (k == 0) {
            axis(maxRolls);
            for (int i = 0; i < n; i++) if (M[i].rolls) dl->AddRectFilled(ImVec2(c.X(i) - bw / 2, c.Y((float)M[i].rolls)), ImVec2(c.X(i) + bw / 2, c.b.y), rgb(0x6F7C8C, 0.55f));
            std::vector<ImVec2> pts;
            for (int i = 0; i < n; i++) pts.push_back(ImVec2(c.X(i), c.b.y - (c.b.y - c.a.y) * clampf(M[i].value / (maxValue * 1.15f), 0, 1)));
            polyline(pts, rgb(C_RED, 0.9f), 1.8f * S);
        } else if (k == 1) {
            c.hi = 100;
            static const char* const BAND[] = {"poor", "fair", "good", "v.good", "excel."};
            for (int t = 0; t < 5; t++) dl->AddText(F.tiny, F.tiny->FontSize, ImVec2(ga.x - 42 * S, c.Y(t * 20 + 10.f) - F.tiny->FontSize / 2), rgb(C_INK_FADED), BAND[t]);
            std::vector<ImVec2> a1, a2;
            for (int i = 0; i < n; i++) { a1.push_back(ImVec2(c.X(i), c.Y(M[i].rez))); a2.push_back(ImVec2(c.X(i), c.Y(M[i].centre))); }
            polyline(a1, rgb(C_INK, 0.85f), 1.8f * S);
            polyline(a2, rgb(C_RED, 0.9f), 1.8f * S);
        } else if (k == 2) {
            axis(std::max(maxAgents, maxSpot));
            for (int i = 0; i < n; i++) if (M[i].spotted) dl->AddRectFilled(ImVec2(c.X(i) - bw / 2, c.Y((float)M[i].spotted)), ImVec2(c.X(i) + bw / 2, c.b.y), rgb(0x8C9A6E, 0.55f));
            std::vector<ImVec2> pts;
            for (int i = 0; i < n; i++) {
                if (i > 0) pts.push_back(ImVec2(c.X(i), c.Y((float)M[i - 1].agents)));
                pts.push_back(ImVec2(c.X(i), c.Y((float)M[i].agents)));
            }
            polyline(pts, rgb(C_INK, 0.9f), 1.8f * S);
        } else {
            c.lo = -0.5f; c.hi = 3.5f;
            static const char* const KR[] = {"not seen", "periodic", "regular", "priority"};
            for (int t = 0; t < 4; t++) dl->AddText(F.tiny, F.tiny->FontSize, ImVec2(ga.x - 44 * S, c.Y((float)t) - F.tiny->FontSize / 2), rgb(C_INK_FADED), KR[t]);
            std::vector<ImVec2> pts;
            int last = -1;
            for (int i = 0; i < n; i++) {
                if (M[i].kr < 0 || (i > 0 && M[i].theatre != M[i - 1].theatre)) { polyline(pts, rgb(C_PENCIL_BLUE, 0.9f), 2 * S); pts.clear(); last = -1; if (M[i].kr < 0) continue; }
                int band = M[i].kr < 22 ? 0 : M[i].kr < 45 ? 1 : M[i].kr < 68 ? 2 : 3;
                if (last >= 0) pts.push_back(ImVec2(c.X(i), c.Y((float)last)));
                pts.push_back(ImVec2(c.X(i), c.Y((float)band)));
                last = band;
            }
            polyline(pts, rgb(C_PENCIL_BLUE, 0.9f), 2 * S);
        }
        if (mouseX >= c.a.x - bw && mouseX <= c.b.x + bw && ImGui::IsMouseHoveringRect(ga, gb)) {
            hover = (int)std::lround((mouseX - c.a.x) / std::max(1.f, c.b.x - c.a.x) * (n - 1));
            hover = std::max(0, std::min(n - 1, hover));
            dl->AddLine(ImVec2(c.X(hover), ga.y), ImVec2(c.X(hover), gb.y), rgb(C_INK, 0.5f), 1);
        }
    }
    ImGui::Dummy(area);
    if (hover >= 0) {
        const MonthRec& m = M[hover];
        Date d = dateOf(m.day);
        static const char* const KR[] = {"not noticed", "periodic coverage", "regular coverage", "priority target"};
        ImGui::SetTooltip("%s %d - %s\nMaterial graded: %d (worth %.0f)\nMeetings: %d   aborted acts: %d\nNew contacts: %d   agents on the books: %d\nStanding: chief %s, Centre %s\nLine KR: %s\nCash at the month's end: %d %s",
                          MONTH[d.m], d.y, THEATRES[m.theatre].city, m.rolls, m.value, m.meetings, m.aborts, m.spotted, m.agents, word(m.rez, W_STAND, 5).c_str(),
                          word(m.centre, W_STAND, 5).c_str(), m.kr < 0 ? "no assessment" : KR[m.kr < 22 ? 0 : m.kr < 45 ? 1 : m.kr < 68 ? 2 : 3], m.funds, THEATRES[m.theatre].currency);
    }
    docEnd();
}

// ============================================================================ ACCOUNTS
// ============================================================================ private life
void App::tabPrivate() {
    GameState& g = sim.g;
    const Theatre& t = sim.T();
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float leftW = std::floor(avail.x * 0.54f);
    if (docPanel("privDoc", ImVec2(leftW, avail.y), C_PAPER, 18, "Private life", "None of this is in any report")) {
        uiTyped(sim.privateText(), rgb(C_INK), F.body, [](const std::string& l, ImU32& c, ImFont*& f) {
            if (heading(l)) f = F.bold;
            if (has(l, "very bad") || has(l, "strained") || has(l, "Not taken") || has(l, "NOT FULFILLED") || has(l, "hostile")) c = rgb(C_RED);
        }, 77);
    }
    docEnd();
    ImGui::SameLine(0, 16 * S);
    if (docPanel("privAct", ImVec2(ImGui::GetContentRegionAvail().x, avail.y), C_CARD, 18, "What can be done", nullptr)) {
        if (sim.training()) { ImGui::TextUnformatted("Nothing, on the course."); docEnd(); return; }
        float W = ImGui::GetContentRegionAvail().x;
        auto head = [&](const char* s) { ImGui::Dummy(ImVec2(1, 6 * S)); ImGui::PushFont(F.uiBold); ImGui::TextUnformatted(s); ImGui::PopFont(); };
        auto hint = [&](const std::string& s) { ImGui::PushFont(F.ui); ImGui::PushStyleColor(ImGuiCol_Text, rgb4(C_INK_FADED)); ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + W - 8 * S); ImGui::TextUnformatted(s.c_str()); ImGui::PopTextWrapPos(); ImGui::PopStyleColor(); ImGui::PopFont(); };
        // ---- home
        head("AT HOME");
        if (g.wifeHere) {
            if (uiBtn(fstr("Something for the family  (%d %s)##gift", sim.P(15), t.currency).c_str())) note = sim.buyHomeGift();
            uiRec("home:gift");
            hint("Evenings and weekends at home mend things slowly; evenings out, weekends worked and escort duty wear them down. Money helps for a while.");
            std::vector<Opt> ss;
            ss.push_back({"nowhere in particular", -1});
            for (auto& s : g.sites) if (!s.retired && s.type == ST_SIGNAL && !sim.eastSide(s.district)) ss.push_back({clip(s.name, 52), s.id});
            wifeSite = g.wifeSignal;
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Her shopping takes her past");
            if (uiCombo("##wifesig", wifeSite, ss, W - 8 * S)) { std::string r = sim.setWifeSignal(wifeSite); if (wifeSite >= 0 || g.wifeSignal != wifeSite) note = r; }
            hint("Each weekday she reads the signal site for you, which saves you the walk. It costs her something, and if they follow you they follow her.");
        } else hint("Your wife and children are in Moscow.");
        // ---- leave
        head("LEAVE");
        if (sim.onLeave(g.day)) hint("You are on leave. Use 'Skip ahead' to pass the days.");
        else if (g.leaveFrom > g.day) {
            if (uiBtn("Cancel the leave")) note = sim.cancelLeave();
        } else {
            std::vector<Opt> ws;
            for (int w : {2, 3, 4, 6, 8, 12, 16, 20}) ws.push_back({fstr("from %s (in %d weeks)", fmtDay(g.day + w * 7).c_str(), w), w});
            uiCombo("##leavew", leaveWeeks, ws, W * 0.62f);
            ImGui::SameLine();
            if (uiBtn("Apply for leave")) note = sim.bookLeave(g.day + leaveWeeks * 7);
            uiRec("home:leave");
            hint("Thirty days in the Union, once a year after the first eight months. Nothing you ordered for those days is done; agreed meetings move to your return; nobody empties your drops. "
                 "You come back rested, with things mended at home - and you may call on people at Centre.");
        }
        // ---- money
        head("YOUR OWN MONEY");
        {
            Flow fl;
            if (fl.btn(fstr("Buy something for Moscow  (%d %s)##goods", sim.P(40), t.currency).c_str())) note = sim.buyGoods();
            uiRec("home:goods");
            if (fl.btn(fstr("%s  (%d %s)##chief", t.role < 3 ? "A present for the Rezident" : "A parcel for the head of department", sim.P(30), t.currency).c_str())) note = sim.giftChief();
            for (int k = 0; k < 3; k++)
                if (!(g.assets & (1 << k)) && fl.btn(fstr("%s  (%d %s)##asset%d", ASSET_LABEL[k], sim.P(ASSET_COST[k]), t.currency, k).c_str())) note = sim.buyAsset(k);
        }
        hint("Things bought here are worth three times as much in Moscow: as presents to the people who decide your next posting, or in a commission shop. Both happen on leave.");
        std::vector<Opt> sk = {{"nothing - every agent gets what he signs for", 0}, {"a tenth of what you hand them", 1}, {"a fifth", 2}, {"three tenths", 3}};
        skimSel = g.skim;
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Keep back from the agents' money");
        if (uiCombo("##skim", skimSel, sk, W - 8 * S)) {
            if (skimSel > g.skim) {
                int v = skimSel;
                confirmText = "Keep back part of what your agents are paid?\n\nThe agent signs for the whole sum, so the books balance. He knows, of course, and it does not make him love you; an agent who is really theirs tells them; "
                              "an auditor who compares receipts with what agents say may find it; and an agent handed to another officer may tell him. A small sum is a reprimand. A large one is the end of a career, unless somebody at Centre speaks for you.";
                confirmAction = [this, v]() { note = sim.setSkim(v); };
            } else note = sim.setSkim(skimSel);
        }
    }
    docEnd();
}

void App::tabAccounts() {
    GameState& g = sim.g;
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float leftW = std::floor(avail.x * 0.56f);
    if (docPanel("accDoc", ImVec2(leftW, avail.y), C_LEDGER, 12, "Cash book", "Operational funds")) {
        ImGui::PushFont(F.ui);
        if (ImGui::BeginTable("acc", 5, TBL, ImVec2(0, ImGui::GetContentRegionAvail().y))) {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Date", 0, 1.2f);
            ImGui::TableSetupColumn("Entry", 0, 6);
            ImGui::TableSetupColumn("Out", 0, 0.9f);
            ImGui::TableSetupColumn("In", 0, 0.9f);
            ImGui::TableSetupColumn("Balance", 0, 1);
            uiHeaders();
            ImGuiListClipper cl;
            cl.Begin((int)g.ledger.size());
            while (cl.Step())
                for (int r = cl.DisplayStart; r < cl.DisplayEnd; r++) {
                    Ledger& l = g.ledger[g.ledger.size() - 1 - r];
                    ImGui::TableNextRow();
                    cell(fmtShort(l.day)); cell(l.what);
                    cellCol(l.amt < 0 ? fstr("%d", -l.amt) : std::string(""), rgb(C_RED));
                    cell(l.amt > 0 ? fstr("%d", l.amt) : std::string(""));
                    cellCol(fstr("%d", l.bal), l.bal < 0 ? rgb(C_RED) : rgb(C_INK));
                }
            ImGui::EndTable();
        }
        ImGui::PopFont();
    }
    docEnd();
    ImGui::SameLine(0, 16 * S);
    ImVec2 p = ImGui::GetCursorScreenPos();
    float rw = ImGui::GetContentRegionAvail().x;
    if (docPanel("accText", ImVec2(rw, avail.y), C_PAPER, 16, "Account", nullptr)) {
        uiTyped(sim.accSummary(), rgb(C_INK), F.small, [](const std::string& l, ImU32&, ImFont*& f) { if (heading(l)) f = F.bold; }, 41);
    }
    docEnd();
    const Theatre& t = sim.T();
    cash(ImGui::GetWindowDrawList(), p + ImVec2(rw - 70 * S, 70 * S), 70 * S, curSym(t.currency));
}

// ============================================================================ CAREER, MANUAL
void App::tabCareer() {
    GameState& g = sim.g;
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float sideW = 250 * S;
    if (docPanel("career", ImVec2(avail.x - sideW - 16 * S, avail.y), C_PAPER_OLD, 22, "Service record", nullptr)) {
        uiTyped(sim.careerText(), rgb(C_INK), F.small, [](const std::string& l, ImU32& c, ImFont*& f) {
            if (heading(l)) f = F.bold;
            if (has(l, "PERSONA NON GRATA") || has(l, "RECALLED") || has(l, "YOU ARE HERE")) c = rgb(C_RED);
        }, 51);
    }
    docEnd();
    ImGui::SameLine(0, 16 * S);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    // a velvet board with the decorations, in a frame
    ImVec2 a = p, b = p + ImVec2(sideW, avail.y);
    skin::frame(dl, a, b, S);
    float bh = skin::titleBar(dl, a, sideW, S, "Decorations", nullptr);
    ImVec2 va = a + ImVec2(8 * S, bh + 6 * S), vb = b - ImVec2(8 * S, 58 * S);
    dl->AddRectFilledMultiColor(va, vb, rgb(0x55161A), rgb(0x55161A), rgb(0x2C0B0D), rgb(0x2C0B0D));
    dl->AddRect(va + ImVec2(6 * S, 6 * S), vb - ImVec2(6 * S, 6 * S), rgb(C_GOLD, 0.35f), 0, 0, 1);
    dl->AddRect(va, vb, rgb(0, 0.5f), 0, 0, 2 * S);
    float oy = va.y - a.y - 4 * S;                          // everything on the board sits below the bar
    hammerSickle(dl, ImVec2(a.x + sideW / 2, a.y + oy + 40 * S), 52 * S, rgb(C_GOLD));
    photo(dl, myPhoto(), a + ImVec2(sideW / 2 - 55 * S, oy + 78 * S), ImVec2(110 * S, 146 * S), 0, S, false);
    std::string nm = ru(g.officer);
    float nw = F.stamp->CalcTextSizeA(F.stamp->FontSize, FLT_MAX, 0, nm.c_str()).x;
    dl->AddText(F.stamp, F.stamp->FontSize, ImVec2(a.x + (sideW - nw) / 2, a.y + oy + 234 * S), rgb(C_GOLD), nm.c_str());
    float rx = F.tiny->CalcTextSizeA(F.tiny->FontSize, FLT_MAX, 0, RANK[g.rank]).x;
    dl->AddText(F.tiny, F.tiny->FontSize, ImVec2(a.x + (sideW - rx) / 2, a.y + oy + 234 * S + F.stamp->FontSize), rgb(0xF0E2C0), RANK[g.rank]);
    ImGui::SetCursorScreenPos(ImVec2(a.x + 12 * S, b.y - 46 * S));
    if (uiBtn("Export as a text file", ImVec2(sideW - 24 * S, 0))) {
        std::string nm2;
        for (char ch : g.officer) if (isalnum((unsigned char)ch)) nm2 += ch;
        std::string path = saveDir + "/Personal file - " + nm2 + ".txt";
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::u8path(saveDir), ec);
        std::ofstream f(std::filesystem::u8path(path), std::ios::binary);
        if (f) { std::string t = sim.personalFile(); f.write(t.data(), (std::streamsize)t.size()); }
        note = f ? "Your personal file, as it stands today, is written to:\n" + path : std::string("The file could not be written.");
        if (f && openFile) { f.close(); openFile(path); }
    }
    if (ImGui::IsItemHovered() && cfg.tooltips) ImGui::SetTooltip("Writes the service record, attestations, private life and notebook to a text file\nin the saved-games folder - to keep, or to show somebody.");
    float y = a.y + oy + 288 * S;
    if (g.awards.empty()) {
        const char* t = "no decorations yet";
        float tw = F.small->CalcTextSizeA(F.small->FontSize, FLT_MAX, 0, t).x;
        dl->AddText(F.small, F.small->FontSize, ImVec2(a.x + (sideW - tw) / 2, y + 20 * S), rgb(0xF0E2C0, 0.6f), t);
    }
    for (size_t i = 0; i < g.awards.size() && y < vb.y - 90 * S; i++) {
        const std::string& aw = g.awards[i];
        medal(dl, ImVec2(a.x + 50 * S, y + 50 * S), 56 * S, has(aw, "Banner"));
        ImGui::SetCursorScreenPos(ImVec2(a.x + 92 * S, y + 22 * S));
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + sideW - 104 * S);
        ImGui::PushFont(F.uiSmall);
        ImGui::TextColored(rgb4(0xF0E2C0), "%s", aw.c_str());
        ImGui::PopFont();
        ImGui::PopTextWrapPos();
        y += 104 * S;
    }
    ImGui::SetCursorScreenPos(p);
    ImGui::Dummy(ImVec2(sideW, avail.y));
}


// ============================================================================ CASE BOARD
/**
 * A cork board on the wall of your office: the officers of the station on the left, your agents and
 * the people you are developing in the middle, the sites on the right - and string between them for
 * everything your own files say one knows of the other. Nothing hidden is used: a red string is only
 * drawn where the file records that an officer knows of a man or a place.
 */
void App::tabBoard() {
    GameState& g = sim.g;
    struct Card { int kind, id; ImVec2 c; float w, h; std::string name, sub; ImU32 flag; std::string flagTxt, more; };
    struct Link { int a, b, kind; };      // kind 0: knows of (red string), 1: his communications plan (black thread), 2: runs him (heavy)
    enum { K_OFF, K_AGENT, K_CONTACT, K_SITE };
    std::vector<Card> cards;
    std::vector<Link> links;
    std::map<long long, int> at;
    auto key = [](int kind, int id) { return (long long)kind * 1000000 + id; };
    auto inPlan = [](const Person& p, int sid) { return p.dropSite == sid || p.signalSite == sid || std::find(p.meetSites.begin(), p.meetSites.end(), sid) != p.meetSites.end(); };

    // ---- who goes on the board
    static const char* const OROLE[] = {"Rezident", "Security (Line KR)", "Technical officer", "Colleague", "Your officer"};
    for (auto& o : g.officers) {
        if (o.gone) continue;
        int n = 0;
        for (auto& p : g.persons) if (p.live() && (p.handler == o.id || std::find(p.witting.begin(), p.witting.end(), o.id) != p.witting.end())) n++;
        Card c{K_OFF, o.id, {}, 0, 0, o.title(), n ? fstr("knows %d of yours", n) : std::string("knows none of yours"), 0, "", OROLE[std::max(0, std::min(4, o.role))]};
        if (n >= 3) { c.flag = rgb(0xD08A2A); c.flagTxt = "knows a great deal"; }
        for (auto& f : o.file) if (has(f, "stake-out was waiting")) { c.flag = rgb(0xC0392B); c.flagTxt = "a marked card was answered"; }
        cards.push_back(c);
    }
    for (int pass = 0; pass < 2; pass++)
        for (auto& p : g.persons) {
            if (!p.live() || (pass == 0) != (p.stage == AGENT)) continue;
            Card c{pass == 0 ? K_AGENT : K_CONTACT, p.id, {}, 0, 0, p.label(), "", 0, "", p.label() == p.name ? std::string("") : p.name};
            c.sub = pass == 0 ? (p.handler ? "run by " + sim.handlerName(p) : std::string("run by you")) + std::string("  -  ") + SECTOR_SHORT[p.sector]
                              : std::string(STAGE_LABEL[p.stage]) + "  -  " + word(p.rapport, W_RAPPORT, 5);
            int queried = 0;
            for (auto& x : g.rolls) if (x.agentId == p.id && has(x.grade, "queried")) queried++;
            bool seen = false;
            for (auto& f : p.file) if (has(f, "SURVEILLANCE OBSERVED") || has(f, "KNOWN TO THE ADVERSARY") || has(f, "observed by the adversary")) seen = true;
            Site* sg = sim.site(p.signalSite);
            if (sg && sg->markSeen == 2) { c.flag = rgb(0xC0392B); c.flagTxt = "DANGER signal seen"; }
            else if (p.verify == 3 && p.verifyResult != "CONSISTENT") { c.flag = rgb(0xC0392B); c.flagTxt = "failed verification"; }
            else if (seen) { c.flag = rgb(0xC0392B); c.flagTxt = "seen with you by the other side"; }
            else if (queried) { c.flag = rgb(0xD08A2A); c.flagTxt = fstr("%d item%s queried by Centre", queried, queried == 1 ? "" : "s"); }
            else if (p.traceBad) { c.flag = rgb(0xD08A2A); c.flagTxt = "adverse trace"; }
            cards.push_back(c);
        }
    for (auto& s : g.sites) {
        if (s.retired) continue;
        bool used = !s.witting.empty();
        for (auto& p : g.persons) if (p.live() && p.stage == AGENT && inPlan(p, s.id)) used = true;
        if (!used) continue;
        Card c{K_SITE, s.id, {}, 0, 0, sim.shortSite(&s), std::string(s.flat ? "Safe flat" : SITETYPE_LABEL[s.type]) + fstr("  -  used %d", s.uses), 0, "", s.name};
        if (s.exposure > 55) { c.flag = rgb(0xD08A2A); c.flagTxt = "over-used"; }
        cards.push_back(c);
    }
    for (size_t i = 0; i < cards.size(); i++) at[key(cards[i].kind, cards[i].id)] = (int)i;

    // ---- the string
    for (auto& p : g.persons) {
        if (!p.live()) continue;
        auto pi = at.find(key(p.stage == AGENT ? K_AGENT : K_CONTACT, p.id));
        if (pi == at.end()) continue;
        for (int oid : p.witting) { auto oi = at.find(key(K_OFF, oid)); if (oi != at.end() && oid != p.handler) links.push_back({oi->second, pi->second, 0}); }
        if (p.handler) { auto oi = at.find(key(K_OFF, p.handler)); if (oi != at.end()) links.push_back({oi->second, pi->second, 2}); }
        if (p.stage == AGENT)
            for (auto& s : g.sites) if (!s.retired && inPlan(p, s.id)) { auto si = at.find(key(K_SITE, s.id)); if (si != at.end()) links.push_back({pi->second, si->second, 1}); }
    }
    for (auto& s : g.sites) {
        auto si = at.find(key(K_SITE, s.id));
        if (si == at.end()) continue;
        for (int oid : s.witting) { auto oi = at.find(key(K_OFF, oid)); if (oi != at.end()) links.push_back({oi->second, si->second, 0}); }
    }

    // ---- the frame and the cork
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImVec2 a = ImGui::GetCursorScreenPos(), fb = a + avail;
    ImDrawList* fdl = ImGui::GetWindowDrawList();
    skin::frame(fdl, a, fb, S);
    float bh = skin::titleBar(fdl, a, avail.x, S, "Case board", "Who knows whom, as your own files have it. Hover a card to follow its strings; double-click to open the file.");
    ImGui::SetCursorScreenPos(a + ImVec2(8 * S, bh + 6 * S));
    ImVec2 area = fb - ImVec2(8 * S, 8 * S) - ImGui::GetCursorScreenPos();
    ImGui::BeginChild("##board", area, 0, ImGuiWindowFlags_NoBackground);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 o0 = ImGui::GetCursorScreenPos();
    float W = ImGui::GetContentRegionAvail().x;

    // columns: officers | agents | contacts | sites
    const float colX[4] = {0.02f, 0.27f, 0.52f, 0.77f}, colW = 0.21f;
    const char* const HEAD[4] = {"THE STATION", "AGENTS", "IN DEVELOPMENT", "PLACES"};
    float cw = 74 * S, chh = cw * 4 / 3, labelH = 38 * S, gapY = 14 * S, top = 34 * S;
    float bottom = top;
    for (int k = 0; k < 4; k++) {
        float x0 = o0.x + W * colX[k], x1 = x0 + W * colW;
        int perRow = std::max(1, (int)((x1 - x0) / (cw + 22 * S)));
        float cellW = (x1 - x0) / perRow;
        int n = 0;
        for (auto& c : cards) {
            if (c.kind != k) continue;
            float w = k == K_SITE ? cw * 1.15f : cw, h = k == K_SITE ? w * 0.75f : chh;
            int row = n / perRow, col = n % perRow;
            float rowH = chh + labelH + gapY;
            c.w = w; c.h = h;
            c.c = ImVec2(x0 + cellW * (col + 0.5f) + ((row + col) % 2 ? 4 * S : -4 * S), o0.y + top + row * rowH + h / 2);
            bottom = std::max(bottom, c.c.y + h / 2 + labelH + gapY);
            n++;
        }
    }
    float H = std::max(area.y - 4 * S, bottom + 70 * S);
    ImVec2 o1 = o0 + ImVec2(W, H);
    // cork: a warm brown with darker and lighter specks, the same every frame
    dl->AddRectFilledMultiColor(o0, o1, rgb(0x9A7148), rgb(0x8F663E), rgb(0x84603A), rgb(0x936B43));
    tile(dl, tex("tx:grain", [] { return art::paperGrain(256, 5); }), o0, o1, 200 * S, IM_COL32(60, 35, 15, 110));
    for (int i = 0, nSpeck = std::min(450, (int)(W * H / (1600 * S * S))); i < nSpeck; i++) {   // few: one draw list holds 65536 vertices
        uint32_t h1 = hsh(77, (uint32_t)i), h2 = hsh(h1, 3);
        ImVec2 p(o0.x + (h1 % 10000) / 10000.f * W, o0.y + (h2 % 10000) / 10000.f * H);
        dl->AddCircleFilled(p, (0.6f + (h1 >> 20) % 3 * 0.5f) * S, (h2 >> 9) & 1 ? IM_COL32(60, 38, 18, 90) : IM_COL32(196, 160, 112, 70), 4);
    }
    for (int k = 0; k < 4; k++) {
        float x0 = o0.x + W * colX[k];
        ImVec2 la(x0 + 4 * S, o0.y + 6 * S);
        sheet(dl, la, la + ImVec2(W * colW - 8 * S, 22 * S), rgb(C_CARD), S, true);
        skin::tracked(dl, F.uiCaps, F.uiCaps->FontSize, la + ImVec2(8 * S, 11 * S - F.uiCaps->FontSize / 2), rgb(C_INK), HEAD[k], 1.2f * S);
    }

    // ---- which card is under the pointer
    ImVec2 m = ImGui::GetMousePos();
    int hot = -1;
    bool inside = ImGui::IsWindowHovered();
    for (size_t i = 0; i < cards.size() && inside; i++) {
        const Card& c = cards[i];
        if (m.x >= c.c.x - c.w / 2 && m.x <= c.c.x + c.w / 2 && m.y >= c.c.y - c.h / 2 && m.y <= c.c.y + c.h / 2 + labelH) hot = (int)i;
    }
    if (hot >= 0) boardFocus = (int)key(cards[hot].kind, cards[hot].id);
    std::set<int> lit;
    if (hot >= 0) { lit.insert(hot); for (auto& l : links) if (l.a == hot || l.b == hot) { lit.insert(l.a); lit.insert(l.b); } }

    // ---- string first, so the cards lie on top of it
    auto pinAt = [&](const Card& c) { return ImVec2(c.c.x, c.c.y - c.h / 2 + 3 * S); };
    for (int pass = 0; pass < 2; pass++)
        for (auto& l : links) {
            bool on = hot >= 0 && (l.a == hot || l.b == hot);
            if ((pass == 1) != on) continue;
            ImVec2 p0 = pinAt(cards[l.a]), p1 = pinAt(cards[l.b]);
            float sag = std::min(60 * S, std::fabs(p1.x - p0.x) * 0.12f + 12 * S);
            ImVec2 mid((p0.x + p1.x) / 2, std::max(p0.y, p1.y) + sag);
            float alpha = hot < 0 ? 0.85f : on ? 1.f : 0.18f;
            ImU32 col = l.kind == 1 ? rgb(0x1E1A16, alpha) : rgb(l.kind == 2 ? 0xA01812 : 0xC8281C, alpha);
            float th = (l.kind == 2 ? 2.6f : l.kind == 1 ? 1.3f : 1.7f) * S * (on ? 1.25f : 1.f);
            dl->AddBezierQuadratic(p0 + ImVec2(1.5f * S, 2 * S), mid + ImVec2(1.5f * S, 3 * S), p1 + ImVec2(1.5f * S, 2 * S), rgb(0, alpha * 0.25f), th, 12);
            dl->AddBezierQuadratic(p0, mid, p1, col, th, 14);
        }

    // ---- the cards
    for (size_t i = 0; i < cards.size(); i++) {
        Card& c = cards[i];
        bool dim = hot >= 0 && !lit.count((int)i);
        float deg = ((int)(hsh((uint32_t)c.id, (uint32_t)c.kind) % 7) - 3) * 0.9f;
        ImVec2 tl = c.c - ImVec2(c.w / 2, c.h / 2);
        ImTextureID t = c.kind == K_OFF ? officerPhoto(*sim.officer(c.id)) : c.kind == K_SITE ? sitePhoto(*sim.site(c.id)) : facePhoto(*sim.person(c.id));
        photo(dl, t, tl, ImVec2(c.w, c.h), deg, S, false);
        // a slip of paper under the photograph with the name in pencil
        ImVec2 la(c.c.x - c.w * 0.62f, c.c.y + c.h / 2 + 4 * S), lb(c.c.x + c.w * 0.62f, la.y + labelH - 6 * S);
        sheet(dl, la, lb, rgb(c.kind == K_AGENT ? 0xF3E9C8u : (unsigned)C_CARD), S, true);
        std::string nm = clip(c.name, 20);
        ImFont* nf = c.kind == K_AGENT ? F.uiBold : F.uiSmall;
        float fs = nf->FontSize * 0.95f;
        ImVec2 ts = nf->CalcTextSizeA(fs, FLT_MAX, 0, nm.c_str());
        dl->AddText(nf, fs, ImVec2(c.c.x - ts.x / 2, la.y + 2 * S), rgb(c.kind == K_AGENT ? C_RED : C_INK), nm.c_str());
        // the second line, as large as the slip allows (a long one is shortened rather than shrunk to nothing)
        std::string sub = c.sub;
        float ss = F.uiSmall->FontSize * 0.82f, room = lb.x - la.x - 6 * S;
        while (sub.size() > 8 && F.uiSmall->CalcTextSizeA(ss, FLT_MAX, 0, sub.c_str()).x > room) sub = clip(sub, ulen(sub) - 2);
        ImVec2 ts2 = F.uiSmall->CalcTextSizeA(ss, FLT_MAX, 0, sub.c_str());
        dl->AddText(F.uiSmall, ss, ImVec2(c.c.x - ts2.x / 2, la.y + fs + 3 * S), rgb(C_INK_FADED), sub.c_str());
        if (c.flag) {                     // a coloured flag pinned to the corner: something on the file wants looking at
            ImVec2 f0 = tl + ImVec2(c.w - 6 * S, -2 * S);
            dl->AddTriangleFilled(f0, f0 + ImVec2(16 * S, 6 * S), f0 + ImVec2(0, 12 * S), c.flag);
            dl->AddLine(f0, f0 + ImVec2(0, 18 * S), rgb(0x2A2018), 1.2f * S);
        }
        ImU32 head = c.kind == K_AGENT ? rgb(0xC0392B) : c.kind == K_OFF ? rgb(0x2D4C8A) : c.kind == K_SITE ? rgb(0x2F6B3A) : rgb(0xD8B23C);
        pushpin(dl, pinAt(c), S, head);
        if (dim) dl->AddRectFilled(tl - ImVec2(6 * S, 8 * S), lb + ImVec2(2 * S, 2 * S), IM_COL32(70, 48, 26, 140));
        // an invisible button over the card, for the tests and for double-clicking into the file
        ImGui::SetCursorScreenPos(ImVec2(c.c.x - c.w / 2, tl.y));
        ImGui::PushID((int)key(c.kind, c.id));
        ImGui::InvisibleButton("card", ImVec2(c.w, c.h + labelH));
        ImGui::PopID();
        uiRec(std::string("board:") + c.name);
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
            if (c.kind == K_OFF) { selOfficer = c.id; wantTab = T_STATION; }
            else if (c.kind == K_SITE) { selSite = c.id; wantTab = T_SITES; }
            else { selPerson = c.id; wantTab = T_PEOPLE; }
        }
        if (ImGui::IsItemHovered() && cfg.tooltips) {
            int nl = 0;
            for (auto& l : links) if (l.a == (int)i || l.b == (int)i) nl++;
            std::string tip = c.name + (c.more.empty() ? "" : ", " + c.more) + "\n" + c.sub + (c.flagTxt.empty() ? "" : "\n\n! " + c.flagTxt) + fstr("\n\n%d string%s on the board.", nl, nl == 1 ? "" : "s");
            ImGui::SetTooltip("%s", tip.c_str());
        }
    }
    if (cards.empty()) pencil(dl, o0 + ImVec2(40 * S, 80 * S), "Nothing pinned up yet.", rgb(0xF2E6C8), -2, F.handBig);

    // ---- the key, pinned at the bottom
    {
        ImVec2 ka(o0.x + 12 * S, o0.y + H - 58 * S), kb = ka + ImVec2(560 * S, 48 * S);
        sheet(dl, ka, kb, rgb(C_CARD), S, true);
        pushpin(dl, ka + ImVec2(10 * S, 8 * S), S, rgb(0x777777));
        const struct { ImU32 c; float th; const char* t; } K[] = {{rgb(0xC8281C), 1.7f, "knows of it, by your files"}, {rgb(0xA01812), 2.6f, "runs him"}, {rgb(0x1E1A16), 1.3f, "in his communications plan"}};
        for (int i = 0; i < 3; i++) {
            ImVec2 p = ka + ImVec2(18 * S + i * 180 * S, 16 * S);
            dl->AddLine(p, p + ImVec2(28 * S, 0), K[i].c, K[i].th * S);
            dl->AddText(F.uiSmall, F.uiSmall->FontSize, p + ImVec2(34 * S, -F.uiSmall->FontSize / 2), rgb(C_INK), K[i].t);
        }
        ImVec2 p = ka + ImVec2(18 * S, 33 * S);
        dl->AddTriangleFilled(p - ImVec2(0, 5 * S), p + ImVec2(10 * S, 0), p + ImVec2(0, 5 * S), rgb(0xD08A2A));
        dl->AddText(F.uiSmall, F.uiSmall->FontSize, p + ImVec2(16 * S, -F.uiSmall->FontSize / 2), rgb(C_INK),
                    "a flag: something on the file wants looking at (hover for what). An officer with string to everything is worth a marked card.");
    }
    ImGui::SetCursorScreenPos(o0);
    ImGui::Dummy(ImVec2(W, H));
    ImGui::EndChild();
}
