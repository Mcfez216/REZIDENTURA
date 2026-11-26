#!/bin/sh
# Linux: builds the two test programs. Neither needs a display.
#   build/bot      plays whole careers with no screen and prints how they went
#   build/uitest   runs the real screens through a software rasteriser, clicks through them by script,
#                  and writes PNG screenshots
#   ./build_test.sh bot    builds only the first
set -e
cd "$(dirname "$0")"
mkdir -p build
CXX="${CXX:-g++}"
FLAGS="-std=c++17 -O2 -g -Wall -Wextra -Wno-unused-parameter -Isrc -Ithird_party/imgui"
HDRS="src/core.h src/sim.h src/content.h src/ui.h src/bot.h src/gfx.h src/art.h src/records.h src/skin.h"
obj() {  # obj <source> <object>
  stale=0
  [ -f "$2" ] || stale=1
  for h in "$1" $HDRS; do [ -f "$h" ] && [ "$h" -nt "$2" ] && stale=1; done
  if [ $stale = 1 ]; then echo "  compiling $1"; $CXX $FLAGS -c "$1" -o "$2"; fi
}
SIMSRC="content sim_day sim_night sim_career sim_spy sim_tutorial sim_life sim_world sim_events sim_office sim_trade sim_text"
for f in $SIMSRC bot; do obj src/$f.cpp build/$f.o; done
SIM=""; for f in $SIMSRC; do SIM="$SIM build/$f.o"; done
$CXX -o build/bot $SIM build/bot.o
echo built build/bot
[ "$1" = "bot" ] && exit 0
for f in art gfx skin ui ui_tabs ui_extra ui_launch main_test; do obj src/$f.cpp build/$f.o; done
for f in imgui imgui_draw imgui_tables imgui_widgets; do
  { [ -f build/$f.o ] && [ ! third_party/imgui/$f.cpp -nt build/$f.o ] && [ ! third_party/imgui/imgui.h -nt build/$f.o ]; } || { echo "  compiling $f.cpp"; $CXX -std=c++17 -O2 -Ithird_party/imgui -c third_party/imgui/$f.cpp -o build/$f.o; }
done
$CXX -o build/uitest $SIM build/art.o build/gfx.o build/skin.o build/ui.o build/ui_tabs.o build/ui_extra.o build/ui_launch.o build/main_test.o build/imgui.o build/imgui_draw.o build/imgui_tables.o build/imgui_widgets.o
echo built build/uitest
