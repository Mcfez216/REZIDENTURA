#!/bin/sh
# Linux: builds the two test programs. Neither needs a display.
#   build/bot      plays whole careers with no screen and prints how they went
#   build/uitest   runs the real screens through a software rasteriser, clicks through them by script,
#                  and writes PNG screenshots
set -e
cd "$(dirname "$0")"
mkdir -p build
CXX="${CXX:-g++}"
FLAGS="-std=c++17 -O2 -g -Wall -Wextra -Wno-unused-parameter -Isrc -Ithird_party/imgui"
obj() {  # obj <source> <object>
  if [ ! -f "$2" ] || [ "$1" -nt "$2" ] || [ src/core.h -nt "$2" ] || [ src/sim.h -nt "$2" ] || [ src/content.h -nt "$2" ] || [ src/ui.h -nt "$2" ] || [ src/bot.h -nt "$2" ]; then
    echo "  compiling $1"; $CXX $FLAGS -c "$1" -o "$2"
  fi
}
for f in content sim_day sim_night sim_career sim_text bot ui main_test; do obj src/$f.cpp build/$f.o; done
for f in imgui imgui_draw imgui_tables imgui_widgets; do
  [ -f build/$f.o ] || { echo "  compiling $f.cpp"; $CXX -std=c++17 -O2 -Ithird_party/imgui -c third_party/imgui/$f.cpp -o build/$f.o; }
done
SIM="build/content.o build/sim_day.o build/sim_night.o build/sim_career.o build/sim_text.o"
$CXX -o build/bot $SIM build/bot.o
$CXX -o build/uitest $SIM build/ui.o build/main_test.o build/imgui.o build/imgui_draw.o build/imgui_tables.o build/imgui_widgets.o
echo built build/bot build/uitest
