// main_test.cpp - TEST ONLY. Runs the real screens with no window: frames are drawn by a small
// software rasteriser into memory and written out as PNG files, and the mouse is driven by script.
// This is how the interface is checked on a machine with no display (and no Windows).
#include "bot.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "ui.h"
#include <chrono>
#include <cstdio>
#include <fstream>
#include <map>

static int W = 1600, H = 1000;
static std::vector<uint32_t> fb;
struct Tex { int w = 0, h = 0; std::vector<uint32_t> px; };      // RGBA bytes, as ImGui and DirectX keep them
static std::map<ImU64, Tex> textures;
static ImU64 nextTex = 2;                                           // 1 is the font atlas

static inline void texel(const Tex& t, float u, float v, float out[4]) {
    // bilinear, clamped to the edge (as the game's DirectX sampler is)
    float x = u * t.w - 0.5f, y = v * t.h - 0.5f;
    int x0 = (int)std::floor(x), y0 = (int)std::floor(y);
    float fx = x - x0, fy = y - y0;
    auto at = [&](int xx, int yy) { xx = std::min(t.w - 1, std::max(0, xx)); yy = std::min(t.h - 1, std::max(0, yy)); return t.px[(size_t)yy * t.w + xx]; };
    uint32_t c[4] = {at(x0, y0), at(x0 + 1, y0), at(x0, y0 + 1), at(x0 + 1, y0 + 1)};
    float w[4] = {(1 - fx) * (1 - fy), fx * (1 - fy), (1 - fx) * fy, fx * fy};
    for (int k = 0; k < 4; k++) out[k] = 0;
    for (int i = 0; i < 4; i++)
        for (int k = 0; k < 4; k++) out[k] += ((c[i] >> (8 * k)) & 255) * w[i];
    for (int k = 0; k < 4; k++) out[k] /= 255.f;
}

// ---------------------------------------------------------------------------- rasteriser
static inline void blend(uint32_t& dst, float r, float g, float b, float a) {
    if (a <= 0) return;
    float dr = (float)(dst & 255), dg = (float)((dst >> 8) & 255), db = (float)((dst >> 16) & 255);
    dr += (r - dr) * a; dg += (g - dg) * a; db += (b - db) * a;
    dst = 0xFF000000u | ((uint32_t)db << 16) | ((uint32_t)dg << 8) | (uint32_t)dr;
}
static void tri(const ImDrawVert& a, const ImDrawVert& b, const ImDrawVert& c, const ImVec4& clip, const Tex& t, bool font) {
    float minx = std::max(clip.x, std::floor(std::min(a.pos.x, std::min(b.pos.x, c.pos.x))));
    float maxx = std::min(clip.z, std::ceil(std::max(a.pos.x, std::max(b.pos.x, c.pos.x))));
    float miny = std::max(clip.y, std::floor(std::min(a.pos.y, std::min(b.pos.y, c.pos.y))));
    float maxy = std::min(clip.w, std::ceil(std::max(a.pos.y, std::max(b.pos.y, c.pos.y))));
    float area = (b.pos.x - a.pos.x) * (c.pos.y - a.pos.y) - (b.pos.y - a.pos.y) * (c.pos.x - a.pos.x);
    if (area == 0 || minx >= maxx || miny >= maxy) return;
    float inv = 1.f / area;
    ImVec4 ca = ImGui::ColorConvertU32ToFloat4(a.col), cb = ImGui::ColorConvertU32ToFloat4(b.col), cc = ImGui::ColorConvertU32ToFloat4(c.col);
    for (int y = (int)miny; y < (int)maxy; y++)
        for (int x = (int)minx; x < (int)maxx; x++) {
            float px = x + 0.5f, py = y + 0.5f;
            float w0 = ((b.pos.x - px) * (c.pos.y - py) - (b.pos.y - py) * (c.pos.x - px)) * inv;
            float w1 = ((c.pos.x - px) * (a.pos.y - py) - (c.pos.y - py) * (a.pos.x - px)) * inv;
            float w2 = 1 - w0 - w1;
            if (w0 < 0 || w1 < 0 || w2 < 0) continue;
            float u = a.uv.x * w0 + b.uv.x * w1 + c.uv.x * w2, v = a.uv.y * w0 + b.uv.y * w1 + c.uv.y * w2;
            float tc[4] = {1, 1, 1, 1};
            if (t.w > 0) {
                if (font) {                                   // the font atlas: nearest texel
                    int tx = std::min(t.w - 1, std::max(0, (int)(u * t.w))), ty = std::min(t.h - 1, std::max(0, (int)(v * t.h)));
                    uint32_t p = t.px[(size_t)ty * t.w + tx];
                    tc[0] = (p & 255) / 255.f; tc[1] = ((p >> 8) & 255) / 255.f; tc[2] = ((p >> 16) & 255) / 255.f; tc[3] = (p >> 24) / 255.f;
                } else texel(t, u, v, tc);
            }
            blend(fb[(size_t)y * W + x], (ca.x * w0 + cb.x * w1 + cc.x * w2) * 255 * tc[0], (ca.y * w0 + cb.y * w1 + cc.y * w2) * 255 * tc[1],
                  (ca.z * w0 + cb.z * w1 + cc.z * w2) * 255 * tc[2], (ca.w * w0 + cb.w * w1 + cc.w * w2) * tc[3]);
        }
}
static void raster(ImDrawData* dd) {
    std::fill(fb.begin(), fb.end(), 0xFF000000u);
    for (int n = 0; n < dd->CmdListsCount; n++) {
        const ImDrawList* l = dd->CmdLists[n];
        for (const ImDrawCmd& cmd : l->CmdBuffer) {
            ImVec4 clip(std::max(0.f, cmd.ClipRect.x), std::max(0.f, cmd.ClipRect.y), std::min((float)W, cmd.ClipRect.z), std::min((float)H, cmd.ClipRect.w));
            const ImDrawIdx* idx = l->IdxBuffer.Data + cmd.IdxOffset;
            const ImDrawVert* vtx = l->VtxBuffer.Data + cmd.VtxOffset;
            static Tex none;
            auto it = textures.find((ImU64)cmd.GetTexID());
            const Tex& t = it == textures.end() ? none : it->second;
            for (unsigned i = 0; i + 2 < cmd.ElemCount; i += 3) tri(vtx[idx[i]], vtx[idx[i + 1]], vtx[idx[i + 2]], clip, t, (ImU64)cmd.GetTexID() == 1);
        }
    }
}

