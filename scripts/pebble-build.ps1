<#
.SYNOPSIS
  Build Trio Pebble watchface and/or Trio Remote app using Docker.
.DESCRIPTION
  Runs `pebble build` inside the rebble/pebble-sdk Docker container.
  Produces .pbw files in the respective build/ directories.
  Automatically handles sdkVersion swap (3 for local, 4.9.148 for CloudPebble).
.PARAMETER Target
  What to build: "watchface", "remote", or "all" (default: "all").
.PARAMETER Clean
  If set, removes build/ and .waf-* before building.
.EXAMPLE
  .\scripts\pebble-build.ps1
  .\scripts\pebble-build.ps1 -Target watchface
  .\scripts\pebble-build.ps1 -Target remote -Clean
#>
param(
    [ValidateSet("watchface", "remote", "all")]
    [string]$Target = "all",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path $PSScriptRoot -Parent
$dockerImage = "rebble/pebble-sdk:4.5-2"

if (-not $env:PATH.Contains("Docker\Docker\resources\bin")) {
    $env:PATH = "C:\Program Files\Docker\Docker\resources\bin;" + $env:PATH
}

function Test-Docker {
    try {
        $null = & docker version 2>&1
        return $LASTEXITCODE -eq 0
    } catch {
        return $false
    }
}

function Invoke-PebbleBuild {
    param([string]$ProjectDir, [string]$Label)

    Write-Host "`n========================================" -ForegroundColor Cyan
    Write-Host "  Building: $Label" -ForegroundColor Cyan
    Write-Host "  Source:   $ProjectDir" -ForegroundColor Cyan
    Write-Host "========================================`n" -ForegroundColor Cyan

    if ($Clean) {
        $buildDir = Join-Path $ProjectDir "build"
        if (Test-Path $buildDir) {
            Remove-Item $buildDir -Recurse -Force
            Write-Host "Cleaned build/" -ForegroundColor Yellow
        }
        Get-ChildItem $ProjectDir -Filter ".waf-*" -Directory | Remove-Item -Recurse -Force
    }

    & docker run --rm `
        -e HOME=/home/pebble `
        -v "${ProjectDir}:/project" `
        -w /project `
        --user root `
        $dockerImage `
        sh -c "touch /home/pebble/.pebble-sdk/NO_TRACKING 2>/dev/null; su pebble -c 'pebble build'"

    if ($LASTEXITCODE -ne 0) {
        Write-Host "`n  BUILD FAILED: $Label" -ForegroundColor Red
        return $false
    }

    $pbw = Get-ChildItem (Join-Path $ProjectDir "build") -Filter "*.pbw" -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($pbw) {
        Write-Host "`n  SUCCESS: $($pbw.FullName)" -ForegroundColor Green
        Write-Host "  Size:    $([math]::Round($pbw.Length / 1KB, 1)) KB" -ForegroundColor Green
    } else {
        Write-Host "`n  Build completed but no .pbw found" -ForegroundColor Yellow
    }
    return $true
}

if (-not (Test-Docker)) {
    Write-Host @"

  Docker is not running or not installed.

  Install Docker Desktop for Windows:
    https://docs.docker.com/desktop/setup/install/windows-install/

  Then pull the Pebble SDK image:
    docker pull $dockerImage

"@ -ForegroundColor Red
    exit 1
}

$results = @()

if ($Target -eq "watchface" -or $Target -eq "all") {
    $ok = Invoke-PebbleBuild -ProjectDir $repoRoot -Label "Trio Pebble (watchface) v$((Get-Content (Join-Path $repoRoot 'package.json') | ConvertFrom-Json).version)"
    $results += @{ Name = "Trio Pebble"; Success = $ok }
}

if ($Target -eq "remote" -or $Target -eq "all") {
    $remoteDir = Join-Path $repoRoot "remote-app"
    if (-not (Test-Path $remoteDir)) {
        Write-Host "remote-app/ not found at $remoteDir" -ForegroundColor Red
        exit 1
    }
    $ok = Invoke-PebbleBuild -ProjectDir $remoteDir -Label "Trio Remote (watch app) v$((Get-Content (Join-Path $remoteDir 'package.json') | ConvertFrom-Json).version)"
    $results += @{ Name = "Trio Remote"; Success = $ok }
}

Write-Host "`n======== Build Summary ========" -ForegroundColor Cyan
foreach ($r in $results) {
    $color = if ($r.Success) { "Green" } else { "Red" }
    $status = if ($r.Success) { "PASS" } else { "FAIL" }
    Write-Host "  [$status] $($r.Name)" -ForegroundColor $color
}
Write-Host ""

if ($results | Where-Object { -not $_.Success }) { exit 1 }
