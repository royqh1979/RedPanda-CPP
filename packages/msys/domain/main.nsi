####################################################################
# Startup

CRCCheck on
SetCompressor /SOLID /FINAL lzma
SetCompressorDictSize 128
SetDatablockOptimize on
Unicode True

!define FINALNAME "redpanda-cpp-${VERSION}-${ARCH}.exe"
!define DISPLAY_NAME "Red Panda C++ ${VERSION} (${ARCH})"

!define UNINSTKEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\RedPanda-C++"
!define MULTIUSER_INSTALLMODE_DEFAULT_REGISTRY_KEY "${UNINSTKEY}"
!define MULTIUSER_INSTALLMODE_DEFAULT_REGISTRY_VALUENAME "CurrentUser"
!define MULTIUSER_INSTALLMODE_INSTDIR "RedPanda-CPP"
!define MULTIUSER_EXECUTIONLEVEL Highest
!define MULTIUSER_MUI
!define MULTIUSER_INSTALLMODE_COMMANDLINE

!if "${ARCH}" != "x86"
  !define MULTIUSER_USE_PROGRAMFILES64
!endif

!include "x64.nsh"
!include "WinVer.nsh"
!include "MultiUser.nsh"
!include "MUI2.nsh"
!include "lang.nsh"

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
SetOverwrite try
XPStyle on
ManifestDPIAware true

InstType "Full" ;1
InstType "Minimal" ;2

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

Section "" SecUninstallPrevious
  SetRegView 32
  Call UninstallExisting
  SetRegView 64
  Call UninstallExisting
!if "${ARCH}" == "x86"
  SetRegView 32
!endif
SectionEnd

####################################################################
# Files, by option section

Section "$(SectionMainName)" SectionMain
  SectionIn 1 2 RO

  SetOutPath $INSTDIR

  ; Allways create an uninstaller
  WriteUninstaller "$INSTDIR\uninstall.exe"
  WriteRegStr ShCtx "${UNINSTKEY}" "DisplayName" "Red Panda C++"
  WriteRegStr ShCtx "${UNINSTKEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr ShCtx "${UNINSTKEY}" "UninstallString" "$INSTDIR\uninstall.exe"
  WriteRegStr ShCtx "${UNINSTKEY}" "DisplayVersion" "${VERSION}"
  WriteRegStr ShCtx "${UNINSTKEY}" "DisplayIcon" "$INSTDIR\RedPandaIDE.exe"
  WriteRegStr ShCtx "${UNINSTKEY}" "Publisher" "Roy Qu (royqh1979@gmail.com)"
  WriteRegStr ShCtx "${UNINSTKEY}" $MultiUser.InstallMode 1

  ; Write required files
  File "RedPandaIDE.exe"
  File "consolepauser.exe"
  File "redpanda-win-git-askpass.exe"
  File "astyle.exe"
  File "qt.conf"
  File "LICENSE"
  File "NEWS.md"
  File "README.md"
  File "compiler_hint.lua"

  ; Write required paths
  SetOutPath $INSTDIR\templates
  File /nonfatal /r "templates\*"
SectionEnd

Section "$(SectionOpenConsoleName)" SectionOpenConsole
  SectionIn 1
  SetOutPath $INSTDIR
  File "OpenConsole.exe"
SectionEnd

!if "${ARCH}" == "x86"
Section "$(SectionMingw32Name)" SectionMingw32
  SectionIn 1
  SetOutPath $INSTDIR\mingw32
  File /nonfatal /r "mingw32\*"
SectionEnd
!endif

!if "${ARCH}" == "x64"
Section "$(SectionMingw64Name)" SectionMingw64
  SectionIn 1
  SetOutPath $INSTDIR\mingw64
  File /nonfatal /r "mingw64\*"
SectionEnd
!endif

Section "$(SectionLlvmName)" SectionLlvm
  SectionIn 1
  SetOutPath $INSTDIR\llvm-mingw
  File /nonfatal /r "llvm-mingw\*"
SectionEnd

