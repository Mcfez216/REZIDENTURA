@echo off
rem Rebuilds Rezidentura.exe from the source in this folder.
rem Works from a "Developer Command Prompt for Visual Studio" (cl) or with MinGW-w64 (g++) on the PATH.
cd /d "%~dp0"
set SRC=src\content.cpp src\sim_day.cpp src\sim_night.cpp src\sim_career.cpp src\sim_text.cpp src\ui.cpp src\main_win32.cpp
set IMGUI=third_party\imgui\imgui.cpp third_party\imgui\imgui_draw.cpp third_party\imgui\imgui_tables.cpp third_party\imgui\imgui_widgets.cpp third_party\imgui\backends\imgui_impl_win32.cpp third_party\imgui\backends\imgui_impl_dx11.cpp
set INC=-Isrc -Ithird_party\imgui -Ithird_party\imgui\backends

where cl >nul 2>nul
if not errorlevel 1 goto msvc
where g++ >nul 2>nul
if not errorlevel 1 goto mingw
echo No C++ compiler found.
echo Either open the "Developer Command Prompt for Visual Studio" and run this file from there,
echo or install MinGW-w64 (for example from winlibs.com) and put its bin folder on the PATH.
pause
exit /b 1

:msvc
if not exist build mkdir build
cl /nologo /std:c++17 /O2 /EHsc /utf-8 /DNDEBUG /DUNICODE /D_UNICODE /D_CRT_SECURE_NO_WARNINGS %INC% %SRC% %IMGUI% /Fobuild\ /Fe:Rezidentura.exe /link /SUBSYSTEM:WINDOWS d3d11.lib d3dcompiler.lib dxgi.lib dwmapi.lib user32.lib gdi32.lib imm32.lib shell32.lib
if errorlevel 1 goto failed
goto ok

:mingw
g++ -std=c++17 -O2 -DNDEBUG -DUNICODE -D_UNICODE %INC% %SRC% %IMGUI% -o Rezidentura.exe -municode -mwindows -ld3d11 -ld3dcompiler -ldxgi -ldwmapi -lgdi32 -limm32 -static
if errorlevel 1 goto failed

:ok
echo.
echo Built Rezidentura.exe
pause
exit /b 0

:failed
echo.
echo BUILD FAILED - read the messages above.
pause
exit /b 1
