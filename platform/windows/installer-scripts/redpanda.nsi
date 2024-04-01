####################################################################
# Startup
SetFont "Segoe UI" 11
Unicode True

!define APP_NAME_EN "Red Panda C++"
!define APP_NAME_ZH_CN "小熊猫 C++"
!define DISPLAY_NAME "$(StrAppName) ${APP_VERSION} (${ARCH})"

!define INSTALL_NAME "RedPanda-Cpp"
!define REGISTRY_PROGRAM_ID "RedPanda-C++"
!define UNINSTKEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${REGISTRY_PROGRAM_ID}"

!define MULTIUSER_EXECUTIONLEVEL Highest
!define MULTIUSER_INSTALLMODE_COMMANDLINE
!define MULTIUSER_INSTALLMODE_DEFAULT_REGISTRY_KEY  "${UNINSTKEY}"
!define MULTIUSER_INSTALLMODE_DEFAULT_REGISTRY_VALUENAME "CurrentUser"
!define MULTIUSER_INSTALLMODE_INSTDIR "${INSTALL_NAME}"
!define MULTIUSER_MUI
!if "${ARCH}" != "x86"
  !define MULTIUSER_USE_PROGRAMFILES64
!endif

!include "Integration.nsh"
!include "LogicLib.nsh"
!include "MUI2.nsh"
!include "MultiUser.nsh"
!include "WinVer.nsh"
!include "WordFunc.nsh"
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

InstType "$(StrInstTypeFull)"    ;1
InstType "$(StrInstTypeMinimal)" ;2
InstType "$(StrInstTypeSafe)"    ;3

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
!insertmacro MULTIUSER_PAGE_INSTALLMODE
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
# Silently uninstall previous version

Section "" SecUninstallPrevious
  !insertmacro CheckPreviousInstaller
SectionEnd

####################################################################
# Files, by option section

