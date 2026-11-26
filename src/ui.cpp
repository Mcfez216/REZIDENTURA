// ui.cpp - the frame, the title screen, the header and the folder tabs, popups, and the order form.
// The desk screens themselves are in ui_tabs.cpp.
#include "ui.h"
#include <cstring>
#include "imgui.h"
#include "imgui_internal.h"
#include <cfloat>
#include <cmath>
#include <filesystem>
#include <fstream>

using namespace gfx;

extern const char* const TAB_RU[App::N_TAB];
extern const char* const TAB_EN[App::N_TAB];
const char* const TAB_RU[App::N_TAB] = {"ПЛАН РАБОТЫ", "ПАМЯТКА", "АГЕНТУРА", "КАРТА ГОРОДА", "МЕСТА И МАРШРУТЫ", "СХЕМА СВЯЗЕЙ", "МАТЕРИАЛЫ", "ЦЕНТР", "СВОДКА", "РЕЗИДЕНТУРА", "НАРУЖКА", "КАССА", "ОТЧЁТНОСТЬ", "ЛИЧНАЯ ЖИЗНЬ", "ЛИЧНОЕ ДЕЛО", "ИНСТРУКЦИЯ"};
const char* const TAB_EN[App::N_TAB] = {"Planner", "Checklist", "Contacts", "City map", "Sites", "Case board", "Material", "Centre", "Daily report", "Station", "Surveillance", "Accounts", "Results", "Private life", "Career", "Handbook"};

bool App::recordRects = false;
std::map<std::string, std::array<float, 4>> App::rects;

// ============================================================================ shared widgets
void uiRec(const std::string& key) {
    if (!App::recordRects) return;
    ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
    App::rects[key] = {a.x, a.y, b.x, b.y};
}
/** A button drawn in the game's style: a dark plate with a brass edge, or (primary) a red one. */
static bool skinButton(const char* label, ImVec2 sizeArg, int kind) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    float fh = ImGui::GetFrameHeight();
    ImFont* f = F.btn ? F.btn : ImGui::GetFont();
    // a trailing ▾ ("opens a list") is drawn as a small brass triangle, not as a glyph the font may lack
    const char* end = ImGui::FindRenderedTextEnd(label);
    bool caret = end - label >= 3 && std::memcmp(end - 3, "\xE2\x96\xBE", 3) == 0;
    if (caret) { end -= 3; while (end > label && end[-1] == ' ') end--; }
    float cw = caret ? 16 * skin::scale : 0;
    ImVec2 ls = f->CalcTextSizeA(f->FontSize, FLT_MAX, 0, label, end);
    ls.x += cw;
    ImVec2 pos = window->DC.CursorPos;
    if (window->DC.CurrLineTextBaseOffset > style.FramePadding.y) pos.y += window->DC.CurrLineTextBaseOffset - style.FramePadding.y;
    ImVec2 size = ImGui::CalcItemSize(sizeArg, ls.x + style.FramePadding.x * 2.0f + 6 * skin::scale, std::max(fh, ls.y + style.FramePadding.y * 2.0f));
    const ImRect bb(pos, pos + size);
    ImGui::ItemSize(size, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, id)) return false;
    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, 0);
    float al = style.Alpha;
    skin::button(window->DrawList, bb.Min, bb.Max, skin::scale, hovered, held, kind, al);
    ImVec2 tp(std::floor(bb.Min.x + (size.x - ls.x) * 0.5f), std::floor(bb.Min.y + (size.y - ls.y) * 0.5f) + (held ? 1.f : 0.f));
    window->DrawList->PushClipRect(bb.Min, bb.Max, true);
    window->DrawList->AddText(f, f->FontSize, tp + ImVec2(0, 1), IM_COL32(0, 0, 0, (int)(140 * al)), label, end);
    window->DrawList->AddText(f, f->FontSize, tp, IM_COL32(0xF2, 0xEA, 0xD8, (int)(255 * al)), label, end);
    if (caret) {
        float S = skin::scale;
        ImVec2 c(tp.x + ls.x - 5 * S, tp.y + ls.y * 0.55f);
        window->DrawList->AddTriangleFilled(c + ImVec2(-4.5f * S, -2.5f * S), c + ImVec2(4.5f * S, -2.5f * S), c + ImVec2(0, 3 * S), rgb(skin::K_BRASS_HI, al));
    }
    window->DrawList->PopClipRect();
    return pressed;
}
bool uiBtn(const char* label, ImVec2 size) { bool r = skinButton(label, size, skin::B_NORMAL); uiRec(label); return r; }
bool uiPrimary(const char* label, ImVec2 size) { bool r = skinButton(label, size, skin::B_PRIMARY); uiRec(label); return r; }

bool isPaper(unsigned c) { return c != C_CARD && c != C_LEDGER; }

static std::vector<bool> modalStack;
bool uiModal(const char* name, bool* open, ImGuiWindowFlags flags, const char* title) {
    float S = skin::scale;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(22 * S, 14 * S));
    bool r = ImGui::BeginPopupModal(name, open, flags | ImGuiWindowFlags_NoTitleBar);
    ImGui::PopStyleVar();
    if (!r) return false;
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    ImDrawList* dl = w->DrawList;
    std::string t = title ? title : std::string(name, ImGui::FindRenderedTextEnd(name));
    dl->PushClipRect(w->Pos, w->Pos + w->Size, false);
    skin::trim(dl, w->Pos, w->Pos + w->Size, S);
    float bh = skin::titleBar(dl, w->Pos, w->Size.x, S, t.c_str(), nullptr);
    dl->PopClipRect();
    // the close cross in the corner of the bar
    if (open) {
        ImVec2 c(w->Pos.x + w->Size.x - 22 * S, w->Pos.y + 3 * S + skin::titleBarH(S) * 0.5f);
        ImVec2 keepMax = w->DC.CursorMaxPos;                  // (the cross must not widen a window that sizes itself to its contents)
        ImGui::SetCursorScreenPos(c - ImVec2(12 * S, 12 * S));
        if (ImGui::InvisibleButton("##close", ImVec2(24 * S, 24 * S))) { *open = false; ImGui::CloseCurrentPopup(); }
        skin::icon(dl, skin::IC_CLOSE, c, 20 * S, rgb(ImGui::IsItemHovered() ? skin::K_BRASS_HI : skin::K_BRASS));
        w->DC.CursorMaxPos = keepMax;
    }
    // the content starts under the bar; the window is at least as wide as its title
    ImGui::SetCursorScreenPos(ImVec2(w->Pos.x + 22 * S, w->Pos.y + bh + 12 * S));
    ImGui::Dummy(ImVec2(skin::trackedWidth(F.bar, F.bar->FontSize, skin::upper(t).c_str(), 1.3f * S) + (open ? 70 : 40) * S, 0));
    pushInk(true);
    modalStack.push_back(true);
    return true;
}
void uiEndModal() {
    if (!modalStack.empty()) { modalStack.pop_back(); popInk(); }
    ImGui::EndPopup();
}

void uiSection(const char* title) {
    float S = skin::scale;
    ImGui::Dummy(ImVec2(1, 4 * S));
    ImVec2 p = ImGui::GetCursorScreenPos();
    float w = ImGui::GetContentRegionAvail().x;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    bool paper = !darkInk();
    std::string t = skin::upper(title);
    float tw = skin::trackedWidth(F.uiCaps, F.uiCaps->FontSize, t.c_str(), 1.2f * S);
    skin::tracked(dl, F.uiCaps, F.uiCaps->FontSize, p, paper ? rgb(0x7A2418) : rgb(skin::K_BRASS), t.c_str(), 1.2f * S);
    float y = p.y + F.uiCaps->FontSize * 0.55f;
    ImU32 c = paper ? rgb(0x7A6A4A, 0.45f) : rgb(skin::K_BRASS, 0.35f);
    dl->AddRectFilledMultiColor(ImVec2(p.x + tw + 10 * S, y), ImVec2(p.x + w, y + 1), c, rgb(0, 0), rgb(0, 0), c);
    ImGui::Dummy(ImVec2(w, F.uiCaps->FontSize + 3 * S));
}

void uiHeaders() {
    ImGuiTable* t = ImGui::GetCurrentTable();
    if (!t) return;
    ImGui::PushFont(F.uiCaps);
    ImGui::PushStyleColor(ImGuiCol_Text, darkInk() ? rgb4(skin::K_BRASS) : rgb4(0x5E4A2A));
    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
    for (int c = 0; c < t->ColumnsCount; c++) {
        if (!ImGui::TableSetColumnIndex(c)) continue;
        std::string name = skin::upper(ImGui::TableGetColumnName(c));
        ImGui::PushID(c);
        ImGui::TableHeader(name.c_str());
        ImGui::PopID();
    }
    ImGui::PopStyleColor();
    ImGui::PopFont();
}
static int textResize(ImGuiInputTextCallbackData* d) {
    if (d->EventFlag == ImGuiInputTextFlags_CallbackResize) { std::string* s = (std::string*)d->UserData; s->resize((size_t)d->BufTextLen); d->Buf = &(*s)[0]; }
    return 0;
}
bool uiText(const char* id, std::string& s, ImVec2 size, bool multiline, const char* hint) {
    bool r;
    if (multiline) r = ImGui::InputTextMultiline(id, &s[0], s.capacity() + 1, size, ImGuiInputTextFlags_CallbackResize, textResize, &s);
    else {
        if (size.x != 0) ImGui::SetNextItemWidth(size.x);
        r = ImGui::InputTextWithHint(id, hint ? hint : "", &s[0], s.capacity() + 1, ImGuiInputTextFlags_CallbackResize, textResize, &s);
    }
    uiRec(id);
    return r;
}
bool uiCheck(const char* label, bool* v) { bool r = ImGui::Checkbox(label, v); uiRec(label); return r; }