// ---------------------------------------------------------------------------- a minimal PNG writer (stored, uncompressed)
static uint32_t crcTable[256];
static uint32_t crc32(const uint8_t* d, size_t n, uint32_t c = 0) {
    if (!crcTable[1]) for (uint32_t i = 0; i < 256; i++) { uint32_t k = i; for (int j = 0; j < 8; j++) k = k & 1 ? 0xEDB88320u ^ (k >> 1) : k >> 1; crcTable[i] = k; }
    c = ~c;
    for (size_t i = 0; i < n; i++) c = crcTable[(c ^ d[i]) & 255] ^ (c >> 8);
    return ~c;
}
static void png(const std::string& path) {
    std::vector<uint8_t> raw;
    int PW = std::min(W, (int)ImGui::GetIO().DisplaySize.x), PH = std::min(H, (int)ImGui::GetIO().DisplaySize.y);
    raw.reserve((size_t)(PW * 3 + 1) * PH);
    for (int y = 0; y < PH; y++) { raw.push_back(0); for (int x = 0; x < PW; x++) { uint32_t p = fb[(size_t)y * W + x]; raw.push_back(p & 255); raw.push_back((p >> 8) & 255); raw.push_back((p >> 16) & 255); } }
    std::vector<uint8_t> z = {0x78, 0x01};
    uint32_t a = 1, b = 0;
    for (size_t i = 0; i < raw.size(); i++) { a = (a + raw[i]) % 65521; b = (b + a) % 65521; }
    for (size_t off = 0; off < raw.size(); off += 65535) {
        size_t n = std::min<size_t>(65535, raw.size() - off);
        z.push_back(off + n >= raw.size() ? 1 : 0);
        z.push_back(n & 255); z.push_back((n >> 8) & 255); z.push_back(~n & 255); z.push_back((~n >> 8) & 255);
        z.insert(z.end(), raw.begin() + off, raw.begin() + off + n);
    }
    uint32_t ad = (b << 16) | a;
    for (int i = 3; i >= 0; i--) z.push_back((ad >> (i * 8)) & 255);
    std::ofstream f(path, std::ios::binary);
    auto be = [&](uint32_t v) { for (int i = 3; i >= 0; i--) f.put((char)((v >> (i * 8)) & 255)); };
    auto chunk = [&](const char* type, const std::vector<uint8_t>& d) {
        be((uint32_t)d.size());
        std::vector<uint8_t> t(type, type + 4);
        t.insert(t.end(), d.begin(), d.end());
        f.write((const char*)t.data(), (std::streamsize)t.size());
        be(crc32(t.data(), t.size()));
    };
    f.write("\x89PNG\r\n\x1a\n", 8);
    std::vector<uint8_t> ih;
    for (uint32_t v : {(uint32_t)PW, (uint32_t)PH}) for (int i = 3; i >= 0; i--) ih.push_back((v >> (i * 8)) & 255);
    ih.insert(ih.end(), {8, 2, 0, 0, 0});
    chunk("IHDR", ih); chunk("IDAT", z); chunk("IEND", {});
}

// ---------------------------------------------------------------------------- driving the screens
static App* app;
static std::string outDir;
static int fails = 0;

