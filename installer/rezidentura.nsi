; rezidentura.nsi - the Windows installer (NSIS 3).
;   Linux:   makensis installer/rezidentura.nsi      (build_win.sh does this for you)
;   Windows: install NSIS from nsis.sourceforge.io, then build.bat does it, or right-click this file > Compile NSIS Script
; The game is installed for the current user only, so no administrator rights are needed:
;   program    %LOCALAPPDATA%\Programs\Rezidentura
;   saves      %USERPROFILE%\Saved Games\Rezidentura   (written by the game itself)

Unicode true
Target amd64-unicode                ; the game is 64-bit only, and so is its installer
!include "MUI2.nsh"
!include "FileFunc.nsh"

!define APP        "Rezidentura"
!define VERSION    "1.6.0"
!define PUBLISHER  "Rezidentura"
!define UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP}"

Name "${APP}"
Caption "${APP} ${VERSION} Setup"
OutFile "..\dist\Rezidentura-Setup.exe"
InstallDir "$LOCALAPPDATA\Programs\${APP}"
InstallDirRegKey HKCU "Software\${APP}" "InstallDir"
RequestExecutionLevel user
SetCompressor /SOLID lzma
BrandingText "${APP} ${VERSION}"
ShowInstDetails nevershow
ShowUninstDetails nevershow

VIProductVersion "1.6.0.0"
VIAddVersionKey /LANG=1033 "ProductName" "${APP}"
VIAddVersionKey /LANG=1033 "FileDescription" "${APP} Setup"
VIAddVersionKey /LANG=1033 "FileVersion" "${VERSION}"
VIAddVersionKey /LANG=1033 "ProductVersion" "${VERSION}"
VIAddVersionKey /LANG=1033 "CompanyName" "${PUBLISHER}"
VIAddVersionKey /LANG=1033 "LegalCopyright" "Typefaces under the SIL Open Font License; Dear ImGui under the MIT licence."

; ---------------------------------------------------------------- the look
!define MUI_ICON "rezidentura.ico"
!define MUI_UNICON "rezidentura.ico"
!define MUI_WELCOMEFINISHPAGE_BITMAP "welcome.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "welcome.bmp"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_RIGHT
!define MUI_HEADERIMAGE_BITMAP "header.bmp"
!define MUI_HEADERIMAGE_UNBITMAP "header.bmp"
!define MUI_ABORTWARNING

!define MUI_WELCOMEPAGE_TITLE "Rezidentura"
!define MUI_WELCOMEPAGE_TEXT "A KGB case officer's career, 1968-1991.$\r$\n$\r$\nThis will install Rezidentura for your Windows account. No administrator rights are needed.$\r$\n$\r$\nYour saved games are kept in your Saved Games folder, and are not touched when you install a new version.$\r$\n$\r$\nClick Next to continue."
!define MUI_FINISHPAGE_TITLE "Rezidentura is installed"
!define MUI_FINISHPAGE_TEXT "Start it from the Start menu$\r$\n(or the desktop, if you chose the shortcut).$\r$\n$\r$\nFor a first game, tick the training course when you begin a new career."
!define MUI_FINISHPAGE_RUN "$INSTDIR\Rezidentura.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Start Rezidentura now"
!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\Officer's Handbook.pdf"
!define MUI_FINISHPAGE_SHOWREADME_TEXT "Open the Officer's Handbook"
!define MUI_FINISHPAGE_SHOWREADME_NOTCHECKED

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

; ---------------------------------------------------------------- the game is not running while files are replaced
!macro CHECK_RUNNING
  check:
    FindWindow $0 "RezidenturaWindow"
    StrCmp $0 0 notRunning
    MessageBox MB_RETRYCANCEL|MB_ICONEXCLAMATION "Rezidentura is running. Close it (it saves itself), then click Retry." /SD IDCANCEL IDRETRY check
    Abort
  notRunning:
!macroend

Function .onInit
  !insertmacro CHECK_RUNNING
FunctionEnd

Function un.onInit
  !insertmacro CHECK_RUNNING
FunctionEnd

