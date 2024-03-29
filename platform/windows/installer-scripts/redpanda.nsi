####################################################################
# Startup
SetFont "Segoe UI" 11
Unicode True
!define DISPLAY_NAME "Red Panda C++ ${APP_VERSION} (${ARCH})"

!include "Integration.nsh"
!include "LogicLib.nsh"
!include "MUI2.nsh"
!include "WinVer.nsh"
!include "x64.nsh"

!include "lang.nsh"
!include "utils.nsh"

!define MUI_CUSTOMFUNCTION_GUIINIT myGuiInit

####################################################################
# Installer Attributes

Name "${DISPLAY_NAME}"
OutFile "${FINALNAME}"
Caption "${DISPLAY_NAME}"

LicenseData "LICENSE"
!if "${ARCH}" == "x86"
  InstallDir $PROGRAMFILES\RedPanda-Cpp
!else
  InstallDir $PROGRAMFILES64\RedPanda-Cpp
!endif
####################################################################
# Interface Settings

ShowInstDetails show
AutoCloseWindow false
SilentInstall normal
CRCCheck on
SetCompressor /SOLID /FINAL lzma
SetCompressorDictSize 64
SetDatablockOptimize on
SetOverwrite try
XPStyle on

ManifestDPIAware true

InstType "Full";1
InstType "Minimal";2
InstType "Safe";3

## Remember the installer language
!define MUI_LANGDLL_REGISTRY_ROOT "ShCtx"
!define MUI_LANGDLL_REGISTRY_KEY "Software\RedPanda-C++"
!define MUI_LANGDLL_REGISTRY_VALUENAME "Installer Language"

####################################################################
# Pages

!define MUI_ICON "devcpp.ico"
!define MUI_UNICON "devcpp.ico"
!define MUI_ABORTWARNING
!define MUI_LANGDLL_ALLLANGUAGES
!define MUI_FINISHPAGE_RUN "$INSTDIR\RedPandaIDE.exe"
!define MUI_FINISHPAGE_NOREBOOTSUPPORT
!define MUI_COMPONENTSPAGE_SMALLDESC

!insertmacro MUI_PAGE_LICENSE "LICENSE"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

####################################################################
# Languages

!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "SimpChinese"

####################################################################
# Files, by option section

Section "$(SectionMainName)" SectionMain
  SectionIn 1 2 3 RO
  
  SetOutPath $INSTDIR

  ; Allways create an uninstaller
  WriteUninstaller "$INSTDIR\uninstall.exe"
  WriteRegStr ShCtx "Software\Microsoft\Windows\CurrentVersion\Uninstall\RedPanda-C++" "DisplayName" "Redpanda-C++"
  WriteRegStr ShCtx "Software\Microsoft\Windows\CurrentVersion\Uninstall\RedPanda-C++" "UninstallString" "$INSTDIR\uninstall.exe"
  WriteRegStr ShCtx "Software\Microsoft\Windows\CurrentVersion\Uninstall\RedPanda-C++" "DisplayVersion" "${APP_VERSION}"
  WriteRegStr ShCtx "Software\Microsoft\Windows\CurrentVersion\Uninstall\RedPanda-C++" "DisplayIcon" "$INSTDIR\RedPandaIDE.exe"
  WriteRegStr ShCtx "Software\Microsoft\Windows\CurrentVersion\Uninstall\RedPanda-C++" "Publisher" "Roy Qu(royqh1979@gmail.com)"


  ; Write required files
  File "RedPandaIDE.exe"
  File "ConsolePauser.exe"
  File "redpanda-win-git-askpass.exe"
  File "astyle.exe"
  File "LICENSE"
  File "NEWS.md"
  File "README.md"
  File "qt.conf"
  
  ; Write required paths
  SetOutPath $INSTDIR\Templates
  File /nonfatal /r "Templates\*"

SectionEnd

!ifdef HAVE_MINGW32
  Section "$(SectionMinGW32Name)" SectionMinGW32
    SectionIn 1 3
    SetOutPath $INSTDIR

    File /nonfatal /r "mingw32"
  SectionEnd
!endif

