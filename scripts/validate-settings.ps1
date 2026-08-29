[CmdletBinding()]
param(
    [string]$Path = (Join-Path (Split-Path -Parent $PSScriptRoot) "config\NavigateVRMapFramework.json")
)

$ErrorActionPreference = "Stop"
$resolved = Resolve-Path -LiteralPath $Path
$document = Get-Content -Raw -LiteralPath $resolved | ConvertFrom-Json

if ($document.schemaVersion -ne 1) {
    throw "$resolved has unsupported schemaVersion '$($document.schemaVersion)'."
}

$controller = $document.navigateVRController
if ($null -eq $controller) {
    throw "$resolved has no navigateVRController object."
}
if ([string]::IsNullOrWhiteSpace([string]$controller.plugin)) {
    throw "$resolved has no navigateVRController.plugin value."
}

$hasEditorID = -not [string]::IsNullOrWhiteSpace([string]$controller.editorID)
$hasFormID = $null -ne $controller.formID
if (-not $hasEditorID -and -not $hasFormID) {
    throw "$resolved requires navigateVRController.editorID, formID, or both."
}
if ($hasFormID -and $controller.formID -is [string] -and
    [string]$controller.formID -notmatch '^0[xX][0-9A-Fa-f]{1,8}$') {
    throw "$resolved navigateVRController.formID must be hexadecimal, such as 0x037482."
}
if ($hasFormID -and $controller.formID -isnot [string] -and
    ([int64]$controller.formID -lt 0 -or [uint64]$controller.formID -gt [uint32]::MaxValue)) {
    throw "$resolved navigateVRController.formID must fit in an unsigned 32-bit FormID."
}

Write-Host "Valid framework settings: $resolved"