bool uiCombo(const char* label, int& value, const std::vector<Opt>& opts, float width) {
    const char* preview = "";
    for (auto& o : opts) if (o.value == value) preview = o.label.c_str();
    if (width != 0) ImGui::SetNextItemWidth(width);
    bool changed = false;
    // the box is drawn by ImGui; the value and a brass chevron in a well at its right end are drawn here
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    ImDrawList* dl = win->DrawList;
    const ImGuiStyle& st = ImGui::GetStyle();
    ImVec2 p = win->DC.CursorPos;
    if (win->DC.CurrLineTextBaseOffset > st.FramePadding.y) p.y += win->DC.CurrLineTextBaseOffset - st.FramePadding.y;
    float w = ImGui::CalcItemWidth(), h = ImGui::GetFrameHeight(), S = skin::scale;
    ImU32 textCol = ImGui::GetColorU32(ImGuiCol_Text);
    bool open = ImGui::BeginCombo(label, "", ImGuiComboFlags_HeightLarge | ImGuiComboFlags_NoArrowButton);
    uiRec(std::string("combo:") + label);
    if (!win->SkipItems) {
        float ax = p.x + w - h;
        dl->AddRectFilled(ImVec2(ax, p.y + 1), ImVec2(p.x + w - 1, p.y + h - 1), rgb(0x000000, darkInk() ? 0.25f : 0.05f));
        dl->AddLine(ImVec2(ax, p.y + 3 * S), ImVec2(ax, p.y + h - 3 * S), rgb(skin::K_BRASS_LO, 0.7f), 1);
        ImVec2 c(ax + h * 0.5f, p.y + h * 0.5f + (open ? -1.f : 1.f) * 0.5f * S);
        ImU32 cc = rgb(darkInk() ? skin::K_BRASS : 0x7A2418);
        float d = open ? -1.f : 1.f;
        dl->AddLine(c + ImVec2(-4.5f * S, -2.2f * S * d), c + ImVec2(0, 2.4f * S * d), cc, 1.7f * S);
        dl->AddLine(c + ImVec2(0, 2.4f * S * d), c + ImVec2(4.5f * S, -2.2f * S * d), cc, 1.7f * S);
        ImVec2 tp(p.x + st.FramePadding.x, p.y + st.FramePadding.y);
        dl->PushClipRect(ImVec2(p.x, p.y), ImVec2(ax - 4 * S, p.y + h), true);
        dl->AddText(tp, textCol, preview);
        dl->PopClipRect();
    }
    if (open) {
        for (size_t i = 0; i < opts.size(); i++) {
            ImGui::PushID((int)i);
            bool sel = opts[i].value == value;
            if (ImGui::Selectable(opts[i].label.c_str(), sel)) { value = opts[i].value; changed = true; }
            uiRec("opt:" + opts[i].label);
            if (sel) ImGui::SetItemDefaultFocus();
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
    return changed;
}

void uiTyped(const std::string& text, ImU32 col, ImFont* font, const LineStyle& style, uint32_t seed) {
    if (!font) font = F.body;
    float wrap = std::max(50.f, ImGui::GetContentRegionAvail().x);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const char* s = text.c_str();
    const char* end = s + text.size();
    uint32_t n = 0;
    while (s <= end) {
        const char* nl = (const char*)std::memchr(s, '\n', (size_t)(end - s));
        if (!nl) nl = end;
        ImU32 c = col;
        ImFont* f = font;
        if (style) style(std::string(s, nl), c, f);
        float size = f->FontSize, lh = std::floor(size * 1.32f);
        if (s == nl) ImGui::Dummy(ImVec2(wrap, lh * 0.7f));
        // a typed gauge "[#####.....]" is drawn as a bar, and a run of leader dots as a faint dotted line
        {
            std::string line(s, nl);
            size_t lb = line.find('['), rb = lb == std::string::npos ? lb : line.find(']', lb);
            bool gauge = rb != std::string::npos && rb - lb >= 6 && line.find_first_not_of("#.", lb + 1) == rb;
            size_t ld = line.find(" ...");
            size_t le = ld == std::string::npos ? ld : line.find_first_not_of('.', ld + 1);
            bool leader = !gauge && ld != std::string::npos && le != std::string::npos && le - ld >= 4 && line[le] == ' ' && ld > 0;
            float tw = f->CalcTextSizeA(size, FLT_MAX, 0, s, nl).x;
            // a typed rule - a line of "=====" or "-----" - is drawn as a rule
            size_t first = line.find_first_not_of(' ');
            if (first != std::string::npos && line.size() - first >= 12 && (line.find_first_not_of('=', first) == std::string::npos || line.find_first_not_of('-', first) == std::string::npos)) {
                ImVec2 pos = ImGui::GetCursorScreenPos();
                bool dbl = line[first] == '=';
                float y = pos.y + lh * 0.5f;
                dl->AddLine(ImVec2(pos.x, y - (dbl ? 1.5f : 0)), ImVec2(pos.x + std::min(wrap, tw), y - (dbl ? 1.5f : 0)), (c & 0x00FFFFFF) | 0x60000000u, 1);
                if (dbl) dl->AddLine(ImVec2(pos.x, y + 1.5f), ImVec2(pos.x + std::min(wrap, tw), y + 1.5f), (c & 0x00FFFFFF) | 0x60000000u, 1);
                ImGui::Dummy(ImVec2(wrap, lh));
                n++;
                if (nl == end) break;
                s = nl + 1;
                continue;
            }
            if ((gauge || leader) && tw <= wrap) {
                ImVec2 pos = ImGui::GetCursorScreenPos();
                if (ImGui::IsRectVisible(ImVec2(wrap, lh))) {
                    size_t cut0 = gauge ? lb : ld + 1, cut1 = gauge ? rb + 1 : le;
                    float x0 = f->CalcTextSizeA(size, FLT_MAX, 0, s, s + cut0).x, x1 = f->CalcTextSizeA(size, FLT_MAX, 0, s, s + cut1).x;
                    typeLine(dl, f, size, pos, c, s, s + cut0, seed * 7919u + n);
                    if (gauge) {
                        int full = 0, all = (int)(rb - lb - 1);
                        for (size_t k = lb + 1; k < rb; k++) if (line[k] == '#') full++;
                        float pad = size * 0.25f;
                        ImVec2 a0(pos.x + x0 + pad, pos.y + size * 0.36f), a1(pos.x + x1 - pad, pos.y + size * 0.78f);
                        dl->AddRectFilled(a0, a1, rgb(0x8E8062, 0.22f), 2);
                        dl->AddRectFilled(a0, ImVec2(a0.x + (a1.x - a0.x) * full / std::max(1, all), a1.y), (c & 0x00FFFFFF) | 0xC8000000u, 2);
                    } else {
                        float step = size * 0.5f;
                        for (float x = pos.x + x0 + step * 0.5f; x < pos.x + x1 - step * 0.6f; x += step) dl->AddCircleFilled(ImVec2(x, pos.y + size * 0.78f), std::max(0.8f, size * 0.06f), (c & 0x00FFFFFF) | 0x50000000u, 6);
                    }
                    typeLine(dl, f, size, ImVec2(pos.x + x1, pos.y), c, s + cut1, nl, seed * 7919u + n + 1);
                }
                ImGui::Dummy(ImVec2(wrap, lh));
                n++;
                if (nl == end) break;
                s = nl + 1;
                continue;
            }
        }
        const char* p = s;
        while (p < nl) {
            const char* w = f->CalcWordWrapPositionA(1.0f, p, nl, wrap);
            if (w <= p) w = p + 1;
            ImVec2 pos = ImGui::GetCursorScreenPos();
            if (ImGui::IsRectVisible(ImVec2(wrap, lh))) typeLine(dl, f, size, pos, c, p, w, seed * 7919u + n);
            ImGui::Dummy(ImVec2(wrap, lh));
            n++;
            p = w;
            while (p < nl && *p == ' ') p++;
        }
        if (nl == end) break;
        s = nl + 1;
    }
}

bool uiSlip(const char* id, const char* ru_, const char* en, ImVec2 size, float S, unsigned paper, ImU32 ink) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    bool r = ImGui::InvisibleButton(id, size);
    uiRec(id);
    bool hov = ImGui::IsItemHovered();
    bool act = ImGui::IsItemActive();
    bool disabled = (ImGui::GetItemFlags() & ImGuiItemFlags_Disabled) != 0;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 o = act ? ImVec2(1 * S, 1 * S) : ImVec2(0, 0);
    sheet(dl, p + o, p + size + o, rgb(hov ? 0xFFF8E6 : paper, disabled ? 0.6f : 1), S, !act);
    if (!ink) ink = rgb(C_INK);
    if (disabled) ink = rgb(C_INK_FADED, 0.6f);
    float h1 = F.stamp->FontSize, h2 = F.tiny->FontSize;
    float y = p.y + o.y + (size.y - (h1 + h2)) / 2;
    ImVec2 t1 = F.stamp->CalcTextSizeA(h1, FLT_MAX, 0, ru_);
    dl->AddText(F.stamp, h1, ImVec2(p.x + o.x + (size.x - t1.x) / 2, y), ink, ru_);
    ImVec2 t2 = F.tiny->CalcTextSizeA(h2, FLT_MAX, 0, en);
    dl->AddText(F.tiny, h2, ImVec2(p.x + o.x + (size.x - t2.x) / 2, y + h1), disabled ? rgb(C_INK_FADED, 0.6f) : rgb(C_INK_FADED), en);
    return r;
}

static float lineH() { return ImGui::GetFrameHeightWithSpacing(); }
static std::string up(std::string s) { for (auto& c : s) c = (char)toupper((unsigned char)c); return s; }

// ============================================================================ theme
void App::theme() {
    using namespace skin;
    skin::scale = S;
    ImGuiStyle& st = ImGui::GetStyle();
    ImGui::StyleColorsDark();
    st.WindowRounding = 0; st.ChildRounding = 0; st.FrameRounding = 1; st.PopupRounding = 0; st.GrabRounding = 1; st.TabRounding = 0; st.ScrollbarRounding = 0;
    st.WindowBorderSize = 1; st.ChildBorderSize = 0; st.FrameBorderSize = 1; st.PopupBorderSize = 1; st.TabBorderSize = 0;
    st.WindowPadding = ImVec2(16, 14); st.FramePadding = ImVec2(10, 5); st.ItemSpacing = ImVec2(8, 6); st.ItemInnerSpacing = ImVec2(7, 4);
    st.CellPadding = ImVec2(8, 5); st.IndentSpacing = 18;
    st.ScrollbarSize = 10; st.GrabMinSize = 14;
    st.SeparatorTextBorderSize = 1; st.SeparatorTextPadding = ImVec2(0, 4);
    st.SelectableTextAlign = ImVec2(0, 0.5f);
    st.WindowTitleAlign = ImVec2(0, 0.5f);
    st.DisabledAlpha = 0.45f;
    ImVec4* c = st.Colors;
    c[ImGuiCol_Text] = rgb4(K_TEXT); c[ImGuiCol_TextDisabled] = rgb4(K_FAINT);
    c[ImGuiCol_WindowBg] = rgb4(K_PANEL_LO); c[ImGuiCol_ChildBg] = rgb4(0, 0); c[ImGuiCol_PopupBg] = rgb4(0x1F1A15, 0.985f);
    c[ImGuiCol_Border] = rgb4(K_BRASS_LO, 0.9f); c[ImGuiCol_BorderShadow] = rgb4(0, 0);
    c[ImGuiCol_FrameBg] = rgb4(K_INSET); c[ImGuiCol_FrameBgHovered] = rgb4(0x19150F); c[ImGuiCol_FrameBgActive] = rgb4(0x1D1812);
    c[ImGuiCol_Button] = rgb4(0x3B3128); c[ImGuiCol_ButtonHovered] = rgb4(0x4A3D30); c[ImGuiCol_ButtonActive] = rgb4(0x221C16);
    c[ImGuiCol_Header] = rgb4(0x8A2A1C, 0.62f); c[ImGuiCol_HeaderHovered] = rgb4(K_BRASS, 0.13f); c[ImGuiCol_HeaderActive] = rgb4(0x8E2A1C, 0.9f);
    c[ImGuiCol_TableHeaderBg] = rgb4(0x2F261D); c[ImGuiCol_TableBorderStrong] = rgb4(K_BRASS_LO, 0.9f); c[ImGuiCol_TableBorderLight] = rgb4(K_BRASS, 0.09f);
    c[ImGuiCol_TableRowBg] = rgb4(0, 0); c[ImGuiCol_TableRowBgAlt] = rgb4(0xFFF0D0, 0.028f);
    c[ImGuiCol_ScrollbarBg] = rgb4(0, 0.15f); c[ImGuiCol_ScrollbarGrab] = rgb4(K_BRASS_LO); c[ImGuiCol_ScrollbarGrabHovered] = rgb4(K_BRASS); c[ImGuiCol_ScrollbarGrabActive] = rgb4(K_BRASS_HI);
    c[ImGuiCol_CheckMark] = rgb4(K_BRASS_HI); c[ImGuiCol_SliderGrab] = rgb4(K_BRASS); c[ImGuiCol_SliderGrabActive] = rgb4(K_BRASS_HI);
    c[ImGuiCol_Separator] = rgb4(K_BRASS_LO, 0.8f); c[ImGuiCol_SeparatorHovered] = rgb4(K_BRASS); c[ImGuiCol_SeparatorActive] = rgb4(K_BRASS_HI);
    c[ImGuiCol_ModalWindowDimBg] = rgb4(0x050302, 0.62f);
    c[ImGuiCol_NavCursor] = rgb4(K_BRASS); c[ImGuiCol_TextSelectedBg] = rgb4(K_BRASS, 0.35f);
    c[ImGuiCol_TitleBg] = rgb4(0x1E1914); c[ImGuiCol_TitleBgActive] = rgb4(K_BAR); c[ImGuiCol_TitleBgCollapsed] = rgb4(0x1E1914);
    c[ImGuiCol_Tab] = rgb4(0x241E18); c[ImGuiCol_TabHovered] = rgb4(0x3B3128); c[ImGuiCol_TabSelected] = rgb4(K_BAR); c[ImGuiCol_TabSelectedOverline] = rgb4(K_BRASS);
    c[ImGuiCol_TabDimmed] = rgb4(0x1E1914); c[ImGuiCol_TabDimmedSelected] = rgb4(0x2A231C); c[ImGuiCol_TabDimmedSelectedOverline] = rgb4(K_BRASS_LO);
    c[ImGuiCol_DockingPreview] = rgb4(K_BRASS, 0.35f); c[ImGuiCol_DockingEmptyBg] = rgb4(K_DESK);
    c[ImGuiCol_ResizeGrip] = rgb4(K_BRASS_LO, 0.6f); c[ImGuiCol_ResizeGripHovered] = rgb4(K_BRASS); c[ImGuiCol_ResizeGripActive] = rgb4(K_BRASS_HI);
    c[ImGuiCol_MenuBarBg] = rgb4(K_BAR); c[ImGuiCol_PlotLines] = rgb4(K_BRASS); c[ImGuiCol_PlotHistogram] = rgb4(K_BRASS);
    st.ScaleAllSizes(S);
}

// ============================================================================ files
bool App::saveTo(const std::string& path) {
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::u8path(path).parent_path(), ec);
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
    resetSelection();
    forgetTextures("p");
    return true;
}
void App::autosave() { saveTo(saveDir + "/autosave.rez"); }
void App::resetSelection() {
    selPerson = selSite = selRoll = selReq = selCable = selOfficer = selLog = selDistrict = -1;
    reportDay = -1; selRow = 0; selCol = 1;
}