static void frame() {
    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = 1.f / 60;
    io.DisplaySize = app->inGame ? ImVec2((float)W, (float)H) : app->launcherSize();     // the launcher is a small window of its own
    App::rects.clear();
    ImGui::NewFrame();
    app->frame();
    ImGui::Render();
}
static void frames(int n) { for (int i = 0; i < n; i++) frame(); }
static void shot(const std::string& name) {
    frames(3);
    raster(ImGui::GetDrawData());
    png(outDir + "/" + name + ".png");
}
static void check(bool ok, const std::string& what) { printf("%s  %s\n", ok ? "ok  " : "FAIL", what.c_str()); if (!ok) fails++; }
static bool click(const std::string& key, bool dbl = false);
static bool clickRaw(const std::string& key, bool dbl);
static bool click(const std::string& key, bool dbl) {
    frames(2);
    // an action kept under "More actions": open the menu first, as a player would
    if (!App::rects.count(key) && App::rects.count("More actions")) { clickRaw("More actions", false); frames(2); }
    return clickRaw(key, dbl);
}
static bool clickRaw(const std::string& key, bool dbl) {
    frames(1);
    auto it = App::rects.find(key);
    if (it == App::rects.end()) { printf("FAIL  nothing on screen called '%s'\n", key.c_str()); fails++; return false; }
    ImGuiIO& io = ImGui::GetIO();
    float x = (it->second[0] + it->second[2]) / 2, y = (it->second[1] + it->second[3]) / 2;
    x = std::min(x, it->second[0] + 60);              // long rows: click near the left end
    io.AddMousePosEvent(x, y);
    frame();
    for (int k = 0; k < (dbl ? 2 : 1); k++) { io.AddMouseButtonEvent(0, true); frame(); io.AddMouseButtonEvent(0, false); frame(); }
    frames(2);
    return true;
}
static bool onScreen(const std::string& key) { frames(2); return App::rects.count(key) > 0; }
static void play(int days, bool careful = true) {
    Bot bot(app->sim, careful);
    for (int i = 0; i < days && app->sim.g.phase == PH_PLAY; i++) { bot.plan(); app->sim.endDay(); }
}