Section "$(SectionMainName)" SectionMain
  SectionIn 1 2 3 RO
  
  SetOutPath $INSTDIR

  ; Allways create an uninstaller
  WriteUninstaller "$INSTDIR\uninstall.exe"
  WriteRegStr ShCtx "${UNINSTKEY}" "DisplayName" "$(StrAppName) (${ARCH})"
  WriteRegStr ShCtx "${UNINSTKEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr ShCtx "${UNINSTKEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr ShCtx "${UNINSTKEY}" "QuietUninstallString" '"$INSTDIR\uninstall.exe" /S'
  WriteRegStr ShCtx "${UNINSTKEY}" "DisplayVersion" "${APP_VERSION}"
  WriteRegStr ShCtx "${UNINSTKEY}" "DisplayIcon" "$INSTDIR\RedPandaIDE.exe"
  WriteRegStr ShCtx "${UNINSTKEY}" "Publisher" "Roy Qu (royqh1979@gmail.com)"
  WriteRegStr ShCtx "${UNINSTKEY}" "$MultiUser.InstallMode" "1"

  ; Write required files
  File "RedPandaIDE.exe"
  File "ConsolePauser.exe"
  File "redpanda-win-git-askpass.exe"
  File "astyle.exe"
  File "LICENSE"
  File "NEWS.md"
  File "README.md"
  File "qt.conf"
  !ifdef HAVE_OPENCONSOLE
    File "OpenConsole.exe"
  !endif
  !ifdef HAVE_COMPILER_HINT
    File "compiler_hint.lua"
  !endif

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

!ifdef HAVE_LLVM
  Section "$(SectionLlvmName)" SectionLlvm
    SectionIn 1 3
    SetOutPath $INSTDIR

    File /nonfatal /r "llvm-mingw"
  SectionEnd
!endif

####################################################################
# File association
SectionGroup "$(SectionAssocsName)" SectionAssocs
Section "$(SectionAssocExtNameBegin) .dev $(SectionAssocExtNameEnd)"
  SectionIn 1 3

  StrCpy $0 $INSTDIR\RedPandaIDE.exe
  WriteRegStr ShCtx "Software\Classes\.dev" "" "DevCpp.dev"
  WriteRegStr ShCtx "Software\Classes\DevCpp.dev" "" "$(StrAppName) $(StrProjectFile)"
  WriteRegStr ShCtx "Software\Classes\DevCpp.dev\DefaultIcon" "" '$0,3'
  WriteRegStr ShCtx "Software\Classes\DevCpp.dev\Shell\Open\Command" "" '$0 "%1"'
  ${NotifyShell_AssocChanged}
SectionEnd

Section "$(SectionAssocExtNameBegin) .c $(SectionAssocExtNameEnd)"
  SectionIn 1 3

  StrCpy $0 $INSTDIR\RedPandaIDE.exe
  WriteRegStr ShCtx "Software\Classes\.c" "" "DevCpp.c"
  WriteRegStr ShCtx "Software\Classes\DevCpp.c" "" "C $(StrSourceFile)"
  WriteRegStr ShCtx "Software\Classes\DevCpp.c\DefaultIcon" "" '$0,4'
  WriteRegStr ShCtx "Software\Classes\DevCpp.c\Shell\Open\Command" "" '$0 "%1"'
  ${NotifyShell_AssocChanged}
SectionEnd

Section "$(SectionAssocExtNameBegin) .cpp $(SectionAssocExtNameEnd)"
  SectionIn 1 3

  StrCpy $0 $INSTDIR\RedPandaIDE.exe
  WriteRegStr ShCtx "Software\Classes\.cpp" "" "DevCpp.cpp"
  WriteRegStr ShCtx "Software\Classes\DevCpp.cpp" "" "C++ $(StrSourceFile)"
  WriteRegStr ShCtx "Software\Classes\DevCpp.cpp\DefaultIcon" "" '$0,5'
  WriteRegStr ShCtx "Software\Classes\DevCpp.cpp\Shell\Open\Command" "" '$0 "%1"'
  ${NotifyShell_AssocChanged}
SectionEnd

Section "$(SectionAssocExtNameBegin) .cxx $(SectionAssocExtNameEnd)"
  SectionIn 1 3

  StrCpy $0 $INSTDIR\RedPandaIDE.exe
  WriteRegStr ShCtx "Software\Classes\.cxx" "" "DevCpp.cxx"
  WriteRegStr ShCtx "Software\Classes\DevCpp.cxx" "" "C++ $(StrSourceFile)"
  WriteRegStr ShCtx "Software\Classes\DevCpp.cxx\DefaultIcon" "" '$0,5'
  WriteRegStr ShCtx "Software\Classes\DevCpp.cxx\Shell\Open\Command" "" '$0 "%1"'
  ${NotifyShell_AssocChanged}
SectionEnd

Section "$(SectionAssocExtNameBegin) .cc $(SectionAssocExtNameEnd)"
  SectionIn 1 3

  StrCpy $0 $INSTDIR\RedPandaIDE.exe
  WriteRegStr ShCtx "Software\Classes\.cc" "" "DevCpp.cc"
  WriteRegStr ShCtx "Software\Classes\DevCpp.cc" "" "C++ $(StrSourceFile)"
  WriteRegStr ShCtx "Software\Classes\DevCpp.cc\DefaultIcon" "" '$0,5'
  WriteRegStr ShCtx "Software\Classes\DevCpp.cc\Shell\Open\Command" "" '$0 "%1"'
  ${NotifyShell_AssocChanged}
SectionEnd

Section "$(SectionAssocExtNameBegin) .hxx $(SectionAssocExtNameEnd)"
  SectionIn 1 3

  StrCpy $0 $INSTDIR\RedPandaIDE.exe
  WriteRegStr ShCtx "Software\Classes\.hxx" "" "DevCpp.hxx"
  WriteRegStr ShCtx "Software\Classes\DevCpp.hxx" "" "C++ $(StrHeaderFile)"
  WriteRegStr ShCtx "Software\Classes\DevCpp.hxx\DefaultIcon" "" '$0,7'
  WriteRegStr ShCtx "Software\Classes\DevCpp.hxx\Shell\Open\Command" "" '$0 "%1"'
  ${NotifyShell_AssocChanged}
SectionEnd

Section "$(SectionAssocExtNameBegin) .h $(SectionAssocExtNameEnd)"
  SectionIn 1 3

  StrCpy $0 $INSTDIR\RedPandaIDE.exe
  WriteRegStr ShCtx "Software\Classes\.h" "" "DevCpp.h"
  WriteRegStr ShCtx "Software\Classes\DevCpp.h" "" "C $(StrHeaderFile)"
  WriteRegStr ShCtx "Software\Classes\DevCpp.h\DefaultIcon" "" '$0,6'
  WriteRegStr ShCtx "Software\Classes\DevCpp.h\Shell\Open\Command" "" '$0 "%1"'
  ${NotifyShell_AssocChanged}
SectionEnd

Section "$(SectionAssocExtNameBegin) .hpp $(SectionAssocExtNameEnd)"
  SectionIn 1 3

  StrCpy $0 $INSTDIR\RedPandaIDE.exe
  WriteRegStr ShCtx "Software\Classes\.hpp" "" "DevCpp.hpp"
  WriteRegStr ShCtx "Software\Classes\DevCpp.hpp" "" "C++ $(StrHeaderFile)"
  WriteRegStr ShCtx "Software\Classes\DevCpp.hpp\DefaultIcon" "" '$0,7'
  WriteRegStr ShCtx "Software\Classes\DevCpp.hpp\Shell\Open\Command" "" '$0 "%1"'
  ${NotifyShell_AssocChanged}
SectionEnd

SectionGroupEnd

####################################################################
# Shortcuts
SectionGroup "$(SectionShortcutsName)" SectionShortcuts

Section "$(SectionMenuLaunchName)" SectionMenuLaunch
  SectionIn 1 3

  StrCpy $0 $SMPROGRAMS ; start menu Programs folder
  CreateDirectory "$0\$(StrAppName)"
  CreateShortCut "$0\$(StrAppName)\$(StrAppName).lnk" "$INSTDIR\RedPandaIDE.exe"
  CreateShortCut "$0\$(StrAppName)\License.lnk" "$INSTDIR\LICENSE"
  CreateShortCut "$0\$(StrAppName)\$(StrUninstallerAppName).lnk" "$INSTDIR\uninstall.exe"
SectionEnd

Section "$(SectionDesktopLaunchName)" SectionDesktopLaunch
  SectionIn 1 3

  CreateShortCut "$DESKTOP\$(StrAppName).lnk" "$INSTDIR\RedPandaIDE.exe"
SectionEnd

SectionGroupEnd

Section "$(SectionCompressName)" SectionCompress
  DetailPrint "$(MessageCompressing)"
  ExecWait '$SYSDIR\compact.exe /C /S /F /EXE:XPRESS16K "$INSTDIR\*"'
SectionEnd

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
!ifdef HAVE_LLVM
  !insertmacro MUI_DESCRIPTION_TEXT ${SectionLlvm}        "$(MessageSectionLlvm)"
!endif
!insertmacro MUI_DESCRIPTION_TEXT ${SectionShortcuts}   "$(MessageSectionShortcuts)"
!insertmacro MUI_DESCRIPTION_TEXT ${SectionAssocs}      "$(MessageSectionAssocs)"
!insertmacro MUI_DESCRIPTION_TEXT ${SectionCompress}    "$(MessageSectionCompress)"
!insertmacro MUI_DESCRIPTION_TEXT ${SectionConfig}      "$(MessageSectionConfig)"
!insertmacro MUI_FUNCTION_DESCRIPTION_END

####################################################################
# Functions, utilities

Function .onInit
  !insertmacro MULTIUSER_INIT
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
  !insertmacro SectionAction_CheckCompress
FunctionEnd

Function myGuiInit
  !insertmacro CheckOsArch
  !insertmacro CheckOsBuild
  !insertmacro CheckV2Installer

  !insertmacro SectionAction_CheckMingw64
  !insertmacro SectionAction_CheckCompress
FunctionEnd

Function un.onInit
  !insertmacro MULTIUSER_UNINIT
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
Var /GLOBAL oldVersion
Var /GLOBAL versionCompareResult

Function UninstallExisting
  ReadRegStr $uninstallString ShCtx  "${UNINSTKEY}"  "UninstallString"
  ${If} $uninstallString != ""
    ReadRegStr $installLocation ShCtx  "${UNINSTKEY}"  "InstallLocation"
    DetailPrint "$(MessageUninstallingExisting)"
    ; uninstallString already quoted; NSIS requires installLocation unquoted
    ExecWait '$uninstallString /S _?=$installLocation'
    Delete "$uninstallString"
    RMDir "$installLocation"
  ${EndIf}
FunctionEnd

Function UninstallV2
  ReadRegStr $oldVersion HKLM "${UNINSTKEY}" "DisplayVersion"
  ${If} $oldVersion != ""
    ${VersionCompare} "3.0" "$oldVersion" $versionCompareResult
    ${If} "$versionCompareResult" == 1  ; 1st version is greater
      ReadRegStr $uninstallString HKLM  "${UNINSTKEY}"  "UninstallString"
      GetFullPathName $installLocation "$uninstallString\.." ; remove '\uninstall.exe'
      MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
        "$(MessageUninstallV2)" \
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
  RMDir /r "$SMPROGRAMS\${APP_NAME_EN}"
  RMDir /r "$SMPROGRAMS\${APP_NAME_ZH_CN}"

  ; Remove desktop stuff
  Delete "$QUICKLAUNCH\${APP_NAME_EN}.lnk"
  Delete "$QUICKLAUNCH\${APP_NAME_ZH_CN}.lnk"
  Delete "$DESKTOP\${APP_NAME_EN}.lnk"
  Delete "$DESKTOP\${APP_NAME_ZH_CN}.lnk"

  DeleteRegKey ShCtx "Software\Classes\DevCpp.dev"
  DeleteRegKey ShCtx "Software\Classes\DevCpp.c"
  DeleteRegKey ShCtx "Software\Classes\DevCpp.cpp"
  DeleteRegKey ShCtx "Software\Classes\DevCpp.cxx"
  DeleteRegKey ShCtx "Software\Classes\DevCpp.cc"
  DeleteRegKey ShCtx "Software\Classes\DevCpp.h"
  DeleteRegKey ShCtx "Software\Classes\DevCpp.hpp"
  DeleteRegKey ShCtx "Software\Classes\DevCpp.hxx"
  ${NotifyShell_AssocChanged}

  Delete "$INSTDIR\NEWS.md"
  Delete "$INSTDIR\RedPandaIDE.exe"
  Delete "$INSTDIR\ConsolePauser.exe"
  Delete "$INSTDIR\redpanda-win-git-askpass.exe"
  Delete "$INSTDIR\astyle.exe"
  Delete "$INSTDIR\LICENSE"
  Delete "$INSTDIR\README.md"
  Delete "$INSTDIR\qt.conf"
  Delete "$INSTDIR\OpenConsole.exe"
  Delete "$INSTDIR\compiler_hint.lua"

  RMDir /r "$INSTDIR\Lang"
  RMDir /r "$INSTDIR\Templates"
  RMDir /r "$INSTDIR\mingw32"
  RMDir /r "$INSTDIR\mingw64"
  RMDir /r "$INSTDIR\llvm-mingw"

  RMDir "$INSTDIR"

  ; Remove registry keys
  DeleteRegKey ShCtx "${UNINSTKEY}"
  DeleteRegKey ShCtx "Software\RedPanda-C++"

  MessageBox MB_YESNO "$(MessageRemoveConfig)" /SD IDNO IDNO SkipRemoveConfig
  RMDir /r "$APPDATA\RedPandaIDE"
SkipRemoveConfig:

SectionEnd