// ============================================================================ the frame
void App::frame() {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::Begin("##main", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoDocking);
    ImGui::PopStyleVar(2);
    skin::desk(ImGui::GetWindowDrawList(), vp->WorkPos, vp->WorkPos + vp->WorkSize, S);
    if (inGame && ImGui::IsKeyPressed(ImGuiKey_F11, false)) { cfg.fullscreen = !cfg.fullscreen; saveSettings(); }
    if (!inGame && lastInGame) { peekDone = false; page = 0; loadList.clear(); }        // back at the launcher: look again at the last game
    lastInGame = inGame;
    if (!inGame) titleScreen();
    else if (sim.g.phase == PH_INTERLUDE) interludeScreen();
    else if (sim.g.phase == PH_OVER) overScreen();
    else gameScreen();
    popups();
    ImGui::End();
    if (inGame && sim.g.phase == PH_PLAY) poppedWindows();
    if (inGame && sim.g.phase == PH_PLAY) drawToasts();
    if (inGame && sim.g.phase == PH_PLAY && cfg.toasts) drawMoment();
}

// ============================================================================ the interlude: an order on posting
void App::interludeScreen() {
    GameState& g = sim.g;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 a = ImGui::GetWindowPos(), sz = ImGui::GetWindowSize();
    float w = std::min(sz.x - 100 * S, 1040 * S);
    ImVec2 pa(a.x + (sz.x - w) / 2, a.y + 34 * S), pb(pa.x + w, a.y + sz.y - 34 * S);
    skin::frame(dl, pa - ImVec2(14 * S, 14 * S), pb + ImVec2(14 * S, 14 * S), S);
    sheet(dl, pa, pb, rgb(C_PAPER), S, true);
    float y = pa.y + 14 * S;
    auto centred = [&](ImFont* f, float size, const std::string& t, ImU32 col) {
        ImVec2 ts = f->CalcTextSizeA(size, FLT_MAX, 0, t.c_str());
        dl->AddText(f, size, ImVec2((pa.x + pb.x - ts.x) / 2, y), col, t.c_str());
        y += ts.y + 2 * S;
    };
    hammerSickle(dl, ImVec2((pa.x + pb.x) / 2, y + 22 * S), 44 * S, rgb(C_INK, 0.85f));
    y += 48 * S;
    centred(F.stamp, F.stamp->FontSize, "КОМИТЕТ ГОСУДАРСТВЕННОЙ БЕЗОПАСНОСТИ СССР", rgb(C_INK));
    centred(F.stamp, F.stamp->FontSize, "ПЕРВОЕ ГЛАВНОЕ УПРАВЛЕНИЕ", rgb(C_INK));
    y += 4 * S;
    centred(F.stampBig, F.stampBig->FontSize, "П Р И К А З", rgb(C_INK));
    centred(F.head, F.head->FontSize, "по личному составу  -  order on personnel", rgb(C_INK_FADED));
    centred(F.small, F.small->FontSize, up(g.interludeTitle), rgb(C_INK_FADED));
    dl->AddLine(ImVec2(pa.x + 30 * S, y + 4 * S), ImVec2(pb.x - 30 * S, y + 4 * S), rgb(C_RULE), 1);
    y += 12 * S;
    float optH = (lineH() + 4 * S) * (float)g.interludeOpts.size() + 16 * S;
    ImGui::SetCursorScreenPos(ImVec2(pa.x + 34 * S, y));
    ImGui::BeginChild("interlude", ImVec2(w - 68 * S, pb.y - y - optH - 16 * S), 0, ImGuiWindowFlags_NoBackground);
    uiTyped(g.interludeText, rgb(C_INK), F.body, [](const std::string& l, ImU32& c, ImFont*& f) {
        if (l.find("Verdict") != std::string::npos || l.find("romoted") != std::string::npos || l.find("warded") != std::string::npos) c = rgb(C_RED);
        bool caps = l.size() > 3;
        for (char ch : l) if (islower((unsigned char)ch)) { caps = false; break; }
        if (caps) f = F.bold;
    }, 11);
    ImGui::EndChild();
    seal(dl, ImVec2(pb.x - 120 * S, pb.y - optH - 64 * S), 60 * S, "КОМИТЕТ ГОСУДАРСТВЕННОЙ БЕЗОПАСНОСТИ * ДЛЯ ПАКЕТОВ * ", "", rgb(C_VIOLET, 0.70f), -12);
    pencil(dl, ImVec2(pb.x - 300 * S, pb.y - optH - 60 * S), "Ознакомлен", rgb(C_PENCIL_BLUE, 0.9f), -4);
    float by = pb.y - optH;
    dl->AddLine(ImVec2(pa.x + 30 * S, by - 6 * S), ImVec2(pb.x - 30 * S, by - 6 * S), rgb(C_RULE, 0.6f), 1);
    for (size_t i = 0; i < g.interludeOpts.size(); i++) {
        ImGui::SetCursorScreenPos(ImVec2(pa.x + 34 * S, by + 4 * S + i * (lineH() + 4 * S)));
        if (uiBtn(g.interludeOpts[i].c_str())) {
            sim.chooseInterlude((int)i);
            autosave();
            forgetTextures("p");
            wantTab = T_CENTRE;
            resetSelection();
            break;
        }
    }
}

// ============================================================================ the end of the career
void App::overScreen() {
    GameState& g = sim.g;
    recordCareer();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 a = ImGui::GetWindowPos(), sz = ImGui::GetWindowSize();
    float w = std::min(sz.x - 120 * S, 1180 * S);
    ImVec2 pa(a.x + (sz.x - w) / 2, a.y + 40 * S), pb(pa.x + w, a.y + sz.y - 40 * S);
    // a cardboard file cover behind the sheet, in a frame on the desk
    skin::frame(dl, pa - ImVec2(28 * S, 22 * S), pb + ImVec2(28 * S, 22 * S), S);
    sheet(dl, pa - ImVec2(14 * S, 8 * S), pb + ImVec2(14 * S, 8 * S), rgb(C_MANILA_DK), S, true);
    sheet(dl, pa, pb, rgb(C_PAPER_OLD), S, true);
    dl->AddText(F.stampBig, F.stampBig->FontSize, pa + ImVec2(30 * S, 18 * S), rgb(C_INK), up(g.overTitle).c_str());
    hammerSickle(dl, ImVec2(pb.x - 100 * S, pa.y + 50 * S), 56 * S, rgb(C_INK, 0.8f));
    dl->AddText(F.small, F.small->FontSize, pa + ImVec2(32 * S, 20 * S + F.stampBig->FontSize), rgb(C_INK_FADED), ("ЛИЧНОЕ ДЕЛО  -  " + ru(g.officer) + "  -  SERVICE RECORD").c_str());
    photo(dl, myPhoto(), ImVec2(pb.x - 150 * S, pa.y + 110 * S), ImVec2(96 * S, 128 * S), 2.5f, S, true);
    float top = pa.y + 34 * S + F.stampBig->FontSize + F.small->FontSize;
    ImGui::SetCursorScreenPos(ImVec2(pa.x + 30 * S, top));
    ImGui::BeginChild("over", ImVec2(w - 220 * S, pb.y - top - 60 * S), 0, ImGuiWindowFlags_NoBackground);
    uiTyped(g.overText, rgb(C_INK), F.body, [](const std::string& l, ImU32& c, ImFont*&) {
        if (l.find("PROVOCATION") != std::string::npos || l.find("TURNED") != std::string::npos || l.find("for the adversary") != std::string::npos
            || l.find("DOUBLE") != std::string::npos) c = rgb(C_RED);
    }, 13);
    ImGui::EndChild();
    stamp(dl, ImVec2(pb.x - 104 * S, pa.y + 300 * S), "РАССЕКРЕЧЕНО", rgb(C_RED, 0.8f), -12, F.stampBig, 28 * S, 1, "DECLASSIFIED 1992");
    ImGui::SetCursorScreenPos(ImVec2(pa.x + 30 * S, pb.y - 46 * S));
    if (uiBtn("Return to the title screen")) inGame = false;
    ImGui::SameLine();
    if (uiBtn("Quit")) wantQuit = true;
}

// ============================================================================ the game screen
void App::doEndDay(bool run) {
    if (!sim.g.decisions.empty()) { note = "A decision is waiting for you."; return; }
    sim.askInStreet = cfg.streetChoices; sim.askScenes = cfg.scenes;
    if (cfg.confirmEndDay && !sureEndDay && sim.g.phase == PH_PLAY) {
        std::string undone;
        for (auto& t : sim.todo()) if (t.level == 2 && t.text.find("Agreed meeting") != 0 && t.text.find("detection route") == std::string::npos) undone += "  - " + t.text + "\n";
        if (!undone.empty()) {
            confirmText = "End the day with this still undone?\n\n" + undone;
            confirmAction = [this, run]() { sureEndDay = true; doEndDay(run); sureEndDay = false; };
            return;
        }
    }
    int day0 = sim.g.day;
    size_t cables0 = sim.g.cables.size();
    for (int n = 0; n < (run ? std::max(2, cfg.runMaxDays) : 1); n++) {
        if (n > 0 && cfg.runStopMeeting) {
            bool mine = false;
            for (auto& p : sim.g.persons) if (p.stage == AGENT && p.handler == 0 && p.nextMeet == sim.g.day) mine = true;
            if (mine) break;
        }
        int unreadBefore = sim.unread(), month = dateOf(sim.g.day).m;
        sim.attention = false;
        sim.endDay();
        if (sim.g.phase == PH_PLAY || sim.g.phase == PH_INTERLUDE) {
            if (cfg.monthlyBackups && dateOf(sim.g.day).m != month) rotateBackups();
            autosave();
        }
        if ((sim.attention && (cfg.runStopAlert || !run)) || sim.g.phase != PH_PLAY || !sim.g.decisions.empty()) break;
        if (cfg.runStopCable && sim.unread() > unreadBefore) break;
        if (sim.g.day <= sim.g.busyUntil && sim.onLeave(sim.g.day)) continue;          // leave passes by itself
    }
    reportDay = sim.g.day - 1;
    selRow = 0;
    if (sim.g.phase != PH_PLAY) return;
    pushToasts(day0, cables0);
    auto it = sim.g.reports.find(sim.g.day - 1);
    bool longReport = it != sim.g.reports.end() && it->second.size() > 5;
    if (sim.training()) wantTab = T_REPORT;
    else if (!cfg.jumpAfterDay) {}
    else if (sim.unread() > 0) wantTab = T_CENTRE;
    else if (sim.attention || longReport) wantTab = T_REPORT;
}

