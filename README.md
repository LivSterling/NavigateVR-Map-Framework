# NavigateVR Map Framework

NavigateVR Map Framework is an SKSEVR plugin that lets independent NavigateVR
map packs register physical maps by worldspace. It adds this support without
replacing NavigateVR's Papyrus scripts and without hard-coding every map pack
into the DLL.

Each addon ships one JSON file. When the player draws NavigateVR's **Map of
Provinces and Isles**, the framework:

1. Determines the player's current exterior worldspace, or uses the last
   exterior worldspace it observed while the player is inside.
2. Finds the highest-priority enabled map definition for that worldspace.
3. Optionally checks for a miscellaneous ownership item.
4. Lets NavigateVR perform its normal draw operation.
5. Replaces the normal displayed map armor with the registered left- or
   right-hand map armor.
6. Removes only the framework-managed armor when the map is stowed.

When no definition matches, NavigateVR is left completely alone.

If NavigateVR already equipped the exact armor selected by a definition, the
framework treats that as success and does not equip it again. Framework-driven
equipment changes are guarded so their equip events cannot recursively trigger
another replacement.

## Requirements

- Skyrim VR 1.4.15
- SKSEVR
- NavigateVR — Equipable Dynamic Compass and Maps
- Microsoft Visual C++ Redistributable 2015–2022 (x64)

The DLL was built with CommonLibSSE-NG from the `ng` branch of
`alandtse/CommonLibVR`.

## Installation

Install with Mod Organizer 2 or another mod manager. The framework package
contains:

```text
SKSE/
└── Plugins/
    ├── NavigateVRMapFramework.dll
    └── NavigateVRMapFramework.json
```

The required settings file identifies NavigateVR's controller weapon by plugin,
EditorID, and an optional plugin-local FormID fallback. The shipped defaults
work with the original NavigateVR plugin. EditorID lookup allows the framework
to find `TRC_WorldMap` after FormID compaction, while the owning-plugin check
prevents an unrelated record with the same EditorID from being accepted.

Users with a renamed or customized NavigateVR plugin can edit:

```json
{
  "schemaVersion": 1,
  "navigateVRController": {
    "plugin": "Navigate VR - Equipable Dynamic Compass and Maps.esp",
    "editorID": "TRC_WorldMap",
    "formID": "0x037482"
  }
}
```

The FormID is local to the named plugin, not a complete load-order FormID. It
may be set to `null` when EditorID lookup is sufficient. At least one of
`editorID` or `formID` must be present. If this file is missing or invalid, the
framework logs the problem and disables map selection instead of using a
compiled plugin name or FormID.

Map addons install definitions under:

```text
SKSE/Plugins/NavigateVRMaps/*.json
```

Do not combine every addon into one central JSON file. Each addon should own
one uniquely named file so it can be installed and removed independently.

## Important runtime behavior

- Selection occurs when the Map of Provinces and Isles is drawn.
- Changing worldspaces while the map remains drawn does not live-switch it.
  Stow and redraw the map after changing worldspaces.
- Interior support uses an in-memory cache of the last exterior worldspace
  observed when the controller map was previously drawn.
- The cache is not saved in the player's save file. Loading directly into an
  interior before the framework has observed its exterior may fall back to
  NavigateVR's normal map.
- Matching is by exact WRLD record. Tamriel hold boundaries are still handled
  by NavigateVR's original logic.
- Worldspace display names are not unique identifiers. For example,
  `BSHeartland.esm:0x0A764B` is named `Tamriel` in-game but is a separate
  worldspace from `Skyrim.esm:0x00003C`. A Bruma definition must target the
  BSHeartland record; targeting Skyrim's Tamriel record would select the Bruma
  map throughout Skyrim.
- Map addon ESPs and their JSON files may be removed independently. Entries
  whose plugins or forms cannot be resolved are skipped safely and logged.

## Ownership

Ownership is optional per map:

```json
"ownership": {
  "required": true,
  "item": {
    "plugin": "NavigateVR - Dawnguard Maps.esp",
    "formID": "0x00080C"
  }
}
```

The item must be a `MISC` record. If ownership is required, the map is selected
only while the player carries at least one copy. Omitting `ownership`, or
setting `required` to `false`, makes the map always available.

## Priorities

More than one addon may target the same worldspace. Higher `priority` values
win. Use `100` for an ordinary exact-worldspace map. Reserve higher values for
intentional replacers or compatibility patches.

When equal-priority definitions target the same worldspace, selection is
deterministic by JSON filename and map ID, but authors should avoid relying on
that tie-break.

## Logging

The framework writes:

```text
Documents/My Games/Skyrim VR/SKSE/NavigateVRMapFramework.log
```

The log reports:

- The framework controller settings loaded
- Whether the NavigateVR controller resolved by EditorID or FormID fallback
- Every JSON file parsed
- Every definition successfully resolved
- Missing plugins, forms, or ownership items
- The worldspace detected when the controller is drawn
- The map selected and the hand used

## Creating an addon

See [Map Addon Author Guide](docs/MAP_ADDON_AUTHOR_GUIDE.md) for the complete
NIF, DDS, ESPFE, ownership, calibration, JSON, packaging, and testing workflow.

The machine-readable schema is
[navigatevr-maps.schema.json](schema/navigatevr-maps.schema.json).
Framework settings use
[navigatevr-map-framework-settings.schema.json](schema/navigatevr-map-framework-settings.schema.json).

## Building from source

Requirements:

- Visual Studio 2022 with Desktop development with C++
- XMake
- Git

Clone recursively, then run:

```powershell
git clone --recurse-submodules https://github.com/LivSterling/NavigateVR-Map-Framework.git
cd NavigateVR-Map-Framework
.\scripts\build.ps1
```

The DLL is produced under:

```text
build/windows/x64/releasedbg/NavigateVRMapFramework.dll
```

## Current scope

This framework selects physical map armors. It does not:

- Replace NavigateVR's stowing or map-case logic
- Add world-map markers by itself
- Alter map textures at runtime
- Detect arbitrary Tamriel subregions
- Automatically manufacture map armor records from textures
- Persist the last exterior worldspace across saved games

Map marker calibration data may coexist in the same addon JSON under the
`markers.calibration.left` and `markers.calibration.right` fields. The selector
ignores marker-presenter fields it does not use.

## License

The framework's original source is released under the [MIT License](LICENSE).
The compiled DLL statically links CommonLibSSE-NG and is distributed with its
GPL-3.0-or-later terms and linking exceptions. See
[Third-party notices](THIRD_PARTY_NOTICES.md).
