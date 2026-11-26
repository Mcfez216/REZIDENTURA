// gfx.cpp - see gfx.h.
#include "gfx.h"
#include "emblem.h"
#include "imgui_internal.h"
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <vector>

namespace gfx {

Fonts F;
TexApi texApi;

static std::vector<bool> inkStack;
void pushInk(bool dark) { inkStack.push_back(dark); }
void popInk() { if (!inkStack.empty()) inkStack.pop_back(); }
bool darkInk() { return !inkStack.empty() && inkStack.back(); }
static unsigned remap(unsigned hex) {
    if (inkStack.empty() || !inkStack.back()) return hex;
    switch (hex) {
        case C_INK: return 0xE9E0CB;
        case C_INK_FADED: case 0x6A6256: case 0x7A6E5A: case 0x5E564A: return 0xA59A85;
        case 0x7A7266: case 0x8A8070: return 0x857B6C;
        case C_RED: case 0xB01E1E: case C_SOVIET_RED: case 0xB3241B: case 0x9E2219: return 0xEC6A50;
        case C_PENCIL_RED: return 0xF07A60;
        case C_VIOLET: return 0xB3A6EE;
        case C_PENCIL_BLUE: case 0x26407A: case 0x1E3A6E: return 0x8FB4EE;
        case 0x2F6B3A: return 0x9CC47C;
        case 0xA0581A: return 0xE4A447;
        case C_RULE: return 0x6E5735;
        default: return hex;
    }
}
ImU32 rgb(unsigned hex, float a) { hex = remap(hex); return IM_COL32((hex >> 16) & 255, (hex >> 8) & 255, hex & 255, (int)(a * 255 + 0.5f)); }
ImVec4 rgb4(unsigned hex, float a) { hex = remap(hex); return ImVec4(((hex >> 16) & 255) / 255.f, ((hex >> 8) & 255) / 255.f, (hex & 255) / 255.f, a); }

static std::string fstr_(const char* f, int v) { char b[64]; std::snprintf(b, sizeof b, f, v); return b; }
static uint32_t hsh(uint32_t a, uint32_t b) { uint32_t h = a * 2654435761u ^ (b + 0x9E3779B9u + (a << 6) + (a >> 2)); h ^= h >> 15; h *= 2246822519u; h ^= h >> 13; return h; }
static uint32_t hstr(const std::string& s) { uint32_t h = 2166136261u; for (unsigned char c : s) { h ^= c; h *= 16777619u; } return h; }

// ============================================================================ fonts
static bool exists(const std::string& p) { ImFileHandle f = ImFileOpen(p.c_str(), "rb"); if (f) ImFileClose(f); return f != nullptr; }   // UTF-8 paths, on Windows too

void loadFonts(const std::string& dir, float scale, const std::string& fallback) {
    ImGuiIO& io = ImGui::GetIO();
    static const ImWchar TEXT[] = {0x0020, 0x00FF, 0x0100, 0x017F, 0x0400, 0x045F, 0x2010, 0x2026, 0x2039, 0x203A, 0x2116, 0x2116, 0x2190, 0x2193, 0x2605, 0x2606, 0};
    static const ImWchar DIGITS[] = {0x0020, 0x0039, 0};
    static const ImWchar CAPS[] = {0x0020, 0x005F, 0x00AB, 0x00AB, 0x00BB, 0x00BB, 0x0410, 0x042F, 0x0401, 0x0401, 0x2116, 0x2116, 0};
    auto path = [&](const char* f) { return dir + "/" + f; };
    // mono: the typewriter face, snapped to whole pixels so that small text stays sharp
    auto add = [&](const char* file, float px, const ImWchar* ranges, bool mono) -> ImFont* {
        std::string p = path(file);
        ImFont* font = nullptr;
        ImFontConfig cfg;
        cfg.OversampleH = mono ? 1 : 2; cfg.OversampleV = 1; cfg.PixelSnapH = mono;
        if (exists(p)) font = io.Fonts->AddFontFromFileTTF(p.c_str(), std::floor(px * scale), &cfg, ranges);
        else if (!fallback.empty() && exists(fallback)) font = io.Fonts->AddFontFromFileTTF(fallback.c_str(), std::floor(px * scale), &cfg, TEXT);
        if (!font) { ImFontConfig d; d.SizePixels = std::floor(13 * scale); return io.Fonts->AddFontDefault(&d); }
        return font;
    };
    // IBM Plex Mono: one family for Latin and Cyrillic alike, in matching weights
    F.body = add("IBMPlexMono-Regular.ttf", 16, TEXT, true);
    F.bold = add("IBMPlexMono-SemiBold.ttf", 16, TEXT, true);
    F.small = add("IBMPlexMono-Regular.ttf", 15, TEXT, true);
    F.tiny = add("IBMPlexMono-Medium.ttf", 13, TEXT, true);
    F.head = add("PT_Sans-Narrow-Web-Bold.ttf", 21, TEXT, false);
    F.stamp = add("PT_Sans-Narrow-Web-Bold.ttf", 19, TEXT, false);
    F.stampBig = add("PT_Sans-Narrow-Web-Bold.ttf", 34, CAPS, false);
    F.title = add("StalinistOne-Regular.ttf", 50, CAPS, false);
    F.cal = add("RussoOne-Regular.ttf", 54, DIGITS, false);
    F.hand = add("BadScript-Regular.ttf", 28, TEXT, false);
    F.handBig = add("BadScript-Regular.ttf", 38, TEXT, false);
    F.serif = add("PT_Serif-Web-Bold.ttf", 24, TEXT, false);
    F.ui = add("PT_Sans-Web-Regular.ttf", 16, TEXT, false);
    F.uiMed = add("PT_Sans-Web-Bold.ttf", 15, TEXT, false);
    F.uiBold = add("PT_Sans-Web-Bold.ttf", 16, TEXT, false);
    F.uiSmall = add("PT_Sans-Caption-Web-Regular.ttf", 13.5f, TEXT, false);
    F.uiItalic = add("PT_Sans-Web-Italic.ttf", 15, TEXT, false);
    F.btn = add("PT_Sans-Web-Bold.ttf", 15.5f, TEXT, false);
    F.uiCaps = add("Oswald-Medium.ttf", 13.5f, TEXT, false);
    F.label = add("Oswald-Regular.ttf", 17.5f, TEXT, false);
    F.bar = add("Oswald-Medium.ttf", 16.5f, TEXT, false);
    F.uiTitle = add("Oswald-Medium.ttf", 20, TEXT, false);
    F.figure = add("Oswald-Medium.ttf", 19, TEXT, false);
    F.uiBig = add("Oswald-Medium.ttf", 30, TEXT, false);
    io.FontDefault = F.ui;
}

// ============================================================================ textures
static std::map<std::string, ImTextureID> cache;

ImTextureID tex(const std::string& key, const std::function<Img()>& make) {
    auto it = cache.find(key);
    if (it != cache.end()) return it->second;
    if (!texApi.create) return (ImTextureID)0;
    ImTextureID t = texApi.create(make());
    cache[key] = t;
    return t;
}
void forgetTextures(const std::string& prefix) {
    for (auto it = cache.begin(); it != cache.end();) {
        if (it->first.compare(0, prefix.size(), prefix) == 0) { if (texApi.destroy) texApi.destroy(it->second); it = cache.erase(it); }
        else ++it;
    }
}
int textureCount() { return (int)cache.size(); }

// ============================================================================ primitives
void rotateSince(ImDrawList* dl, int start, ImVec2 c, float deg) {
    float a = deg * 3.14159265f / 180.f, s = std::sin(a), co = std::cos(a);
    for (int i = start; i < dl->VtxBuffer.Size; i++) {
        ImVec2& p = dl->VtxBuffer[i].pos;
        float dx = p.x - c.x, dy = p.y - c.y;
        p = ImVec2(c.x + dx * co - dy * s, c.y + dx * s + dy * co);
    }
}

void tile(ImDrawList* dl, ImTextureID t, ImVec2 a, ImVec2 b, float ts, ImU32 tint) {
    if (!t) return;
    dl->PushClipRect(a, b, true);
    for (float y = a.y; y < b.y; y += ts)
        for (float x = a.x; x < b.x; x += ts) dl->AddImage(t, ImVec2(x, y), ImVec2(x + ts, y + ts), ImVec2(0, 0), ImVec2(1, 1), tint);
    dl->PopClipRect();
}

void desk(ImDrawList* dl, ImVec2 a, ImVec2 b, float S) {
    dl->AddRectFilled(a, b, rgb(0x2A1E16));
    ImTextureID w = tex("tx:wood", [] { return art::wood(512, 9); });
    tile(dl, w, a, b, 512 * S, rgb(0x9A8A7A));
    // lamp light on the middle of the desk
    ImVec2 c = (a + b) * 0.5f;
    dl->AddRectFilledMultiColor(a, ImVec2(c.x, b.y), rgb(0, 0.35f), rgb(0, 0.0f), rgb(0, 0.0f), rgb(0, 0.35f));
    dl->AddRectFilledMultiColor(ImVec2(c.x, a.y), b, rgb(0, 0.0f), rgb(0, 0.35f), rgb(0, 0.35f), rgb(0, 0.0f));
}

void sheet(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 colour, float S, bool shadow) {
    if (shadow) {
        dl->AddRectFilled(a + ImVec2(3 * S, 4 * S), b + ImVec2(5 * S, 6 * S), rgb(0, 0.22f), 2 * S);
        dl->AddRectFilled(a + ImVec2(1 * S, 2 * S), b + ImVec2(2 * S, 3 * S), rgb(0, 0.25f), 1 * S);
    }
    dl->AddRectFilled(a, b, colour);
    ImTextureID g = tex("tx:grain", [] { return art::paperGrain(256, 5); });
    tile(dl, g, a, b, 256 * S, IM_COL32(255, 255, 255, 80));
    float e = 16 * S;
    ImU32 dark = rgb(0x6B5A3A, 0.16f), none = rgb(0x6B5A3A, 0);
    dl->AddRectFilledMultiColor(a, ImVec2(b.x, a.y + e), dark, dark, none, none);
    dl->AddRectFilledMultiColor(ImVec2(a.x, b.y - e), b, none, none, dark, dark);
    dl->AddRectFilledMultiColor(a, ImVec2(a.x + e, b.y), dark, none, none, dark);
    dl->AddRectFilledMultiColor(ImVec2(b.x - e, a.y), b, none, dark, dark, none);
    dl->AddRect(a, b, rgb(0x7A6A4A, 0.35f));
}

static void wearOver(ImDrawList* dl, ImVec2 a, ImVec2 b, uint32_t seed) {
    ImTextureID w = tex("tx:wear", [] { return art::wear(256, 4); });
    float u = (seed % 97) / 200.f, v = ((seed >> 8) % 97) / 200.f;
    float du = std::min(0.5f, (b.x - a.x) / 300.f), dv = std::min(0.5f, (b.y - a.y) / 300.f);
    dl->AddImage(w, a, b, ImVec2(u, v), ImVec2(u + du, v + dv), rgb(C_PAPER, 0.85f));
}

void stamp(ImDrawList* dl, ImVec2 c, const std::string& text, ImU32 ink, float deg, ImFont* f, float size, int style, const std::string& sub) {
    if (!f) f = F.stamp;
    if (size <= 0) size = f->FontSize;
    ImVec2 ts = f->CalcTextSizeA(size, FLT_MAX, 0, text.c_str());
    float ss = size * 0.52f;
    ImVec2 subSz = sub.empty() ? ImVec2(0, 0) : F.stamp->CalcTextSizeA(ss, FLT_MAX, 0, sub.c_str());
    float padX = size * 0.38f, padY = size * 0.14f;
    float w = std::max(ts.x, subSz.x) + 2 * padX, h = ts.y + subSz.y + 2 * padY;
    ImVec2 a = c - ImVec2(w / 2, h / 2), b = c + ImVec2(w / 2, h / 2);
    int start = dl->VtxBuffer.Size;
    float th = std::max(1.5f, size * 0.075f);
    if (style != 2) dl->AddRect(a, b, ink, size * 0.10f, 0, th);
    if (style == 1) dl->AddRect(a + ImVec2(th * 2, th * 2), b - ImVec2(th * 2, th * 2), ink, size * 0.06f, 0, th * 0.6f);
    dl->AddText(f, size, ImVec2(c.x - ts.x / 2, a.y + padY), ink, text.c_str());
    if (!sub.empty()) dl->AddText(F.stamp, ss, ImVec2(c.x - subSz.x / 2, a.y + padY + ts.y), ink, sub.c_str());
    wearOver(dl, a - ImVec2(4, 4), b + ImVec2(4, 4), hstr(text));
    rotateSince(dl, start, c, deg);
}

void seal(ImDrawList* dl, ImVec2 c, float R, const std::string& ring, const std::string& middle, ImU32 ink, float deg) {
    int start = dl->VtxBuffer.Size;
    float th = std::max(1.5f, R * 0.035f);
    dl->AddCircle(c, R, ink, 64, th);
    dl->AddCircle(c, R * 0.70f, ink, 64, th * 0.8f);
    ImFont* f = F.stamp;
    // fit the ring text round the band between the two circles
    float rt = R * 0.84f, size = R * 0.24f;
    float total = 0;
    std::vector<unsigned int> chars;
    for (const char* s = ring.c_str(); *s;) { unsigned int ch; s += ImTextCharFromUtf8(&ch, s, nullptr); chars.push_back(ch); total += f->GetCharAdvance((ImWchar)ch) * size / f->FontSize; }
    float circ = 2 * 3.14159265f * rt * 0.97f;
    if (total > circ) { size *= circ / total; total = circ; }
    float acc = 0, a0 = -3.14159265f / 2 - (total / rt) / 2;
    for (unsigned int ch : chars) {
        float adv = f->GetCharAdvance((ImWchar)ch) * size / f->FontSize;
        float ang = a0 + (acc + adv / 2) / rt;
        ImVec2 p = c + ImVec2(std::cos(ang), std::sin(ang)) * rt;
        int s0 = dl->VtxBuffer.Size;
        f->RenderChar(dl, size, ImVec2(p.x - adv / 2, p.y - size * 0.55f), ink, (ImWchar)ch);
        rotateSince(dl, s0, p, ang * 180 / 3.14159265f + 90);
        acc += adv;
    }
    if (!middle.empty()) {
        ImVec2 ts = f->CalcTextSizeA(R * 0.22f, FLT_MAX, 0, middle.c_str());
        dl->AddText(f, R * 0.22f, ImVec2(c.x - ts.x / 2, c.y + R * 0.12f), ink, middle.c_str());
        star(dl, c - ImVec2(0, R * 0.16f), R * 0.22f, ink);
    } else star(dl, c, R * 0.32f, ink);
    wearOver(dl, c - ImVec2(R, R), c + ImVec2(R, R), hstr(ring));
    rotateSince(dl, start, c, deg);
}

void pencil(ImDrawList* dl, ImVec2 pos, const std::string& text, ImU32 ink, float deg, ImFont* f, float size) {
    if (!f) f = F.hand;
    if (size <= 0) size = f->FontSize;
    int start = dl->VtxBuffer.Size;
    dl->AddText(f, size, pos, ink, text.c_str());
    ImVec2 ts = f->CalcTextSizeA(size, FLT_MAX, 0, text.c_str());
    rotateSince(dl, start, pos + ts * 0.5f, deg);
}

void paperclip(ImDrawList* dl, ImVec2 p, float S, float deg) {
    int start = dl->VtxBuffer.Size;
    ImU32 steel = rgb(0x8C929A), hi = rgb(0xE8ECF0, 0.9f), sh = rgb(0, 0.25f);
    float w = 1.7f * S;
    dl->AddRect(p + ImVec2(1.5f * S, 2 * S), p + ImVec2(13.5f * S, 46 * S), sh, 6 * S, 0, w);
    dl->AddRect(p, p + ImVec2(12 * S, 44 * S), steel, 6 * S, 0, w);
    dl->AddRect(p + ImVec2(2.5f * S, 7 * S), p + ImVec2(9.5f * S, 36 * S), steel, 3.5f * S, 0, w);
    dl->AddLine(p + ImVec2(1 * S, 10 * S), p + ImVec2(1 * S, 30 * S), hi, 0.8f * S);
    rotateSince(dl, start, p + ImVec2(6 * S, 22 * S), deg);
}

void pushpin(ImDrawList* dl, ImVec2 p, float S, ImU32 head) {
    dl->AddCircleFilled(p + ImVec2(2 * S, 3 * S), 6 * S, rgb(0, 0.3f));
    dl->AddCircleFilled(p, 6 * S, head);
    dl->AddCircleFilled(p - ImVec2(2 * S, 2 * S), 2 * S, rgb(0xFFFFFF, 0.6f));
}

void photo(ImDrawList* dl, ImTextureID t, ImVec2 tl, ImVec2 size, float deg, float S, bool clip) {
    int start = dl->VtxBuffer.Size;
    float bw = 6 * S;
    ImVec2 a = tl, b = tl + size, c = (a + b) * 0.5f;
    dl->AddRectFilled(a - ImVec2(bw, bw) + ImVec2(3 * S, 4 * S), b + ImVec2(bw, bw) + ImVec2(3 * S, 4 * S), rgb(0, 0.30f));
    dl->AddRectFilled(a - ImVec2(bw, bw), b + ImVec2(bw, bw), rgb(0xF6F2E8));
    if (t) dl->AddImage(t, a, b); else dl->AddRectFilled(a, b, rgb(0x777777));
    dl->AddRect(a - ImVec2(bw, bw), b + ImVec2(bw, bw), rgb(0xB8B0A0));
    rotateSince(dl, start, c, deg);
    if (clip) {
        float r = deg * 3.14159265f / 180;
        ImVec2 off(-size.x * 0.22f, -size.y * 0.5f - bw - 10 * S);
        ImVec2 p = c + ImVec2(off.x * std::cos(r) - off.y * std::sin(r), off.x * std::sin(r) + off.y * std::cos(r));
        paperclip(dl, p - ImVec2(6 * S, 4 * S), S, deg - 4);
    }
}

void star(ImDrawList* dl, ImVec2 c, float r, ImU32 fill, float deg, ImU32 edge, float edgeW) {
    ImVec2 pts[10];
    float a0 = -3.14159265f / 2 + deg * 3.14159265f / 180;
    for (int i = 0; i < 10; i++) {
        float rr = (i % 2) ? r * 0.382f : r, a = a0 + i * 3.14159265f / 5;
        pts[i] = c + ImVec2(std::cos(a), std::sin(a)) * rr;
    }
    for (int i = 0; i < 10; i++) dl->AddTriangleFilled(c, pts[i], pts[(i + 1) % 10], fill);
    if (edgeW > 0) dl->AddPolyline(pts, 10, edge, ImDrawFlags_Closed, edgeW);
}

// ---------------------------------------------------------------------------- the emblem
/** The hammer and sickle as a white mask at one of several sizes, so that it is never shrunk more than twofold on screen. */
static ImTextureID emblemTex(float px) {
    static std::vector<std::vector<uint8_t>> levels;
    if (levels.empty()) {
        std::vector<uint8_t> m((size_t)EMBLEM_SIZE * EMBLEM_SIZE, 0);
        size_t k = 0;
        for (size_t i = 0; i + 1 < sizeof(EMBLEM_RLE); i += 2)
            for (int n = 0; n < EMBLEM_RLE[i + 1] && k < m.size(); n++) m[k++] = EMBLEM_RLE[i];
        levels.push_back(m);
        for (int sz = EMBLEM_SIZE / 2; sz >= 32; sz /= 2) {
            const std::vector<uint8_t>& u = levels.back();
            int us = sz * 2;
            std::vector<uint8_t> d((size_t)sz * sz);
            for (int y = 0; y < sz; y++)
                for (int x = 0; x < sz; x++)
                    d[(size_t)y * sz + x] = (uint8_t)((u[(size_t)(2 * y) * us + 2 * x] + u[(size_t)(2 * y) * us + 2 * x + 1] + u[(size_t)(2 * y + 1) * us + 2 * x] + u[(size_t)(2 * y + 1) * us + 2 * x + 1] + 2) / 4);
            levels.push_back(d);
        }
    }
    int li = 0, sz = EMBLEM_SIZE;
    while (li + 1 < (int)levels.size() && sz / 2 >= px) { li++; sz /= 2; }
    return tex(fstr_("tx:emblem:%d", sz), [li, sz] {
        Img im;
        im.w = im.h = sz;
        im.px.resize((size_t)sz * sz);
        for (size_t i = 0; i < im.px.size(); i++) im.px[i] = ((uint32_t)levels[li][i] << 24) | 0x00FFFFFFu;
        return im;
    });
}

void hammerSickle(ImDrawList* dl, ImVec2 c, float size, ImU32 col) {
    ImTextureID t = emblemTex(size);
    if (!t) return;
    ImVec2 h(size / 2, size / 2);
    dl->AddImage(t, c - h, c + h, ImVec2(0, 0), ImVec2(1, 1), col);
}

void carSketch(ImDrawList* dl, ImVec2 c, float w, ImU32 body, int era, uint32_t seed) {
    float h = w * 0.30f, x0 = c.x - w / 2, x1 = c.x + w / 2, base = c.y + h * 0.35f;
    bool estate = (seed % 5) == 0;
    float roofF = era == 2 ? 0.30f : 0.34f, roofB = estate ? 0.94f : (era == 0 ? 0.70f : 0.74f);
    float sill = base - h * 0.55f, roof = base - h * (era == 0 ? 1.05f : 1.0f);
    ImU32 ink = rgb(0x2A2622, 0.85f), glass = rgb(0xC9D3D8, 0.9f);
    // the body: bonnet, cabin, boot
    ImVec2 bodyP[8] = {ImVec2(x0 + w * 0.02f, base - h * 0.10f), ImVec2(x0 + w * 0.03f, sill + h * 0.05f), ImVec2(x0 + w * (roofF - 0.10f), sill),
                       ImVec2(x0 + w * (roofF - 0.02f), roof + h * 0.05f), ImVec2(x0 + w * roofB, roof + h * 0.02f),
                       ImVec2(x0 + w * (roofB + (estate ? 0.02f : 0.10f)), sill), ImVec2(x1 - w * 0.02f, sill + h * 0.08f), ImVec2(x1 - w * 0.02f, base - h * 0.10f)};
    dl->AddRectFilled(ImVec2(x0 + w * 0.01f, sill), ImVec2(x1 - w * 0.01f, base), body, h * (era == 0 ? 0.18f : 0.08f));
    ImVec2 cab[4] = {bodyP[2], bodyP[3], bodyP[4], bodyP[5]};
    dl->AddConvexPolyFilled(cab, 4, body);
    // windows
    float wy0 = roof + h * 0.12f, wy1 = sill - h * 0.02f;
    float mid = x0 + w * (roofF + roofB) / 2;
    ImVec2 wf[4] = {ImVec2(x0 + w * (roofF - 0.06f), wy1), ImVec2(x0 + w * (roofF + 0.01f), wy0), ImVec2(mid - w * 0.015f, wy0), ImVec2(mid - w * 0.015f, wy1)};
    ImVec2 wb[4] = {ImVec2(mid + w * 0.015f, wy1), ImVec2(mid + w * 0.015f, wy0), ImVec2(x0 + w * (roofB - 0.02f), wy0), ImVec2(x0 + w * (roofB + (estate ? 0.0f : 0.06f)), wy1)};
    dl->AddConvexPolyFilled(wf, 4, glass);
    dl->AddConvexPolyFilled(wb, 4, glass);
    // outline, door seams, chrome, lamps
    dl->AddPolyline(bodyP, 8, ink, ImDrawFlags_Closed, 1.3f);
    dl->AddLine(ImVec2(mid, sill), ImVec2(mid, base - h * 0.10f), ink, 1.0f);
    dl->AddLine(ImVec2(x0 + w * (roofF - 0.06f), sill + h * 0.02f), ImVec2(x0 + w * (roofF - 0.06f), base - h * 0.12f), ink, 1.0f);
    dl->AddLine(ImVec2(x0 + w * 0.01f, base - h * 0.16f), ImVec2(x1 - w * 0.01f, base - h * 0.16f), rgb(0xE8ECEE, 0.8f), 1.4f);
    dl->AddRectFilled(ImVec2(x1 - w * 0.04f, sill + h * 0.10f), ImVec2(x1 - w * 0.015f, sill + h * 0.22f), rgb(0xF2E6B0));
    dl->AddRectFilled(ImVec2(x0 + w * 0.015f, sill + h * 0.10f), ImVec2(x0 + w * 0.035f, sill + h * 0.22f), rgb(0xB0302A));
    // wheels
    for (float wx : {x0 + w * 0.20f, x1 - w * 0.20f}) {
        dl->AddCircleFilled(ImVec2(wx, base), h * 0.30f, rgb(0x1E1C1A));
        dl->AddCircleFilled(ImVec2(wx, base), h * 0.13f, rgb(0xA9AEB2));
        dl->AddCircle(ImVec2(wx, base), h * 0.30f, ink, 0, 1.0f);
    }
    // the road
    dl->AddLine(ImVec2(x0 - w * 0.05f, base + h * 0.30f), ImVec2(x1 + w * 0.05f, base + h * 0.30f), rgb(0x2A2622, 0.4f), 1.0f);
}

void typeLine(ImDrawList* dl, ImFont* f, float size, ImVec2 pos, ImU32 col, const char* b, const char* e, uint32_t seed) {
    float x = std::floor(pos.x + 0.5f), y = std::floor(pos.y + 0.5f), k = size / f->FontSize;
    ImVec4 cv = ImGui::ColorConvertU32ToFloat4(col);
    int i = 0;
    for (const char* s = b; s < e;) {
        unsigned int ch;
        int n = ImTextCharFromUtf8(&ch, s, e);
        s += n ? n : 1;
        if (ch == '\n') break;
        uint32_t h = hsh(seed, (uint32_t)i++);
        float a = cv.w * (0.90f + ((h >> 3) & 15) / 15.f * 0.10f);          // the ribbon prints a shade lighter or darker
        if (ch != ' ') f->RenderChar(dl, size, ImVec2(x, y), ImGui::ColorConvertFloat4ToU32(ImVec4(cv.x, cv.y, cv.z, a)), (ImWchar)ch);
        x += f->GetCharAdvance((ImWchar)ch) * k;
    }
}

// ============================================================================ things in the safe
void filmCassette(ImDrawList* dl, ImVec2 c, float s, bool exposed) {
    ImU32 body = rgb(exposed ? 0x5A5F66 : 0x9AA1A8), hi = rgb(0xE6EAEE, 0.8f), dk = rgb(0x30343A);
    float r = s * 0.30f;
    dl->AddCircleFilled(c + ImVec2(-s * 0.42f, 0), r, body);
    dl->AddCircleFilled(c + ImVec2(s * 0.42f, 0), r, body);
    dl->AddRectFilled(c - ImVec2(s * 0.42f, r * 0.7f), c + ImVec2(s * 0.42f, r * 0.7f), body);
    dl->AddCircle(c + ImVec2(-s * 0.42f, 0), r, dk, 0, 1.2f);
    dl->AddCircle(c + ImVec2(s * 0.42f, 0), r, dk, 0, 1.2f);
    dl->AddLine(c + ImVec2(-s * 0.55f, -r * 0.5f), c + ImVec2(-s * 0.30f, -r * 0.7f), hi, 1.5f);
    if (exposed) dl->AddRectFilled(c - ImVec2(s * 0.10f, r * 0.7f), c + ImVec2(s * 0.10f, r * 0.7f), rgb(0xC9A227));
}
void minox(ImDrawList* dl, ImVec2 c, float s) {
    ImVec2 a = c - ImVec2(s * 0.75f, s * 0.20f), b = c + ImVec2(s * 0.75f, s * 0.20f);
    dl->AddRectFilled(a, b, rgb(0xA9B0B6), s * 0.15f);
    dl->AddRectFilled(a + ImVec2(s * 0.10f, s * 0.08f), ImVec2(a.x + s * 0.55f, b.y - s * 0.08f), rgb(0x22252A), s * 0.05f);
    dl->AddCircleFilled(ImVec2(a.x + s * 0.30f, c.y), s * 0.06f, rgb(0x7A8590));
    dl->AddLine(a + ImVec2(s * 0.2f, s * 0.04f), ImVec2(b.x - s * 0.2f, a.y + s * 0.04f), rgb(0xFFFFFF, 0.7f), 1.2f);
    dl->AddRect(a, b, rgb(0x50565C), s * 0.15f);
}
void tapeReel(ImDrawList* dl, ImVec2 c, float s) {
    dl->AddCircleFilled(c + ImVec2(2, 3), s * 0.48f, rgb(0, 0.25f));
    dl->AddCircleFilled(c, s * 0.48f, rgb(0x3D2B1E));
    dl->AddCircleFilled(c, s * 0.46f, rgb(0x5A4030));
    dl->AddCircleFilled(c, s * 0.24f, rgb(0xB9BEC3));
    for (int i = 0; i < 3; i++) {
        float a = i * 2.094f - 1.5708f;
        dl->AddCircleFilled(c + ImVec2(std::cos(a), std::sin(a)) * (s * 0.13f), s * 0.06f, rgb(0x2A1E16));
    }
    dl->AddCircleFilled(c, s * 0.04f, rgb(0x2A1E16));
    dl->AddCircle(c, s * 0.48f, rgb(0x1E1510), 0, 1.2f);
}
void typedSheet(ImDrawList* dl, ImVec2 c, float s) {
    ImVec2 a = c - ImVec2(s * 0.34f, s * 0.45f), b = c + ImVec2(s * 0.34f, s * 0.45f);
    dl->AddRectFilled(a + ImVec2(2, 2), b + ImVec2(2, 2), rgb(0, 0.25f));
    dl->AddRectFilled(a, b, rgb(C_CARD));
    for (int i = 0; i < 6; i++) dl->AddLine(a + ImVec2(s * 0.08f, s * (0.14f + i * 0.12f)), ImVec2(b.x - s * (i == 5 ? 0.30f : 0.08f), a.y + s * (0.14f + i * 0.12f)), rgb(C_INK, 0.55f), 1);
}
void cash(ImDrawList* dl, ImVec2 c, float s, const char* cur) {
    ImVec2 a = c - ImVec2(s * 0.62f, s * 0.30f), b = c + ImVec2(s * 0.62f, s * 0.30f);
    for (int k = 3; k >= 0; k--) dl->AddRectFilled(a + ImVec2(k * 2.f, k * -2.f), b + ImVec2(k * 2.f, k * -2.f), rgb(k ? 0x8FA383 : 0xA9BC98));
    dl->AddRect(a, b, rgb(0x4E6145), 0, 0, 1);
    dl->AddRectFilled(ImVec2(c.x - s * 0.12f, a.y), ImVec2(c.x + s * 0.12f, b.y), rgb(0xE9E1C9));
    ImVec2 ts = F.stamp->CalcTextSizeA(s * 0.36f, FLT_MAX, 0, cur);
    dl->AddText(F.stamp, s * 0.36f, c - ts * 0.5f, rgb(0x2F3D29), cur);
}
void padBook(ImDrawList* dl, ImVec2 c, float s) {
    ImVec2 a = c - ImVec2(s * 0.30f, s * 0.40f), b = c + ImVec2(s * 0.30f, s * 0.40f);
    dl->AddRectFilled(a + ImVec2(2, 2), b + ImVec2(2, 2), rgb(0, 0.25f));
    dl->AddRectFilled(a, b, rgb(0xF2EBD3));
    dl->AddRectFilled(a, ImVec2(b.x, a.y + s * 0.10f), rgb(C_RED));
    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 3; j++) dl->AddRectFilled(a + ImVec2(s * (0.06f + j * 0.18f), s * (0.18f + i * 0.11f)), a + ImVec2(s * (0.18f + j * 0.18f), s * (0.23f + i * 0.11f)), rgb(C_INK, 0.6f));
}
void bolt(ImDrawList* dl, ImVec2 c, float s) {
    dl->AddRectFilled(c - ImVec2(s * 0.10f, s * 0.10f), c + ImVec2(s * 0.10f, s * 0.50f), rgb(0x8E949A));
    for (int i = 0; i < 6; i++) dl->AddLine(c + ImVec2(-s * 0.10f, s * (0.0f + i * 0.08f)), c + ImVec2(s * 0.10f, s * (0.04f + i * 0.08f)), rgb(0x5C6166), 1);
    ImVec2 hex_[6];
    for (int i = 0; i < 6; i++) { float a = i * 1.0472f; hex_[i] = c + ImVec2(std::cos(a) * s * 0.24f, std::sin(a) * s * 0.14f - s * 0.18f); }
    dl->AddConvexPolyFilled(hex_, 6, rgb(0xA7ADB3));
    dl->AddPolyline(hex_, 6, rgb(0x5C6166), ImDrawFlags_Closed, 1);
}
void bug(ImDrawList* dl, ImVec2 c, float s) {
    dl->AddRectFilled(c - ImVec2(s * 0.30f, s * 0.18f), c + ImVec2(s * 0.30f, s * 0.18f), rgb(0x1C1C1C), s * 0.04f);
    dl->AddRectFilled(c - ImVec2(s * 0.22f, s * 0.10f), c + ImVec2(s * 0.00f, s * 0.10f), rgb(0x3A3A3A));
    dl->AddBezierCubic(c + ImVec2(s * 0.30f, -s * 0.05f), c + ImVec2(s * 0.5f, -s * 0.4f), c + ImVec2(s * 0.2f, -s * 0.5f), c + ImVec2(s * 0.45f, -s * 0.62f), rgb(0x8A6A2A), 1.4f);
}
void parcel(ImDrawList* dl, ImVec2 c, float s) {
    ImVec2 a = c - ImVec2(s * 0.55f, s * 0.35f), b = c + ImVec2(s * 0.55f, s * 0.35f);
    dl->AddRectFilled(a + ImVec2(2, 3), b + ImVec2(2, 3), rgb(0, 0.25f));
    dl->AddRectFilled(a, b, rgb(0xA67C4E));
    dl->AddLine(ImVec2(c.x, a.y), ImVec2(c.x, b.y), rgb(0xE8DCC0), 1.5f);
    dl->AddLine(ImVec2(a.x, c.y), ImVec2(b.x, c.y), rgb(0xE8DCC0), 1.5f);
    dl->AddCircleFilled(c, s * 0.12f, rgb(0x9E1B1B));
    dl->AddCircle(c, s * 0.12f, rgb(0x5E0E0E), 0, 1);
}
void medal(ImDrawList* dl, ImVec2 c, float s, bool banner) {
    // a ribbon block, then the order: a red star on silver rays (Red Star), or a red banner over a gold wreath (Red Banner)
    ImVec2 ra = c - ImVec2(s * 0.30f, s * 0.95f), rb = c + ImVec2(s * 0.30f, -s * 0.55f);
    dl->AddRectFilled(ra, rb, rgb(banner ? 0xB3171B : 0x8E1F2A));
    dl->AddRectFilled(ImVec2(c.x - s * 0.05f, ra.y), ImVec2(c.x + s * 0.05f, rb.y), rgb(banner ? 0xF4F0E6 : 0xA0A0A0));
    if (!banner) {
        for (int i = 0; i < 16; i++) { float a = i * 0.3927f; dl->AddLine(c, c + ImVec2(std::cos(a), std::sin(a)) * (s * 0.48f), rgb(0xC9CDD2), s * 0.08f); }
        star(dl, c, s * 0.42f, rgb(0xC0201E), 0, rgb(0x6B0E0E), 1);
    } else {
        for (int i = 0; i < 12; i++) { float a = 2.2f + i * 0.33f; dl->AddCircleFilled(c + ImVec2(std::cos(a), std::sin(a)) * (s * 0.40f), s * 0.07f, rgb(C_GOLD)); }
        dl->AddQuadFilled(c + ImVec2(-s * 0.25f, -s * 0.30f), c + ImVec2(s * 0.30f, -s * 0.35f), c + ImVec2(s * 0.28f, -s * 0.05f), c + ImVec2(-s * 0.25f, 0), rgb(0xC0201E));
        star(dl, c + ImVec2(0, s * 0.14f), s * 0.22f, rgb(0xC0201E), 0, rgb(C_GOLD), 1);
    }
}

