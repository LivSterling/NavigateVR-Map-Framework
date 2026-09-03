[CmdletBinding()]
param(
    [Parameter(Mandatory, ValueFromPipeline)]
    [string[]]$Path
)

$ErrorActionPreference = "Stop"
$seenIDs = [Collections.Generic.HashSet[string]]::new(
    [StringComparer]::OrdinalIgnoreCase)

function Assert-FormSpec {
    param(
        [Parameter(Mandatory)]$Value,
        [Parameter(Mandatory)][string]$Label
    )

    if ($null -eq $Value) {
        throw "$Label is missing."
    }
    if ([string]::IsNullOrWhiteSpace([string]$Value.plugin)) {
        throw "$Label.plugin is missing."
    }
    if ([string]::IsNullOrWhiteSpace([string]$Value.formID)) {
        throw "$Label.formID is missing."
    }

    $text = [string]$Value.formID
    if ($Value.formID -is [string] -and $text -notmatch '^0[xX][0-9A-Fa-f]{1,8}$') {
        throw "$Label.formID must be a hexadecimal string such as 0x000800."
    }
}

function Assert-Matrix {
    param(
        $Value,
        [Parameter(Mandatory)][string]$Label
    )

    if ($null -eq $Value) {
        return
    }
    if ($Value.Count -ne 2) {
        throw "$Label must contain exactly two rows."
    }
    for ($row = 0; $row -lt 2; $row++) {
        if ($Value[$row].Count -ne 3) {
            throw "$Label row $row must contain exactly three numbers."
        }
        foreach ($number in $Value[$row]) {
            if ($number -isnot [ValueType]) {
                throw "$Label contains a non-numeric value."
            }
        }
    }
}

foreach ($inputPath in $Path) {
    $resolved = Resolve-Path -LiteralPath $inputPath
    $document = Get-Content -Raw -LiteralPath $resolved | ConvertFrom-Json

    if ($document.schemaVersion -notin @(1, 2)) {
        throw "$resolved has unsupported schemaVersion '$($document.schemaVersion)'."
    }
    if ([string]::IsNullOrWhiteSpace([string]$document.mapPack)) {
        throw "$resolved has no mapPack name."
    }
    if ($null -eq $document.maps -or $document.maps.Count -eq 0) {
        throw "$resolved has no map definitions."
    }

    foreach ($map in $document.maps) {
        $id = [string]$map.id
        if ([string]::IsNullOrWhiteSpace($id)) {
            throw "$resolved contains a map with no id."
        }
        if (-not $seenIDs.Add($id)) {
            throw "Duplicate map id '$id' was found while validating $resolved."
        }

        Assert-FormSpec $map.worldspace "$id.worldspace"
        Assert-FormSpec $map.items.left "$id.items.left"
        Assert-FormSpec $map.items.right "$id.items.right"

        if ($null -ne $map.selection.match) {
            if ($document.schemaVersion -ne 2) {
                throw "$id.selection.match requires schemaVersion 2."
            }

            $locations = @($map.selection.match.locations | Where-Object { $null -ne $_ })
            $worldspaces = @($map.selection.match.worldspaces | Where-Object { $null -ne $_ })
            if ($locations.Count -eq 0 -and $worldspaces.Count -eq 0) {
                throw "$id.selection.match must declare locations or worldspaces."
            }
            for ($index = 0; $index -lt $locations.Count; $index++) {
                Assert-FormSpec $locations[$index] "$id.selection.match.locations[$index]"
            }
            for ($index = 0; $index -lt $worldspaces.Count; $index++) {
                Assert-FormSpec $worldspaces[$index] "$id.selection.match.worldspaces[$index]"
            }
        }

        if ($map.ownership.required -eq $true) {
            Assert-FormSpec $map.ownership.item "$id.ownership.item"
        }

        if ($null -ne $map.markers.calibration) {
            Assert-Matrix $map.markers.calibration.left "$id.markers.calibration.left"
            Assert-Matrix $map.markers.calibration.right "$id.markers.calibration.right"
        } else {
            # Legacy draft layout retained so older examples remain diagnosable.
            Assert-Matrix $map.markers.CalibrationLeft "$id.markers.CalibrationLeft"
            Assert-Matrix $map.markers.CalibrationRight "$id.markers.CalibrationRight"
        }
    }

    Write-Host "Valid: $resolved ($($document.maps.Count) map definitions)"
}
