[CmdletBinding()]
param(
    [ValidateSet("debug", "releasedbg")]
    [string]$Mode = "releasedbg",

    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent $PSScriptRoot

if (-not (Get-Command xmake -ErrorAction SilentlyContinue)) {
    throw "XMake was not found on PATH. Install XMake and open a new terminal."
}

Push-Location $projectRoot
try {
    if ($Clean) {
        & xmake f -c -m $Mode -y
    } else {
        & xmake f -m $Mode -y
    }
    if ($LASTEXITCODE -ne 0) {
        throw "XMake configuration failed with exit code $LASTEXITCODE."
    }

    & xmake build
    if ($LASTEXITCODE -ne 0) {
        throw "XMake build failed with exit code $LASTEXITCODE."
    }
} finally {
    Pop-Location
}