// ============================================================================ words
const char* RU_MONTH_GEN[13] = {"", "ЯНВАРЯ", "ФЕВРАЛЯ", "МАРТА", "АПРЕЛЯ", "МАЯ", "ИЮНЯ", "ИЮЛЯ", "АВГУСТА", "СЕНТЯБРЯ", "ОКТЯБРЯ", "НОЯБРЯ", "ДЕКАБРЯ"};
const char* RU_WEEKDAY[7] = {"ПОНЕДЕЛЬНИК", "ВТОРНИК", "СРЕДА", "ЧЕТВЕРГ", "ПЯТНИЦА", "СУББОТА", "ВОСКРЕСЕНЬЕ"};

static const std::map<std::string, std::string>& words() {
    static const std::map<std::string, std::string> m = {
        {"GRANIT", "ГРАНИТ"}, {"KORAL", "КОРАЛЛ"}, {"LESNIK", "ЛЕСНИК"}, {"TENOR", "ТЕНОР"}, {"SOKOL", "СОКОЛ"}, {"DROZD", "ДРОЗД"}, {"BEREG", "БЕРЕГ"},
        {"YAKOR", "ЯКОРЬ"}, {"KVARTS", "КВАРЦ"}, {"LIRA", "ЛИРА"}, {"MAYAK", "МАЯК"}, {"TURIST", "ТУРИСТ"}, {"KEDR", "КЕДР"}, {"GORN", "ГОРН"},
        {"VESNA", "ВЕСНА"}, {"AKKORD", "АККОРД"}, {"ROMB", "РОМБ"}, {"ZIMA", "ЗИМА"}, {"PILOT", "ПИЛОТ"}, {"FAKEL", "ФАКЕЛ"}, {"BARS", "БАРС"},
        {"ILMEN", "ИЛЬМЕНЬ"}, {"ONEGA", "ОНЕГА"}, {"RUBIN", "РУБИН"}, {"SATURN", "САТУРН"}, {"TOPAZ", "ТОПАЗ"}, {"VYMPEL", "ВЫМПЕЛ"}, {"ZEFIR", "ЗЕФИР"},
        {"ALMAZ", "АЛМАЗ"}, {"BURAN", "БУРАН"}, {"VOLNA", "ВОЛНА"}, {"GRANAT", "ГРАНАТ"}, {"DUNAI", "ДУНАЙ"}, {"YENISEI", "ЕНИСЕЙ"}, {"ZARYA", "ЗАРЯ"},
        {"IRTYSH", "ИРТЫШ"}, {"KASKAD", "КАСКАД"}, {"LADOGA", "ЛАДОГА"}, {"MERKURI", "МЕРКУРИЙ"}, {"NEVA", "НЕВА"}, {"OKEAN", "ОКЕАН"}, {"PAMIR", "ПАМИР"},
        {"RADUGA", "РАДУГА"}, {"SELENA", "СЕЛЕНА"}, {"TAIGA", "ТАЙГА"}, {"URAL", "УРАЛ"}, {"FREGAT", "ФРЕГАТ"}, {"KHRUSTAL", "ХРУСТАЛЬ"}, {"TSIKLON", "ЦИКЛОН"},
        {"SHTORM", "ШТОРМ"}, {"ELBRUS", "ЭЛЬБРУС"}, {"YUPITER", "ЮПИТЕР"}, {"YANTAR", "ЯНТАРЬ"}, {"ARKTUR", "АРКТУР"}, {"BARKHAN", "БАРХАН"}, {"VEKTOR", "ВЕКТОР"},
        {"GEIZER", "ГЕЙЗЕР"}, {"DELTA", "ДЕЛЬТА"}, {"YEGER", "ЕГЕРЬ"}, {"ZHEMCHUG", "ЖЕМЧУГ"}, {"AGAT", "АГАТ"}, {"BERKUT", "БЕРКУТ"}, {"VITYAZ", "ВИТЯЗЬ"},
        {"GRAD", "ГРАД"}, {"DESNA", "ДЕСНА"}, {"KOMETA", "КОМЕТА"}, {"LOTOS", "ЛОТОС"}, {"MIRAZH", "МИРАЖ"}, {"NARZAN", "НАРЗАН"}, {"ORION", "ОРИОН"},
        {"PLAMYA", "ПЛАМЯ"}, {"RITM", "РИТМ"}, {"SIGNAL", "СИГНАЛ"}, {"TAIFUN", "ТАЙФУН"}, {"UTYOS", "УТЁС"}, {"FOTON", "ФОТОН"}, {"KHIBINY", "ХИБИНЫ"},
        {"CHAIKA", "ЧАЙКА"}, {"SHKVAL", "ШКВАЛ"}, {"ETALON", "ЭТАЛОН"},
        {"Viktor", "Виктор"}, {"Nikolai", "Николай"}, {"Yuri", "Юрий"}, {"Anatoly", "Анатолий"}, {"Gennady", "Геннадий"}, {"Boris", "Борис"}, {"Valery", "Валерий"},
        {"Leonid", "Леонид"}, {"Igor", "Игорь"}, {"Sergei", "Сергей"}, {"Aleksandr", "Александр"}, {"Yevgeny", "Евгений"}, {"Stanislav", "Станислав"}, {"Vadim", "Вадим"},
        {"Konstantin", "Константин"}, {"Pavel", "Павел"}, {"Mikhail", "Михаил"}, {"Arkady", "Аркадий"}, {"Lev", "Лев"}, {"Fyodor", "Фёдор"},
        {"Orlov", "Орлов"}, {"Belov", "Белов"}, {"Titov", "Титов"}, {"Zaitsev", "Зайцев"}, {"Morozov", "Морозов"}, {"Volkov", "Волков"}, {"Lebedev", "Лебедев"},
        {"Kozlov", "Козлов"}, {"Novikov", "Новиков"}, {"Fedorov", "Фёдоров"}, {"Golubev", "Голубев"}, {"Vinogradov", "Виноградов"}, {"Bogdanov", "Богданов"},
        {"Vorobyov", "Воробьёв"}, {"Sorokin", "Сорокин"}, {"Baranov", "Баранов"}, {"Kulikov", "Куликов"}, {"Maksimov", "Максимов"}, {"Gusev", "Гусев"},
        {"Kiselyov", "Киселёв"}, {"Ilyin", "Ильин"}, {"Gerasimov", "Герасимов"}, {"Tarasov", "Тарасов"}, {"Danilov", "Данилов"}, {"Zhuravlev", "Журавлёв"},
        {"Savelyev", "Савельев"}, {"Rybakov", "Рыбаков"}, {"Nikitin", "Никитин"}, {"Sobolev", "Соболев"}, {"Antonov", "Антонов"}, {"Filippov", "Филиппов"},
        {"Yermakov", "Ермаков"}, {"Shubin", "Шубин"}, {"Dyomin", "Дёмин"}, {"Loginov", "Логинов"}, {"Krylov", "Крылов"}, {"Sokolov", "Соколов"},
        {"Capt.", "кап."}, {"Maj.", "майор"}, {"Lt-Col.", "подп."}, {"Col.", "полк."}, {"Maj-Gen.", "ген.-м."}};
    return m;
}

