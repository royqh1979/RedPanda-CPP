#!/usr/bin/env pwsh

$ErrorActionPreference = "Stop"

$versionInfo = Get-Content ../version.json -Raw | ConvertFrom-Json

$APP_VERSION = "$($versionInfo.major).$($versionInfo.minor).$($versionInfo.patch)"

try {
  $gitCount = $(git rev-list HEAD --count 2>$null)
  if ($LASTEXITCODE -eq 0 -and $gitCount) {
    $APP_VERSION = "$APP_VERSION.$gitCount"
  }
} catch {
  # ignore
}

if (![string]::IsNullOrEmpty($versionInfo.preRelease)) {
  $APP_VERSION = "$APP_VERSION-$($versionInfo.preRelease)"
}

Write-Output $APP_VERSION