; ---------------------------------------------------------------- what is installed
Section "Rezidentura (required)" SecMain
  SectionIn RO
  SetOutPath "$INSTDIR"
  File "..\dist\Rezidentura.exe"
  File "..\manual.txt"
  File "..\Officer's Handbook.pdf"
  File "..\README.txt"
  SetOutPath "$INSTDIR\fonts"
  File "..\fonts\*.ttf"
  File "..\fonts\OFL-*.txt"
  SetOutPath "$INSTDIR"

  WriteUninstaller "$INSTDIR\Uninstall.exe"

  ; the Start menu
  CreateDirectory "$SMPROGRAMS\${APP}"
  CreateShortcut "$SMPROGRAMS\${APP}\${APP}.lnk" "$INSTDIR\Rezidentura.exe" "" "$INSTDIR\Rezidentura.exe" 0 SW_SHOWNORMAL "" "A KGB case officer's career, 1968-1991"
  CreateShortcut "$SMPROGRAMS\${APP}\Officer's Handbook.lnk" "$INSTDIR\Officer's Handbook.pdf"
  CreateShortcut "$SMPROGRAMS\${APP}\Uninstall ${APP}.lnk" "$INSTDIR\Uninstall.exe"

  ; Settings > Apps (and Programs and Features)
  WriteRegStr HKCU "Software\${APP}" "InstallDir" "$INSTDIR"
  WriteRegStr HKCU "${UNINST_KEY}" "DisplayName" "${APP}"
  WriteRegStr HKCU "${UNINST_KEY}" "DisplayVersion" "${VERSION}"
  WriteRegStr HKCU "${UNINST_KEY}" "Publisher" "${PUBLISHER}"
  WriteRegStr HKCU "${UNINST_KEY}" "DisplayIcon" "$INSTDIR\Rezidentura.exe,0"
  WriteRegStr HKCU "${UNINST_KEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKCU "${UNINST_KEY}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
  WriteRegStr HKCU "${UNINST_KEY}" "QuietUninstallString" '"$INSTDIR\Uninstall.exe" /S'
  WriteRegDWORD HKCU "${UNINST_KEY}" "NoModify" 1
  WriteRegDWORD HKCU "${UNINST_KEY}" "NoRepair" 1
  ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
  IntFmt $0 "0x%08X" $0
  WriteRegDWORD HKCU "${UNINST_KEY}" "EstimatedSize" "$0"
SectionEnd

Section "Desktop shortcut" SecDesktop
  CreateShortcut "$DESKTOP\${APP}.lnk" "$INSTDIR\Rezidentura.exe" "" "$INSTDIR\Rezidentura.exe" 0 SW_SHOWNORMAL "" "A KGB case officer's career, 1968-1991"
SectionEnd

LangString DESC_Main ${LANG_ENGLISH} "The game, the Officer's Handbook, and the typefaces."
LangString DESC_Desktop ${LANG_ENGLISH} "A shortcut on your desktop."
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecMain} $(DESC_Main)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecDesktop} $(DESC_Desktop)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

; ---------------------------------------------------------------- removing it
Section "Uninstall"
  Delete "$INSTDIR\Rezidentura.exe"
  Delete "$INSTDIR\manual.txt"
  Delete "$INSTDIR\Officer's Handbook.pdf"
  Delete "$INSTDIR\README.txt"
  Delete "$INSTDIR\fonts\*.ttf"
  Delete "$INSTDIR\fonts\OFL-*.txt"
  RMDir "$INSTDIR\fonts"
  Delete "$INSTDIR\Uninstall.exe"
  RMDir "$INSTDIR"

  Delete "$SMPROGRAMS\${APP}\${APP}.lnk"
  Delete "$SMPROGRAMS\${APP}\Officer's Handbook.lnk"
  Delete "$SMPROGRAMS\${APP}\Uninstall ${APP}.lnk"
  RMDir "$SMPROGRAMS\${APP}"
  Delete "$DESKTOP\${APP}.lnk"

  DeleteRegKey HKCU "${UNINST_KEY}"
  DeleteRegKey HKCU "Software\${APP}"

  ; saved games are the player's: kept unless he says otherwise
  IfFileExists "$PROFILE\Saved Games\${APP}\*.*" 0 done
    MessageBox MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2 "Also delete your saved games?$\r$\n$\r$\n$PROFILE\Saved Games\${APP}" /SD IDNO IDNO done
    RMDir /r "$PROFILE\Saved Games\${APP}"
  done:
SectionEnd
