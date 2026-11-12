#!/bin/sh
# Cross-compiles Rezidentura.exe for 64-bit Windows from Linux, using the Zig toolchain
# (pip install ziglang). On Windows itself use build.bat or CMake instead.
set -e
cd "$(dirname "$0")"
mkdir -p build/win dist
ZIG="python3 -m ziglang"
FLAGS="-target x86_64-windows-gnu -O2 -std=c++17 -DNDEBUG -DUNICODE -D_UNICODE -Isrc -Ithird_party/imgui -Ithird_party/imgui/backends"
OBJS=""
for f in src/content.cpp src/sim_day.cpp src/sim_night.cpp src/sim_career.cpp src/sim_text.cpp src/ui.cpp src/main_win32.cpp \
         third_party/imgui/imgui.cpp third_party/imgui/imgui_draw.cpp third_party/imgui/imgui_tables.cpp third_party/imgui/imgui_widgets.cpp \
         third_party/imgui/backends/imgui_impl_win32.cpp third_party/imgui/backends/imgui_impl_dx11.cpp; do
  o="build/win/$(basename "$f" .cpp).obj"
  if [ ! -f "$o" ] || [ "$f" -nt "$o" ] || [ src/core.h -nt "$o" ] || [ src/sim.h -nt "$o" ] || [ src/ui.h -nt "$o" ] || [ src/content.h -nt "$o" ]; then
    echo "  compiling $f"; $ZIG c++ $FLAGS -c "$f" -o "$o"
  fi
  OBJS="$OBJS $o"
done
$ZIG c++ -target x86_64-windows-gnu -O2 $OBJS -o dist/Rezidentura.exe -municode -Wl,--subsystem,windows -ld3d11 -ld3dcompiler_47 -ldxgi -ldwmapi -lgdi32 -limm32 -luser32 -lkernel32 -static -s
echo built dist/Rezidentura.exe
