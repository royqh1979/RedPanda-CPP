#!/usr/bin/env pwsh

function Get-AppVersion {
  $versionInfo = Get-Content version.json -Raw | ConvertFrom-Json
  $AppVersion = "$($versionInfo.major).$($versionInfo.minor).$($versionInfo.patch)"

  try {
    $gitCount = (git rev-list HEAD --count 2>&1 | Out-String)
    if ($LASTEXITCODE -eq 0 -and $gitCount) {
      $gitCount = $gitCount.Trim()
      $AppVersion = "$AppVersion.$gitCount"
    }
  } catch {
    # ignore
  }

  if (![string]::IsNullOrEmpty($versionInfo.preRelease)) {
    $AppVersion = "$AppVersion-$($versionInfo.preRelease)"
  }

  return $AppVersion
}

$script:AppVersion = Get-AppVersion
