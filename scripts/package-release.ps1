[CmdletBinding()]
param(
    [string]$Version = "0.3.2"
)

$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent $PSScriptRoot
$binaryRoot = Join-Path $projectRoot "build\windows\x64\releasedbg"
$dll = Join-Path $binaryRoot "NavigateVRMapFramework.dll"
$pdb = Join-Path $binaryRoot "NavigateVRMapFramework.pdb"
$releaseRoot = Join-Path $projectRoot "release"
$packageName = "NavigateVR Map Framework $Version"
$stage = Join-Path $releaseRoot $packageName
$archive = Join-Path $releaseRoot "$packageName.zip"
$symbols = Join-Path $releaseRoot "$packageName-symbols.zip"

if (-not (Test-Path -LiteralPath $dll)) {
    throw "Build output was not found: $dll"
}

$fileVersion = (Get-Item -LiteralPath $dll).VersionInfo.FileVersion
if ($fileVersion -notlike "$Version.*") {
    throw "DLL version '$fileVersion' does not match requested release '$Version'."
}

if (Test-Path -LiteralPath $stage) {
    throw "Release staging directory already exists: $stage"
}
if (Test-Path -LiteralPath $archive) {
    throw "Release archive already exists: $archive"
}
if (Test-Path -LiteralPath $symbols) {
    throw "Symbol archive already exists: $symbols"
}

New-Item -ItemType Directory -Path (Join-Path $stage "SKSE\Plugins") -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $stage "docs") -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $stage "schema") -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $stage "examples") -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $stage "THIRD_PARTY_LICENSES") -Force | Out-Null

Copy-Item -LiteralPath $dll -Destination (Join-Path $stage "SKSE\Plugins\NavigateVRMapFramework.dll")
Copy-Item -LiteralPath (Join-Path $projectRoot "config\NavigateVRMapFramework.json") -Destination (Join-Path $stage "SKSE\Plugins\NavigateVRMapFramework.json")
Copy-Item -LiteralPath (Join-Path $projectRoot "README.md") -Destination $stage
Copy-Item -LiteralPath (Join-Path $projectRoot "CHANGELOG.md") -Destination $stage
Copy-Item -LiteralPath (Join-Path $projectRoot "LICENSE") -Destination $stage
Copy-Item -LiteralPath (Join-Path $projectRoot "THIRD_PARTY_NOTICES.md") -Destination $stage
Copy-Item -LiteralPath (Join-Path $projectRoot "lib\commonlibsse-ng\COPYING") -Destination (Join-Path $stage "THIRD_PARTY_LICENSES\CommonLibSSE-NG-GPL-3.0-or-later.txt")
Copy-Item -LiteralPath (Join-Path $projectRoot "lib\commonlibsse-ng\EXCEPTIONS.md") -Destination (Join-Path $stage "THIRD_PARTY_LICENSES\CommonLibSSE-NG-EXCEPTIONS.md")
Copy-Item -LiteralPath (Join-Path $projectRoot "docs\MAP_ADDON_AUTHOR_GUIDE.md") -Destination (Join-Path $stage "docs")
Copy-Item -LiteralPath (Join-Path $projectRoot "schema\navigatevr-maps.schema.json") -Destination (Join-Path $stage "schema")
Copy-Item -LiteralPath (Join-Path $projectRoot "schema\navigatevr-map-framework-settings.schema.json") -Destination (Join-Path $stage "schema")
Copy-Item -Path (Join-Path $projectRoot "config\examples\*") -Destination (Join-Path $stage "examples") -Recurse

Compress-Archive -Path (Join-Path $stage "*") -DestinationPath $archive -CompressionLevel Optimal

if (Test-Path -LiteralPath $pdb) {
    Compress-Archive -LiteralPath $pdb -DestinationPath $symbols -CompressionLevel Optimal
}

Write-Host "Release package: $archive"
if (Test-Path -LiteralPath $symbols) {
    Write-Host "Symbols package: $symbols"
}