void App::gameScreen() {
    ImVec2 a = ImGui::GetWindowPos(), sz = ImGui::GetWindowSize(), b = a + sz;
    float H = std::floor(72 * S), NW = std::floor(206 * S);
    if (wantTab >= 0) { if (popped[wantTab]) focusTab = wantTab; else tab = wantTab; wantTab = -1; }
    shortcuts();
    header(a, ImVec2(b.x, a.y + H));
    if (!inGame || sim.g.phase != PH_PLAY) return;
    // the screen on the desk is one that has not been lifted off it
    if (popped[tab]) { for (int i = 0; i < N_TAB; i++) if (!popped[i]) { tab = i; break; } }
    nav(ImVec2(a.x, a.y + H), ImVec2(a.x + NW, b.y));
    ImGui::SetCursorScreenPos(ImVec2(a.x + NW + 12 * S, a.y + H + 12 * S));
    ImGui::BeginChild("##desk", ImVec2(b.x - a.x - NW - 24 * S, b.y - a.y - H - 24 * S), 0,
                      ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    if (popped[tab]) {
        ImGui::PushStyleColor(ImGuiCol_Text, rgb4(0xE9DDBA));
        ImGui::Dummy(ImVec2(1, 40 * S));
        ImGui::TextUnformatted("Every screen has been lifted off the desk into a window of its own.");
        ImGui::PopStyleColor();
        if (uiBtn("Put them all back")) closeWindows();
    } else drawTab(tab);
    ImGui::EndChild();
}
void App::drawTab(int t) {
    switch (t) {
        case T_PLAN: tabPlanner(); break;
        case T_NOTES: tabNotes(); break;
        case T_PEOPLE: tabContacts(); break;
        case T_MAP: tabMap(); break;
        case T_SITES: tabSites(); break;
        case T_BOARD: tabBoard(); break;
        case T_MAT: tabMaterial(); break;
        case T_CENTRE: tabCentre(); break;
        case T_REPORT: tabReport(); break;
        case T_STATION: tabStation(); break;
        case T_SURV: tabSurveillance(); break;
        case T_ACC: tabAccounts(); break;
        case T_STATS: tabReturns(); break;
        case T_HOME: tabPrivate(); break;
        case T_CAREER: tabCareer(); break;
        default: tabManual(); break;
    }
}

/** A square icon button (the top bar's Save and Menu). */
static bool iconButton(const char* id, int ic, ImVec2 p, ImVec2 size, float S) {
    ImGui::SetCursorScreenPos(p);
    bool hit = ImGui::InvisibleButton(id, size);
    uiRec(id);
    bool hov = ImGui::IsItemHovered(), held = ImGui::IsItemActive();
    float al = ImGui::GetStyle().Alpha;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    skin::button(dl, p, p + size, S, hov, held, skin::B_NORMAL, al);
    skin::icon(dl, ic, p + size * 0.5f + ImVec2(0, held ? 1.f : 0.f), std::min(size.x, size.y) * 0.5f, rgb(hov ? skin::K_BRASS_HI : skin::K_BRASS, al));
    return hit;
}

void App::header(ImVec2 a, ImVec2 b) {
    using namespace skin;
    GameState& g = sim.g;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    // the bar: dark leather, a brass edge underneath
    dl->AddRectFilledMultiColor(a, b, rgb(0x2B231B), rgb(0x2B231B), rgb(0x191410), rgb(0x191410));
    tile(dl, tex("tx:grain", [] { return art::paperGrain(256, 5); }), a, b, 256 * S, IM_COL32(255, 240, 220, 8));
    dl->AddLine(ImVec2(a.x, b.y - 3 * S), ImVec2(b.x, b.y - 3 * S), rgb(K_BRASS_LO), 1);
    dl->AddLine(ImVec2(a.x, b.y - 1 * S), ImVec2(b.x, b.y - 1 * S), rgb(K_BRASS, 0.8f), std::max(1.f, 1.5f * S));
    dl->AddLine(ImVec2(a.x, b.y), ImVec2(b.x, b.y), rgb(0, 0.8f), 1);
    float midY = (a.y + b.y - 3 * S) * 0.5f;

    // ---- the officer: the emblem in a red medallion, rank and name, the post
    ImVec2 mc(a.x + 36 * S, midY);
    dl->AddCircleFilled(mc + ImVec2(0, 2 * S), 23 * S, rgb(0, 0.5f), 40);
    dl->AddCircleFilled(mc, 23 * S, rgb(0x7E1C12), 40);
    dl->AddCircleFilled(mc - ImVec2(0, 4 * S), 18 * S, rgb(0x9C2618), 40);
    dl->AddCircle(mc, 23 * S, rgb(K_BRASS), 40, 2 * S);
    dl->AddCircle(mc, 19.5f * S, rgb(K_BRASS_HI, 0.35f), 40, 1);
    hammerSickle(dl, mc + ImVec2(0, 1 * S), 24 * S, rgb(0xF2C230));
    static const char* const ROLE[] = {"case officer", "senior case officer", "head of line", "Rezident"};
    const Theatre& t = sim.T();
    std::string who = sim.training() ? "Senior Lieutenant " + g.officer : std::string(RANK[g.rank]) + " " + g.officer;
    std::string where = sim.training() ? "Trainee  \xC2\xB7  Moscow, field exercises" : std::string(t.city) + "  \xC2\xB7  " + ROLE[t.role];
    float nx = mc.x + 34 * S;
    dl->AddText(F.uiTitle, F.uiTitle->FontSize, ImVec2(nx, midY - F.uiTitle->FontSize + 1 * S), rgb(K_BRASS_HI), who.c_str());
    dl->AddText(F.uiSmall, F.uiSmall->FontSize, ImVec2(nx, midY + 3 * S), rgb(K_DIM), where.c_str());
    float nameW = std::max(F.uiTitle->CalcTextSizeA(F.uiTitle->FontSize, FLT_MAX, 0, who.c_str()).x, F.uiSmall->CalcTextSizeA(F.uiSmall->FontSize, FLT_MAX, 0, where.c_str()).x);
    if (ImGui::IsMouseHoveringRect(ImVec2(nx, a.y), ImVec2(nx + nameW, b.y)) && cfg.tooltips) ImGui::SetTooltip("%s", (std::string(t.cover) + "\n" + t.city).c_str());

    // ---- the buttons on the right: end the day, skip ahead, save, menu
    float bh = std::floor(b.y - a.y - 22 * S), gap = 8 * S, by = std::floor(midY - bh / 2);
    float wEnd = 148 * S, wRun = 126 * S, wIcon = bh;
    float bx = b.x - 14 * S - wIcon * 2 - gap - wRun - gap - wEnd - gap;
    auto bigButton = [&](const char* id, const char* label, int ic, ImVec2 p, ImVec2 size, int kind) {
        ImGui::SetCursorScreenPos(p);
        bool hit = ImGui::InvisibleButton(id, size);
        uiRec(id);
        bool hov = ImGui::IsItemHovered(), held = ImGui::IsItemActive();
        float al = ImGui::GetStyle().Alpha;
        button(dl, p, p + size, S, hov, held, kind, al);
        ImFont* f = F.bar;
        std::string L = upper(label);
        float tw = trackedWidth(f, f->FontSize, L.c_str(), 1.2f * S);
        float total = tw + 26 * S;
        ImVec2 tp(std::floor(p.x + (size.x - total) / 2), std::floor(p.y + (size.y - f->FontSize) / 2 - 1 * S + (held ? 1 : 0)));
        tracked(dl, f, f->FontSize, tp + ImVec2(0, 1), rgb(0, 0.5f * al), L.c_str(), 1.2f * S);
        tracked(dl, f, f->FontSize, tp, rgb(kind == B_PRIMARY ? 0xFFF4DC : K_TEXT, al), L.c_str(), 1.2f * S);
        icon(dl, ic, ImVec2(tp.x + tw + 16 * S, p.y + size.y / 2 + (held ? 1 : 0)), 16 * S, rgb(kind == B_PRIMARY ? 0xFFE6B0 : K_BRASS, al));
        return hit;
    };
    ImGui::BeginDisabled(!g.decisions.empty());
    if (bigButton("END DAY >", "End day", IC_ARROW, ImVec2(bx, by), ImVec2(wEnd, bh), B_PRIMARY)) doEndDay(false);
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && cfg.tooltips)
        ImGui::SetTooltip("%s", g.decisions.empty() ? "Carry out today's orders and move to tomorrow  (Ctrl+Enter)" : "A decision is waiting - answer it first");
    if (bigButton("Run to next event >>", "Skip ahead", IC_SKIP, ImVec2(bx + wEnd + gap, by), ImVec2(wRun, bh), B_NORMAL)) doEndDay(true);
    if (ImGui::IsItemHovered() && cfg.tooltips)
        ImGui::SetTooltip("Let the days pass until something needs you  (Ctrl+Shift+Enter)\nWhat stops it is chosen in the settings.");
    float sx = bx + wEnd + gap + wRun + gap;
    ImGui::BeginDisabled(g.ironman);
    if (iconButton("Save", IC_SAVE, ImVec2(sx, by), ImVec2(wIcon, bh), S) && !g.ironman) openSave = true;
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && cfg.tooltips)
        ImGui::SetTooltip("%s", g.ironman ? "An ironman career keeps one file, saved every night" : "Save a named copy  (Ctrl+S)\nQuick save F5, quick load F9");
    if (iconButton("Menu", IC_MENU, ImVec2(sx + wIcon + gap, by), ImVec2(wIcon, bh), S)) openSettings = true;
    if (ImGui::IsItemHovered() && cfg.tooltips) ImGui::SetTooltip("Settings, windows, keys, and the way out  (F10)");

    // ---- the date, on a brass-edged plaque left of the buttons
    Date d = dateOf(g.day);
    std::string hol = sim.holiday(g.day);
    static const char* const MONTH[] = {"", "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
    static const char* const WEEKDAY[] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
    std::string dateTxt = fstr("%d %s %d", d.d, MONTH[d.m], d.y);
    std::string dayTxt = std::string(WEEKDAY[d.dow]) + (hol.empty() ? "" : "  \xC2\xB7  holiday") + fstr("  \xC2\xB7  %+d\xC2\xB0", g.temp);
    float dw = std::max(F.uiTitle->CalcTextSizeA(F.uiTitle->FontSize, FLT_MAX, 0, dateTxt.c_str()).x, F.uiSmall->CalcTextSizeA(F.uiSmall->FontSize, FLT_MAX, 0, dayTxt.c_str()).x) + 58 * S;
    ImVec2 da(bx - 16 * S - dw, by), db(bx - 16 * S, by + bh);
    well(dl, da, db, S);
    dl->AddRect(da, db, rgb(K_BRASS_LO), 0, 0, 1);
    // the weather, drawn small in the left of the plaque
    {
        ImVec2 wc(da.x + 22 * S, (da.y + db.y) / 2);
        ImU32 pc = rgb(K_BRASS);
        float r = 7 * S;
        auto cloud = [&](ImVec2 c) {
            dl->AddCircle(c + ImVec2(-r * 0.6f, 0), r * 0.55f, pc, 12, 1.3f * S);
            dl->AddCircle(c + ImVec2(r * 0.2f, -r * 0.35f), r * 0.7f, pc, 14, 1.3f * S);
            dl->AddLine(c + ImVec2(-r * 1.1f, r * 0.5f), c + ImVec2(r * 0.9f, r * 0.5f), pc, 1.3f * S);
        };
        switch (g.wx) {
            case WX_CLEAR:
                dl->AddCircle(wc, r * 0.55f, pc, 14, 1.4f * S);
                for (int k = 0; k < 8; k++) { float an = k * 0.785398f; ImVec2 u(std::cos(an), std::sin(an)); dl->AddLine(wc + u * (r * 0.85f), wc + u * (r * 1.2f), pc, 1.2f * S); }
                break;
            case WX_CLOUD: cloud(wc); break;
            case WX_RAIN: cloud(wc - ImVec2(0, 3 * S)); for (int k = 0; k < 3; k++) dl->AddLine(wc + ImVec2((k - 1) * 4.5f * S, 4 * S), wc + ImVec2((k - 1) * 4.5f * S - 2 * S, 9 * S), pc, 1.2f * S); break;
            case WX_FOG: for (int k = 0; k < 3; k++) dl->AddLine(wc + ImVec2(-r + (k % 2) * 2 * S, (k - 1) * 4.5f * S), wc + ImVec2(r + (k % 2) * 2 * S, (k - 1) * 4.5f * S), pc, 1.3f * S); break;
            default: cloud(wc - ImVec2(0, 3 * S)); for (int k = 0; k < 3; k++) { ImVec2 c = wc + ImVec2((k - 1) * 5.f * S, 7 * S); dl->AddLine(c - ImVec2(2 * S, 0), c + ImVec2(2 * S, 0), pc, 1); dl->AddLine(c - ImVec2(0, 2 * S), c + ImVec2(0, 2 * S), pc, 1); } break;
        }
    }
    float tx = da.x + 42 * S;
    // the day just ended rolls down out of the plaque while the new one rolls in from above
    float inA = 1, inY = 0;
    {
        double now = ImGui::GetTime();
        if (calDay != g.day) { if (calDay >= 0 && calDay < g.day && g.day - calDay <= 31) { tornDay = calDay; tornT = now; } calDay = g.day; }
        double age = now - tornT;
        if (tornDay >= 0 && age < 0.4) {
            float k = (float)(age / 0.4), e = k * k * (3 - 2 * k);
            Date od = dateOf(tornDay);
            std::string ot = fstr("%d %s %d", od.d, MONTH[od.m], od.y);
            dl->PushClipRect(da + ImVec2(1, 1), db - ImVec2(1, 1), true);
            dl->AddText(F.uiTitle, F.uiTitle->FontSize, ImVec2(tx, da.y + (bh - F.uiTitle->FontSize - F.uiSmall->FontSize) / 2 - 1 * S + bh * e), rgb(K_BRASS_HI, 1 - e), ot.c_str());
            dl->PopClipRect();
            inA = 0.3f + 0.7f * e; inY = -bh * (1 - e);
        }
    }
    dl->PushClipRect(da + ImVec2(1, 1), db - ImVec2(1, 1), true);
    dl->AddText(F.uiTitle, F.uiTitle->FontSize, ImVec2(tx, da.y + (bh - F.uiTitle->FontSize - F.uiSmall->FontSize) / 2 - 1 * S + inY), rgb(d.dow == 6 || !hol.empty() ? 0xF08A6A : K_BRASS_HI, inA), dateTxt.c_str());
    dl->AddText(F.uiSmall, F.uiSmall->FontSize, ImVec2(tx, da.y + (bh + F.uiTitle->FontSize - F.uiSmall->FontSize) / 2 - 1 * S + inY), rgb(K_DIM, inA), dayTxt.c_str());
    dl->PopClipRect();
    ImGui::SetCursorScreenPos(da);
    ImGui::InvisibleButton("calendar", db - da);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", (fmtDay(g.day) + (hol.empty() ? std::string("") : "  -  " + hol + " (public holiday)") + "\n" + sim.weatherLine() + "\nTomorrow: " + g.forecast + ".").c_str());

    // ---- the figures that matter, each an icon, a value and a label, between the name and the date
    static const char* const COVER[] = {"blown", "thin", "adequate", "good", "solid"};
    std::string cur = t.currency;
    std::string funds = cur == "pounds" ? fstr("\xC2\xA3%d", g.funds) : cur == "dollars" ? fstr("$%d", g.funds) : fstr("%d %s", g.funds, cur.c_str());
    struct Fig { const char* label; std::string value; int warn; float bar; const char* tip; int ic; };
    float fat = g.fatigue;
    std::vector<Fig> figs = {
        {"Funds", funds, g.funds < 40 ? 1 : 0, -1, "Operational funds in the station safe", IC_FUNDS},
        {"Fatigue", fstr("%d", (int)std::lround(fat)), fat > 70 ? 2 : fat > 50 ? 1 : 0, fat / 100, "Above 50 you make mistakes in the street; above 70 they become likely", IC_FATIGUE},
        {"Cover", word(g.cover, COVER, 5), g.cover < 40 ? 2 : g.cover < 55 ? 1 : 0, -1, "How well your cover job is being done - the Embassy notices", IC_COVER},
        {"Reports", fstr("%.0f h", std::ceil(g.paperwork)), g.paperwork > 8 ? 2 : g.paperwork > 5 ? 1 : 0, -1, "Hours of written reports outstanding. Over 8 they are in arrears.", IC_PEN},
        {t.role == 3 ? "Department" : "Rezident", word(g.standRez, W_STAND, 5), g.standRez < 25 ? 2 : g.standRez < 38 ? 1 : 0, -1, "How your chief rates you", IC_CHIEF},
        {"Centre", word(g.standCentre, W_STAND, 5), g.standCentre < 25 ? 2 : g.standCentre < 38 ? 1 : 0, -1, "How Moscow rates you", IC_KREMLIN},
        {"Watched", g.krEst < 0 ? "not yet known" : g.krEst < 22 ? "little" : g.krEst < 45 ? "at times" : g.krEst < 68 ? "regularly" : "closely", g.krEst >= 68 ? 2 : g.krEst >= 45 ? 1 : 0, clampf(g.krEst / 100.f, 0.02f, 1.f),
         "How closely the other side watches you, by Line KR's assessment on Monday - an estimate, not the truth.\nPredictable habits and carelessness raise it; quiet weeks let it fall.", IC_SURV},
    };
    float fx = nx + nameW + 30 * S, fEnd = da.x - 14 * S;
    // a status tag first, when today has one thing that matters most
    {
        const char* tag = nullptr; unsigned col = 0;
        if (!g.decisions.empty()) { tag = "Decision waiting"; col = 0x8E2A1C; }
        else if (g.day <= g.standDownUntil) { tag = "Stand-down"; col = 0x8E2A1C; }
        else if (sim.onLeave(g.day)) { tag = "On leave"; col = 0x40366E; }
        else if (g.day <= g.busyUntil) { tag = "Taken up"; col = 0x40366E; }
        else if (g.fatigue > 70) { tag = "Over-tired"; col = 0x8E2A1C; }
        else if (g.paperwork > 8) { tag = "Reports in arrears"; col = 0x8A5418; }
        if (tag) {
            std::string T = upper(tag);
            float tw = trackedWidth(F.uiCaps, F.uiCaps->FontSize, T.c_str(), 1 * S);
            ImVec2 p(fEnd - tw - 22 * S, midY - 11 * S), q(fEnd, midY + 11 * S);
            if (p.x > fx + 200 * S) {
                dl->AddRectFilled(p, q, rgb(col));
                dl->AddRect(p, q, rgb(K_BRASS, 0.8f), 0, 0, 1);
                tracked(dl, F.uiCaps, F.uiCaps->FontSize, ImVec2(p.x + 11 * S, midY - F.uiCaps->FontSize * 0.58f), rgb(0xFFF1DA), T.c_str(), 1 * S);
                fEnd = p.x - 14 * S;
            }
        }
    }
    dl->AddLine(ImVec2(fx - 15 * S, a.y + 12 * S), ImVec2(fx - 15 * S, b.y - 15 * S), rgb(K_BRASS_LO, 0.8f), 1);
    for (auto& f : figs) {
        float vw = F.figure->CalcTextSizeA(F.figure->FontSize, FLT_MAX, 0, f.value.c_str()).x;
        float lw = F.uiCaps->CalcTextSizeA(F.uiCaps->FontSize, FLT_MAX, 0, upper(f.label).c_str()).x + 4 * S;
        float w = 34 * S + std::max(vw, lw);
        if (fx + w > fEnd) break;
        ImU32 vc = f.warn == 2 ? rgb(K_BAD) : f.warn == 1 ? rgb(K_WARN) : rgb(K_TEXT);
        ImVec2 ic(fx + 12 * S, midY);
        dl->AddCircleFilled(ic, 14 * S, rgb(0x0E0B09), 24);
        dl->AddCircle(ic, 14 * S, rgb(f.warn ? (f.warn == 2 ? 0xB0402E : 0xA0782E) : K_BRASS_LO), 24, 1.2f * S);
        icon(dl, f.ic, ic, 17 * S, f.warn ? vc : rgb(K_BRASS));
        float x2 = fx + 33 * S;
        tracked(dl, F.uiCaps, F.uiCaps->FontSize, ImVec2(x2, midY - F.uiCaps->FontSize - 3 * S), rgb(K_FAINT), upper(f.label).c_str(), 0.6f * S);
        dl->AddText(F.figure, F.figure->FontSize, ImVec2(x2, midY - 3 * S), vc, f.value.c_str());
        if (f.bar >= 0) gauge(dl, ImVec2(x2 + vw + 6 * S, midY + 6 * S), ImVec2(x2 + vw + 36 * S, midY + 11 * S), f.bar, vc, S);
        ImGui::SetCursorScreenPos(ImVec2(fx, a.y + 6 * S));
        ImGui::InvisibleButton(f.label, ImVec2(w, b.y - a.y - 12 * S));
        if (ImGui::IsItemHovered() && cfg.tooltips) ImGui::SetTooltip("%s", f.tip);
        fx += w + (f.bar >= 0 ? 40 * S : 22 * S);
    }
}

