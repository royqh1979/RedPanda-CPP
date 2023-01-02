#!/usr/bin/env pwsh

Set-PSDebug -Trace 1

$arch = "aarch64"

docker run --rm -v "$(Get-Location):/build/RedPanda-CPP" -e CARCH=$arch redpanda-builder-$arch /build/RedPanda-CPP/packages/appimage/01-in-docker.sh
