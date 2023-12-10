####################################################################
# Startup

CRCCheck on
SetCompressor /SOLID /FINAL lzma
SetCompressorDictSize 128
SetDatablockOptimize on
Unicode True

!ifdef USER_MODE
  !define MODE "User"
!else
  !define MODE "System"
!endif
!define FINALNAME "RedPandaCPP-${VERSION}-${ARCH}-${MODE}.exe"
!define DISPLAY_NAME "Red Panda C++ ${VERSION} ${ARCH}"

!include "x64.nsh"
!include "WinVer.nsh"
!include "MUI2.nsh"
!include "lang.nsh"

!define MUI_CUSTOMFUNCTION_GUIINIT myGuiInit

####################################################################
# Installer Attributes

Name "${DISPLAY_NAME}"
OutFile "${FINALNAME}"
Caption "${DISPLAY_NAME}"

LicenseData "LICENSE"

!ifdef USER_MODE
  RequestExecutionLevel user
  InstallDir "$LOCALAPPDATA\RedPanda-CPP"
!else
  RequestExecutionLevel admin
  !if "${ARCH}" == "x86"
  InstallDir "$PROGRAMFILES\RedPanda-CPP"
  !else
  InstallDir "$PROGRAMFILES64\RedPanda-CPP"
  !endif
!endif

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

## Remember the installer language
!ifdef USER_MODE
  !define MUI_LANGDLL_REGISTRY_ROOT "HKCU"
!else
  !define MUI_LANGDLL_REGISTRY_ROOT "HKLM"
!endif
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
  SectionIn 1 2 RO

  SetOutPath $INSTDIR

  ; Allways create an uninstaller
  WriteUninstaller "$INSTDIR\uninstall.exe"