####################################################################
# File association
SectionGroup "$(SectionAssocsName)" SectionAssocs
  Section "$(SectionAssocExtNameBegin) .dev $(SectionAssocExtNameEnd)"
    SectionIn 1

    StrCpy $0 $INSTDIR\RedPandaIDE.exe
    WriteRegStr ShCtx "Software\Classes\.dev" "" "DevCpp.dev"
    WriteRegStr ShCtx "Software\Classes\DevCpp.dev" "" "Dev-C++ Project File"
    WriteRegStr ShCtx "Software\Classes\DevCpp.dev\DefaultIcon" "" '$0,3'
    WriteRegStr ShCtx "Software\Classes\DevCpp.dev\Shell\Open\Command" "" '$0 "%1"'
  SectionEnd

  Section "$(SectionAssocExtNameBegin) .c $(SectionAssocExtNameEnd)"
    SectionIn 1

    StrCpy $0 $INSTDIR\RedPandaIDE.exe
    WriteRegStr ShCtx "Software\Classes\.c" "" "DevCpp.c"
    WriteRegStr ShCtx "Software\Classes\DevCpp.c" "" "C Source File"
    WriteRegStr ShCtx "Software\Classes\DevCpp.c\DefaultIcon" "" '$0,4'
    WriteRegStr ShCtx "Software\Classes\DevCpp.c\Shell\Open\Command" "" '$0 "%1"'
  SectionEnd

  Section "$(SectionAssocExtNameBegin) .cpp $(SectionAssocExtNameEnd)"
    SectionIn 1

    StrCpy $0 $INSTDIR\RedPandaIDE.exe
    WriteRegStr ShCtx "Software\Classes\.cpp" "" "DevCpp.cpp"
    WriteRegStr ShCtx "Software\Classes\DevCpp.cpp" "" "C++ Source File"
    WriteRegStr ShCtx "Software\Classes\DevCpp.cpp\DefaultIcon" "" '$0,5'
    WriteRegStr ShCtx "Software\Classes\DevCpp.cpp\Shell\Open\Command" "" '$0 "%1"'
  SectionEnd

  Section "$(SectionAssocExtNameBegin) .cxx $(SectionAssocExtNameEnd)"
    SectionIn 1

    StrCpy $0 $INSTDIR\RedPandaIDE.exe
    WriteRegStr ShCtx "Software\Classes\.cxx" "" "DevCpp.cxx"
    WriteRegStr ShCtx "Software\Classes\DevCpp.cxx" "" "C++ Source File"
    WriteRegStr ShCtx "Software\Classes\DevCpp.cxx\DefaultIcon" "" '$0,5'
    WriteRegStr ShCtx "Software\Classes\DevCpp.cxx\Shell\Open\Command" "" '$0 "%1"'
  SectionEnd

  Section "$(SectionAssocExtNameBegin) .cc $(SectionAssocExtNameEnd)"
    SectionIn 1

    StrCpy $0 $INSTDIR\RedPandaIDE.exe
    WriteRegStr ShCtx "Software\Classes\.cc" "" "DevCpp.cc"
    WriteRegStr ShCtx "Software\Classes\DevCpp.cc" "" "C++ Source File"
    WriteRegStr ShCtx "Software\Classes\DevCpp.cc\DefaultIcon" "" '$0,5'
    WriteRegStr ShCtx "Software\Classes\DevCpp.cc\Shell\Open\Command" "" '$0 "%1"'
  SectionEnd

  Section "$(SectionAssocExtNameBegin) .hxx $(SectionAssocExtNameEnd)"
    SectionIn 1

    StrCpy $0 $INSTDIR\RedPandaIDE.exe
    WriteRegStr ShCtx "Software\Classes\.hxx" "" "DevCpp.hxx"
    WriteRegStr ShCtx "Software\Classes\DevCpp.hxx" "" "C++ Header File"
    WriteRegStr ShCtx "Software\Classes\DevCpp.hxx\DefaultIcon" "" '$0,7'
    WriteRegStr ShCtx "Software\Classes\DevCpp.hxx\Shell\Open\Command" "" '$0 "%1"'
  SectionEnd

  Section "$(SectionAssocExtNameBegin) .h $(SectionAssocExtNameEnd)"
    SectionIn 1

    StrCpy $0 $INSTDIR\RedPandaIDE.exe
    WriteRegStr ShCtx "Software\Classes\.h" "" "DevCpp.h"
    WriteRegStr ShCtx "Software\Classes\DevCpp.h" "" "C Header File"
    WriteRegStr ShCtx "Software\Classes\DevCpp.h\DefaultIcon" "" '$0,6'
    WriteRegStr ShCtx "Software\Classes\DevCpp.h\Shell\Open\Command" "" '$0 "%1"'
  SectionEnd

  Section "$(SectionAssocExtNameBegin) .hpp $(SectionAssocExtNameEnd)"
    SectionIn 1

    StrCpy $0 $INSTDIR\RedPandaIDE.exe
    WriteRegStr ShCtx "Software\Classes\.hpp" "" "DevCpp.hpp"
    WriteRegStr ShCtx "Software\Classes\DevCpp.hpp" "" "C++ Header File"
    WriteRegStr ShCtx "Software\Classes\DevCpp.hpp\DefaultIcon" "" '$0,7'
    WriteRegStr ShCtx "Software\Classes\DevCpp.hpp\Shell\Open\Command" "" '$0 "%1"'
  SectionEnd
