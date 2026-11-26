// ui.h - the screens. Everything is drawn with Dear ImGui and is the same on every platform; the
// platform file (main_win32.cpp) opens the window, supplies textures, and calls App::frame().
//
//   ui.cpp       the frame, the title screen, the header and the folder tabs, popups, the order form
//   ui_tabs.cpp  the desk screens: planner, files, map, sites, material, Centre, report ...
//   ui_extra.cpp the conveniences: windows lifted off the desk, settings, keys, the checklist and notebook, the handbook
#pragma once
#define REZ_VERSION "1.6"
#include "gfx.h"
#include "skin.h"
#include "sim.h"
#include <array>
#include <functional>
#include <set>

extern const char* const ORDER_HINT[N_OTYPE];     // one line on each kind of order (ui.cpp)
#include <map>

/** What the player has chosen about the program itself (kept in settings.ini beside the saved games, not in a save). */
struct Settings {
    bool multiMonitor = true;                 // windows may be dragged out of the main window, on to another screen
    bool rememberWindows = true;              // where the windows were is remembered between sessions
    float uiScale = 1.0f;                     // size of everything; takes effect when the game is next started
    bool fullscreen = false;
    bool monthlyBackups = true;               // keep the autosaves of the last three months as well
    bool runStopMeeting = true, runStopCable = false, runStopAlert = true;     // what stops "Run to the next event"
    int runMaxDays = 30;
    bool jumpAfterDay = true;                 // after End Day, turn to the report or the cables by itself
    bool tooltips = true;
    bool toasts = true;                       // after each day, short notices of what happened
    bool streetChoices = true;
    bool scenes = true;                       // play your own long meetings and recruitment approaches as scenes                // decide yourself when a meeting or a drop meets surveillance
    bool confirmEndDay = false;               // ask before ending a day on which something urgent is still undone
};

/** One finished career, as read back from service-records.txt. */
struct Record {
    std::string officer, rank, seed, ended, ending, postings;
    int background = 0, start = 0, difficulty = 1, recruited = 0, awards = 0;
    bool ironman = false;
    std::vector<int> cites;
};

struct App {
    Sim sim;
    Settings cfg;
    bool viewportsOk = false;                 // the platform can put windows outside the main one
    std::string iniPath;                      // where window positions are kept
    std::string pdfPath;                      // the handbook as a PDF, if it is installed
    std::function<void(const std::string&)> openFile;   // open a file with whatever the system uses for it
    void loadSettings();
    void saveSettings();
    bool inGame = false, wantQuit = false, wantMinimise = false;
    // the launcher: the small window the game opens in (ui_launch.cpp)
    ImVec2 launcherSize() const;
    bool launcherDragAt(float x, float y) const;   // may the window be dragged from this point (it has no frame)?
    void recordCareer();                      // enter a finished career in the service records
    std::string saveDir = "saves";
    std::string manual = "manual.txt was not found beside the program.";
    float S = 1;                              // size of everything, relative to the design size

    void theme();                             // colours and spacing; call once after the fonts are loaded
    void frame();                             // draw one frame

    bool saveTo(const std::string& path);
    bool loadFrom(const std::string& path);
    void autosave();
    void doEndDay(bool run);

    enum { T_PLAN, T_NOTES, T_PEOPLE, T_MAP, T_SITES, T_BOARD, T_MAT, T_CENTRE, T_REPORT, T_STATION, T_SURV, T_ACC, T_STATS, T_HOME, T_CAREER, T_MANUAL, N_TAB };
    int tab = T_PLAN;
    bool popped[N_TAB] = {};                  // which screens have been lifted off the desk into windows of their own
    void popOut(int t, bool out);
    void resetWindows();
    void closeWindows();

    // --- for the automated UI test: where each named widget was drawn last frame
    static bool recordRects;
    static std::map<std::string, std::array<float, 4>> rects;

    void showMoment(const char* ru, const char* en, unsigned ink);     // for the tests: bring a stamp down now
private:
    int wantTab = -1;
    int selRow = 0, selCol = 1;               // planner cell
    int selPerson = -1, selSite = -1, selRoll = -1, selReq = -1, selCable = -1, selOfficer = -1, selLog = -1, selDistrict = -1;
    int reportDay = -1;
    int calDay = -1, tornDay = -1;            // the calendar page torn off when a day ends
    double tornT = -10;
    bool showClosed = false, showRetired = false, cardOpen = true, showRoutine = true;
    Order copied;
    bool hasClip = false;

    // popups
    std::string note;
    std::string confirmText;
    std::function<void()> confirmAction;
    bool openEditor = false, openNew = false, openSave = false, openLoad = false, openInstr = false;
    int edDay = 0, edBlock = 0;
    std::set<long long> prefilled;           // agreed meetings whose agenda has been filled in this session
    Order ed;
    std::string edError;
    char saveBuf[40] = "posting";
    int newDifficulty = 1;
    std::vector<std::string> loadList;
    int instrPerson = -1, instrTempo = -1, instrReq = -2;
    bool instrCallout = false;
    bool openFlat = false;
    int flatDistrict = -1;
    bool openKomp = false;
    int kompPerson = -1, kompKind = 0;
    int leaveWeeks = 3, wifeSite = -1, skimSel = 0;
    // conveniences (ui_extra.cpp)
    bool openSettings = false, resetPending[N_TAB] = {};
    int focusTab = -1;
    bool sureEndDay = false;
    std::string filter;                       // the card index, filtered
    std::string manualFind;
    int manualSection = -1;
    int renamePerson = -1, notePerson = -1;
    std::string renameBuf;
    int edRepeat = 0;
    std::string lastQuick;
    struct Toast { std::string text; int tab; double t0; bool cable; };
    std::vector<Toast> toastList;
    void pushToasts(int fromDay, size_t cablesBefore);
    void drawToasts();
    // a rubber stamp brought down across the desk for the moments that matter: a recruitment, a man brought out, a promotion
    struct Moment { std::string ru, en; unsigned ink = 0; double t0 = -100; };
    Moment moment;
    void drawMoment();
    void drawTab(int t);
    void poppedWindows();
    void settingsPopup(ImVec2 centre);
    void extraPopups(ImVec2 centre);
    void shortcuts();
    void tabNotes();
    void rotateBackups();

