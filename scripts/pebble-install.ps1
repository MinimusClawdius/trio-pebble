<#
.SYNOPSIS
  Install a .pbw to the Pebble watch via the phone's Rebble/Pebble app.
.DESCRIPTION
  Uses `pebble install --phone <IP>` inside Docker, or copies the .pbw to a
  location you can open on the phone (AirDrop, shared folder, etc.).
.PARAMETER Target
  Which app to install: "watchface" or "remote".
.PARAMETER PhoneIP
  IP address of the phone running the Rebble/Pebble app (developer connection).
  If omitted, the script prints the .pbw path for manual sideloading.
.PARAMETER Emulator
  If set, installs to the Pebble emulator (aplite/basalt/chalk/diorite/emery).
.PARAMETER Platform
  Emulator platform to target. Default: "basalt".
.EXAMPLE
  .\scripts\pebble-install.ps1 -Target watchface -PhoneIP 192.168.1.42
  .\scripts\pebble-install.ps1 -Target remote
  .\scripts\pebble-install.ps1 -Target watchface -Emulator -Platform chalk
#>
param(
    [Parameter(Mandatory)]
    [ValidateSet("watchface", "remote")]
    [string]$Target,

    [string]$PhoneIP,

    [switch]$Emulator,

    [ValidateSet("aplite", "basalt", "chalk", "diorite", "emery")]
    [string]$Platform = "basalt"
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path $PSScriptRoot -Parent
$dockerImage = "rebble/pebble-sdk:4.5-2"

$projectDir = if ($Target -eq "watchface") { $repoRoot } else { Join-Path $repoRoot "remote-app" }
$label = if ($Target -eq "watchface") { "Trio Pebble (watchface)" } else { "Trio Remote (watch app)" }

$buildDir = Join-Path $projectDir "build"
$pbw = Get-ChildItem $buildDir -Filter "*.pbw" -ErrorAction SilentlyContinue | Select-Object -First 1

if (-not $pbw) {
    Write-Host "No .pbw found in $buildDir" -ForegroundColor Red
    Write-Host "Run pebble-build.ps1 first." -ForegroundColor Yellow
    exit 1
}

Write-Host "`nInstalling: $label" -ForegroundColor Cyan
Write-Host "PBW:        $($pbw.FullName)" -ForegroundColor Cyan

if ($Emulator) {
    Write-Host "Target:     Emulator ($Platform)`n" -ForegroundColor Cyan
    & docker run --rm `
        -v "${projectDir}:/project" `
        -w /project `
        $dockerImage `
        pebble install --emulator $Platform
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Emulator install failed." -ForegroundColor Red
        exit 1
    }
    Write-Host "`nInstalled to $Platform emulator." -ForegroundColor Green
}
elseif ($PhoneIP) {
    Write-Host "Target:     Phone at $PhoneIP`n" -ForegroundColor Cyan

    & docker run --rm `
        --network host `
        -v "${projectDir}:/project" `
        -w /project `
        $dockerImage `
        pebble install --phone $PhoneIP
    if ($LASTEXITCODE -ne 0) {
        Write-Host "`nDirect install failed. Try manual sideload instead." -ForegroundColor Yellow
        Write-Host "See instructions below.`n" -ForegroundColor Yellow
    } else {
        Write-Host "`nInstalled to watch via $PhoneIP" -ForegroundColor Green
        exit 0
    }
}

Write-Host @"

======== Manual Sideload ========

  PBW file: $($pbw.FullName)

  To install on your phone:

  iOS:
    1. Transfer the .pbw to your iPhone (AirDrop, iCloud Drive, email, etc.)
    2. Open the file and choose "Open in Rebble" (or Pebble app)
    3. Confirm the install on the watch

  Android:
    1. Transfer the .pbw to your phone (USB, Google Drive, email, etc.)
    2. Open the file with the Pebble/Rebble app
    3. Confirm the install on the watch

"@ -ForegroundColor White