void App::nav(ImVec2 a, ImVec2 b) {
    using namespace skin;
    GameState& g = sim.g;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const char* const* EN = TAB_EN;
    dl->AddRectFilledMultiColor(a, b, rgb(0x1E1914), rgb(0x1E1914), rgb(0x151210), rgb(0x151210));
    dl->AddLine(ImVec2(b.x - 2, a.y), ImVec2(b.x - 2, b.y), rgb(K_BRASS_LO), 1);
    dl->AddLine(ImVec2(b.x - 1, a.y), ImVec2(b.x - 1, b.y), rgb(0, 0.7f), 1);
    static const int ICON[N_TAB] = {IC_PLAN, IC_CHECK, IC_PEOPLE, IC_MAP, IC_SITES, IC_BOARD, IC_MAT, IC_CENTRE, IC_REPORT, IC_STATION, IC_SURV, IC_ACC, IC_RESULTS, IC_HOME, IC_CAREER, IC_BOOK};
    static const char* const GROUP[N_TAB] = {"Today", nullptr, "Operations", nullptr, nullptr, nullptr, nullptr, "Communications", nullptr, "Station", nullptr, nullptr, nullptr, "Personal", nullptr, nullptr};
    float cardH = sim.training() ? 160 * S : 0;
    float avail = b.y - a.y - cardH - 16 * S;
    float th = std::floor(std::min(34 * S, avail / (N_TAB + 5 * 0.9f))), gh = std::floor(th * 0.9f), y = a.y + 6 * S;
    int urgent = 0;
    for (auto& t : sim.todo()) if (t.level == 2) urgent++;
    for (int i = 0; i < N_TAB; i++) {
        if (GROUP[i]) {
            std::string G = upper(GROUP[i]);
            float gy = y + gh - F.uiCaps->FontSize - 5 * S;
            float gw = trackedWidth(F.uiCaps, F.uiCaps->FontSize, G.c_str(), 1.4f * S);
            tracked(dl, F.uiCaps, F.uiCaps->FontSize, ImVec2(a.x + 18 * S, gy), rgb(K_BRASS, 0.75f), G.c_str(), 1.4f * S);
            float ly = gy + F.uiCaps->FontSize * 0.55f;
            dl->AddRectFilledMultiColor(ImVec2(a.x + 26 * S + gw, ly), ImVec2(b.x - 14 * S, ly + 1), rgb(K_BRASS_LO), rgb(K_BRASS_LO, 0), rgb(K_BRASS_LO, 0), rgb(K_BRASS_LO));
            y += gh;
        }
        bool sel = tab == i;
        ImVec2 ta(a.x + 6 * S, y), tb(b.x - 6 * S, y + th - 2 * S);
        ImGui::SetCursorScreenPos(ta);
        ImGui::SetNextItemAllowOverlap();
        if (ImGui::InvisibleButton((std::string("tab:") + EN[i]).c_str(), tb - ta)) { if (popped[i]) focusTab = i; else tab = i; }
        uiRec(std::string("tab:") + EN[i]);
        bool hov = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) popOut(i, !popped[i]);
        // the mark that lifts the screen off the desk into a window of its own (or puts it back)
        ImVec2 pc(tb.x - 16 * S, (ta.y + tb.y) / 2);
        ImGui::SetCursorScreenPos(pc - ImVec2(9 * S, 9 * S));
        if (ImGui::InvisibleButton((std::string("pop:") + EN[i]).c_str(), ImVec2(18 * S, 18 * S))) popOut(i, !popped[i]);
        uiRec(std::string("pop:") + EN[i]);
        bool phov = ImGui::IsItemHovered();
        if (phov && cfg.tooltips) ImGui::SetTooltip("%s", popped[i] ? "Put this screen back on the desk" : "Open this screen in its own window\n(it can be dragged to another monitor; right-clicking the name does the same)");
        hov = hov || phov;
        if (sel) {
            dl->AddRectFilledMultiColor(ta, tb, rgb(0x6A1E14), rgb(0x3A140E, 0.6f), rgb(0x3A140E, 0.6f), rgb(0x5A1A12));
            dl->AddLine(ImVec2(ta.x, ta.y), ImVec2(tb.x, ta.y), rgb(K_BRASS, 0.45f), 1);
            dl->AddLine(ImVec2(ta.x, tb.y), ImVec2(tb.x, tb.y), rgb(K_BRASS, 0.25f), 1);
            dl->AddRectFilled(ta, ImVec2(ta.x + 3 * S, tb.y), rgb(K_BRASS_HI));
        } else if (hov) dl->AddRectFilled(ta, tb, rgb(0xFFE8C0, 0.06f));
        ImU32 ink = sel ? rgb(0xFFF1D8) : popped[i] ? rgb(K_FAINT) : rgb(hov ? K_TEXT : 0xC9BEA6);
        icon(dl, ICON[i], ImVec2(ta.x + 22 * S, (ta.y + tb.y) / 2), 19 * S, sel ? rgb(K_BRASS_HI) : popped[i] ? rgb(K_FAINT) : rgb(hov ? K_BRASS_HI : K_BRASS));
        dl->AddText(F.label, F.label->FontSize, ImVec2(ta.x + 42 * S, std::floor((ta.y + tb.y - F.label->FontSize) / 2 - 1 * S)), ink, EN[i]);
        int badge = i == T_CENTRE ? sim.unread() : i == T_NOTES ? urgent : 0;
        float rx = tb.x - 8 * S;
        if (hov || popped[i]) {
            icon(dl, IC_POPOUT, pc, 13 * S, rgb(popped[i] ? 0xE08A6A : K_BRASS, phov ? 1.f : 0.7f));
            rx = pc.x - 13 * S;
        }
        if (badge > 0) {
            std::string n = badge > 99 ? "99+" : fstr("%d", badge);
            ImVec2 ts = F.uiCaps->CalcTextSizeA(F.uiCaps->FontSize, FLT_MAX, 0, n.c_str());
            float bw = std::max(ts.x + 10 * S, 20 * S), cy = (ta.y + tb.y) / 2;
            ImVec2 p(rx - bw, cy - 9 * S);
            dl->AddRectFilled(p, p + ImVec2(bw, 18 * S), rgb(i == T_NOTES ? 0x9A5A16 : 0xA8281A), 9 * S);
            dl->AddRect(p, p + ImVec2(bw, 18 * S), rgb(K_BRASS_HI, 0.5f), 9 * S, 0, 1);
            dl->AddText(F.uiCaps, F.uiCaps->FontSize, ImVec2(std::floor(p.x + (bw - ts.x) / 2), std::floor(cy - ts.y / 2)), rgb(0xFFFFFF), n.c_str());
        }
        y += th;
    }
    // during the course, the instructor's card at the foot of the list
    if (sim.training()) {
        ImVec2 ca(a.x + 12 * S, b.y - cardH), cb(b.x - 12 * S, b.y - 12 * S);
        sheet(dl, ca, cb, rgb(0xF7F2DF), S, true);
        for (float ly = ca.y + 36 * S; ly < cb.y - 6 * S; ly += 16 * S) dl->AddLine(ImVec2(ca.x + 4 * S, ly), ImVec2(cb.x - 4 * S, ly), rgb(0x7A9CC6, 0.35f), 1);
        dl->AddLine(ImVec2(ca.x + 4 * S, ca.y + 32 * S), ImVec2(cb.x - 4 * S, ca.y + 32 * S), rgb(C_PENCIL_RED, 0.5f), 1);
        pushpin(dl, ImVec2((ca.x + cb.x) / 2, ca.y + 7 * S), S, rgb(C_RED));
        pencil(dl, ca + ImVec2(12 * S, 8 * S), fstr("Упражнение № %d", std::min(g.tut, 9)), rgb(C_PENCIL_BLUE), -2);
        const char* l[] = {"Today's exercise is on the", "Planner. The instructor", "marks it in the Daily report."};
        for (int i = 0; i < 3; i++) dl->AddText(F.uiSmall, F.uiSmall->FontSize, ca + ImVec2(10 * S, (37 + 16 * i) * S), rgb(C_INK), l[i]);
        ImGui::SetCursorScreenPos(ImVec2(ca.x + 10 * S, cb.y - 34 * S));
        if (uiBtn("End the course")) {
            confirmText = "Leave the course now and fly to London?\n\nThe passing-out report will say that the course was not completed.";
            confirmAction = [this]() { sim.skipTraining(); forgetTextures("p"); resetSelection(); wantTab = T_CENTRE; };
        }
    } else if (b.y - y > 120 * S) {
        // the emblem, embossed, in the empty foot of the rail
        ImVec2 ec((a.x + b.x) / 2, b.y - std::min(70 * S, (b.y - y) / 2));
        hammerSickle(dl, ec + ImVec2(0, 1.5f * S), 54 * S, rgb(0, 0.5f));
        hammerSickle(dl, ec, 54 * S, rgb(K_BRASS, 0.16f));
    }
}

