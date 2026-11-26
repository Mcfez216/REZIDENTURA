// skin.cpp - see skin.h.
#include "skin.h"
#include "imgui_internal.h"
#include <cfloat>
#include <cmath>

namespace skin {
using namespace gfx;

static const float PI = 3.14159265f;
float scale = 1;

// ============================================================================ surfaces
void desk(ImDrawList* dl, ImVec2 a, ImVec2 b, float S) {
    dl->AddRectFilled(a, b, rgb(K_DESK));
    ImTextureID w = tex("tx:wood", [] { return art::wood(512, 9); });
    tile(dl, w, a, b, 512 * S, rgb(0x342B24));                        // dark walnut, barely there
    ImVec2 c = (a + b) * 0.5f;
    ImU32 dark = rgb(0x050403, 0.55f), none = rgb(0x050403, 0);
    dl->AddRectFilledMultiColor(a, ImVec2(c.x, b.y), dark, none, none, dark);
    dl->AddRectFilledMultiColor(ImVec2(c.x, a.y), b, none, dark, dark, none);
    dl->AddRectFilledMultiColor(a, ImVec2(b.x, a.y + 120 * S), dark, dark, none, none);
}

void diamond(ImDrawList* dl, ImVec2 c, float r, ImU32 fill, ImU32 edge) {
    ImVec2 p[4] = {c + ImVec2(0, -r), c + ImVec2(r, 0), c + ImVec2(0, r), c + ImVec2(-r, 0)};
    dl->AddConvexPolyFilled(p, 4, fill);
    if (edge) dl->AddPolyline(p, 4, edge, ImDrawFlags_Closed, 1);
}

void trim(ImDrawList* dl, ImVec2 a, ImVec2 b, float S) {
    dl->AddRect(a, b, rgb(0x080605), 0, 0, 1);                                         // the cut edge
    float i1 = std::floor(2 * S) + 0.5f, i2 = std::floor(5 * S) + 0.5f;
    dl->AddRect(a + ImVec2(i1, i1), b - ImVec2(i1, i1), rgb(K_BRASS_LO), 0, 0, 1);     // the brass line
    dl->AddRect(a + ImVec2(i2, i2), b - ImVec2(i2, i2), rgb(K_BRASS, 0.16f), 0, 0, 1); // and its echo
    // the corners: a bracket in brass, a small diamond, and a thinner bracket inside it
    float L = std::min(16 * S, std::min(b.x - a.x, b.y - a.y) * 0.3f), t = std::max(1.f, 2 * S);
    ImVec2 cs[4] = {a + ImVec2(i1, i1), ImVec2(b.x - i1, a.y + i1), b - ImVec2(i1, i1), ImVec2(a.x + i1, b.y - i1)};
    float dx[4] = {1, -1, -1, 1}, dy[4] = {1, 1, -1, -1};
    for (int k = 0; k < 4; k++) {
        ImVec2 p = cs[k];
        ImVec2 l1[3] = {p + ImVec2(0, dy[k] * L), p, p + ImVec2(dx[k] * L, 0)};
        dl->AddPolyline(l1, 3, rgb(K_BRASS), 0, t);
        ImVec2 q = p + ImVec2(dx[k] * 4 * S, dy[k] * 4 * S);
        ImVec2 l2[3] = {q + ImVec2(0, dy[k] * L * 0.55f), q, q + ImVec2(dx[k] * L * 0.55f, 0)};
        dl->AddPolyline(l2, 3, rgb(K_BRASS, 0.55f), 0, 1);
        diamond(dl, p + ImVec2(dx[k] * 7.5f * S, dy[k] * 7.5f * S), 1.8f * S, rgb(K_BRASS_HI));
    }
}

void frame(ImDrawList* dl, ImVec2 a, ImVec2 b, float S, bool shadow) {
    if (shadow) {
        dl->AddRectFilled(a + ImVec2(2 * S, 4 * S), b + ImVec2(4 * S, 7 * S), rgb(0, 0.30f), 3 * S);
        dl->AddRectFilled(a + ImVec2(0, 1 * S), b + ImVec2(1 * S, 2 * S), rgb(0, 0.45f));
    }
    dl->AddRectFilledMultiColor(a, b, rgb(K_PANEL), rgb(K_PANEL), rgb(K_PANEL_LO), rgb(K_PANEL_LO));
    ImTextureID g = tex("tx:grain", [] { return art::paperGrain(256, 5); });
    tile(dl, g, a, b, 256 * S, IM_COL32(255, 240, 220, 9));
    trim(dl, a, b, S);
}

float titleBarH(float S) { return std::floor(34 * S); }

float titleBar(ImDrawList* dl, ImVec2 a, float w, float S, const char* title, const char* hint) {
    float h = titleBarH(S), in = std::floor(3 * S);
    ImVec2 p0 = a + ImVec2(in, in), p1 = ImVec2(a.x + w - in, a.y + in + h);
    dl->AddRectFilledMultiColor(p0, p1, rgb(K_BAR), rgb(K_BAR), rgb(K_BAR_LO), rgb(K_BAR_LO));
    dl->AddLine(ImVec2(p0.x, p1.y), ImVec2(p1.x, p1.y), rgb(K_BRASS_LO), 1);
    dl->AddLine(ImVec2(p0.x, p1.y + 1), ImVec2(p1.x, p1.y + 1), rgb(0, 0.6f), 1);
    // a red diamond, the title in letter-spaced capitals, and the hint after it
    float cy = (p0.y + p1.y) * 0.5f;
    float x = p0.x + 18 * S;
    diamond(dl, ImVec2(x, cy), 4.2f * S, rgb(K_RED), rgb(K_BRASS));
    x += 13 * S;
    std::string t = upper(title ? title : "");
    ImFont* f = F.bar ? F.bar : ImGui::GetFont();
    float fs = f->FontSize;
    float tw = trackedWidth(f, fs, t.c_str(), 1.3f * S);
    tracked(dl, f, fs, ImVec2(x, std::floor(cy - fs * 0.62f)), rgb(K_BRASS_HI), t.c_str(), 1.3f * S);
    x += tw + 12 * S;
    if (hint && *hint && F.uiSmall) {
        float hs = F.uiSmall->FontSize;
        float avail = p1.x - 12 * S - x;
        if (avail > 60 * S) {
            dl->PushClipRect(ImVec2(x, p0.y), ImVec2(p1.x - 10 * S, p1.y), true);
            dl->AddText(F.uiSmall, hs, ImVec2(x, std::floor(cy - hs * 0.55f)), rgb(K_DIM), hint);
            dl->PopClipRect();
        }
    }
    return h + in;
}

void divider(ImDrawList* dl, ImVec2 a, float w, float S, float alpha) {
    float cx = a.x + w * 0.5f;
    ImU32 c = rgb(K_BRASS, 0.55f * alpha), none = rgb(K_BRASS, 0);
    dl->AddRectFilledMultiColor(ImVec2(a.x, a.y), ImVec2(cx - 8 * S, a.y + 1), none, c, c, none);
    dl->AddRectFilledMultiColor(ImVec2(cx + 8 * S, a.y), ImVec2(a.x + w, a.y + 1), c, none, none, c);
    diamond(dl, ImVec2(cx, a.y + 0.5f), 3 * S, rgb(K_BRASS, alpha), 0);
}

void well(ImDrawList* dl, ImVec2 a, ImVec2 b, float S) {
    dl->AddRectFilled(a, b, rgb(K_INSET));
    dl->AddLine(a, ImVec2(b.x, a.y), rgb(0, 0.7f), 1);
    dl->AddLine(ImVec2(a.x, b.y), b, rgb(K_BRASS, 0.18f), 1);
    (void)S;
}

void gauge(ImDrawList* dl, ImVec2 a, ImVec2 b, float k, ImU32 col, float S) {
    well(dl, a, b, S);
    k = std::max(0.f, std::min(1.f, k));
    if (k <= 0) return;
    ImVec2 e(a.x + 1 + (b.x - a.x - 2) * k, b.y - 1);
    dl->AddRectFilled(a + ImVec2(1, 1), e, col);
    dl->AddRectFilled(a + ImVec2(1, 1), ImVec2(e.x, a.y + 1 + (b.y - a.y) * 0.4f), rgb(0xFFFFFF, 0.12f));
}

// ============================================================================ text
std::string upper(const std::string& s) {
    std::string o;
    o.reserve(s.size());
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = (unsigned char)s[i];
        if (c < 0x80) { o += (char)toupper(c); continue; }
        if (c == 0xD0 && i + 1 < s.size()) {
            unsigned char d = (unsigned char)s[i + 1];
            o += (char)0xD0;
            o += (char)(d >= 0xB0 && d <= 0xBF ? d - 0x20 : d);
            i++;
            continue;
        }
        if (c == 0xD1 && i + 1 < s.size()) {
            unsigned char d = (unsigned char)s[i + 1];
            if (d >= 0x80 && d <= 0x8F) { o += (char)0xD0; o += (char)(d + 0x20); }
            else if (d == 0x91) { o += (char)0xD0; o += (char)0x81; }
            else { o += (char)c; o += (char)d; }
            i++;
            continue;
        }
        o += (char)c;
    }
    return o;
}