    // the launcher (ui_launch.cpp)
    int page = 0;                             // 0 home, 1 a new career, 2 load, 3 service records
    bool peekDone = false, peekOk = false, lastInGame = false;
    std::string peekLine1, peekLine2;
    std::string newName = "Sokolov", newSeed;
    int newStart = 0, newBackground = 0;
    bool newIronman = false;
    std::vector<std::string> loadInfo;
    std::vector<Record> archive;
    bool archiveRead = false;
    void peekAutosave();
    void readArchive();
    std::vector<bool> citationsEarned();
    void launchHome();
    void launchNew();
    void launchLoad();
    void launchRecords();

    // screens (ui.cpp)
    void titleScreen();
    void gameScreen();
    void interludeScreen();
    void overScreen();
    void header(ImVec2 a, ImVec2 b);
    void nav(ImVec2 a, ImVec2 b);
    void popups();
    void orderEditor();
    void beginEdit(int day, int block);
    std::string place(Order o, int day, int block);
    void resetSelection();

    // desk screens (ui_tabs.cpp)
    void tabPlanner();
    void tabContacts();
    void tabMap();
    void tabSites();
    void tabMaterial();
    void tabCentre();
    void tabReport();
    void tabStation();
    void tabSurveillance();
    void tabAccounts();
    void tabReturns();
    void tabPrivate();
    void vehicleCards(float width, float height);
    void tabCareer();
    void tabManual();
    void dossierView(const Person& p);
    void officerView(const Officer& o);
    void cableView(const Cable& c, int index);
    void drawMap(ImVec2 a, ImVec2 b);
    void instructorCard(float width);

    // documents and pictures
    /** A panel on the desk: a dark frame with brass trim and, if `title` is given, a title bar. A paper colour
        (C_PAPER, C_TELETYPE, ...) puts a sheet of that paper inside the frame, for documents; C_CARD and
        C_LEDGER make a dark register. The content goes in a child window; close it with docEnd(). */
    bool docPanel(const char* id, ImVec2 size, unsigned colour, float pad, const char* title, const char* hint = nullptr, ImGuiWindowFlags flags = 0);
    bool docBegin(const char* id, ImVec2 size, unsigned colour, float pad = 14, ImGuiWindowFlags flags = 0) { return docPanel(id, size, colour, pad, nullptr, nullptr, flags); }
    void docEnd();
    struct DocFrame { ImVec2 a, size; int colours; };
    std::vector<DocFrame> docStack;
    void docTitle(const char* ru, const char* title, const char* hint = nullptr);
    int era() const;
    ImTextureID facePhoto(const Person& p);
    void tabBoard();                          // the case board: who knows whom (ui_tabs.cpp)
    int boardFocus = -1;                      // card the pointer last rested on (key: kind * 1000000 + id)
    ImTextureID candidPhoto(const Person& p);
    ImTextureID officerPhoto(const Officer& o);
    ImTextureID myPhoto();
    ImTextureID sitePhoto(const Site& s);
    ImTextureID scenePhoto(int key, int scene);
};

// ---------------------------------------------------------------------------- widgets shared by ui.cpp and ui_tabs.cpp
void uiRec(const std::string& key);                       // remember where the last item was drawn (for the UI test)
bool uiBtn(const char* label, ImVec2 size = ImVec2(0, 0));
/** The one button on a screen that matters most: filled, in red. */
bool uiPrimary(const char* label, ImVec2 size = ImVec2(0, 0));
bool uiCheck(const char* label, bool* v);
/** A dialogue: a framed modal with a title bar of its own (the part of `name` before "##"). End it with uiEndModal(). */
bool uiModal(const char* name, bool* open = nullptr, ImGuiWindowFlags flags = 0, const char* title = nullptr);
void uiEndModal();
/** A heading inside a panel: letter-spaced capitals in brass, and a rule. */
void uiSection(const char* title);
/** A table's header row, in the panel style. */
void uiHeaders();
/** True when the paper colour is a document (a sheet inside the frame) rather than a dark register. */
bool isPaper(unsigned colour);
struct Opt { std::string label; int value; };
bool uiCombo(const char* label, int& value, const std::vector<Opt>& opts, float width = 0);
/** A text box that edits a std::string (one line, or several if a height is given). */
bool uiText(const char* id, std::string& s, ImVec2 size = ImVec2(0, 0), bool multiline = false, const char* hint = nullptr);
/** How one line of typed text is to look: its colour and font may be changed. */
typedef std::function<void(const std::string& line, ImU32& col, ImFont*& f)> LineStyle;
/** Typewritten text, wrapped to the width available, laid out as ImGui items so that it scrolls. */
void uiTyped(const std::string& text, ImU32 col, ImFont* f = nullptr, const LineStyle& style = nullptr, uint32_t seed = 1);
/** A slip of paper that works as a button: a Russian word, and the English under it. */
bool uiSlip(const char* id, const char* ru, const char* en, ImVec2 size, float S, unsigned paper = gfx::C_CARD, ImU32 ink = 0);