// ============================================================================ the order form
void App::beginEdit(int day, int block) {
    edDay = day; edBlock = block;
    ed = sim.order(day, block);
    // an agreed meeting opened for the first time: fill in the usual agenda, which the player can change
    if (Person* p = sim.person(ed.person))
        if (ed.appointment && ed.type == O_MEET_CLAN && p->stage == AGENT && ed.pay == 0 && ed.film == 0 && !ed.camera && !ed.train && !ed.brush
            && prefilled.insert(((long long)day * 4 + block) * 100000 + p->id).second)
            sim.usualAgenda(ed, *p, day);
    edError.clear();
    openEditor = true;
}

std::string App::place(Order o, int day, int block) {
    if (sim.isSet(day, block) && sim.order(day, block).appointment) return "that block holds an agreed meeting.";
    o.cs = false;                              // the team is arranged day by day; ask again
    o.appointment = false;
    std::string err = sim.validate(o, day, block);
    if (!err.empty()) return err;
    sim.setOrder(day, block, o);
    return "";
}

const char* const ORDER_HINT[N_OTYPE] = {
    "Your cover work. About five blocks a week keeps it credible; if the watchers are on you, they see a diplomat doing his job.",
    "Write up contact reports and accounts. Clears up to 4.5 hours of outstanding paperwork.",
    "Recovers fatigue. A tired officer misses surveillance.",
    "A reception, lecture or society meeting. The usual way to meet new people. Afternoon or evening.",
    "Go over one of your sites again, or walk one of your routes: raises its casing (max 3), sharpens your estimates, and is the cheapest way to notice men waiting at a dead drop.",
    "Look for a new meeting place, dead drop, signal site or detection route in a district.",
    "A surveillance detection route. It only helps the act in the block IMMEDIATELY after it. A prepared route beats an improvised one - until you have walked it too often.",
    "Coffee (morning), lunch (afternoon) or dinner (evening) in the open. Builds the relationship and your assessment. For contacts only - never for a recruited agent.",
    "A meeting at one of your meeting sites. Needed for a recruitment approach. Agents are met only on the agreed date, at the agreed site.",
    "Leave money, film or written instructions in the agent's dead drop. He collects them at his next servicing.",
    "Empty a dead drop. Read the signal site first: 'drop loaded' means there is something to fetch; the danger signal means stay away.",
    "Walk past a signal site and see what mark is up. Little time, little risk.",
    "Put up your call-out mark. If the agent sees it he comes to the next meeting site in his rotation three evenings later.",
    "Place the sealed package for an illegal in a dead drop that has never been used, in the district Centre named. You will never meet the man who collects it.",
    "A marked card. Tell exactly ONE colleague that you will service this drop at this hour, and go and see whether anybody is waiting. Use a drop nobody else has heard of."};

