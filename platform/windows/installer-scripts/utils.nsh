Var /GLOBAL osArch
Var /GLOBAL sectionDepFlag
Var /GLOBAL sectionDepTemp

!macro DetectOsArch
  ${If} ${IsNativeIA32}
    StrCpy $osArch "x86"
  ${ElseIf} ${IsNativeAMD64}
    StrCpy $osArch "x64"
  ${ElseIf} ${IsNativeARM64}
    StrCpy $osArch "arm64"
  ${Else}
    StrCpy $osArch "unknown"
  ${EndIf}
!macroend

!macro CheckOsArch
  !ifdef STRICT_ARCH_CHECK
    ${If} $osArch != "${ARCH}"
      MessageBox MB_OK|MB_ICONSTOP "$(ErrorStrictArchMismatch)"
      Abort
    ${EndIf}
  !else
    !if "${ARCH}" == "x64"
      ${If} $osArch == "x86"
        MessageBox MB_OK|MB_ICONSTOP "$(ErrorArchMismatch)"
        Abort
      ${EndIf}
    !endif

    !if "${ARCH}" == "arm64"
      ${If} $osArch != "arm64"
        MessageBox MB_OK|MB_ICONSTOP "$(ErrorArchMismatch)"
        Abort
      ${EndIf}
    !endif

    ; warning if not matching
    ${If} $osArch != "${ARCH}"
      MessageBox MB_OK|MB_ICONEXCLAMATION "$(WarningArchMismatch)"
    ${EndIf}
  !endif
!macroend

!macro CheckOsBuild
  ${IfNot} ${IsNT}
  ${OrIfNot} ${AtLeastBuild} ${REQUIRED_WINDOWS_BUILD}
    MessageBox MB_OK|MB_ICONSTOP "$(ErrorWindowsBuildRequired)"
    Abort
  ${EndIf}
!macroend

!macro CheckV2Installer
  SetRegView 32
  Call UninstallV2
  SetRegView 64
  Call UninstallV2
!macroend

!macro CheckPreviousInstaller
  SetRegView 32
  Call UninstallExisting
  SetRegView 64
  Call UninstallExisting
  !if "${ARCH}" == "x86"
    SetRegView 32
  !else
    SetRegView 64
  !endif
!macroend

!macro DisableSection section
  !insertmacro UnselectSection ${section}
  !insertmacro SetSectionFlag ${section} ${SF_RO}
!macroend

!macro EnableSection section
  !insertmacro ClearSectionFlag ${section} ${SF_RO}
!macroend

!macro SectionAction_CheckMingw64
  !ifdef HAVE_MINGW64
    ${If} $osArch == "x86"
      !insertmacro DisableSection ${SectionMingw64}
    ${EndIf}
  !endif
!macroend

!macro SectionAction_CheckCompress
  ; compact os is available since windows 10
  ${IfNot} ${AtLeastBuild} 10240
    !insertmacro DisableSection ${SectionCompress}
  ${EndIf}
!macroend

!macro SectionAction_CheckUcrt
  !ifdef HAVE_UCRT
    !insertmacro SectionFlagIsSet ${SectionUcrtExtract} ${SF_SELECTED} extract_ucrt_check_version no_extract_ucrt_disable_install

  extract_ucrt_check_version:
    ${If} ${AtLeastBuild} 10240
      ; Windows 10: cannot install UCRT (system component)
      !insertmacro DisableSection ${SectionUcrtInstall}
    !if "${ARCH}" == "x64"
      ${ElseIfNot} ${AtLeastBuild} 6000
        ; Windows Server 2003 x64 Edition: x86 vcredist does not install x64 UCRT
        !insertmacro DisableSection ${SectionUcrtInstall}
    !endif
    ${Else}
      !insertmacro EnableSection ${SectionUcrtInstall}
    ${EndIf}
    goto done_ucrt_check

  no_extract_ucrt_disable_install:
    !insertmacro DisableSection ${SectionUcrtInstall}

  done_ucrt_check:
  !endif
!macroend