!ifdef HAVE_MINGW64
  Section "$(SectionMinGW64Name)" SectionMinGW64
    SectionIn 1 3
    SetOutPath $INSTDIR

    File /nonfatal /r "mingw64"
  SectionEnd
!endif

####################################################################
# File association
SectionGroup "$(SectionAssocsName)" SectionAssocs
Section "$(SectionAssocExtNameBegin) .dev $(SectionAssocExtNameEnd)"
  SectionIn 1 3

  StrCpy $0 $INSTDIR\RedPandaIDE.exe
  WriteRegStr HKCR ".dev" "" "DevCpp.dev"
  WriteRegStr HKCR "DevCpp.dev" "" "Dev-C++ Project File"
  WriteRegStr HKCR "DevCpp.dev\DefaultIcon" "" '$0,3'
  WriteRegStr HKCR "DevCpp.dev\Shell\Open\Command" "" '$0 "%1"'
  ${NotifyShell_AssocChanged}
SectionEnd

Section "$(SectionAssocExtNameBegin) .c $(SectionAssocExtNameEnd)"
  SectionIn 1 3

  StrCpy $0 $INSTDIR\RedPandaIDE.exe
  WriteRegStr HKCR ".c" "" "DevCpp.c"
  WriteRegStr HKCR "DevCpp.c" "" "C Source File"
  WriteRegStr HKCR "DevCpp.c\DefaultIcon" "" '$0,4'
  WriteRegStr HKCR "DevCpp.c\Shell\Open\Command" "" '$0 "%1"'
  ${NotifyShell_AssocChanged}
SectionEnd

Section "$(SectionAssocExtNameBegin) .cpp $(SectionAssocExtNameEnd)"
  SectionIn 1 3

  StrCpy $0 $INSTDIR\RedPandaIDE.exe
  WriteRegStr HKCR ".cpp" "" "DevCpp.cpp"
  WriteRegStr HKCR "DevCpp.cpp" "" "C++ Source File"
  WriteRegStr HKCR "DevCpp.cpp\DefaultIcon" "" '$0,5'
  WriteRegStr HKCR "DevCpp.cpp\Shell\Open\Command" "" '$0 "%1"'
  ${NotifyShell_AssocChanged}
SectionEnd

Section "$(SectionAssocExtNameBegin) .cxx $(SectionAssocExtNameEnd)"
  SectionIn 1 3

  StrCpy $0 $INSTDIR\RedPandaIDE.exe
  WriteRegStr HKCR ".cxx" "" "DevCpp.cxx"
  WriteRegStr HKCR "DevCpp.cxx" "" "C++ Source File"
  WriteRegStr HKCR "DevCpp.cxx\DefaultIcon" "" '$0,5'
  WriteRegStr HKCR "DevCpp.cxx\Shell\Open\Command" "" '$0 "%1"'
  ${NotifyShell_AssocChanged}
SectionEnd

Section "$(SectionAssocExtNameBegin) .cc $(SectionAssocExtNameEnd)"
  SectionIn 1 3

  StrCpy $0 $INSTDIR\RedPandaIDE.exe
  WriteRegStr HKCR ".cc" "" "DevCpp.cc"
  WriteRegStr HKCR "DevCpp.cc" "" "C++ Source File"
  WriteRegStr HKCR "DevCpp.cc\DefaultIcon" "" '$0,5'
  WriteRegStr HKCR "DevCpp.cc\Shell\Open\Command" "" '$0 "%1"'
  ${NotifyShell_AssocChanged}
SectionEnd

Section "$(SectionAssocExtNameBegin) .hxx $(SectionAssocExtNameEnd)"
  SectionIn 1 3

  StrCpy $0 $INSTDIR\RedPandaIDE.exe
  WriteRegStr HKCR ".hxx" "" "DevCpp.hxx"
  WriteRegStr HKCR "DevCpp.hxx" "" "C++ Header File"
  WriteRegStr HKCR "DevCpp.hxx\DefaultIcon" "" '$0,7'
  WriteRegStr HKCR "DevCpp.hxx\Shell\Open\Command" "" '$0 "%1"'
  ${NotifyShell_AssocChanged}