void App::orderEditor() {
    GameState& g = sim.g;
    if (openEditor) { ImGui::OpenPopup("Order"); openEditor = false; }
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    std::string ordTitle = "Orders  -  " + fmtDay(edDay) + ", " + BLOCK_LABEL[edBlock];
    bool open = uiModal("Order", nullptr, ImGuiWindowFlags_AlwaysAutoResize, ordTitle.c_str());

    if (!open) return;
    const float W = 540 * S, LW = 272 * S;
    Order& o = ed;
    bool appt = o.appointment;
    Person* p = sim.person(o.person);
    bool agent = p && p->stage == AGENT;
    // the head of the form
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 hp = ImGui::GetCursorScreenPos();
    if (appt) stamp(dl, ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowWidth() - 110 * S, hp.y + 10 * S), "ЯВКА", rgb(0xE0503A, 0.85f), -6, F.stamp, 22 * S, 1, "AGREED MEETING: FIXED");
    ImGui::PushTextWrapPos(LW + W + 20 * S);
    ImGui::PushStyleColor(ImGuiCol_Text, rgb4(C_INK_FADED));
    ImGui::PushFont(F.uiSmall);
    ImGui::TextUnformatted(ORDER_HINT[o.type]);
    ImGui::PopFont();
    ImGui::PopStyleColor();
    ImGui::PopTextWrapPos();
    ImGui::Separator();
    auto label = [&](const char* s) { ImGui::AlignTextToFramePadding(); ImGui::TextUnformatted(s); ImGui::SameLine(LW); };
    // the body of the form scrolls if the screen is short; the head and the buttons stay in view
    ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, std::max(200.f, ImGui::GetMainViewport()->WorkSize.y * 0.96f - 230 * S)));
    ImGui::BeginChild("##orderbody", ImVec2(0, 0), ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY);

    // ---- what
    std::vector<Opt> types;
    bool lineN = false;
    for (auto& q : g.reqs) if (!q.closed && q.kind == RK_LINEN) lineN = true;
    for (int t = 0; t < N_OTYPE; t++) {
        if (t == O_LINE_N && !lineN && o.type != O_LINE_N) continue;
        if (sim.training() && t == O_CONTROL) continue;
        types.push_back({OTYPE_LABEL[t], t});
    }
    label("Activity");
    ImGui::BeginDisabled(appt);
    if (uiCombo("##type", o.type, types, W)) { o.person = -1; o.site = -1; o.pitch = -1; o.am = -1; edError.clear(); p = nullptr; agent = false; }
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
        uiCombo("##person", o.person, ps, W);
        ImGui::EndDisabled();
        p = sim.person(o.person);
        agent = p && p->stage == AGENT;
    }
    auto siteOpts = [&](int type, int only, int district) {
        std::vector<Opt> v;
        for (auto& s : g.sites) {
            if (s.retired || (type >= 0 && s.type != type) || (only >= 0 && s.id != only) || (district >= 0 && s.district != district)) continue;
            std::string l = clip(s.name, 60);
            if (s.flat) l += "  (SAFE FLAT";
            else if (s.type == ST_ROUTE) l += "  (quality " + word(s.estSight, W_LEVEL, 5) + fstr(", known %d/3", s.casing);
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
        uiCombo("##linen", o.lineN, qs, W);
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
        if (ss.empty()) ImGui::TextDisabled("(none available)"); else uiCombo("##site", o.site, ss, W);
        ImGui::EndDisabled();
    }
    if (t == O_SDR) {
        std::vector<Opt> rs = {{"Improvise as you go", -1}};
        for (auto& x : siteOpts(ST_ROUTE, -1, -1)) rs.push_back(x);
        label("Route");
        uiCombo("##route", o.route, rs, W);
    }
    if (t == O_SCOUT) {
        std::vector<Opt> ds, ks;
        for (int i = 0; i < 8; i++)
            ds.push_back({std::string(sim.T().district[i]) + (sim.T().dist[i] == 0 ? "  (on the doorstep)" : sim.T().dist[i] == 1 ? "  (near)" : sim.T().dist[i] == 2 ? "  (moderate)" : "  (far)")
                              + (sim.eastSide(i) ? "  - our side" : ""), i});
        for (int i = 0; i < N_SITETYPE; i++) ks.push_back({SITETYPE_LABEL[i], i});
        label("District");
        uiCombo("##district", o.district, ds, W);
        label("Looking for");
        uiCombo("##kind", o.siteType, ks, W);
    }
    if (t == O_FUNCTION) {
        std::vector<Opt> sc;
        for (int i = 0; i < N_SECTOR; i++) sc.push_back({SECTOR_LABEL[i], i});
        label("Circle");
        uiCombo("##sector", o.sector, sc, W);
    }
    if (t == O_CONTROL) {
        std::vector<Opt> os;
        for (auto& x : g.officers) if (!x.gone && (x.role == ROLE_COLLEAGUE || x.role == ROLE_SUB || x.role == ROLE_TECH)) os.push_back({x.title(), x.id});
        if (o.officer < 0 && !os.empty()) o.officer = os[0].value;
        label("The one officer told");
        uiCombo("##officer", o.officer, os, W);
    }
    // ---- security
    bool approach = t == O_CASE || t == O_SCOUT || t == O_MEET_CLAN || t == O_LOAD_DROP || t == O_UNLOAD_DROP || t == O_LINE_N || t == O_CONTROL;
    if (approach) {
        ImGui::SeparatorText("Security");
        std::vector<Opt> pol;
        for (int i = 0; i < 3; i++) pol.push_back({POLICY[i], i});
        label("If surveillance is seen");
        uiCombo("##policy", o.policy, pol, W);
        std::vector<Opt> tv;
        for (int i = 0; i < 3; i++) tv.push_back({TRAVEL[i], i});
        label("Getting there");
        uiCombo("##travel", o.travel, tv, W);
        if (o.travel > 0) {
            ImGui::PushStyleColor(ImGuiCol_Text, rgb4(C_INK_FADED));
            ImGui::PushTextWrapPos(LW + W + 20 * S);
            ImGui::PushFont(F.uiSmall);
            ImGui::TextUnformatted(o.travel == 1 ? "Saves your legs on the far districts. But a car is followed in relays and is hard to shake, and its plates are known: seen parked by the place, it marks the place."
                                                 : sim.T().dist[0] >= 0 ? fstr("Fares about %d %s. Changing vehicles breaks a team's contact; some drivers report their fares.", sim.P(2) + sim.P(1) * 2, sim.T().currency).c_str() : "");
            ImGui::PopFont();
            ImGui::PopTextWrapPos();
            ImGui::PopStyleColor();
        }
        auto cs = g.csDays.find(edDay);
        bool arranged = cs != g.csDays.end() && cs->second.state == 2;
        if (arranged) {
            Officer* a = sim.officer(cs->second.o1); Officer* b = sim.officer(cs->second.o2);
            uiCheck(("Countersurveillance team covers this act: " + (a ? a->title() : std::string("?")) + ", " + (b ? b->title() : std::string("?"))).c_str(), &o.cs);
        } else { o.cs = false; ImGui::TextDisabled("No countersurveillance team is arranged for this day (Planner, under the plan)."); }
    }
    // ---- agenda
    bool meet = t == O_MEET_CLAN, load = t == O_LOAD_DROP, overt = t == O_MEET_OVERT;
    bool pitching = meet && !agent && o.pitch >= 0;
    if (meet || load || overt) {
        ImGui::SeparatorText("Agenda");
        if (meet && agent && p) {
            if (uiBtn("Fill in the usual agenda")) sim.usualAgenda(o, *p, edDay);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", gx(p, "Pay what he is owed, bring his film up to two cassettes, a camera if he has none, instruction in what he is weakest at - and a present, if it is his birthday.").c_str());
        }
        if (meet && agent) uiCheck("Brush pass: twelve seconds in a crowd - packages only, no talk", &o.brush);
        if (!(meet && agent)) o.brush = false;
        if (o.brush) { o.longMeet = false; o.train = false; o.exfil = false; o.bug = false; o.pads = 0; o.docs = false; o.standing = false; }
        if (meet && !o.brush) uiCheck("Long meeting, about two hours (untick for a ten-minute contact)", &o.longMeet);
        if (meet && agent) uiCheck(gx(p, "Take delivery of his exposed film (untick for a talk-only meeting)").c_str(), &o.collect);
        label(agent || load ? "Money to hand over" : "Money / gift to offer");
        ImGui::SetNextItemWidth(150 * S);
        ImGui::InputInt("##pay", &o.pay, sim.P(5), sim.P(20));
        o.pay = std::max(0, std::min(o.pay, 99999));
        ImGui::SameLine();
        if (agent || load) ImGui::TextDisabled("%s", gx(p, fstr("(he is owed %d; you hold %d %s)", p ? std::max(0, p->owed) : 0, g.funds, sim.T().currency)).c_str());
        else ImGui::TextDisabled("(you hold %d %s)", g.funds, sim.T().currency);
        if ((meet && (agent || pitching)) || load) {
            label("Fresh cassettes to give");
            ImGui::SetNextItemWidth(150 * S);
            ImGui::InputInt("##film", &o.film, 1, 2);
            o.film = std::max(0, std::min(o.film, 6));
            ImGui::SameLine(); ImGui::TextDisabled("(you hold %d)", g.film);
            uiCheck(fstr("Issue a Minox camera (you hold %d)", g.cameras).c_str(), &o.camera);
        }
        if (meet && (agent || pitching) && !o.brush) {
            label("One-time pad sheets to give");
            ImGui::SetNextItemWidth(150 * S);
            ImGui::InputInt("##pads", &o.pads, 1, 2);
            o.pads = std::max(0, std::min(o.pads, 6));
            ImGui::SameLine(); ImGui::TextDisabled("%s", gx(p, fstr("(you hold %d; with them he can receive broadcasts)", g.pads)).c_str());
        }
        if (load) uiCheck(fstr("Use a concealment container (you hold %d)", g.containers).c_str(), &o.container);
        if (meet && agent && !o.brush) {
            uiCheck("Give instruction (long meetings only)", &o.train);
            if (o.train) {
                std::vector<Opt> sk;
                const float lv[] = {p ? p->craft : 0, p ? p->skPhoto : 0, p ? p->skElicit : 0, p ? p->skSpot : 0, p ? p->skComms : 0};
                for (int i = 0; i < 5; i++) sk.push_back({std::string(SKILL_LABEL[i]) + "  - now " + word(lv[i], W_LEVEL, 5), i});
                label("Instruction in");
                uiCombo("##trainWhat", o.trainWhat, sk, W);
            }
            Site* at = sim.site(o.site);
            if (at && at->flat) {
                uiCheck(gx(p, "He brings the documents themselves, and you photograph them here (2 cassettes; long meeting)").c_str(), &o.docs);
                if (o.docs) o.longMeet = true;
            } else o.docs = false;
            if ((g.bugs > 0 && p && p->bug == 0) || o.bug) uiCheck(gx(p, fstr("Give him a listening device to place in his office (you hold %d; long meeting)", g.bugs)).c_str(), &o.bug);
            if ((p && p->exfil == 2) || o.exfil) { if (uiCheck(gx(p, "HAND OVER THE TRAVEL DOCUMENTS: he leaves the country tonight").c_str(), &o.exfil) && o.exfil) o.standing = false; }
            if ((p && p->exfil == 2) || o.standing) {
                if (uiCheck(gx(p, "Or: rehearse it as a STANDING ESCAPE PLAN - he stays in place, with a way out").c_str(), &o.standing) && o.standing) o.exfil = false;
                if (o.standing) o.longMeet = true;
            }
        }
        if ((meet && !agent) || overt) uiCheck("Ask a small favour (builds commitment; may be refused)", &o.favour);
        if (p && (meet || overt) && !o.brush && sim.presentDue(*p, edDay)) {
            int bd = sim.nearBirthday(*p, edDay), turns = dateOf(bd).y - sim.birthYear(*p);
            bool round = turns % 10 == 0;
            uiCheck(gx(p, fstr("A present for his birthday (%s%s, about %d %s)", fmtDay(bd).c_str(), round ? fstr(" - his %dth", turns).c_str() : "",
                               sim.P(round ? 12 : 5), sim.T().currency)).c_str(), &o.present);
            if (ImGui::IsItemHovered() && cfg.tooltips) ImGui::SetTooltip("%s", round ? "A round birthday. Remembered, it is worth a great deal; forgotten, it will be mentioned." : "People like to be remembered.");
        } else o.present = false;
        if ((meet && (agent || pitching)) || load) {
            std::vector<Opt> tp = {{"(no change)", -1}};
            for (int i = 0; i < 4; i++) tp.push_back({TEMPO[i], i});
            std::vector<Opt> rq = {{"(no change)", -2}, {"General collection (no requirement)", -1}};
            for (auto& q : g.reqs) if (!q.closed && q.kind == RK_COLLECT) rq.push_back({fstr("No. %d  ", q.id) + q.title + " [" + SECTOR_SHORT[q.sector] + "]", q.id});
            label("Tempo of collection");
            uiCombo("##tempo", o.tempo, tp, W);
            label("Task against requirement");
            uiCombo("##task", o.reqId, rq, W);
        }
        if ((meet && !o.brush) || overt) {
            std::vector<Opt> am = {{"(none)", -1}};
            for (auto& q : g.reqs) if (!q.closed && q.kind == RK_ACTIVE) am.push_back({fstr("No. %d  ", q.id) + q.title, q.id});
            if (am.size() > 1) { label("Pass Service A material"); uiCombo("##am", o.am, am, W); }
        }
        if (meet && !agent) {
            std::vector<Opt> pi = {{"(no approach at this meeting)", -1}};
            for (int i = 0; i < 4; i++) pi.push_back({std::string("Pitch on: ") + PITCH[i], i});
            label("Recruitment approach");
            uiCombo("##pitch", o.pitch, pi, W);
            if (o.pitch >= 0 && p && p->sanction != 2) ImGui::TextColored(rgb4(C_RED), "Centre has NOT sanctioned this recruitment.");
        }
    }
    // ---- communications plan
    if (meet && (agent || pitching) && !o.brush) {
        ImGui::SeparatorText("Communications plan - briefed at this meeting");
        std::vector<Opt> me = {{"(no change)", -1}, {"every 14 days", 14}, {"every 21 days", 21}, {"every 28 days", 28}, {"every 42 days", 42}, {"every 60 days", 60}, {"none - drops and signals only", 0}};
        label("Personal meetings");
        uiCombo("##meetEvery", o.meetEvery, me, W);
        uiCheck(gx(p, "Change the meeting sites in his rotation").c_str(), &o.setMeetSites);
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
        uiCombo("##dropEvery", o.dropEvery, de, W);
        std::vector<Opt> dsites = {{"(no change)", -2}, {"(none)", -1}}, ssites = dsites;
        for (auto& s : siteOpts(ST_DROP, -1, -1)) dsites.push_back(s);
        for (auto& s : siteOpts(ST_SIGNAL, -1, -1)) ssites.push_back(s);
        label("Dead drop");
        uiCombo("##dropSite", o.dropSite, dsites, W);
        label("Signal site");
        uiCombo("##signalSite", o.signalSite, ssites, W);
    }
    ImGui::EndChild();
    ImGui::Separator();
    if (!edError.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, rgb4(C_RED));
        ImGui::PushTextWrapPos(LW + W);
        ImGui::TextUnformatted(edError.c_str());
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
    }
    if (!o.appointment) {
        static const std::vector<Opt> REP = {{"this once", 0}, {"and every day for a week", 1}, {"and every second day for a fortnight", 2}, {"and every third day for a month", 3},
                                             {"and weekly, for four weeks", 4}, {"and weekly, for eight weeks", 5}};
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Enter it");
        ImGui::SameLine(LW + ImGui::GetCurrentWindow()->WindowPadding.x);       // (in line with the fields of the form above, which sit in a child)
        uiCombo("##repeat", edRepeat, REP, W);
    }
    if (uiBtn("OK", ImVec2(110 * S, 0))) {
        Order out = o;
        if (!usesPerson) out.person = -1;
        if (!usesSite) out.site = -1;
        if (!o.setMeetSites) out.meetSites.clear();
        std::string err = sim.validate(out, edDay, edBlock);
        if (err.empty()) {
            sim.setOrder(edDay, edBlock, out);
            if (edRepeat > 0 && !o.appointment) {       // the same order on later days, wherever the block is free and the order is allowed
                static const int STEP[] = {0, 1, 2, 3, 7, 7}, COUNT[] = {0, 6, 7, 10, 4, 8};
                int done = 0, skipped = 0;
                for (int k = 1; k <= COUNT[edRepeat]; k++) {
                    int day = edDay + STEP[edRepeat] * k;
                    Order again = out;
                    again.cs = false;
                    if (sim.isSet(day, edBlock) || !sim.validate(again, day, edBlock).empty()) { skipped++; continue; }
                    sim.setOrder(day, edBlock, again);
                    done++;
                }
                note = fstr("Entered on %d further day%s", done, done == 1 ? "" : "s") + (skipped ? fstr("; %d left alone (the block was taken, or the order is not possible that day).", skipped) : std::string("."));
            }
            edRepeat = 0;
            ImGui::CloseCurrentPopup();
        }
        else edError = err;
    }
    ImGui::SameLine();
    if (uiBtn("Cancel", ImVec2(110 * S, 0))) ImGui::CloseCurrentPopup();
    uiEndModal();
}