SectionGroupEnd

####################################################################
# Shortcuts
SectionGroup "$(SectionShortcutsName)" SectionShortcuts
  Section "$(SectionMenuLaunchName)" SectionMenuLaunch
    SectionIn 1 2

    StrCpy $0 $SMPROGRAMS ; start menu Programs folder
    CreateDirectory "$0\$(MessageAppName)"
    CreateShortCut "$0\$(MessageAppName)\$(MessageAppName).lnk" "$INSTDIR\RedPandaIDE.exe"
    CreateShortCut "$0\$(MessageAppName)\License.lnk" "$INSTDIR\LICENSE"
    CreateShortCut "$0\$(MessageAppName)\Uninstall $(MessageAppName).lnk" "$INSTDIR\uninstall.exe"
  SectionEnd

  Section "$(SectionDesktopLaunchName)" SectionDesktopLaunch
    SectionIn 1 2

    CreateShortCut "$DESKTOP\$(MessageAppName).lnk" "$INSTDIR\RedPandaIDE.exe"
  SectionEnd
SectionGroupEnd

Section "$(SectionConfigName)" SectionConfig
  RMDir /r "$APPDATA\RedPandaIDE"
SectionEnd

####################################################################

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
!insertmacro MUI_DESCRIPTION_TEXT ${SectionMain}        "$(MessageSectionMain)"
!insertmacro MUI_DESCRIPTION_TEXT ${SectionOpenConsole} "$(MessageSectionOpenConsole)"
!if "${ARCH}" == "x86"
!insertmacro MUI_DESCRIPTION_TEXT ${SectionMingw32}     "$(MessageSectionMingw32)"
!endif
!if "${ARCH}" == "x64"
!insertmacro MUI_DESCRIPTION_TEXT ${SectionMingw64}     "$(MessageSectionMingw64)"
!endif
!insertmacro MUI_DESCRIPTION_TEXT ${SectionLlvm}        "$(MessageSectionLlvm)"
!insertmacro MUI_DESCRIPTION_TEXT ${SectionShortcuts}   "$(MessageSectionShortcuts)"
!insertmacro MUI_DESCRIPTION_TEXT ${SectionAssocs}      "$(MessageSectionAssocs)"
!insertmacro MUI_DESCRIPTION_TEXT ${SectionConfig}      "$(MessageSectionConfig)"
!insertmacro MUI_FUNCTION_DESCRIPTION_END

####################################################################
# Functions, utilities

Function .onInit
  !insertmacro MULTIUSER_INIT
  !insertmacro MUI_LANGDLL_DISPLAY
!if "${ARCH}" != "x86"
  SetRegView 64
!endif
  ${IfNot} ${AtLeastBuild} 17763 ; OpenConsole.exe requires Windows 10 v1809 ConPTY
!if "${ARCH}" == "x86"
  ${OrIfNot} ${IsNativeIA32} ; OpenConsole.exe x86 only works on x86, while x64 works on arm64
!endif
    SectionSetFlags ${SectionOpenConsole} ${SF_RO}
  ${EndIf}
FunctionEnd

