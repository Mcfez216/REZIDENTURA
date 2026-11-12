// ui.h - the screens. Everything here is drawn with Dear ImGui and is the same on every platform;
// the platform file (main_win32.cpp) only opens the window and calls App::frame() once per frame.
#pragma once
#include "sim.h"
#include <array>
#include <functional>

struct App {
    Sim sim;
    bool inGame = false, wantQuit = false;
    std::string saveDir = "saves";
    std::string manual = "manual.txt was not found beside the program.";
    float S = 1;                              // size of everything, relative to a 15-pixel font

    void theme();                             // colours and spacing; call once after creating the ImGui context
    void frame();                             // draw one frame

    bool saveTo(const std::string& path);
    bool loadFrom(const std::string& path);
    void autosave();
    void doEndDay(bool run);

    // --- for the automated UI test: where each named widget was drawn last frame
    static bool recordRects;
    static std::map<std::string, std::array<float, 4>> rects;

private:
    // which screen / tab / row
    int wantTab = -1, curTab = 0;
    int selRow = 0, selCol = 1;               // planner cell
    int selPerson = -1, selSite = -1, selRoll = -1, selReq = -1, selCable = -1, selOfficer = -1;
    int reportDay = -1;
    bool showClosed = false, showRetired = false;
    Order copied;
    bool hasClip = false;

    // popups
    std::string note;                         // a message to show
    std::string confirmText;
    std::function<void()> confirmAction;
    bool openEditor = false, openNew = false, openSave = false, openLoad = false, openInstr = false;
    int edDay = 0, edBlock = 0;
    Order ed;
    std::string edError;
    char nameBuf[32] = "Sokolov", seedBuf[16] = "", saveBuf[40] = "posting";
    std::vector<std::string> loadList;
    int instrPerson = -1, instrTempo = -1, instrReq = -2;
    bool instrCallout = false;

    void titleScreen();
    void gameScreen();
    void interludeScreen();
    void overScreen();
    void popups();
    void orderEditor();

    void tabPlanner();
    void tabContacts();
    void tabSites();
    void tabMaterial();
    void tabCentre();
    void tabReport();
    void tabStation();
    void tabSurveillance();
    void tabAccounts();
    void textTab(const char* id, const std::string& text);

    bool tab(const char* label, int idx);
    void beginEdit(int day, int block);
    std::string place(Order o, int day, int block);
};