static const char* nextChar(const char* s) {
    unsigned int c;
    int n = ImTextCharFromUtf8(&c, s, nullptr);
    return s + (n > 0 ? n : 1);
}
float trackedWidth(ImFont* f, float size, const char* text, float tracking) {
    float w = 0;
    for (const char* s = text; *s;) {
        const char* e = nextChar(s);
        w += f->CalcTextSizeA(size, FLT_MAX, 0, s, e).x + tracking;
        s = e;
    }
    return w > 0 ? w - tracking : 0;
}
void tracked(ImDrawList* dl, ImFont* f, float size, ImVec2 pos, ImU32 col, const char* text, float tracking) {
    float x = pos.x;
    for (const char* s = text; *s;) {
        const char* e = nextChar(s);
        dl->AddText(f, size, ImVec2(std::floor(x + 0.5f), pos.y), col, s, e);
        x += f->CalcTextSizeA(size, FLT_MAX, 0, s, e).x + tracking;
        s = e;
    }
}

// ============================================================================ buttons
void button(ImDrawList* dl, ImVec2 a, ImVec2 b, float S, bool hov, bool held, int kind, float al) {
    unsigned top, bot, edge, inner;
    if (kind == B_PRIMARY) {
        top = held ? 0x6A1A10 : hov ? 0xB83A24 : 0xA02E1C;
        bot = held ? 0x8E2516 : hov ? 0x7E2215 : 0x6A1B11;
        edge = hov ? K_BRASS_HI : K_BRASS; inner = K_BRASS_HI;
    } else if (kind == B_QUIET) {
        top = hov ? 0x2E261E : 0x221C17; bot = hov ? 0x251F19 : 0x1B1713;
        edge = hov ? K_BRASS_LO : 0x3E3427; inner = 0;
    } else {
        top = held ? 0x221C16 : hov ? 0x4A3D30 : 0x3B3128;
        bot = held ? 0x352C23 : hov ? 0x342B22 : 0x282119;
        edge = hov ? K_BRASS : K_BRASS_LO; inner = 0;
    }
    dl->AddRectFilled(a + ImVec2(0, 2 * S), b + ImVec2(0, 2 * S), rgb(0, 0.35f * al));
    dl->AddRectFilledMultiColor(a, b, rgb(top, al), rgb(top, al), rgb(bot, al), rgb(bot, al));
    if (!held) dl->AddLine(a + ImVec2(1, 1), ImVec2(b.x - 1, a.y + 1), rgb(0xFFF0D0, 0.10f * al), 1);
    dl->AddRect(a, b, rgb(edge, al), 0, 0, 1);
    if (inner) dl->AddRect(a + ImVec2(2 * S, 2 * S), b - ImVec2(2 * S, 2 * S), rgb(inner, 0.28f * al), 0, 0, 1);
}