Function myGuiInit
  ${IfNot} ${AtLeastWin7}
    MessageBox MB_OK|MB_ICONSTOP "$(MessageWin7RequiredError)"
    Abort
  ${EndIf}
  ${IfNot} ${AtLeastBuild} 17763
    MessageBox MB_OK|MB_ICONSTOP "$(MessageWin10v1809RecommendedWarning)"
  ${EndIf}
!if "${ARCH}" == "x86"
  ${If} ${IsNativeAMD64}
  ${OrIf} ${IsNativeARM64}
    # note user if installing x86 on 64-bit OS
    MessageBox MB_OK|MB_ICONINFORMATION "$(Message64bitBuildWarning)"
  ${EndIf}
!else if "${ARCH}" == "x64"
  ${If} ${IsNativeIA32}
    MessageBox MB_OK|MB_ICONSTOP "$(Message64bitRequiredError)"
    Abort
  ${ElseIf} ${IsNativeARM64}
    # x64 can be installed on arm64 ...
    MessageBox MB_OK|MB_ICONINFORMATION "$(MessageArm64BuildWarning)"
    # ... but only works since Windows 11
    ${IfNot} ${AtLeastBuild} 22000
      MessageBox MB_OK|MB_ICONSTOP "$(MessageWin11RequiredError)"
      Abort
    ${EndIf}
  ${EndIf}
!else if "${ARCH}" == "arm64"
  ${IfNot} ${IsNativeARM64}
    MessageBox MB_OK|MB_ICONSTOP "$(MessageArm64RequiredError)"
    Abort
  ${EndIf}
!endif
FunctionEnd

Function .onSelChange
  ${IfNot} ${AtLeastBuild} 17763
!if "${ARCH}" == "x86"
  ${OrIfNot} ${IsNativeIA32}
!endif
    SectionSetFlags ${SectionOpenConsole} ${SF_RO}
  ${EndIf}
FunctionEnd

Function un.onInit
  !insertmacro MULTIUSER_UNINIT
  !insertmacro MUI_UNGETLANGUAGE
!if "${ARCH}" != "x86"
  SetRegView 64
!endif
FunctionEnd

Function UninstallExisting
  ReadRegStr $R0 ShCtx "${UNINSTKEY}" "UninstallString"
  ${If} $R0 != ""
    GetFullPathName $R1 "$R0\.." ; remove \uninstall.exe
    DetailPrint "$(MessageUninstallingExisting)"
    ExecWait '"$R0" /S _?=$R1'
    Delete $R0
    RMDir $R1
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

  DeleteRegKey ShCtx "Software\Classes\DevCpp.dev"
  DeleteRegKey ShCtx "Software\Classes\DevCpp.c"
  DeleteRegKey ShCtx "Software\Classes\DevCpp.cpp"
  DeleteRegKey ShCtx "Software\Classes\DevCpp.cxx"
  DeleteRegKey ShCtx "Software\Classes\DevCpp.cc"
  DeleteRegKey ShCtx "Software\Classes\DevCpp.h"
  DeleteRegKey ShCtx "Software\Classes\DevCpp.hpp"
  DeleteRegKey ShCtx "Software\Classes\DevCpp.hxx"

  Delete "$INSTDIR\NEWS.md"
  Delete "$INSTDIR\RedPandaIDE.exe"
  Delete "$INSTDIR\consolepauser.exe"
  Delete "$INSTDIR\OpenConsole.exe"
  Delete "$INSTDIR\redpanda-win-git-askpass.exe"
  Delete "$INSTDIR\astyle.exe"
  Delete "$INSTDIR\qt.conf"
  Delete "$INSTDIR\LICENSE"
  Delete "$INSTDIR\README.md"
  Delete "$INSTDIR\compiler_hint.lua"

  RMDir /r "$INSTDIR\templates"
  RMDir /r "$INSTDIR\mingw32"
  RMDir /r "$INSTDIR\mingw64"
  RMDir /r "$INSTDIR\llvm-mingw"

  RMDir "$INSTDIR"

  ; Remove registry keys
  DeleteRegKey ShCtx "${UNINSTKEY}"

  MessageBox MB_YESNO "$(MessageRemoveConfig)" /SD IDNO IDNO SkipRemoveConfig
  RMDir /r "$APPDATA\RedPandaIDE"
SkipRemoveConfig:
SectionEnd
