// gfx.h - how things on the desk are drawn: paper, ink, stamps, photographs, paperclips, the objects
// in your safe, the emblem. Pure drawing on ImGui draw lists; nothing here knows the game rules.
#pragma once
#include "art.h"
#include "imgui.h"
#include <functional>
#include <string>

namespace gfx {

// ---------------------------------------------------------------------------- the palette
ImU32 rgb(unsigned hex, float a = 1);
ImVec4 rgb4(unsigned hex, float a = 1);
/** While drawing on a dark surface, the ink colours (C_INK, C_RED, the pencils...) are lightened so that
    text written for paper reads on it. Pushed by dark panels and dialogues, popped when they end. */
void pushInk(bool dark);
void popInk();
bool darkInk();
enum : unsigned {
    C_PAPER = 0xEFE6CF, C_PAPER_OLD = 0xE6D9B8, C_CARD = 0xF5EFDF, C_MANILA = 0xD8C08A, C_MANILA_DK = 0xC2A66C,
    C_LEDGER = 0xE4E9D2, C_MAP = 0xE9E3CB, C_TELETYPE = 0xF1EAC4,
    C_INK = 0x23201B, C_INK_FADED = 0x55504A, C_RULE = 0x9C8F72,
    C_RED = 0xA8231B, C_VIOLET = 0x4A3E8E, C_PENCIL_RED = 0xC0392B, C_PENCIL_BLUE = 0x2D4C8A,
    C_HIGHLIGHT = 0xF2DF7E, C_GOLD = 0xE2B23C, C_SOVIET_RED = 0xB3171B,
};

// ---------------------------------------------------------------------------- fonts
struct Fonts {
    ImFont *body = nullptr, *bold = nullptr, *small = nullptr, *tiny = nullptr;   // typewriter (Latin and Cyrillic)
    ImFont *head = nullptr, *stamp = nullptr, *stampBig = nullptr;                // condensed bold, for forms and rubber stamps
    ImFont *title = nullptr, *cal = nullptr;                                     // the title; the calendar's day number
    ImFont *hand = nullptr, *handBig = nullptr;                                  // pencil
    ImFont *serif = nullptr;                                                     // newspaper headlines
    // the interface itself: PT Sans for reading, Oswald (condensed capitals) for titles and labels; the
    // typewriter is kept for what is typed - cables, reports, files
    ImFont *ui = nullptr, *uiMed = nullptr, *uiBold = nullptr, *uiSmall = nullptr, *uiCaps = nullptr, *uiTitle = nullptr, *uiBig = nullptr, *uiItalic = nullptr;
    ImFont *label = nullptr, *bar = nullptr, *btn = nullptr, *figure = nullptr;
};
extern Fonts F;
/** Load the game's fonts from a folder. If a file is missing, `fallback` (a .ttf path) or ImGui's own font is used. */
void loadFonts(const std::string& dir, float scale, const std::string& fallback = "");

// ---------------------------------------------------------------------------- textures
/** The platform supplies these two: make a GPU texture from an image, and free one. */
struct TexApi {
    std::function<ImTextureID(const Img&)> create;
    std::function<void(ImTextureID)> destroy;
};
extern TexApi texApi;
/** A texture made once and kept: `make` is called only the first time a key is asked for. */
ImTextureID tex(const std::string& key, const std::function<Img()>& make);
void forgetTextures(const std::string& prefix);
int textureCount();

// ---------------------------------------------------------------------------- primitives
inline ImVec2 operator+(ImVec2 a, ImVec2 b) { return ImVec2(a.x + b.x, a.y + b.y); }
inline ImVec2 operator-(ImVec2 a, ImVec2 b) { return ImVec2(a.x - b.x, a.y - b.y); }
inline ImVec2 operator*(ImVec2 a, float s) { return ImVec2(a.x * s, a.y * s); }

/** Rotate everything drawn into `dl` since vertex `start` about `c`. */
void rotateSince(ImDrawList* dl, int start, ImVec2 c, float degrees);
/** Lay a texture over a rectangle at its natural size, repeating it. */
void tile(ImDrawList* dl, ImTextureID t, ImVec2 a, ImVec2 b, float tileSize, ImU32 tint);
void desk(ImDrawList* dl, ImVec2 a, ImVec2 b, float S);
/** A sheet: shadow, colour, grain, darker edges. */
void sheet(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 colour, float S, bool shadow = true);
/** A rubber stamp: framed text in ink, slightly rotated and worn. style 0 box, 1 double box, 2 no frame. */
void stamp(ImDrawList* dl, ImVec2 centre, const std::string& text, ImU32 ink, float degrees, ImFont* f, float size, int style = 0, const std::string& sub = "");
/** A round official seal with text running round the edge and a star in the middle. */
void seal(ImDrawList* dl, ImVec2 centre, float radius, const std::string& ring, const std::string& middle, ImU32 ink, float degrees);
/** Text written in pencil, at an angle. */
void pencil(ImDrawList* dl, ImVec2 pos, const std::string& text, ImU32 ink, float degrees, ImFont* f = nullptr, float size = 0);
/** A photograph: white border, shadow, optional paperclip, at an angle. */
void photo(ImDrawList* dl, ImTextureID t, ImVec2 topLeft, ImVec2 size, float degrees, float S, bool clip = true);
void paperclip(ImDrawList* dl, ImVec2 p, float S, float degrees);
void pushpin(ImDrawList* dl, ImVec2 p, float S, ImU32 head);
void star(ImDrawList* dl, ImVec2 c, float r, ImU32 fill, float degrees = 0, ImU32 edge = 0, float edgeW = 0);
/** The hammer and sickle, in a square of side `size` centred on c, in one colour. */
void hammerSickle(ImDrawList* dl, ImVec2 c, float size, ImU32 colour);
/** A sedan seen from the side, as sketched on a surveillance card: `w` wide, centred on c. */
void carSketch(ImDrawList* dl, ImVec2 c, float w, ImU32 body, int era, uint32_t seed);
/** Text with the small irregularities of a typewriter: each letter a little high or low, a little light or heavy. */
void typeLine(ImDrawList* dl, ImFont* f, float size, ImVec2 pos, ImU32 col, const char* b, const char* e, uint32_t seed);

// ---------------------------------------------------------------------------- things in the safe
void filmCassette(ImDrawList* dl, ImVec2 c, float s, bool exposed);
void minox(ImDrawList* dl, ImVec2 c, float s);
void tapeReel(ImDrawList* dl, ImVec2 c, float s);
void typedSheet(ImDrawList* dl, ImVec2 c, float s);
void cash(ImDrawList* dl, ImVec2 c, float s, const char* currency);
void padBook(ImDrawList* dl, ImVec2 c, float s);
void bolt(ImDrawList* dl, ImVec2 c, float s);
void bug(ImDrawList* dl, ImVec2 c, float s);
void parcel(ImDrawList* dl, ImVec2 c, float s);
void medal(ImDrawList* dl, ImVec2 c, float s, bool banner);

// ---------------------------------------------------------------------------- words
/** Russian forms of codenames and Soviet officers' names; anything else is transliterated. */
std::string ru(const std::string& latin);
extern const char* RU_MONTH_GEN[13];
extern const char* RU_WEEKDAY[7];

}  // namespace gfx