// ============================================================================ popups
void App::popups() {
    GameState& g = sim.g;
    ImVec2 centre = ImGui::GetMainViewport()->GetCenter();
    for (ImGuiViewport* v : ImGui::GetPlatformIO().Viewports) {        // a window on another monitor gets its questions there
        ImVec2 m = ImGui::GetIO().MousePos;
        if (m.x >= v->Pos.x && m.y >= v->Pos.y && m.x < v->Pos.x + v->Size.x && m.y < v->Pos.y + v->Size.y && v->Size.x > 700 * S && v->Size.y > 500 * S) centre = v->GetCenter();
    }
    const float PW = 660 * S;
    extraPopups(centre);
    orderEditor();

    // ---- radio / instruction to an agent
    // ---- renting a safe flat
    if (openFlat) { ImGui::OpenPopup("A safe flat"); openFlat = false; }
    ImGui::SetNextWindowPos(centre, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (uiModal("A safe flat", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        const Theatre& t = sim.T();
        ImGui::PushTextWrapPos(PW);
        ImGui::TextUnformatted("A flat rented through a cut-out - a local who signs the lease and asks no questions. Meetings indoors can be long and unhurried; "
                               "an agent can bring out documents for the evening and you photograph them properly on the kitchen table; instruction there goes further. "
                               "But every visitor is seen by somebody on the stairs, and the concierge notices patterns. Meet there only agents, never contacts you are still developing.");
        ImGui::PopTextWrapPos();
        ImGui::Spacing();
        std::vector<Opt> ds;
        for (int i = 0; i < 8; i++) {
            if (sim.eastSide(i)) continue;
            ds.push_back({std::string(t.district[i]) + fstr("  -  %d %s a month", sim.flatCost(i), t.currency) + (t.dist[i] == 0 ? "  (on the doorstep)" : t.dist[i] == 1 ? "  (near)" : t.dist[i] == 2 ? "  (moderate)" : "  (far)"), i});
        }
        if (flatDistrict < 0 && !ds.empty()) flatDistrict = ds.back().value;
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("District");
        ImGui::SameLine(110 * S);
        uiCombo("##flatd", flatDistrict, ds, 470 * S);
        ImGui::TextDisabled("%s", fstr("Three months' rent is paid in advance. You hold %d %s.", g.funds, t.currency).c_str());
        ImGui::Spacing();
        if (uiBtn("Rent it", ImVec2(130 * S, 0))) { note = sim.rentFlat(flatDistrict); ImGui::CloseCurrentPopup(); }
        ImGui::SameLine();
        if (uiBtn("Cancel", ImVec2(130 * S, 0))) ImGui::CloseCurrentPopup();
        uiEndModal();
    }

    // ---- a compromise operation against a contact
    if (openKomp) { ImGui::OpenPopup("A compromise operation"); openKomp = false; }
    ImGui::SetNextWindowPos(centre, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (uiModal("A compromise operation", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        Person* kp = sim.person(kompPerson);
        const Theatre& t = sim.T();
        ImGui::PushTextWrapPos(PW);
        ImGui::TextUnformatted(gx(kp, std::string("Against ") + (kp ? kp->name : std::string("?")) + ".\n\nA man who will not come of his own accord can be given a reason. Centre must sanction it (90 groups, a week or two); "
                                      "then it is staged at your next dinner with him. If it works you hold material against him, and an approach by coercion bites much harder. "
                                      "Whether it works depends on what he really is - which is not always what your file says. If he sees what was tried, you lose him, and he may go to his security officer.").c_str());
        ImGui::PopTextWrapPos();
        ImGui::Spacing();
        static const char* const WHY[] = {"works on somebody who needs money", "works on somebody with a weakness and a marriage", "weak, but it rarely fails; worse for him the stricter his employer"};
        for (int k = 0; k < 3; k++) {
            ImGui::RadioButton(gx(kp, std::string(KOMP_LABEL[k]) + fstr("  (%d %s)", sim.kompCost(k), t.currency)).c_str(), &kompKind, k);
            uiRec(fstr("komp:%d", k));
            ImGui::SameLine();
            ImGui::TextDisabled("- %s", gx(kp, WHY[k]).c_str());
        }
        ImGui::Spacing();
        if (uiBtn("Propose it to Centre", ImVec2(220 * S, 0))) { if (kp) note = sim.prepareCompromise(*kp, kompKind); ImGui::CloseCurrentPopup(); }
        ImGui::SameLine();
        if (uiBtn("Cancel", ImVec2(130 * S, 0))) ImGui::CloseCurrentPopup();
        uiEndModal();
    }

    if (openInstr) { ImGui::OpenPopup("Instruction to an agent"); openInstr = false; }
    ImGui::SetNextWindowPos(centre, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (uiModal("Instruction to an agent", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
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
            uiCombo("Tempo", instrTempo, tp, 400 * S);
            uiCombo("Tasking", instrReq, rq, 400 * S);
            if (!viaOfficer) uiCheck("Summon him to his next meeting site, three evenings after the broadcast", &instrCallout);
            if (uiBtn("Send")) { note = viaOfficer ? sim.instruct(*p, instrTempo, instrReq) : sim.radioMsg(*p, instrTempo, instrReq, instrCallout); ImGui::CloseCurrentPopup(); }
            ImGui::SameLine();
            if (uiBtn("Cancel##instr")) ImGui::CloseCurrentPopup();
        }
        uiEndModal();
    }

    // ---- save
    if (openSave) { ImGui::OpenPopup("Save"); openSave = false; }
    ImGui::SetNextWindowPos(centre, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (uiModal("Save", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("The game saves itself every night (autosave). To keep a named copy as well:");
        ImGui::SetNextItemWidth(300 * S);
        ImGui::InputText("##save", saveBuf, sizeof saveBuf);
        if (uiBtn("Save##do")) {
            std::string n;
            for (char ch : std::string(saveBuf)) if (isalnum((unsigned char)ch) || ch == '-' || ch == '_' || ch == ' ') n += ch;
            if (n.empty()) n = "posting";
            note = saveTo(saveDir + "/" + n + ".rez") ? "Saved as " + n + ".rez in the saves folder." : std::string("The file could not be written.");
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (uiBtn("Cancel##save")) ImGui::CloseCurrentPopup();
        uiEndModal();
    }

    // ---- confirm
    if (!confirmText.empty() && !ImGui::IsPopupOpen("Confirm")) ImGui::OpenPopup("Confirm");
    ImGui::SetNextWindowPos(centre, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (uiModal("Confirm", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::PushTextWrapPos(PW);
        ImGui::TextUnformatted(confirmText.c_str());
        ImGui::PopTextWrapPos();
        ImGui::Spacing();
        if (uiBtn("Yes", ImVec2(90 * S, 0))) { auto f = confirmAction; confirmText.clear(); confirmAction = nullptr; ImGui::CloseCurrentPopup(); if (f) f(); }
        ImGui::SameLine();
        if (uiBtn("No", ImVec2(90 * S, 0))) { confirmText.clear(); confirmAction = nullptr; ImGui::CloseCurrentPopup(); }
        uiEndModal();
    }

    // ---- a decision that cannot be put off
    bool otherOpen = ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopup);
    if (inGame && g.phase == PH_PLAY && !g.decisions.empty() && !otherOpen && note.empty()) ImGui::OpenPopup("Decision");
    ImGui::SetNextWindowPos(centre, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    std::string decTitle = g.decisions.size() > 1 ? fstr("A decision  -  1 of %d", (int)g.decisions.size()) : std::string("A decision");
    if (uiModal("Decision", nullptr, ImGuiWindowFlags_AlwaysAutoResize, decTitle.c_str())) {
        if (g.decisions.empty()) ImGui::CloseCurrentPopup();
        else {
            Decision d = g.decisions[0];
            if (d.kind == DK_SCENE) {              // the man across the table
                Person* sp = sim.person(sim.order(g.day, std::max(0, std::min(2, d.ref))).person);
                if (sp) {
                    ImDrawList* pdl = ImGui::GetWindowDrawList();
                    float pw = 150 * S;
                    ImVec2 pp = ImGui::GetCursorScreenPos() + ImVec2((PW - pw) / 2, 8 * S);
                    photo(pdl, facePhoto(*sp), pp, ImVec2(pw, pw * 4 / 3), 1.5f, S, true);
                    ImGui::Dummy(ImVec2(PW, pw * 4 / 3 + 22 * S));
                }
            }
            if (d.kind == DK_STREET) {             // a photograph from the street: what you are looking at as you decide
                ImDrawList* pdl = ImGui::GetWindowDrawList();
                float pw = std::min(PW * 0.55f, 330 * S);
                ImVec2 pp = ImGui::GetCursorScreenPos() + ImVec2((PW - pw) / 2, 8 * S);
                photo(pdl, scenePhoto(d.id, art::SC_WATCHERS), pp, ImVec2(pw, pw * 0.75f), -1.5f, S, false);
                ImGui::Dummy(ImVec2(PW, pw * 0.75f + 22 * S));
            }
            ImGui::PushFont(F.uiTitle);
            ImGui::TextColored(rgb4(skin::K_BRASS_HI), "%s", d.title.c_str());
            ImGui::PopFont();
            ImGui::Dummy(ImVec2(1, 2 * S));
            ImGui::PushTextWrapPos(PW);
            ImGui::TextUnformatted(d.text.c_str());
            ImGui::PopTextWrapPos();
            ImGui::Dummy(ImVec2(1, 4 * S));
            skin::divider(ImGui::GetWindowDrawList(), ImGui::GetCursorScreenPos(), PW, S);
            ImGui::Dummy(ImVec2(1, 8 * S));
            // each choice a full-width row: its number (also a key), and what it means
            ImDrawList* dl = ImGui::GetWindowDrawList();
            for (size_t i = 0; i < d.opts.size(); i++) {
                ImGui::PushID((int)i);
                ImVec2 p = ImGui::GetCursorScreenPos();
                float tw = PW - 46 * S;
                ImVec2 ts = F.ui->CalcTextSizeA(F.ui->FontSize, FLT_MAX, tw, d.opts[i].c_str());
                float h = std::max(34 * S, ts.y + 16 * S);
                bool hit = ImGui::InvisibleButton("opt", ImVec2(PW, h));
                uiRec(fstr("decision:%d", (int)i));
                bool hov = ImGui::IsItemHovered();
                if (!hit && ImGui::IsKeyPressed((ImGuiKey)(ImGuiKey_1 + (int)i), false) && i < 9) hit = true;
                skin::button(dl, p, p + ImVec2(PW, h), S, hov, ImGui::IsItemActive(), skin::B_QUIET);
                if (hov) dl->AddRectFilled(p + ImVec2(1, 1), ImVec2(p.x + 3 * S, p.y + h - 1), rgb(skin::K_RED));
                skin::diamond(dl, p + ImVec2(21 * S, h / 2), 11 * S, rgb(hov ? 0x8E2A1C : 0x3A2F25), rgb(hov ? skin::K_BRASS_HI : skin::K_BRASS));
                std::string n = fstr("%d", (int)i + 1);
                ImVec2 ns = F.uiCaps->CalcTextSizeA(F.uiCaps->FontSize, FLT_MAX, 0, n.c_str());
                dl->AddText(F.uiCaps, F.uiCaps->FontSize, p + ImVec2(21 * S, h / 2) - ns * 0.5f, rgb(skin::K_BRASS_HI), n.c_str());
                dl->AddText(F.ui, F.ui->FontSize, p + ImVec2(42 * S, (h - ts.y) / 2), rgb(hov ? 0xFFF6E2 : skin::K_TEXT), d.opts[i].c_str(), nullptr, tw);
                ImGui::Dummy(ImVec2(1, 2 * S));
                ImGui::PopID();
                if (hit) {
                    int day0 = sim.g.day;
                    size_t cables0 = sim.g.cables.size();
                    sim.askInStreet = cfg.streetChoices; sim.askScenes = cfg.scenes;
                    sim.decide(d.id, (int)i);
                    autosave();
                    ImGui::CloseCurrentPopup();
                    if ((d.kind == DK_STREET || d.kind == DK_SCENE) && sim.g.day != day0 && sim.g.phase == PH_PLAY) {    // the day ran on to its end
                        reportDay = sim.g.day - 1; selRow = 0;
                        pushToasts(day0, cables0);
                        if (cfg.jumpAfterDay) wantTab = T_REPORT;
                    }
                    break;
                }
            }
        }
        uiEndModal();
    }

    // ---- a note
    if (!note.empty() && !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopup)) ImGui::OpenPopup("Rezidentura");
    ImGui::SetNextWindowPos(centre, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (uiModal("Rezidentura", nullptr, ImGuiWindowFlags_AlwaysAutoResize, "Note")) {
        ImGui::PushTextWrapPos(PW);
        ImGui::TextUnformatted(note.c_str());
        ImGui::PopTextWrapPos();
        ImGui::Dummy(ImVec2(1, 6 * S));
        if (uiBtn("OK##note", ImVec2(100 * S, 0)) || note.empty()) { note.clear(); ImGui::CloseCurrentPopup(); }
        uiEndModal();
    }
}