!ifdef USER_MODE
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\RedPanda-C++" "DisplayName" "Red Panda C++"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\RedPanda-C++" "InstallLocation" "$INSTDIR"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\RedPanda-C++" "UninstallString" "$INSTDIR\uninstall.exe"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\RedPanda-C++" "DisplayVersion" "${VERSION}"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\RedPanda-C++" "DisplayIcon" "$INSTDIR\RedPandaIDE.exe"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\RedPanda-C++" "Publisher" "Roy Qu (royqh1979@gmail.com)"
!else
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\RedPanda-C++" "DisplayName" "Red Panda C++"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\RedPanda-C++" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\RedPanda-C++" "UninstallString" "$INSTDIR\uninstall.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\RedPanda-C++" "DisplayVersion" "${VERSION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\RedPanda-C++" "DisplayIcon" "$INSTDIR\RedPandaIDE.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\RedPanda-C++" "Publisher" "Roy Qu (royqh1979@gmail.com)"
!endif

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

Section "$(SectionMingw32Name)" SectionMingw32
  SectionIn 1
  SetOutPath $INSTDIR\mingw32
  File /nonfatal /r "mingw32\*"
SectionEnd

!if "${ARCH}" != "x86"
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
    WriteRegStr HKCR ".dev" "" "DevCpp.dev"
    WriteRegStr HKCR "DevCpp.dev" "" "Dev-C++ Project File"
    WriteRegStr HKCR "DevCpp.dev\DefaultIcon" "" '$0,3'
    WriteRegStr HKCR "DevCpp.dev\Shell\Open\Command" "" '$0 "%1"'
  SectionEnd

  Section "$(SectionAssocExtNameBegin) .c $(SectionAssocExtNameEnd)"
    SectionIn 1

    StrCpy $0 $INSTDIR\RedPandaIDE.exe
    WriteRegStr HKCR ".c" "" "DevCpp.c"
    WriteRegStr HKCR "DevCpp.c" "" "C Source File"
    WriteRegStr HKCR "DevCpp.c\DefaultIcon" "" '$0,4'
    WriteRegStr HKCR "DevCpp.c\Shell\Open\Command" "" '$0 "%1"'
  SectionEnd

  Section "$(SectionAssocExtNameBegin) .cpp $(SectionAssocExtNameEnd)"
    SectionIn 1

    StrCpy $0 $INSTDIR\RedPandaIDE.exe
    WriteRegStr HKCR ".cpp" "" "DevCpp.cpp"
    WriteRegStr HKCR "DevCpp.cpp" "" "C++ Source File"
    WriteRegStr HKCR "DevCpp.cpp\DefaultIcon" "" '$0,5'
    WriteRegStr HKCR "DevCpp.cpp\Shell\Open\Command" "" '$0 "%1"'
  SectionEnd

  Section "$(SectionAssocExtNameBegin) .cxx $(SectionAssocExtNameEnd)"
    SectionIn 1

    StrCpy $0 $INSTDIR\RedPandaIDE.exe
    WriteRegStr HKCR ".cxx" "" "DevCpp.cxx"
    WriteRegStr HKCR "DevCpp.cxx" "" "C++ Source File"
    WriteRegStr HKCR "DevCpp.cxx\DefaultIcon" "" '$0,5'
    WriteRegStr HKCR "DevCpp.cxx\Shell\Open\Command" "" '$0 "%1"'
  SectionEnd

  Section "$(SectionAssocExtNameBegin) .cc $(SectionAssocExtNameEnd)"
    SectionIn 1

    StrCpy $0 $INSTDIR\RedPandaIDE.exe
    WriteRegStr HKCR ".cc" "" "DevCpp.cc"
    WriteRegStr HKCR "DevCpp.cc" "" "C++ Source File"
    WriteRegStr HKCR "DevCpp.cc\DefaultIcon" "" '$0,5'
    WriteRegStr HKCR "DevCpp.cc\Shell\Open\Command" "" '$0 "%1"'
  SectionEnd

  Section "$(SectionAssocExtNameBegin) .hxx $(SectionAssocExtNameEnd)"
    SectionIn 1

    StrCpy $0 $INSTDIR\RedPandaIDE.exe
    WriteRegStr HKCR ".hxx" "" "DevCpp.hxx"
    WriteRegStr HKCR "DevCpp.hxx" "" "C++ Header File"
    WriteRegStr HKCR "DevCpp.hxx\DefaultIcon" "" '$0,7'
    WriteRegStr HKCR "DevCpp.hxx\Shell\Open\Command" "" '$0 "%1"'
  SectionEnd

  Section "$(SectionAssocExtNameBegin) .h $(SectionAssocExtNameEnd)"
    SectionIn 1

    StrCpy $0 $INSTDIR\RedPandaIDE.exe
    WriteRegStr HKCR ".h" "" "DevCpp.h"
    WriteRegStr HKCR "DevCpp.h" "" "C Header File"
    WriteRegStr HKCR "DevCpp.h\DefaultIcon" "" '$0,6'
    WriteRegStr HKCR "DevCpp.h\Shell\Open\Command" "" '$0 "%1"'
  SectionEnd

  Section "$(SectionAssocExtNameBegin) .hpp $(SectionAssocExtNameEnd)"
    SectionIn 1

    StrCpy $0 $INSTDIR\RedPandaIDE.exe
    WriteRegStr HKCR ".hpp" "" "DevCpp.hpp"
    WriteRegStr HKCR "DevCpp.hpp" "" "C++ Header File"
    WriteRegStr HKCR "DevCpp.hpp\DefaultIcon" "" '$0,7'
    WriteRegStr HKCR "DevCpp.hpp\Shell\Open\Command" "" '$0 "%1"'
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

!ifdef USER_MODE
Section "$(SectionConfigName)" SectionConfig
  RMDir /r "$APPDATA\RedPandaIDE"
SectionEnd
!endif

####################################################################

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
!insertmacro MUI_DESCRIPTION_TEXT ${SectionMain}        "$(MessageSectionMain)"
!insertmacro MUI_DESCRIPTION_TEXT ${SectionOpenConsole} "$(MessageSectionOpenConsole)"
!insertmacro MUI_DESCRIPTION_TEXT ${SectionMingw32}     "$(MessageSectionMingw32)"
!if "${ARCH}" != "x86"
!insertmacro MUI_DESCRIPTION_TEXT ${SectionMingw64}     "$(MessageSectionMingw64)"
!endif
!insertmacro MUI_DESCRIPTION_TEXT ${SectionLlvm}        "$(MessageSectionLlvm)"
!insertmacro MUI_DESCRIPTION_TEXT ${SectionShortcuts}   "$(MessageSectionShortcuts)"
!insertmacro MUI_DESCRIPTION_TEXT ${SectionAssocs}      "$(MessageSectionAssocs)"
!ifdef USER_MODE
!insertmacro MUI_DESCRIPTION_TEXT ${SectionConfig}      "$(MessageSectionConfig)"
!endif
!insertmacro MUI_FUNCTION_DESCRIPTION_END

####################################################################
# Functions, utilities

Function .onInit
  !insertmacro MUI_LANGDLL_DISPLAY
!if "${ARCH}" != "x86"
  SetRegView 64
!endif
!ifdef USER_MODE
  SetShellVarContext current
!else
  SetShellVarContext all
!endif
  ${IfNot} ${AtLeastBuild} 17763 ; OpenConsole.exe requires Windows 10 v1809 ConPTY
!if "${ARCH}" == "x86"
  ${OrIfNot} ${IsNativeIA32} ; OpenConsole.exe x86 only works on x86, while x64 works on arm64
!endif
    SectionSetFlags ${SectionOpenConsole} ${SF_RO}
  ${EndIf}
!if "${ARCH}" == "arm64"
  ${IfNot} ${AtLeastBuild} 22000 ; mingw64 on arm64
    SectionSetFlags ${SectionMingw64} ${SF_RO}
  ${EndIf}
!endif
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

  SetRegView 32
  Call UninstallExisting
  SetRegView 64
  Call UninstallExisting
!if "${ARCH}" == "x86"
  SetRegView 32
!endif
FunctionEnd

Function .onSelChange
  ${IfNot} ${AtLeastBuild} 17763
!if "${ARCH}" == "x86"
  ${OrIfNot} ${IsNativeIA32}
!endif
    SectionSetFlags ${SectionOpenConsole} ${SF_RO}
  ${EndIf}
!if "${ARCH}" == "arm64"
  ${IfNot} ${AtLeastBuild} 22000
    SectionSetFlags ${SectionMingw64} ${SF_RO}
  ${EndIf}
!endif
FunctionEnd

Function un.onInit
  !insertmacro MUI_UNGETLANGUAGE
!if "${ARCH}" != "x86"
  SetRegView 64
!endif
!ifdef USER_MODE
  SetShellVarContext current
!else
  SetShellVarContext all
!endif
FunctionEnd

Function UninstallExisting
!ifdef USER_MODE
  ReadRegStr $R0 HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\RedPanda-C++" "UninstallString"
!else
  ReadRegStr $R0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\RedPanda-C++" "UninstallString"
!endif
  ${If} $R0 != ""
    GetFullPathName $R1 "$R0\.." ; remove \uninstall.exe
    MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
      "$(MessageUninstallExisting)" \
      IDOK uninst
    Abort
    uninst:
      ClearErrors
      ExecWait '"$R0" /S _?=$R1'
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
!ifdef USER_MODE
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\RedPanda-C++"
  DeleteRegKey HKCU "Software\RedPanda-C++"
!else
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\RedPanda-C++"
  DeleteRegKey HKLM "Software\RedPanda-C++"
!endif

!ifdef USER_MODE
  MessageBox MB_YESNO "$(MessageRemoveConfig)" /SD IDNO IDNO SkipRemoveConfig
  RMDir /r "$APPDATA\RedPandaIDE"
SkipRemoveConfig:
!endif
SectionEnd
