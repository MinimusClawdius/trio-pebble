# Sync trio-pebble/remote-app -> sibling ../trio-pebble-remote (standalone CloudPebble repo).
# Run from anywhere:  pwsh -File scripts/sync-trio-pebble-remote.ps1
$ErrorActionPreference = "Stop"
$repoRoot = Split-Path $PSScriptRoot -Parent
$src = Join-Path $repoRoot "remote-app"
$dst = Join-Path (Split-Path $repoRoot -Parent) "trio-pebble-remote"
if (-not (Test-Path $src)) { throw "remote-app not found: $src" }
if (-not (Test-Path $dst)) { throw "trio-pebble-remote not found: $dst" }

Copy-Item (Join-Path $src "package.json") (Join-Path $dst "package.json") -Force
Copy-Item (Join-Path $src "wscript") (Join-Path $dst "wscript") -Force
Copy-Item (Join-Path $src "src\*") (Join-Path $dst "src") -Recurse -Force
Write-Host "Synced remote-app -> $dst (ensure resources/images/menu_icon.png exists in destination)."
