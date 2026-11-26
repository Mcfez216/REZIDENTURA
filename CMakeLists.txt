# Rezidentura - build with CMake (Visual Studio, MinGW, or Linux for the test programs).
#   cmake -B build -DCMAKE_BUILD_TYPE=Release
#   cmake --build build --config Release
cmake_minimum_required(VERSION 3.16)
project(Rezidentura CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SIM src/content.cpp src/sim_day.cpp src/sim_night.cpp src/sim_career.cpp src/sim_spy.cpp src/sim_tutorial.cpp src/sim_life.cpp src/sim_world.cpp src/sim_events.cpp src/sim_office.cpp src/sim_trade.cpp src/sim_text.cpp src/sim_deputy.cpp src/sim_ops.cpp)
set(UI src/art.cpp src/gfx.cpp src/skin.cpp src/ui.cpp src/ui_tabs.cpp src/ui_extra.cpp src/ui_launch.cpp)
set(IMGUI third_party/imgui/imgui.cpp third_party/imgui/imgui_draw.cpp third_party/imgui/imgui_tables.cpp third_party/imgui/imgui_widgets.cpp)
include_directories(src third_party/imgui third_party/imgui/backends)
if(MSVC)
  add_compile_options(/utf-8 /W3)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

if(WIN32)
  # the game
  add_executable(Rezidentura WIN32 ${SIM} ${UI} src/main_win32.cpp src/rezidentura.rc ${IMGUI}
                 third_party/imgui/backends/imgui_impl_win32.cpp third_party/imgui/backends/imgui_impl_dx11.cpp)
  target_compile_definitions(Rezidentura PRIVATE UNICODE _UNICODE)
  target_link_libraries(Rezidentura d3d11 d3dcompiler dxgi dwmapi imm32 shell32 ole32)
  if(MINGW)
    target_link_options(Rezidentura PRIVATE -municode -static)
  endif()
endif()

# the test player with no screen:   bot 100 careful
add_executable(bot ${SIM} src/bot.cpp)
# the scripted interface test (writes PNG screenshots):   uitest <output folder>
add_executable(uitest ${SIM} ${UI} src/main_test.cpp ${IMGUI})

# the typefaces travel with the program
if(WIN32)
  add_custom_command(TARGET Rezidentura POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/fonts $<TARGET_FILE_DIR:Rezidentura>/fonts
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/manual.txt $<TARGET_FILE_DIR:Rezidentura>/manual.txt
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/Officer's Handbook.pdf" "$<TARGET_FILE_DIR:Rezidentura>/Officer's Handbook.pdf")
endif()