SectionEnd

Section "$(SectionAssocExtNameBegin) .h $(SectionAssocExtNameEnd)"
  SectionIn 1 3

  StrCpy $0 $INSTDIR\RedPandaIDE.exe
  WriteRegStr HKCR ".h" "" "DevCpp.h"
  WriteRegStr HKCR "DevCpp.h" "" "C Header File"
  WriteRegStr HKCR "DevCpp.h\DefaultIcon" "" '$0,6'
  WriteRegStr HKCR "DevCpp.h\Shell\Open\Command" "" '$0 "%1"'
  ${NotifyShell_AssocChanged}
SectionEnd

Section "$(SectionAssocExtNameBegin) .hpp $(SectionAssocExtNameEnd)"
  SectionIn 1 3

  StrCpy $0 $INSTDIR\RedPandaIDE.exe
  WriteRegStr HKCR ".hpp" "" "DevCpp.hpp"
  WriteRegStr HKCR "DevCpp.hpp" "" "C++ Header File"
  WriteRegStr HKCR "DevCpp.hpp\DefaultIcon" "" '$0,7'
  WriteRegStr HKCR "DevCpp.hpp\Shell\Open\Command" "" '$0 "%1"'
  ${NotifyShell_AssocChanged}
SectionEnd

SectionGroupEnd

####################################################################
# Shortcuts
SectionGroup "$(SectionShortcutsName)" SectionShortcuts

Section "$(SectionMenuLaunchName)" SectionMenuLaunch
  SectionIn 1 3

  StrCpy $0 $SMPROGRAMS ; start menu Programs folder
  CreateDirectory "$0\$(MessageAppName)"
  CreateShortCut "$0\$(MessageAppName)\$(MessageAppName).lnk" "$INSTDIR\RedPandaIDE.exe"
  CreateShortCut "$0\$(MessageAppName)\License.lnk" "$INSTDIR\LICENSE"
  CreateShortCut "$0\$(MessageAppName)\Uninstall $(MessageAppName).lnk" "$INSTDIR\uninstall.exe"
SectionEnd

Section "$(SectionDesktopLaunchName)" SectionDesktopLaunch
  SectionIn 1 3

  CreateShortCut "$DESKTOP\$(MessageAppName).lnk" "$INSTDIR\RedPandaIDE.exe"
SectionEnd

SectionGroupEnd

Section "$(SectionConfigName)" SectionConfig
  SectionIn 3

  RMDir /r "$APPDATA\RedPandaIDE"
  
SectionEnd

####################################################################

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
!insertmacro MUI_DESCRIPTION_TEXT ${SectionMain}        "$(MessageSectionMain)"
!ifdef HAVE_MINGW32
  !insertmacro MUI_DESCRIPTION_TEXT ${SectionMinGW32}      "$(MessageSectionMinGW32)"
!endif
!ifdef HAVE_MINGW64
  !insertmacro MUI_DESCRIPTION_TEXT ${SectionMinGW64}      "$(MessageSectionMinGW64)"
!endif
!insertmacro MUI_DESCRIPTION_TEXT ${SectionShortcuts}   "$(MessageSectionShortcuts)"
!insertmacro MUI_DESCRIPTION_TEXT ${SectionAssocs}      "$(MessageSectionAssocs)"
!insertmacro MUI_DESCRIPTION_TEXT ${SectionConfig}      "$(MessageSectionConfig)"
!insertmacro MUI_FUNCTION_DESCRIPTION_END

####################################################################
# Functions, utilities

Function .onInit
  !insertmacro MUI_LANGDLL_DISPLAY
  !insertmacro DetectOsArch

  IfFileExists "C:\Dev-Cpp\devcpp.exe" 0 +2
    SectionSetFlags ${SectionConfig} ${SF_SELECTED} # Remove old Dev-Cpp config files
	
  IfFileExists "$APPDATA\Dev-Cpp\devcpp.cfg" 0 +2 # deprecated config file
    SectionSetFlags ${SectionConfig} ${SF_SELECTED}

  SetShellVarContext all
  !if "${ARCH}" == "x86"
    SetRegView 32
  !else
    SetRegView 64
  !endif