// ============================================================================ icons
void icon(ImDrawList* dl, int id, ImVec2 c, float s, ImU32 col) {
    float t = std::max(1.2f, s * 0.085f);
    auto P = [&](float x, float y) { return ImVec2(c.x + x * s, c.y + y * s); };
    auto line = [&](float x0, float y0, float x1, float y1, float th = 0) { dl->AddLine(P(x0, y0), P(x1, y1), col, th > 0 ? th : t); };
    auto rect = [&](float x0, float y0, float x1, float y1) { dl->AddRect(P(x0, y0), P(x1, y1), col, 0, 0, t); };
    auto rectF = [&](float x0, float y0, float x1, float y1) { dl->AddRectFilled(P(x0, y0), P(x1, y1), col); };
    auto circ = [&](float x, float y, float r) { dl->AddCircle(P(x, y), r * s, col, 0, t); };
    auto circF = [&](float x, float y, float r) { dl->AddCircleFilled(P(x, y), r * s, col); };
    auto poly = [&](std::initializer_list<ImVec2> pts, bool closed) {
        ImVec2 q[16];
        int n = 0;
        for (auto& p : pts) if (n < 16) q[n++] = P(p.x, p.y);
        dl->AddPolyline(q, n, col, closed ? ImDrawFlags_Closed : 0, t);
    };
    auto arc = [&](float x, float y, float r, float a0, float a1) {
        dl->PathArcTo(P(x, y), r * s, a0, a1, 16);
        dl->PathStroke(col, 0, t);
    };
    switch (id) {
        case IC_PLAN:                                            // a calendar
            rect(-0.40f, -0.30f, 0.40f, 0.42f);
            rectF(-0.40f, -0.30f, 0.40f, -0.12f);
            line(-0.20f, -0.42f, -0.20f, -0.22f); line(0.20f, -0.42f, 0.20f, -0.22f);
            for (int r = 0; r < 2; r++) for (int k = 0; k < 3; k++) rectF(-0.26f + k * 0.2f, 0.0f + r * 0.18f, -0.14f + k * 0.2f, 0.10f + r * 0.18f);
            break;
        case IC_CHECK:                                           // a clipboard with a tick
            rect(-0.32f, -0.36f, 0.32f, 0.44f);
            rectF(-0.14f, -0.46f, 0.14f, -0.30f);
            poly({ImVec2(-0.17f, 0.06f), ImVec2(-0.04f, 0.20f), ImVec2(0.19f, -0.10f)}, false);
            break;
        case IC_PEOPLE:                                          // a head and shoulders
            circ(0, -0.17f, 0.19f);
            dl->PathArcTo(P(0, 0.48f), 0.38f * s, PI * 1.05f, PI * 1.95f, 16);
            dl->PathStroke(col, 0, t);
            break;
        case IC_MAP:                                             // a folded map
            poly({ImVec2(-0.42f, -0.28f), ImVec2(-0.14f, -0.38f), ImVec2(0.14f, -0.28f), ImVec2(0.42f, -0.38f), ImVec2(0.42f, 0.30f), ImVec2(0.14f, 0.40f),
                  ImVec2(-0.14f, 0.30f), ImVec2(-0.42f, 0.40f)}, true);
            line(-0.14f, -0.38f, -0.14f, 0.30f, t * 0.8f); line(0.14f, -0.28f, 0.14f, 0.40f, t * 0.8f);
            break;
        case IC_SITES:                                           // a marker pin
            dl->PathArcTo(P(0, -0.12f), 0.26f * s, PI * 0.80f, PI * 2.20f, 18);
            dl->PathLineTo(P(0, 0.44f));
            dl->PathStroke(col, ImDrawFlags_Closed, t);
            circF(0, -0.12f, 0.09f);
            break;
        case IC_MAT:                                             // a camera
            rect(-0.42f, -0.20f, 0.42f, 0.32f);
            rectF(-0.30f, -0.32f, -0.08f, -0.20f);
            circ(0.04f, 0.06f, 0.15f);
            circF(0.30f, -0.10f, 0.04f);
            break;
        case IC_CENTRE:                                          // a radio mast
            poly({ImVec2(-0.20f, 0.44f), ImVec2(0, -0.08f), ImVec2(0.20f, 0.44f)}, false);
            line(-0.12f, 0.24f, 0.12f, 0.24f, t * 0.8f);
            circF(0, -0.14f, 0.07f);
            arc(0, -0.14f, 0.24f, PI * 0.80f, PI * 1.20f); arc(0, -0.14f, 0.24f, -PI * 0.20f, PI * 0.20f);
            arc(0, -0.14f, 0.40f, PI * 0.82f, PI * 1.18f); arc(0, -0.14f, 0.40f, -PI * 0.18f, PI * 0.18f);
            break;
        case IC_REPORT:                                          // a typed sheet
            poly({ImVec2(-0.30f, -0.42f), ImVec2(0.14f, -0.42f), ImVec2(0.30f, -0.26f), ImVec2(0.30f, 0.42f), ImVec2(-0.30f, 0.42f)}, true);
            for (int k = 0; k < 4; k++) line(-0.18f, -0.18f + k * 0.15f, k == 3 ? 0.02f : 0.18f, -0.18f + k * 0.15f, t * 0.8f);
            break;
        case IC_STATION:                                         // a building with columns
            poly({ImVec2(-0.44f, -0.12f), ImVec2(0, -0.40f), ImVec2(0.44f, -0.12f)}, true);
            rectF(-0.42f, 0.32f, 0.42f, 0.42f);
            for (float x : {-0.28f, -0.09f, 0.09f, 0.28f}) line(x, -0.06f, x, 0.30f);
            break;
        case IC_SURV:                                            // an eye
            dl->PathArcTo(P(0, 0.38f), 0.62f * s, PI * 1.24f, PI * 1.76f, 14);
            dl->PathArcTo(P(0, -0.38f), 0.62f * s, PI * 0.24f, PI * 0.76f, 14);
            dl->PathStroke(col, ImDrawFlags_Closed, t);
            circF(0, 0, 0.12f);
            break;
        case IC_ACC:                                             // coins in a stack
            for (int k = 0; k < 3; k++) {
                float y = 0.28f - k * 0.18f;
                dl->AddEllipseFilled(P(-0.06f, y), ImVec2(0.30f * s, 0.11f * s), rgb(0, 0.25f));
                dl->AddEllipse(P(-0.06f, y), ImVec2(0.30f * s, 0.11f * s), col, 0, 0, t);
            }
            circ(0.24f, -0.20f, 0.17f);
            break;
        case IC_RESULTS:                                         // a bar chart
            line(-0.42f, 0.40f, 0.42f, 0.40f);
            rectF(-0.32f, 0.10f, -0.14f, 0.36f); rectF(-0.09f, -0.12f, 0.09f, 0.36f); rectF(0.14f, -0.34f, 0.32f, 0.36f);
            break;
        case IC_HOME:                                            // a house
            poly({ImVec2(-0.44f, 0.0f), ImVec2(0, -0.38f), ImVec2(0.44f, 0.0f)}, false);
            poly({ImVec2(-0.30f, -0.10f), ImVec2(-0.30f, 0.40f), ImVec2(0.30f, 0.40f), ImVec2(0.30f, -0.10f)}, false);
            rect(-0.08f, 0.14f, 0.08f, 0.40f);
            break;
        case IC_CAREER:                                          // a medal on its ribbon
            poly({ImVec2(-0.20f, -0.44f), ImVec2(0, -0.04f), ImVec2(0.20f, -0.44f)}, false);
            circ(0, 0.18f, 0.24f);
            star(dl, P(0, 0.19f), 0.13f * s, col);
            break;
        case IC_BOOK:                                            // an open book
            poly({ImVec2(0, -0.24f), ImVec2(-0.44f, -0.34f), ImVec2(-0.44f, 0.30f), ImVec2(0, 0.40f), ImVec2(0.44f, 0.30f), ImVec2(0.44f, -0.34f)}, true);
            line(0, -0.24f, 0, 0.40f);
            break;
        case IC_FUNDS:                                           // a coin
            circ(0, 0, 0.38f); circ(0, 0, 0.26f);
            star(dl, c, 0.15f * s, col);
            break;
        case IC_FATIGUE:                                         // the moon
            dl->PathArcTo(c, 0.36f * s, PI * 0.35f, PI * 1.65f, 20);
            dl->PathArcTo(P(0.17f, -0.05f), 0.27f * s, PI * 1.55f, PI * 0.45f, 16);
            dl->PathStroke(col, ImDrawFlags_Closed, t);
            break;
        case IC_COVER:                                           // a diplomat's tie
            poly({ImVec2(-0.10f, -0.42f), ImVec2(0.10f, -0.42f), ImVec2(0.06f, -0.28f), ImVec2(-0.06f, -0.28f)}, true);
            poly({ImVec2(-0.06f, -0.28f), ImVec2(-0.15f, 0.26f), ImVec2(0, 0.44f), ImVec2(0.15f, 0.26f), ImVec2(0.06f, -0.28f)}, true);
            break;
        case IC_PEN:                                             // a pen
            line(-0.32f, 0.32f, 0.28f, -0.28f, t * 1.8f);
            poly({ImVec2(-0.32f, 0.32f), ImVec2(-0.42f, 0.42f)}, false);
            line(0.18f, -0.38f, 0.38f, -0.18f);
            break;
        case IC_CHIEF:                                           // a shoulder board with a star
            dl->AddRect(P(-0.44f, -0.18f), P(0.44f, 0.18f), col, 0.12f * s, 0, t);
            line(-0.44f, 0, 0.44f, 0, t * 0.6f);
            star(dl, P(0.16f, 0), 0.11f * s, col);
            star(dl, P(-0.16f, 0), 0.11f * s, col);
            break;
        case IC_KREMLIN:                                         // a Kremlin tower and its star
            rect(-0.16f, -0.02f, 0.16f, 0.44f);
            poly({ImVec2(-0.22f, -0.02f), ImVec2(0, -0.30f), ImVec2(0.22f, -0.02f)}, true);
            star(dl, P(0, -0.38f), 0.10f * s, col);
            rect(-0.05f, 0.24f, 0.05f, 0.44f);
            break;
        case IC_SAVE:                                            // a file folder
            poly({ImVec2(-0.42f, -0.30f), ImVec2(-0.12f, -0.30f), ImVec2(-0.04f, -0.20f), ImVec2(0.42f, -0.20f), ImVec2(0.42f, 0.36f), ImVec2(-0.42f, 0.36f)}, true);
            line(-0.42f, -0.08f, 0.42f, -0.08f, t * 0.8f);
            break;
        case IC_MENU:
            for (int k = 0; k < 3; k++) line(-0.36f, -0.24f + k * 0.24f, 0.36f, -0.24f + k * 0.24f, t * 1.2f);
            break;
        case IC_SKIP:
            poly({ImVec2(-0.36f, -0.30f), ImVec2(-0.04f, 0), ImVec2(-0.36f, 0.30f)}, false);
            poly({ImVec2(0.04f, -0.30f), ImVec2(0.36f, 0), ImVec2(0.04f, 0.30f)}, false);
            break;
        case IC_ARROW:
            poly({ImVec2(-0.14f, -0.30f), ImVec2(0.18f, 0), ImVec2(-0.14f, 0.30f)}, false);
            break;
        case IC_CLOSE:
            line(-0.26f, -0.26f, 0.26f, 0.26f); line(0.26f, -0.26f, -0.26f, 0.26f);
            break;
        case IC_MIN:
            line(-0.28f, 0.10f, 0.28f, 0.10f);
            break;
        case IC_NEW:                                             // a star (a new beginning)
            star(dl, c, 0.40f * s, 0, 0, col, t);
            break;
        case IC_LOAD:                                            // an open folder
            poly({ImVec2(-0.42f, -0.32f), ImVec2(-0.14f, -0.32f), ImVec2(-0.06f, -0.22f), ImVec2(0.32f, -0.22f), ImVec2(0.32f, -0.06f)}, false);
            poly({ImVec2(-0.42f, -0.32f), ImVec2(-0.42f, 0.36f), ImVec2(0.30f, 0.36f), ImVec2(0.44f, -0.06f), ImVec2(-0.28f, -0.06f), ImVec2(-0.42f, 0.36f)}, false);
            break;
        case IC_RECORDS:                                         // a medal
            circ(0, 0.08f, 0.28f);
            star(dl, P(0, 0.09f), 0.16f * s, col);
            line(-0.14f, -0.44f, -0.06f, -0.20f); line(0.14f, -0.44f, 0.06f, -0.20f);
            break;
        case IC_GEAR: {
            int n = 8;
            for (int k = 0; k < n; k++) {
                float an = k * 2 * PI / n;
                ImVec2 u(std::cos(an), std::sin(an));
                dl->AddLine(c + u * (0.26f * s), c + u * (0.42f * s), col, t * 1.9f);
            }
            circ(0, 0, 0.27f); circ(0, 0, 0.10f);
            break;
        }
        case IC_QUIT:                                            // a door with an arrow
            poly({ImVec2(0.06f, -0.40f), ImVec2(-0.36f, -0.40f), ImVec2(-0.36f, 0.40f), ImVec2(0.06f, 0.40f)}, false);
            line(-0.12f, 0, 0.40f, 0);
            poly({ImVec2(0.24f, -0.16f), ImVec2(0.40f, 0), ImVec2(0.24f, 0.16f)}, false);
            break;
        case IC_POPOUT:                                          // a sheet lifted off another
            rect(-0.40f, -0.08f, 0.10f, 0.40f);
            dl->AddRectFilled(P(-0.10f, -0.40f), P(0.40f, 0.10f), col);
            break;
        case IC_STAR:
            star(dl, c, 0.42f * s, col);
            break;
        case IC_ALERT:                                           // a warning triangle
            poly({ImVec2(0, -0.40f), ImVec2(0.42f, 0.36f), ImVec2(-0.42f, 0.36f)}, true);
            line(0, -0.10f, 0, 0.12f);
            circF(0, 0.24f, 0.04f);
            break;
        case IC_CABLE:                                           // an envelope
            rect(-0.42f, -0.28f, 0.42f, 0.30f);
            poly({ImVec2(-0.42f, -0.28f), ImVec2(0, 0.06f), ImVec2(0.42f, -0.28f)}, false);
            break;
        case IC_CLOCK:
            circ(0, 0, 0.38f);
            line(0, 0, 0, -0.22f); line(0, 0, 0.16f, 0.08f);
            break;
        default: break;
    }
}

}  // namespace skin
