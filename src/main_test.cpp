// main_test.cpp - TEST ONLY. Runs the real screens with no window: frames are drawn by a small
// software rasteriser into memory and written out as PNG files, and the mouse is driven by script.
// This is how the interface is checked on a machine with no display (and no Windows).
#include "bot.h"
#include "imgui.h"
#include "ui.h"
#include <cstdio>
#include <fstream>

static int W = 1600, H = 1000;
static std::vector<uint32_t> fb;
static unsigned char* texPix = nullptr;
static int texW = 0, texH = 0;

// ---------------------------------------------------------------------------- rasteriser
static inline void blend(uint32_t& dst, float r, float g, float b, float a) {
    if (a <= 0) return;
    float dr = (float)(dst & 255), dg = (float)((dst >> 8) & 255), db = (float)((dst >> 16) & 255);
    dr += (r - dr) * a; dg += (g - dg) * a; db += (b - db) * a;
    dst = 0xFF000000u | ((uint32_t)db << 16) | ((uint32_t)dg << 8) | (uint32_t)dr;
}
static void tri(const ImDrawVert& a, const ImDrawVert& b, const ImDrawVert& c, const ImVec4& clip) {
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
            int tx = std::min(texW - 1, std::max(0, (int)(u * texW))), ty = std::min(texH - 1, std::max(0, (int)(v * texH)));
            float ta = texPix[(ty * texW + tx) * 4 + 3] / 255.f;
            blend(fb[(size_t)y * W + x], (ca.x * w0 + cb.x * w1 + cc.x * w2) * 255, (ca.y * w0 + cb.y * w1 + cc.y * w2) * 255, (ca.z * w0 + cb.z * w1 + cc.z * w2) * 255,
                  (ca.w * w0 + cb.w * w1 + cc.w * w2) * ta);
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
            for (unsigned i = 0; i + 2 < cmd.ElemCount; i += 3) tri(vtx[idx[i]], vtx[idx[i + 1]], vtx[idx[i + 2]], clip);
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
    raw.reserve((size_t)(W * 3 + 1) * H);
    for (int y = 0; y < H; y++) { raw.push_back(0); for (int x = 0; x < W; x++) { uint32_t p = fb[(size_t)y * W + x]; raw.push_back(p & 255); raw.push_back((p >> 8) & 255); raw.push_back((p >> 16) & 255); } }
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
    for (uint32_t v : {(uint32_t)W, (uint32_t)H}) for (int i = 3; i >= 0; i--) ih.push_back((v >> (i * 8)) & 255);
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
static bool click(const std::string& key, bool dbl = false) {
    frames(2);
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
    std::string font = argc > 2 ? argv[2] : "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf";
    if (argc > 4) { W = atoi(argv[3]); H = atoi(argv[4]); }
    float scale = argc > 5 ? (float)atof(argv[5]) : 1.f;
    fb.assign((size_t)W * H, 0);
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = ImVec2((float)W, (float)H);
    if (!io.Fonts->AddFontFromFileTTF(font.c_str(), std::floor(15.f * scale))) io.Fonts->AddFontDefault();
    io.Fonts->GetTexDataAsRGBA32(&texPix, &texW, &texH);
    io.Fonts->SetTexID((ImTextureID)1);
    App a;
    app = &a;
    a.S = scale;
    a.theme();
    a.saveDir = outDir + "/saves";
    { std::ifstream f("manual.txt"); if (f) a.manual.assign((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>()); }
    App::recordRects = true;
    Sim& s = a.sim;

    shot("00-title");
    click("Begin a new career");
    shot("01-new-career");
    click("Begin - London, 2 September 1968");
    check(a.inGame && s.g.phase == PH_PLAY && s.g.day == 0, "new career starts in London on day 0");
    // a fixed world, so that the pictures are repeatable
    s.newCareer(11, "Sokolov");
    click("cable:0");
    shot("02-centre");
    const char* tabs[] = {"Day Planner", "Contacts & Agents", "Sites & Routes", "Material & Stores", "Centre", "Operations Report", "Station", "Surveillance Log", "Accounts", "Career", "Manual"};
    for (auto t : tabs) check(click(std::string("tab:") + t), std::string("tab opens: ") + t);

    // ---- give an order through the form
    click("tab:Day Planner");
    click("plan:6:1", true);
    check(onScreen("combo:##type"), "double-click on a planner block opens the order form");
    shot("03-order-default");
    click("combo:##type");
    click(std::string("opt:") + OTYPE_LABEL[O_SCOUT]);
    shot("04-order-scout");
    click("OK");
    check(s.isSet(6, 1) && s.order(6, 1).type == O_SCOUT, "order stored: " + s.describe(s.order(6, 1)));
    // ---- an illegal order is refused
    click("plan:1:0");
    click("Edit order...");
    click("combo:##type");
    click(std::string("opt:") + OTYPE_LABEL[O_FUNCTION]);
    click("OK");
    check(onScreen("combo:##type") && !s.isSet(1, 0), "a function in the morning is refused and the form stays open");
    shot("05-order-refused");
    click("Cancel");
    // ---- repeat, copy, paste
    click("plan:6:1");
    click("Repeat weekly x3");
    check(s.order(13, 1).type == O_SCOUT && s.order(27, 1).type == O_SCOUT, "repeat weekly fills the next three weeks");
    click("Copy"); click("plan:2:2"); click("Paste");
    check(s.order(2, 2).type == O_SCOUT, "copy and paste");
    click("Back to routine");
    check(!s.isSet(2, 2), "back to routine");
    // ---- the agreed meeting: open its agenda
    click("plan:5:2", true);
    shot("06-order-agent-meeting");
    click("OK");
    // ---- end days
    for (int i = 0; i < 6; i++) { click("END DAY >"); if (onScreen("OK##note")) click("OK##note"); }
    check(s.g.day == 6, fstr("six presses of END DAY -> day %d (%s)", s.g.day, fmtDay(s.g.day).c_str()));
    click("tab:Operations Report");
    click("< Earlier");
    shot("07-report");
    click("tab:Contacts & Agents");
    for (auto& p : s.g.persons) if (p.stage == AGENT) { click("person:" + p.name); break; }
    shot("08-dossier");
    click("Request trace");
    check(onScreen("OK##note"), "a staff action answers with a note");
    click("OK##note");
    // ---- let the test player work for five months, then look at every screen
    play(150);
    const char* files[] = {"10-planner", "11-contacts", "12-sites", "13-material", "14-centre", "15-report", "16-station", "17-surveillance", "18-accounts", "19-career", "20-manual"};
    for (int i = 0; i < 11 && s.g.phase == PH_PLAY; i++) {
        click(std::string("tab:") + tabs[i]);
        if (i == 1) for (auto& p : s.g.persons) if (p.stage == AGENT) { click("person:" + p.name); break; }
        if (i == 4) click("cable:0");
        shot(files[i]);
    }
    // ---- save and load through the screens
    if (s.g.phase == PH_PLAY) {
        int day = s.g.day;
        click("Save"); shot("21-save"); click("Save##do");
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
        click(s.g.interludeOpts[0]);
        check(s.g.phase == PH_PLAY && s.g.theatre == posting, std::string("interlude choice starts the posting in ") + s.T().city);
        click("cable:0");
        shot(fstr("4%d-arrival", posting));
        play(200);
        if (s.g.phase != PH_PLAY) { shot(fstr("4%d-ended-early", posting)); continue; }
        if (!s.g.decisions.empty()) { click("tab:Station"); shot(fstr("4%d-decision", posting)); click("decision:0"); }
        click("tab:Station");
        for (auto& o : s.g.officers) if (o.role == ROLE_SUB && !o.gone) { click("officer:" + o.name); break; }
        shot(fstr("4%d-station", posting));
        click("tab:Contacts & Agents");
        for (auto& p : s.g.persons) if (p.stage == AGENT && p.handler != 0) { click("person:" + p.name); if (onScreen("Instruct...")) { click("Instruct..."); shot(fstr("4%d-instruct", posting)); click("Cancel##instr"); } break; }
        shot(fstr("4%d-contacts", posting));
        click("tab:Day Planner");
        shot(fstr("4%d-planner", posting));
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
    click("Load a saved game...");
    shot("51-load");
    printf("%d failure(s)\n", fails);
    return fails ? 1 : 0;
}
