#!/bin/sh
# Cross-compiles Rezidentura.exe for 64-bit Windows from Linux, using the Zig toolchain
# (pip install ziglang). On Windows itself use build.bat or CMake instead.
set -e
cd "$(dirname "$0")"
mkdir -p build/win dist
ZIG="python3 -m ziglang"
FLAGS="-target x86_64-windows-gnu -O2 -std=c++17 -DNDEBUG -DUNICODE -D_UNICODE -Isrc -Ithird_party/imgui -Ithird_party/imgui/backends"
OBJS=""
for f in src/content.cpp src/sim_day.cpp src/sim_night.cpp src/sim_career.cpp src/sim_spy.cpp src/sim_tutorial.cpp src/sim_life.cpp src/sim_world.cpp src/sim_events.cpp src/sim_office.cpp src/sim_trade.cpp src/sim_text.cpp \
         src/art.cpp src/gfx.cpp src/skin.cpp src/ui.cpp src/ui_tabs.cpp src/ui_extra.cpp src/ui_launch.cpp src/main_win32.cpp \
         third_party/imgui/imgui.cpp third_party/imgui/imgui_draw.cpp third_party/imgui/imgui_tables.cpp third_party/imgui/imgui_widgets.cpp \
         third_party/imgui/backends/imgui_impl_win32.cpp third_party/imgui/backends/imgui_impl_dx11.cpp; do
  o="build/win/$(basename "$f" .cpp).obj"
  if [ ! -f "$o" ] || [ "$f" -nt "$o" ] || [ src/core.h -nt "$o" ] || [ src/sim.h -nt "$o" ] || [ src/ui.h -nt "$o" ] || [ src/content.h -nt "$o" ] || [ src/gfx.h -nt "$o" ] || [ src/art.h -nt "$o" ] || [ src/skin.h -nt "$o" ] || [ src/records.h -nt "$o" ]; then
    echo "  compiling $f"; $ZIG c++ $FLAGS -c "$f" -o "$o"
  fi
  OBJS="$OBJS $o"
done
# the handbook as a PDF (needs: pip install reportlab pillow); the one in the folder is kept if that is missing
if [ manual.txt -nt "Officer's Handbook.pdf" ] || [ tools/make_handbook.py -nt "Officer's Handbook.pdf" ]; then
  python3 tools/make_handbook.py || echo "the PDF handbook was not rebuilt"
fi
# the icon and version information
( cd src && $ZIG rc /fo ../build/win/rezidentura.res rezidentura.rc )
$ZIG c++ -target x86_64-windows-gnu -O2 $OBJS build/win/rezidentura.res -o dist/Rezidentura.exe -municode -Wl,--subsystem,windows -ld3d11 -ld3dcompiler_47 -ldxgi -ldwmapi -lgdi32 -limm32 -luser32 -lkernel32 -lshell32 -lole32 -static -s
echo built dist/Rezidentura.exe
# the installer (needs NSIS: apt install nsis, or nsis.sourceforge.io on Windows)
if command -v makensis >/dev/null 2>&1; then
  makensis -V2 installer/rezidentura.nsi && echo built dist/Rezidentura-Setup.exe
else
  echo "makensis not found: no installer built"
fi
