REZIDENTURA
A KGB case officer's career, 1968-1991. Windows 10 / 11, 64-bit.

TO PLAY
  Double-click Rezidentura.exe. Nothing to install.
  It needs DirectX 11, which every Windows 10 / 11 PC has. On a machine with no graphics
  card (a virtual machine, a remote desktop) it falls back to Windows' software renderer.
  The game autosaves every night into the "saves" folder beside the program.
  Keep manual.txt beside the program: it is what the Manual tab shows. Read section 14
  ("Your first week") before pressing END DAY.

THE CAREER
  1968-1971  London ......... case officer
  1973-1977  Vienna ......... senior case officer, one officer under you
  1979-1984  Washington ..... head of line, three officers
  1986-1991  Bonn ........... Rezident, five officers
  August 1991, Moscow.
  Being expelled ends a posting, not the game: the career goes on (up to a point).

WHAT YOU MANAGE
  - Your days: three blocks a day, planned five weeks ahead.
  - People whose real qualities you never see: spotting, assessing, developing, recruiting.
  - Agents: tempo, tasking, pay, cameras and film, their own tradecraft, their nerves, and an
    investigation at their office that you only hear about second-hand.
  - Communications plans: meeting rotations, fall-backs, dead drops, signal sites, call-outs,
    one-way radio with one-time pads.
  - Surveillance you can only infer: short checks, prepared detection routes that wear out,
    countersurveillance teams, abort policies, burned and staked-out sites.
  - Provocations: dangles, walk-ins, fabricators, agents who are caught and turned.
  - The chain to Moscow: cassette -> drop or meeting -> lab -> Thursday pouch -> evaluation.
  - Cipher traffic rationed in groups a day; stores with lead times; money to the last pound.
  - Service A (active measures) and Line N (illegals support) tasks.
  - Other officers: who has seen which agent and which site; defectors; a possible traitor in
    the station; the marked-card test; reports to Directorate K.
  - Your own officers: assignments, cases, orders, and their mistakes.
  - The real calendar of 1968-1991 in the background.

TO CHANGE THE GAME
  The full source is in this folder. Edit, then run build.bat (Visual Studio or MinGW-w64),
  or use CMake (CMakeLists.txt).

    src\content.cpp ..... the four postings: districts, streets, names, jobs, requirements,
                          historical events. The easiest place to start changing things.
    src\core.h .......... the data only: people, sites, film, orders, officers, the game state,
                          and the save-file format. Fields marked HIDDEN are never shown to the player.
    src\sim_day.cpp ..... the planner, staff actions, and everything that happens in the street.
    src\sim_night.cpp ... your officers' day, the night, the dawn, Centre's answers, history.
    src\sim_career.cpp .. generators, starting and ending a posting, the years at Centre, decisions.
    src\sim_text.cpp .... the texts the screens show (dossier, stores, station, accounts ...).
    src\ui.cpp .......... every screen (Dear ImGui).
    src\main_win32.cpp .. the Windows window and DirectX 11.
    manual.txt .......... the handbook.

  Two test programs build on any system (build_test.sh on Linux, or the CMake targets):
    bot ......... plays whole careers with no screen and prints how they went. Run it after
                  changing a rule:   bot 200 careful      bot 200 reckless
    uitest ...... runs the real screens with no window, clicks through them by script, and
                  writes PNG screenshots:   uitest <output folder>

  third_party\imgui is Dear ImGui 1.91.5 by Omar Cornut and contributors (MIT licence, see
  its LICENSE.txt).

All people in the game are invented. The places and the dates are real.