FunctionEnd

Function .onSelChange
  !insertmacro SectionAction_CheckMingw64
FunctionEnd

Function myGuiInit
  !insertmacro CheckOsArch
  !insertmacro CheckOsBuild

  ; uninstall existing
  SetRegView 32
  Call UninstallExisting
  SetRegView 64
  Call UninstallExisting

  !if "${ARCH}" == "x86"
    SetRegView 32
  !else
    SetRegView 64
  !endif

  !insertmacro SectionAction_CheckMingw64
FunctionEnd

Function un.onInit
  !insertmacro MUI_UNGETLANGUAGE

  SetShellVarContext all
  !if "${ARCH}" == "x86"
    SetRegView 32
  !else
    SetRegView 64
  !endif
FunctionEnd

Var /GLOBAL uninstallString
Var /GLOBAL installLocation

Function UninstallExisting
  ReadRegStr $uninstallString ShCtx  "Software\Microsoft\Windows\CurrentVersion\Uninstall\RedPanda-C++"  "UninstallString"
  ${If} $uninstallString != ""
    GetFullPathName $installLocation "$uninstallString\.." ; remove '\uninstall.exe'
    MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
      "$(MessageUninstallExisting)" \
      /SD IDNO \
      IDOK uninst
    Abort
  uninst:
    ClearErrors
    HideWindow
    ClearErrors
    ExecWait '"$uninstallString" _?=$installLocation'
    Delete "$uninstallString"
    RMDir "$installLocation"
    BringToFront
  ${EndIf}
FunctionEnd

####################################################################
# uninstall

UninstallText "$(MessageUninstallText)"
ShowUninstDetails show

Section "Uninstall"

  ; Remove uninstaller
  Delete "$INSTDIR\uninstall.exe"

  ; Remove start menu stuff
  Delete "$SMPROGRAMS\$(MessageAppName)\$(MessageAppName).lnk"
  Delete "$SMPROGRAMS\$(MessageAppName)\License.lnk"
  Delete "$SMPROGRAMS\$(MessageAppName)\Uninstall $(MessageAppName).lnk"
  RMDir "$SMPROGRAMS\$(MessageAppName)"

  ; Remove desktop stuff
  Delete "$QUICKLAUNCH\$(MessageAppName).lnk"
  Delete "$DESKTOP\$(MessageAppName).lnk"

  DeleteRegKey HKCR "DevCpp.dev"
  DeleteRegKey HKCR "DevCpp.c"
  DeleteRegKey HKCR "DevCpp.cpp"
  DeleteRegKey HKCR "DevCpp.cxx"
  DeleteRegKey HKCR "DevCpp.cc"
  DeleteRegKey HKCR "DevCpp.h"
  DeleteRegKey HKCR "DevCpp.hpp"
  DeleteRegKey HKCR "DevCpp.hxx"
  ${NotifyShell_AssocChanged}

  Delete "$INSTDIR\NEWS.md"
  Delete "$INSTDIR\RedPandaIDE.exe"
  Delete "$INSTDIR\ConsolePauser.exe"
  Delete "$INSTDIR\redpanda-win-git-askpass.exe"
  Delete "$INSTDIR\astyle.exe"
  Delete "$INSTDIR\LICENSE"
  Delete "$INSTDIR\README.md"
  Delete "$INSTDIR\qt.conf"

  RMDir /r "$INSTDIR\Lang"
  RMDir /r "$INSTDIR\Templates"
  RMDir /r "$INSTDIR\mingw32"
  RMDir /r "$INSTDIR\mingw64"

  RMDir "$INSTDIR"

  ; Remove registry keys
  DeleteRegKey ShCtx "Software\Microsoft\Windows\CurrentVersion\Uninstall\RedPanda-C++"
  DeleteRegKey ShCtx "Software\RedPanda-C++"

  MessageBox MB_YESNO "$(MessageRemoveConfig)" /SD IDNO IDNO SkipRemoveConfig
  RMDir /r "$APPDATA\RedPandaIDE"
SkipRemoveConfig:

SectionEnd
