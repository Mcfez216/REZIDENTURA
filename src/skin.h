// skin.h - the game's chrome: dark panels framed in brass, title bars, buttons, dividers, and the icon
// set. Everything here is drawn on ImGui draw lists; nothing knows the game rules.
#pragma once
#include "gfx.h"

namespace skin {

extern float scale;                                    // the interface scale (App::S), set by App::theme()

// ---------------------------------------------------------------------------- the palette
constexpr unsigned K_DESK = 0x15120F;                          // what lies under everything
constexpr unsigned K_PANEL = 0x262019, K_PANEL_LO = 0x1C1814;   // a panel, top and bottom of its gradient
constexpr unsigned K_BAR = 0x30271E, K_BAR_LO = 0x251E18;       // a title bar
constexpr unsigned K_INSET = 0x110E0C;                          // wells: inputs, gauges
constexpr unsigned K_ROW = 0x2C251E;                            // a hovered row
constexpr unsigned K_BRASS = 0xBE9A5E, K_BRASS_HI = 0xEACB8A, K_BRASS_LO = 0x6E5735;
constexpr unsigned K_TEXT = 0xE9E0CB, K_DIM = 0xA59A85, K_FAINT = 0x72695C;
constexpr unsigned K_RED = 0xB8331F, K_RED_LO = 0x5E1A12;
constexpr unsigned K_GOOD = 0x9CC47C, K_WARN = 0xE4A447, K_BAD = 0xE8634B;

// ---------------------------------------------------------------------------- surfaces
/** The dark leather of the desk, with a vignette. */
void desk(ImDrawList* dl, ImVec2 a, ImVec2 b, float S);
/** A panel: shadow, dark fill, brass trim with ornamented corners. */
void frame(ImDrawList* dl, ImVec2 a, ImVec2 b, float S, bool shadow = true);
/** Only the trim of a frame, for windows whose background ImGui draws itself. */
void trim(ImDrawList* dl, ImVec2 a, ImVec2 b, float S);
/** The title bar across the top of a frame (a is the frame's top-left, w its width). Returns its height. */
float titleBar(ImDrawList* dl, ImVec2 a, float w, float S, const char* title, const char* hint = nullptr);
float titleBarH(float S);
/** A divider: a brass line with a small diamond in the middle. */
void divider(ImDrawList* dl, ImVec2 a, float w, float S, float alpha = 1);
/** A well: a sunken dark rectangle (gauges, value boxes). */
void well(ImDrawList* dl, ImVec2 a, ImVec2 b, float S);
/** A gauge in a well, filled to `k` (0..1). */
void gauge(ImDrawList* dl, ImVec2 a, ImVec2 b, float k, ImU32 col, float S);

// ---------------------------------------------------------------------------- text
/** Letter-spaced text (for capitals). */
void tracked(ImDrawList* dl, ImFont* f, float size, ImVec2 pos, ImU32 col, const char* text, float tracking);
float trackedWidth(ImFont* f, float size, const char* text, float tracking);
std::string upper(const std::string& s);              // A-Z and Cyrillic

// ---------------------------------------------------------------------------- buttons
enum { B_NORMAL, B_PRIMARY, B_QUIET, B_TAB };
void button(ImDrawList* dl, ImVec2 a, ImVec2 b, float S, bool hov, bool held, int kind, float alpha = 1);
void diamond(ImDrawList* dl, ImVec2 c, float r, ImU32 fill, ImU32 edge = 0);

// ---------------------------------------------------------------------------- icons
enum {
    IC_PLAN, IC_CHECK, IC_PEOPLE, IC_MAP, IC_SITES, IC_MAT, IC_CENTRE, IC_REPORT, IC_STATION, IC_SURV, IC_ACC, IC_RESULTS, IC_HOME, IC_CAREER, IC_BOOK,
    IC_FUNDS, IC_FATIGUE, IC_COVER, IC_PEN, IC_CHIEF, IC_KREMLIN, IC_SAVE, IC_MENU, IC_SKIP, IC_ARROW, IC_CLOSE, IC_MIN, IC_NEW, IC_LOAD, IC_RECORDS,
    IC_GEAR, IC_QUIT, IC_POPOUT, IC_STAR, IC_ALERT, IC_CABLE, IC_CLOCK,
    N_ICON
};
/** An icon drawn in lines, `size` across, centred on c. */
void icon(ImDrawList* dl, int id, ImVec2 c, float size, ImU32 col);

}  // namespace skin