int main(int argc, char** argv) {
    outDir = argc > 1 ? argv[1] : ".";
    std::string fontDir = argc > 2 ? argv[2] : "fonts";
    if (argc > 4) { W = atoi(argv[3]); H = atoi(argv[4]); }
    float scale = argc > 5 ? (float)atof(argv[5]) : 1.f;
    fb.assign((size_t)W * H, 0);
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.DisplaySize = ImVec2((float)W, (float)H);
    gfx::loadFonts(fontDir, scale, "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf");
    {
        unsigned char* pix; int tw, th;
        io.Fonts->GetTexDataAsRGBA32(&pix, &tw, &th);
        Tex& t = textures[1];
        t.w = tw; t.h = th;
        t.px.assign((const uint32_t*)pix, (const uint32_t*)pix + (size_t)tw * th);
        io.Fonts->SetTexID((ImTextureID)1);
    }
    gfx::texApi.create = [&](const Img& im) {
        ImU64 id = nextTex++;
        Tex& t = textures[id];
        t.w = im.w; t.h = im.h; t.px = im.px;
        return (ImTextureID)id;
    };
    gfx::texApi.destroy = [](ImTextureID id) { textures.erase((ImU64)id); };

    App a;
    app = &a;
    a.S = scale;
    a.theme();
    a.saveDir = outDir + "/saves";
    { std::ifstream f("manual.txt"); if (f) a.manual.assign((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>()); }
    App::recordRects = true;
    a.cfg.toasts = false;                               // the notices after each day are tested on their own, below
    Sim& s = a.sim;

    auto t0 = std::chrono::steady_clock::now();
    shot("00-title");
    click("Service records");
    shot("00-records");
    click("\xE2\x80\xB9 Back");
    click("Load a saved game");
    shot("00-load");
    click("\xE2\x80\xB9 Back");
    click("New career");
    click("combo:##start");
    shot("01-new-career-start");
    click("opt:" + std::string("Moscow, June 1968 - the training course"));
    click("combo:##background");
    click("opt:Institute of Languages");
    shot("01-new-career");
    click("Begin career");
    check(a.inGame && s.g.phase == PH_PLAY && s.training(), "a new career with the course starts in Moscow");
    check(s.g.background == 2 && s.g.langs[0] >= 80, "the background chosen is the background played");
    // ---- careers begun later: the rank, the posting, and some weeks of play
    for (int th : {TH_VIENNA, TH_DELHI, TH_WASHINGTON, TH_BERLIN1, TH_BONN}) {
        Sim::CareerOpts o;
        o.start = th; o.background = th % 4; o.difficulty = th % 3; o.ironman = th == TH_BONN;
        s.newCareer(7 + (uint64_t)th, "Petrov", o);
        check(s.g.phase == PH_PLAY && s.g.theatre == th && s.g.rank == Sim::startRank(th) && s.g.startTheatre == th, std::string("a career can begin in ") + THEATRES[th].city);
        const Officer* rz = s.rezident();
        check(THEATRES[th].role == 3 ? rz == nullptr : (rz && rz->temper >= 0 && rz->temper < 5), "the Rezident has a temperament");
        play(60);
        check(s.g.phase != PH_OVER || s.g.overTitle.size() > 0, std::string("sixty days in ") + THEATRES[th].city);
    }
    {
        Sim::CareerOpts o;
        o.start = TH_LONDON;
        s.newCareer(5, "Petrov", o);
        int t1 = s.rezident()->temper;
        s.newCareer(5, "Ivanov", o);
        check(s.rezident()->temper == t1, "the same world number gives the same Rezident");
    }
    s.newCareer(11, "Sokolov", 1, true);
    // ---- the course, played by the test player, with pictures on the way
    shot("02-course-planner");
    click("tab:City map");
    shot("02-course-map");
    for (int d = 0; d < 40 && s.g.phase == PH_PLAY; d++) {
        Bot bot(s, true);
        bot.plan();
        click("END DAY >");
        if (onScreen("OK##note")) click("OK##note");
        if (d == 1) shot("03-course-report");
        if (d == 4) { click("tab:Contacts"); for (auto& p : s.g.persons) if (p.live()) { click("person:" + p.name); break; } shot("03-course-dossier"); }
    }
    check(s.g.phase == PH_INTERLUDE && s.g.nextTheatre == TH_LONDON, "the course ends with an order to London");
    shot("04-course-order");
    if (s.g.phase == PH_INTERLUDE) click(s.g.interludeOpts[0]);
    check(s.g.phase == PH_PLAY && s.g.theatre == TH_LONDON, "the course is followed by London");
    // a fixed world, so that the pictures are repeatable
    s.newCareer(11, "Sokolov");
    click("cable:0");
    shot("05-centre");
    const char* tabs[] = {"Planner", "Checklist", "Contacts", "City map", "Sites", "Case board", "Material", "Centre", "Daily report", "Station", "Surveillance", "Accounts", "Results", "Private life", "Career", "Handbook"};
    for (auto t : tabs) check(click(std::string("tab:") + t), std::string("tab opens: ") + t);

    // ---- give an order through the form
    const int D = s.g.day;                              // the planner's top row
    click("tab:Planner");
    click("plan:6:1", true);
    check(onScreen("combo:##type"), "double-click on a planner block opens the order form");
    shot("03-order-default");
    click("combo:##type");
    click(std::string("opt:") + OTYPE_LABEL[O_SCOUT]);
    shot("04-order-scout");
    click("OK");
    check(s.isSet(D + 6, 1) && s.order(D + 6, 1).type == O_SCOUT, "order stored: " + s.describe(s.order(D + 6, 1)));
    // ---- an illegal order is refused
    click("plan:1:0");
    click("Edit order...");
    click("combo:##type");
    click(std::string("opt:") + OTYPE_LABEL[O_FUNCTION]);
    click("OK");
    check(onScreen("combo:##type") && !s.isSet(D + 1, 0), "a function in the morning is refused and the form stays open");
    shot("05-order-refused");
    click("Cancel");
    // ---- repeat, copy, paste
    click("plan:6:1");
    click("Repeat for 3 weeks");
    check(s.order(D + 13, 1).type == O_SCOUT && s.order(D + 27, 1).type == O_SCOUT, "repeat weekly fills the next three weeks");
    click("Copy"); click("plan:2:2"); click("Paste");
    check(s.order(D + 2, 2).type == O_SCOUT, "copy and paste");
    click("Back to routine");
    check(!s.isSet(D + 2, 2), "back to routine");
    // ---- the agreed meeting: open its agenda
    click("plan:5:2", true);
    shot("06-order-agent-meeting");
    click("OK");
    // ---- end days
    for (int i = 0; i < 6; i++) { click("END DAY >"); if (onScreen("OK##note")) click("OK##note"); }
    check(s.g.day == D + 6, fstr("six presses of END DAY -> day %d (%s)", s.g.day, fmtDay(s.g.day).c_str()));
    click("tab:Daily report");
    click("\xE2\x80\xB9 Earlier");
    shot("07-report");
    click("tab:Contacts");
    for (auto& p : s.g.persons) if (p.stage == AGENT) { click("person:" + p.name); break; }
    shot("08-dossier");
    click("Request trace");
    check(onScreen("OK##note"), "a staff action answers with a note");
    click("OK##note");
    // ---- let the test player work for five months, then look at every screen
    play(150);
    a.cfg.toasts = true;
    a.showMoment("ЗАВЕРБОВАН", "RECRUITED", 0xB3171B);
    frames(8); shot("09-moment");
    frames(200);
    a.cfg.toasts = false;
    int shotNo = 10;
    for (const char* t : tabs) {
        if (s.g.phase != PH_PLAY) break;
        std::string name = t;
        click("tab:" + name);
        if (name == "Contacts") for (auto& p : s.g.persons) if (p.stage == AGENT) { click("person:" + p.name); break; }
        if (name == "Sites") for (auto& x : s.g.sites) if (!x.retired) { click("site:" + x.name); break; }
        if (name == "Material") for (auto& x : s.g.rolls) if (x.state >= R_YOU) { click(fstr("roll:%d", x.id)); break; }
        if (name == "Centre") click("cable:0");
        if (name == "Station") for (auto& o : s.g.officers) if (o.role == ROLE_REZIDENT) { click("officer:" + o.name); break; }
        std::string slug;
        for (char ch : name) slug += isalnum((unsigned char)ch) ? (char)tolower((unsigned char)ch) : '-';
        shot(fstr("%d-", shotNo++) + slug);
        if (name == "Surveillance" && !s.g.watchers.empty()) {
            int id = s.g.watchers.back().id;
            click("car:0");
            bool struck = false;
            for (auto& w : s.g.watchers) if (w.id == id) struck = w.struck;
            check(struck, "a car can be struck off the log");
            click("car:0");
        }
    }

    // ---- the conveniences: a window lifted off the desk, the settings, the checklist, the notebook, a codename
    if (s.g.phase == PH_PLAY) {
        click("tab:Planner");
        click("pop:City map");
        check(a.popped[App::T_MAP], "a screen can be lifted off the desk into its own window");
        shot("30-window-map");
        click("tab:Contacts");
        click("pop:Centre");
        shot("31-two-windows");
        check(a.popped[App::T_CENTRE] && a.tab == App::T_PEOPLE, "two windows, and the desk still works behind them");
        {   // the two windows sharing a frame, side by side, as if one had been dropped on the edge of the other
            ImGuiID node = ImGui::DockBuilderAddNode(0, 0);
            ImGui::DockBuilderSetNodePos(node, ImVec2(230, 120));
            ImGui::DockBuilderSetNodeSize(node, ImVec2(1330, 840));
            ImGuiID l = 0, r = 0;
            ImGui::DockBuilderSplitNode(node, ImGuiDir_Left, 0.45f, &l, &r);
            ImGui::DockBuilderDockWindow("###win:City map", l);
            ImGui::DockBuilderDockWindow("###win:Centre", r);
            ImGui::DockBuilderFinish(node);
            frames(3);
            shot("31-docked");
            ImGuiWindow* wm = ImGui::FindWindowByName("###win:City map");
            check(wm && wm->DockNode != nullptr, "two windows can share a frame");
        }
        click("Menu");
        check(onScreen("Reset windows") && onScreen("Close all windows"), "the settings have 'Reset windows' and 'Close all windows'");
        shot("32-settings");
        click("Close all windows");
        check(!a.popped[App::T_MAP] && !a.popped[App::T_CENTRE], "'Close all windows' puts every screen back");
        click("setpop:City map");
        check(a.popped[App::T_MAP], "a screen can be lifted off from the settings too");
        shot("32-settings-after");
        click("Reset windows");
        if (onScreen("OK##note")) click("OK##note");
        check(!a.popped[App::T_MAP], "'Reset windows' puts every screen back");
        {
            a.popOut(App::T_MAP, true);
            frames(3);
            ImGuiWindow* wm = ImGui::FindWindowByName("###win:City map");
            check(wm && wm->DockNode == nullptr, "after 'Reset windows' a window comes back on its own, not in the old frame");
            a.popOut(App::T_MAP, false);
            frames(2);
        }
        if (onScreen("Close##settings")) click("Close##settings");
        if (onScreen("OK##note")) click("OK##note");
        click("tab:Checklist");
        check(!s.todo().empty(), "the checklist has something on it");
        s.g.notes = "GRANIT - ask about the new man in registry.";
        shot("33-checklist");
        if (onScreen("todo:0")) { click("todo:0"); check(a.tab != App::T_NOTES || s.todo()[0].tab == 0, "a line of the checklist leads to its screen"); }
        for (auto& p : s.g.persons) if (p.stage == AGENT && p.handler == 0) {
            int id = p.id;
            check(s.renameAgent(p, "kestrel").find("KESTREL") != std::string::npos && s.person(id)->codename == "KESTREL", "an agent can be given a new codename");
            click("tab:Contacts");
            click("person:" + p.name);
            click("Rename");
            shot("34-rename");
            click("Cancel##rn");
            break;
        }
        // the notices after a day
        a.cfg.toasts = true;
        click("tab:Planner");
        s.g.pending.push_back({s.g.day + 1, P_LINEN_PKG, 0, 0});        // (something that will arrive overnight)
        s.g.decisions.clear();
        click("END DAY >");
        if (onScreen("OK##note")) click("OK##note");
        check(onScreen("toast:0"), "a notice appears after the day");
        if (onScreen("toast:0")) {
            shot("36-notices");
            int before = a.tab;
            click("toast:0");
            check(a.tab != before || a.tab == App::T_REPORT || a.tab == App::T_CENTRE, "a notice leads to the report or the cables");
        }
        a.cfg.toasts = false;
        click("tab:Handbook");
        if (onScreen("manual:3")) click("manual:3");
        shot("35-handbook");
        std::vector<uint8_t> blob = s.save();
        Sim t2;
        check(t2.load(blob) && t2.g.notes == s.g.notes, "the notebook is kept in the saved game");
    }

    // ---- save and load through the screens
    if (s.g.phase == PH_PLAY) {
        int day = s.g.day;
        click("Save"); shot("23-save"); click("Save##do");
        check(onScreen("OK##note"), "save reports back"); click("OK##note");
        check(a.loadFrom(a.saveDir + "/posting.rez") && s.g.day == day, "the saved file loads again");
    }
    // ---- on to the end of the posting, the interlude, and the later roles
    int guard = 0;
    while (s.g.phase == PH_PLAY && guard++ < 3000) play(1);
    shot("30-end-of-london");
    check(s.g.phase == PH_INTERLUDE || s.g.phase == PH_OVER, "the London posting comes to an end");
    for (int posting = 1; posting < 4 && s.g.phase == PH_INTERLUDE; posting++) {
        s.g.png = s.g.recalls = s.g.poorPostings = 0;      // the test wants to see every posting
        if (posting == 1) shot("31-order");
        click(s.g.interludeOpts[0]);
        check(s.g.phase == PH_PLAY && s.T().slot == posting, std::string("interlude choice starts the next posting: ") + s.T().city);
        click("cable:0");
        shot(fstr("4%d-arrival", posting));
        play(200);
        if (s.g.phase != PH_PLAY) { shot(fstr("4%d-ended-early", posting)); continue; }
        if (!s.g.decisions.empty()) { click("tab:Station"); shot(fstr("4%d-decision", posting)); click("decision:0"); }
        click("tab:Station");
        for (auto& o : s.g.officers) if (o.role == ROLE_SUB && !o.gone) { click("officer:" + o.name); break; }
        shot(fstr("4%d-station", posting));
        click("tab:Contacts");
        for (auto& p : s.g.persons) if (p.stage == AGENT && p.handler != 0) { click("person:" + p.name); if (onScreen("Instruct...")) { click("Instruct..."); shot(fstr("4%d-instruct", posting)); click("Cancel##instr"); } break; }
        shot(fstr("4%d-contacts", posting));
        // Centre's verification of an agent, through the screens
        for (auto& p : s.g.persons) if (p.stage == AGENT && p.handler == 0 && p.verify == 0) {
            int id = p.id;
            click("person:" + p.name);
            if (click("Ask Centre to verify him")) { click("Yes"); if (onScreen("OK##note")) click("OK##note"); }
            Person* q = s.person(id);
            check(q && q->verify == 1, "verification can be asked of Centre");
            break;
        }
        // a safe flat, through the screens
        if (posting == 1) {
            click("tab:Material");
            s.g.funds += 2000;
            click("Rent a safe flat...");
            shot("4x-flat");
            click("Rent it");
            if (onScreen("OK##note")) { shot("4x-flat-note"); click("OK##note"); }
            bool pend = false;
            for (auto& x : s.g.pending) if (x.kind == P_FLAT) pend = true;
            check(pend, "a safe flat is being rented");
            for (int k = 0; k < 40 && s.g.phase == PH_PLAY; k++) { bool has = false; for (auto& x : s.g.sites) if (x.flat) has = true; if (has) break; play(1); }
            bool has = false;
            for (auto& x : s.g.sites) if (x.flat) { has = true; click("tab:Sites"); click("site:" + x.name); shot("4x-flat-card"); break; }
            check(has, "the flat is ready within some weeks");
            // instruction, and documents at the flat, in the order form for an agreed meeting
            for (auto& p : s.g.persons) if (p.stage == AGENT && p.handler == 0 && p.nextMeet >= s.g.day && p.nextMeet - s.g.day < 30) {
                int r = p.nextMeet - s.g.day;
                Site* fl = nullptr;
                for (auto& x : s.g.sites) if (x.flat) fl = &x;
                if (fl) { p.meetSites = {fl->id}; p.meetIdx = 0; if (s.isSet(p.nextMeet, 2)) s.g.plan[p.nextMeet][2].site = fl->id; }
                click("tab:Planner");
                click(fstr("plan:%d:2", r), true);
                if (!onScreen("combo:##trainWhat")) click("Give instruction (long meetings only)");   // the usual agenda may already have ticked it
                check(onScreen("combo:##trainWhat"), "the order form offers a choice of instruction");
                shot("4x-order-instruction");
                click("Cancel");
                break;
            }
            // the operational game: a lunch is due
            s.g.game = 2; s.g.gameName = "Mr Kessler"; s.g.gameNext = s.g.day; s.g.gameRounds = 2; s.g.gameDoubt = 20; s.g.gameSuspect = 25;
            s.g.gameLog = fmtShort(s.g.day - 40) + "  Centre sanctions the game against Mr Kessler.\n";
            s.g.decisions.clear();
            s.g.evLast[1000] = s.g.day;          // no other event today
            s.endDay();
            bool lunch = false;
            for (auto& d : s.g.decisions) if (d.kind == DK_GAME) lunch = true;
            // (only the lunch is wanted here; whatever else the night brought is put aside)
            s.g.decisions.erase(std::remove_if(s.g.decisions.begin(), s.g.decisions.end(), [](const Decision& d) { return d.kind != DK_GAME; }), s.g.decisions.end());
            if (onScreen("OK##note")) click("OK##note");
            check(lunch, "the operational game asks what to give him");
            click("tab:Station");
            if (onScreen("decision:0")) { shot("4x-game"); click("decision:1"); }
            check(s.g.gameRounds == 3, "a lunch of the game is played");
            click("tab:Station");
            shot("4x-station-game");
            // everything from here to the end of this block is played on a copy of the posting, and then put back
            std::vector<uint8_t> snap = s.save();
            // the officer's own affairs: the plan, a present, a compromise operation, leave
            check(s.g.planLevel >= 0 || !s.g.attest.empty(), "a work plan was asked for");
            for (auto& p : s.g.persons) if ((p.stage == DEVELOPING || p.stage == SPOTTED) && p.handler == 0 && p.kompState == 0) {
                int id = p.id;
                p.meetings = std::max(p.meetings, 3);
                click("tab:Contacts");
                click("person:" + p.name);
                click("Prepare a compromise...");
                shot("4x-compromise");
                click("komp:2");
                click("Propose it to Centre");
                if (onScreen("OK##note")) click("OK##note");
                Person* q = s.person(id);
                check(q && q->kompState == 1 && q->kompKind == 2, "a compromise operation can be proposed");
                // Centre answers, and it is staged at a dinner
                q->kompState = 2;
                Order o; o.type = O_MEET_OVERT; o.person = id;
                s.g.decisions.clear(); s.g.evLast[1000] = s.g.day;
                q->rapport = 95;
                s.setOrder(s.g.day, 2, o);
                s.endDay();
                q = s.person(id);
                check(q && (q->kompState == 3 || q->kompState == 4), "the operation is staged at a dinner");
                click("tab:Contacts");
                if (q) click("person:" + q->name);
                shot("4x-compromise-file");
                break;
            }
            // ---- the trade: a confidential contact, a playback, a letter through the door, a standing escape plan
            bool anyDev = false;
            for (auto& p : s.g.persons) if ((p.stage == DEVELOPING || p.stage == SPOTTED) && p.live()) {
                int id = p.id;
                anyDev = true;
                p.stage = DEVELOPING; p.handler = 0; p.confid = 0;
                p.meetings = std::max(p.meetings, 4); p.rapport = std::max(p.rapport, 60.f);
                for (auto& q : s.g.persons) if (q.id != id && q.stage == DEVELOPING) q.confid = 0;
                click("tab:Contacts");
                click("person:" + p.name);
                click("Register as a confidential contact");
                if (onScreen("OK##note")) click("OK##note");
                Person* q = s.person(id);
                check(q && q->confid == 1, "a contact can be put forward as a confidential contact");
                q->confid = 2;
                size_t rolls = s.g.rolls.size();
                Order o; o.type = O_MEET_OVERT; o.person = id;
                s.g.decisions.clear(); s.g.evLast[1000] = s.g.day;
                // a lunch can still be cancelled by telephone, or lost to exhaustion: give it three days
                for (int k = 0; k < 3 && s.g.rolls.size() == rolls && s.g.phase == PH_PLAY; k++) {
                    if (!(q = s.person(id)) || !q->live()) break;
                    q->rapport = 95; q->lastConfid = -9999; s.g.fatigue = 0; s.g.decisions.clear();
                    s.setOrder(s.g.day, 1, o);
                    s.endDay();
                }
                check(s.g.rolls.size() > rolls, "lunch with a confidential contact yields a report");
                click("tab:Contacts");
                if ((q = s.person(id))) click("person:" + q->name);
                shot("4x-confidential");
                break;
            }
            check(anyDev, "there was a contact to register");
            for (auto& p : s.g.persons) if (p.stage == AGENT && p.handler == 0 && p.play == 0) {
                int id = p.id;
                click("tab:Contacts");
                click("person:" + p.name);
                click("Propose to play him back");
                shot("4x-playback-ask");
                click("Yes");
                if (onScreen("OK##note")) click("OK##note");
                Person* q = s.person(id);
                check(q && q->play == 1, "a playback can be proposed to Centre");
                q->play = 0;
                // and a standing escape plan for the same agent
                if (q->signalSite < 0) for (auto& x : s.g.sites) if (x.type == ST_SIGNAL && !x.retired) { q->signalSite = x.id; break; }
                q->exfil = 2;
                if (q->nextMeet < s.g.day) { q->nextMeet = s.g.day + 2; }
                Order o; o.type = O_MEET_CLAN; o.person = id; o.longMeet = true; o.standing = true;
                if (Site* ms = s.agreedSite(*q)) o.site = ms->id;
                bool was = q->dangle; bool wasD = q->doubled; q->dangle = false; q->doubled = false;
                for (int att = 0; att < 4; att++) {       // (an agent may fail to turn up; the test is of the plan, not of him)
                    q->misses = 0; q->stress = 0;
                    s.g.decisions.clear(); s.g.evLast[1000] = s.g.day;
                    if (q->nextMeet >= 0) s.clearOrder(q->nextMeet, 2);
                    q->nextMeet = s.g.day;
                    s.setOrder(s.g.day, 2, o);
                    s.g.you.covered[2] = false;
                    s.endDay();
                    q = s.person(id);
                    if (!q || q->stage != AGENT || q->escape == 2) break;
                }
                if (q && q->stage == AGENT && q->escape == 2) {
                    check(true, "an escape plan can be left standing");
                    q->suspicion = 74.5f; q->tempo = 2; q->empSec = 90;
                    for (int k = 0; k < 6 && q->escape == 2 && q->stage == AGENT; k++) { s.g.decisions.clear(); s.g.evLast[1000] = s.g.day; s.endDay(); q = s.person(id); q->suspicion = std::max(q->suspicion, 74.9f); }
                    Site* sg = s.site(q->signalSite);
                    check(q->escape == 3 && sg && sg->mark == 4, "a frightened agent gives the escape signal");
                    Order rd; rd.type = O_READ_SIGNAL; rd.site = q->signalSite;
                    s.g.decisions.clear(); s.g.evLast[1000] = s.g.day;
                    s.setOrder(s.g.day, 0, rd);
                    s.endDay();
                    q = s.person(id);
                    check(q && q->escape == 4, "the signal is seen and acknowledged");
                    click("tab:Daily report");
                    shot("4x-escape-signal");
                    for (int k = 0; k < 4 && q->stage == AGENT; k++) { s.g.decisions.clear(); s.g.evLast[1000] = s.g.day; s.endDay(); q = s.person(id); }
                    check(q->stage == EXFILTRATED || q->stage == ARRESTED, "the pick-up succeeds or fails");
                } else {
                    check(false, "an escape plan can be left standing");
                    if (q) printf("      stage %d escape %d exfil %d nextMeet %d day %d\n", q->stage, q->escape, q->exfil, q->nextMeet, s.g.day);
                    auto it = s.g.reports.find(s.g.day - 1);
                    if (it != s.g.reports.end()) for (auto& l : it->second) printf("      | %s\n", l.c_str());
                }
                if (q) { q->dangle = was; q->doubled = wasD; }
                break;
            }
            {
                s.g.decisions.clear();
                s.g.pending.push_back({s.g.day + 1, P_SECRET, 0, 5});
                s.g.evLast[1000] = s.g.day;
                s.endDay();
                bool summoned = false;
                for (auto& d : s.g.decisions) if (d.kind == DK_SUMMONS) summoned = true;
                check(summoned, "an officer with something to hide can be summoned home");
                if (onScreen("decision:0")) shot("4x-summons");
                s.g.decisions.clear();
            }
            while (s.g.phase == PH_PLAY && (s.g.day - s.g.postStart < 245 || s.onLeave(s.g.day))) play(1);
            if (s.g.phase != PH_PLAY) { s.load(snap); s.g.postStart = std::min(s.g.postStart, s.g.day - 245); }   // (the posting ended early in this world)
            {
            if (s.g.leaveFrom > s.g.day) s.cancelLeave();
            s.g.leaveYear = 0; s.g.standRez = std::max(s.g.standRez, 45.f); s.g.purse = std::max(s.g.purse, 100); s.g.lastHomeGift = -9999;
            s.g.decisions.clear();
            click("tab:Private life");
            float h0 = s.g.home;
            click("home:gift");
            if (onScreen("OK##note")) click("OK##note");
            check(s.g.home > h0, "a present mends things at home");
            click("home:goods");
            if (onScreen("OK##note")) click("OK##note");
            check(s.g.goods >= 1, "goods can be put by for Moscow");
            click("home:leave");
            if (onScreen("OK##note")) { shot("4x-leave-note"); click("OK##note"); }
            check(s.g.leaveFrom > s.g.day, "leave can be booked");
            shot("4x-private");
            int from = s.g.leaveFrom;
            for (int k = 0; k < 40 && s.g.phase == PH_PLAY && s.g.day < from; k++) play(1);
            bool asked = false;
            for (auto& d : s.g.decisions) if (d.kind == DK_LEAVE) asked = true;
            check(asked, "leave begins with a choice of how to spend it");
            if (onScreen("decision:1")) { shot("4x-leave"); click("decision:1"); }
            play(3);
            check(s.onLeave(s.g.day), "on leave the days pass without you");
            click("tab:Planner");
            shot("4x-leave-planner");
            play(26);
            check(s.onLeave(s.g.day) && s.g.fatigue < 5, "leave rests you");
            play(4);
            check(!s.onLeave(s.g.day), "and it ends");
            }
            check(s.load(snap), "the posting is put back as it was");
            click("tab:Planner");
        }
        click("tab:Planner");
        shot(fstr("4%d-planner", posting));
        click("tab:City map");
        shot(fstr("4%d-map", posting));
        click("tab:Case board");
        shot(fstr("4%d-board", posting));
        for (auto& p : s.g.persons) if (p.stage == AGENT && onScreen("board:" + p.label())) {
            click("board:" + p.label(), true);
            check(a.tab == App::T_PEOPLE, "double-clicking a card on the case board opens the file");
            break;
        }
        click("tab:Results");
        shot(fstr("4%d-returns", posting));
        click("tab:Surveillance");
        shot(fstr("4%d-surveillance", posting));
        for (int i = (int)s.g.cables.size() - 1; i >= 0; i--)
            if (s.g.cables[i].from == "Embassy bulletin") {
                Cable c = s.g.cables[i];             // (test only: put it on top of the register to open it)
                s.g.cables.push_back(c);
                click("tab:Centre");
                click("cable:0");
                shot(fstr("4%d-bulletin", posting));
                s.g.cables.pop_back();
                break;
            }
        guard = 0;
        while (s.g.phase == PH_PLAY && guard++ < 3000) {
            play(1);
            if (!s.g.decisions.empty()) { frames(3); if (onScreen("decision:0")) { shot(fstr("4%d-decision", posting)); click("decision:0"); } }
        }
        shot(fstr("4%d-end", posting));
    }
    check(s.g.phase == PH_OVER, "the career reaches an ending: " + s.g.overTitle);
    shot("50-ending");
    click("Return to the title screen");
    check(!a.inGame, "back to the title screen");
    {
        std::ifstream f(a.saveDir + "/service-records.txt");
        int lines = 0;
        std::string l;
        while (std::getline(f, l)) if (!l.empty() && l[0] != '#') lines++;
        check(lines == 1, "the finished career is entered in the service records, once");
    }
    click("Service records");
    shot("52-records");
    click("\xE2\x80\xB9 Back");
    check(onScreen("Continue"), "the launcher offers to continue the last game");
    shot("53-title-after");
    click("Load a saved game");
    shot("51-load");
    printf("%d texture(s) alive at the end; %.1f s in all\n", gfx::textureCount(), std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count());
    printf("%d failure(s)\n", fails);
    return fails ? 1 : 0;
}
