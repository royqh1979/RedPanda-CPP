#!/usr/bin/env pwsh

param(
    [Alias("c")]
    [switch]$Clean,

    $QtDir
)

$ErrorActionPreference = "Stop"

. "$PSScriptRoot/config.ps1"

switch ($env:VSCMD_ARG_TGT_ARCH) {
  "x86" { }
  "x64" { }
  "arm64" { }
  default {
    Write-Host "Error: Unsupported architecture '$env:VSCMD_ARG_TGT_ARCH'"
    exit 1
  }
}

$ProjectRoot = $PWD
$Arch = $env:VSCMD_ARG_TGT_ARCH
$BuildDir = "$ProjectRoot/build/msvc-$Arch"
$PkgDir = "$ProjectRoot/build/msvc-$Arch-pkg/RedPanda-CPP"
$DistDir = "$ProjectRoot/dist"

if ([string]::IsNullOrEmpty($QtDir) -and [string]::IsNullOrEmpty($env:CMAKE_TOOLCHAIN_FILE)) {
  Write-Host "Error: Qt directory not specified"
  exit 1
}

function Initialize-Directories {
  if ($Clean) {
    if (Test-Path $BuildDir) {
      Remove-Item -Recurse -Force $BuildDir
    }
    if (Test-Path $PkgDir) {
      Remove-Item -Recurse -Force $PkgDir
    }
  }
  New-Item -ItemType Directory -Force -Path $BuildDir
  New-Item -ItemType Directory -Force -Path $DistDir
}

function Invoke-Build {
  $cmakeArgs = @(
    "-S", ".",
    "-B", $BuildDir,
    "-G", "Ninja",
    "-DCMAKE_BUILD_TYPE=Release",
    "-DCMAKE_INSTALL_PREFIX=$PkgDir"
  )

  if (-not [string]::IsNullOrEmpty($QtDir)) {
    $cmakeArgs += "-DCMAKE_PREFIX_PATH=$QtDir"
  }

  if (-not [string]::IsNullOrEmpty($env:VCPKG_TARGET_TRIPLET)) {
    $cmakeArgs += "-DVCPKG_TARGET_TRIPLET=$env:VCPKG_TARGET_TRIPLET"
  }

  cmake @cmakeArgs
  cmake --build $BuildDir -j $env:NUMBER_OF_PROCESSORS
  cmake --install $BuildDir --strip
}

function Invoke-Package {
  $exePath = "$PkgDir/RedPandaIDE.exe"
  if (-not (Test-Path $exePath)) {
    Write-Host "Error: RedPandaIDE.exe not found at $exePath"
    exit 1
  }

  $windeployqt = "$QtDir/bin/windeployqt.exe"
  if (-not [string]::IsNullOrEmpty($QtDir) -and (Test-Path $windeployqt)) {
    & $windeployqt --release "$exePath"
  }

  $vcRedist = "$PkgDir/vc_redist.$Arch.exe"
  if (Test-Path $vcRedist) {
    Remove-Item -Force $vcRedist
  }

  $archiveName = "redpanda-cpp-msvc-$AppVersion-$Arch.zip"
  Compress-Archive -Path "$PkgDir" -DestinationPath "$DistDir/$archiveName" -Force
}

Initialize-Directories
Invoke-Build
Invoke-Package