/** Latin letters to Cyrillic, good enough for a surname typed in by the player. */
static std::string translit(const std::string& w) {
    static const std::pair<const char*, const char*> DI[] = {{"shch", "щ"}, {"zh", "ж"}, {"kh", "х"}, {"ts", "ц"}, {"ch", "ч"}, {"sh", "ш"}, {"yu", "ю"}, {"ya", "я"}, {"yo", "ё"}, {"ye", "е"}};
    static const char* ONE[26] = {"а", "б", "к", "д", "е", "ф", "г", "х", "и", "дж", "к", "л", "м", "н", "о", "п", "к", "р", "с", "т", "у", "в", "в", "кс", "ы", "з"};
    std::string out;
    std::string lw;
    for (char c : w) lw += (char)tolower((unsigned char)c);
    for (size_t i = 0; i < lw.size();) {
        bool upper = i < w.size() && isupper((unsigned char)w[i]);
        std::string piece;
        size_t used = 1;
        for (auto& d : DI) { size_t n = std::strlen(d.first); if (lw.compare(i, n, d.first) == 0) { piece = d.second; used = n; break; } }
        if (piece.empty()) {
            char c = lw[i];
            if (c == 'y') {
                bool afterVowel = i > 0 && std::strchr("aeiou", lw[i - 1]);
                bool atEnd = i + 1 >= lw.size() || !isalpha((unsigned char)lw[i + 1]);
                piece = (afterVowel && atEnd) || (afterVowel && !std::strchr("aeiou", lw[i + 1])) ? "й" : "ы";
            } else if (c >= 'a' && c <= 'z') piece = ONE[c - 'a'];
            else piece = std::string(1, w[i]);
        }
        if (upper && piece.size() >= 2 && (unsigned char)piece[0] == 0xD0) {
            // the capital of a Cyrillic lower-case letter: а-п are D0 B0-BF -> D0 90-9F; р-я are D1 80-8F -> D0 A0-AF; ё D1 91 -> D0 81
            unsigned char b = (unsigned char)piece[1];
            if (b >= 0xB0 && b <= 0xBF) piece[1] = (char)(b - 0x20);
        } else if (upper && piece.size() >= 2 && (unsigned char)piece[0] == 0xD1) {
            unsigned char b = (unsigned char)piece[1];
            if (b >= 0x80 && b <= 0x8F) { piece[0] = (char)0xD0; piece[1] = (char)(b + 0x20); }
            else if (b == 0x91) { piece[0] = (char)0xD0; piece[1] = (char)0x81; }
        }
        out += piece;
        i += used;
    }
    return out;
}

std::string ru(const std::string& latin) {
    std::string out, wd;
    auto flush = [&]() {
        if (wd.empty()) return;
        std::string base = wd, suffix;
        size_t dash = wd.find('-');
        if (dash != std::string::npos && dash > 0 && isdigit((unsigned char)wd.back())) { base = wd.substr(0, dash); suffix = wd.substr(dash); }
        auto it = words().find(base);
        out += (it != words().end() ? it->second : translit(base)) + suffix;
        wd.clear();
    };
    for (char c : latin) {
        if (c == ' ') { flush(); out += ' '; }
        else wd += c;
    }
    flush();
    return out;
}

}  // namespace gfx
