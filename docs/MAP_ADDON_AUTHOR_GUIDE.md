# NavigateVR Map Addon Author Guide

This guide describes the complete workflow for making a modular map pack for
NavigateVR Map Framework. The Dawnguard pack is the reference implementation.

The finished addon normally contains:

```text
My NavigateVR Map Pack/
├── My NavigateVR Map Pack.esp
├── meshes/
├── textures/
├── SKSE/
│   └── Plugins/
│       └── NavigateVRMaps/
│           └── my-map-pack.json
└── optional acquisition assets
    ├── Scripts/
    ├── Source/Scripts/
    └── SEQ/
```

The framework DLL is shared. Do not include another copy of the DLL in each
map pack.

## 1. Choose the map and worldspace

Find the `WRLD` record represented by the texture in xEdit. Record:

- The plugin that defines the worldspace
- Its plugin-local FormID
- Its EditorID for your notes

Use the defining plugin and local FormID, not the load-order-prefixed runtime
FormID shown as `FE xxx yyy`.

Exact worldspaces are ideal:

- Soul Cairn
- Forgotten Vale
- Dayspring Canyon
- Blackreach
- Bruma
- Wyrmstooth
- The five enclosed vanilla city worldspaces

All Skyrim holds share `Tamriel`, so a WRLD match cannot distinguish one hold
from another. Continue using NavigateVR's original hold-selection behavior for
those maps.

Schema version 2 can select an open town by its `LCTN` while retaining Tamriel
as the map's coordinate worldspace. Optional WRLD aliases cover overhauls that
move the town into a separate worldspace. Uninstalled alias plugins are ignored
safely and do not become ESP masters.

## 2. Prepare textures

Create a diffuse map and normal map for each hand:

```text
LMyMap.dds
LMyMap_n.dds
RMyMap.dds
RMyMap_n.dds
```

Recommended release properties:

- Square texture
- BC7 compression
- Full mip chain
- Unique path owned by your addon

Example:

```text
textures/NavigateVR/MyMapPack/LMyMap.dds
textures/NavigateVR/MyMapPack/LMyMap_n.dds
textures/NavigateVR/MyMapPack/RMyMap.dds
textures/NavigateVR/MyMapPack/RMyMap_n.dds
```

Do not overwrite NavigateVR's original textures unless you are deliberately
releasing a replacer.

## 3. Create the left- and right-hand NIFs

Copy a known-good NavigateVR left-hand map NIF and a matching right-hand map
NIF. Save them under your own mesh path:

```text
meshes/NavigateVR/MyMapPack/LMyMap.nif
meshes/NavigateVR/MyMapPack/RMyMap.nif
```

In NifSkope, edit the `BSShaderTextureSet`:

```text
slot 0 = textures\NavigateVR\MyMapPack\LMyMap.dds
slot 1 = textures\NavigateVR\MyMapPack\LMyMap_n.dds
```

Repeat for the right-hand NIF.

Preserve the donor mesh's skeleton and biped partition:

| Hand | Bone | Biped slot |
|---|---|---:|
| Left | `NPC L Hand [LHnd]` | 45 |
| Right | `NPC R Hand [RHnd]` | 44 |

Do not merely mirror a left-hand NIF and assume it is a valid right-hand map.
Use the correct donor for each hand.

Before authoring the ESP, temporarily repoint or override a working NavigateVR
map with the new NIFs and check:

- Correct side and orientation
- No mirroring
- No stretching
- Correct normal-map direction
- Readable scale and folds
- Both hands

## 4. Create armor addon records

Create one `ARMA` record per hand and map. The Dawnguard naming convention is:

```text
TRC_LMyMapAA
TRC_RMyMapAA
```

The left `ARMA` must:

- Use biped slot 45
- Point its male and female world/first-person model fields at the left NIF
- Retain the NavigateVR donor's compatible race list

The right `ARMA` must:

- Use biped slot 44
- Point at the right NIF
- Retain the compatible race list

Copying a NavigateVR donor record in xEdit is safer than creating an ARMA from
an empty record.

## 5. Create map armor records

Create one `ARMO` per hand:

```text
TRC_LMyMap
TRC_RMyMap
```

Each armor should:

- Reference its corresponding ARMA
- Use slot 45 for left or slot 44 for right
- Remain a hidden/display armor rather than a normal wearable inventory item
- Follow the flags and values of the NavigateVR donor armor

The framework temporarily adds the selected armor, equips it, and removes its
own temporary copy when the controller is stowed.

## 6. Decide whether ownership is required

Ownership is represented by a miscellaneous `MISC` item. The displayed ARMO is
not the item the player buys or discovers.

Create an ownership item when acquisition matters:

```text
NVRMP_MapMyMap
Name: Map of My Map
```

Possible acquisition methods include:

- Vendor dialogue
- Placement in the world
- Quest reward
- Crafting recipe
- Reusing an existing NavigateVR ownership item

Make any acquisition conditions stop presenting the option after the player
owns the item.

If dialogue lives on a new Start-Game-Enabled quest:

1. Provide the compiled result-fragment `.pex`.
2. Ship the `.psc` source for other authors.
3. Generate `SEQ/<plugin filename>.seq`.
4. Recreate the SEQ after changing masters, compacting FormIDs, or changing
   which quests are Start-Game-Enabled.

Unvoiced dialogue still needs deliberate testing. A response such as `...`
avoids written NPC dialogue, but without a silent `.fuz` it may advance
immediately depending on subtitle and dialogue timing.

## 7. Compact and finalize the ESPFE

Do this before writing the final JSON:

1. Finish creating all records.
2. Compact FormIDs for ESL in xEdit if required.
3. Flag the ESP as ESL.
4. Finalize the plugin filename.
5. Save and reload it in xEdit.
6. Run Check for Errors.

Compaction changes the local FormIDs. Never compact after publishing JSON,
Papyrus calls, voice filenames, or other external references to those records.

## 8. Create the modular JSON

The framework-wide `SKSE/Plugins/NavigateVRMapFramework.json` file identifies
NavigateVR's controller weapon. It belongs to the framework package, not to an
individual map addon. Do not bundle or overwrite that settings file from a map
pack. Map addons contribute only their own definitions under
`SKSE/Plugins/NavigateVRMaps`.

Install the canonical definition under:

```text
SKSE/Plugins/NavigateVRMaps/my-map-pack.json
```

Minimal definition:

```json
{
  "schemaVersion": 1,
  "mapPack": "My Map Pack",
  "maps": [
    {
      "id": "my-map",
      "worldspace": {
        "plugin": "MyWorld.esm",
        "formID": "0x001234"
      },
      "items": {
        "left": {
          "plugin": "My NavigateVR Map Pack.esp",
          "formID": "0x000800"
        },
        "right": {
          "plugin": "My NavigateVR Map Pack.esp",
          "formID": "0x000801"
        }
      },
      "ownership": {
        "required": true,
        "item": {
          "plugin": "My NavigateVR Map Pack.esp",
          "formID": "0x000802"
        }
      },
      "selection": {
        "enabled": true,
        "priority": 100,
        "useForInteriors": true
      },
      "markers": {
        "calibration": {
          "left": null,
          "right": null
        }
      }
    }
  ]
}
```

FormIDs in JSON are plugin-local:

```text
Correct:   "plugin": "My Map Pack.esp", "formID": "0x000800"
Incorrect: "formID": "0xFE123800"
```

### Definition fields

| Field | Meaning |
|---|---|
| `schemaVersion` | JSON format version; `1` for exact-WRLD definitions, `2` when using `selection.match` |
| `mapPack` | Human-readable pack name |
| `id` | Stable identifier unique within the installed registry |
| `worldspace` | Exact represented WRLD |
| `items.left` | Left-hand ARMO |
| `items.right` | Right-hand ARMO |
| `ownership.required` | Whether the MISC item is enforced |
| `ownership.item` | Ownership MISC when required |
| `selection.enabled` | Allows temporarily disabling an entry |
| `selection.priority` | Higher wins when two maps share a worldspace |
| `selection.useForInteriors` | Allows cached exterior matching indoors |
| `selection.match.locations` | Optional `LCTN` forms used to select open-town and regional maps |
| `selection.match.worldspaces` | Optional alternate `WRLD` aliases, including overhaul-added spaces |
| `selection.match.includeChildLocations` | Whether descendant locations also match; defaults to `true` |
| `markers.calibration.left` | Left-hand 2x3 world-to-UV matrix |
| `markers.calibration.right` | Right-hand 2x3 world-to-UV matrix |

Unknown top-level map fields are ignored by the selector, allowing other
NavigateVR tools to share the definition.

### Open-town selection

Use schema version 2 when a map depicts a town inside Skyrim's shared Tamriel
worldspace:

```json
{
  "schemaVersion": 2,
  "mapPack": "Town Maps",
  "maps": [
    {
      "id": "town_riverwood",
      "worldspace": {
        "plugin": "Skyrim.esm",
        "formID": "0x00003C"
      },
      "items": {
        "left": {
          "plugin": "NavigateVR - Town Maps.esp",
          "formID": "0x000800"
        },
        "right": {
          "plugin": "NavigateVR - Town Maps.esp",
          "formID": "0x000801"
        }
      },
      "selection": {
        "enabled": true,
        "priority": 100,
        "useForInteriors": true,
        "match": {
          "locations": [
            {
              "plugin": "Skyrim.esm",
              "formID": "0x013163"
            }
          ],
          "includeChildLocations": true
        }
      }
    }
  ]
}
```

The top-level `worldspace` still describes the coordinate system represented
by the texture and is available to map-marker tools. Once `selection.match` is
present, the selector uses only its declared criteria; it does not treat that
top-level Tamriel form as a selection rule.

`locations` and `worldspaces` are OR criteria. Optional aliases can therefore
support a city overhaul without making it a plugin master:

```json
"match": {
  "locations": [
    { "plugin": "Skyrim.esm", "formID": "0x018A49" }
  ],
  "worldspaces": [
    { "plugin": "Holds.esp", "formID": "0x000D62" },
    { "plugin": "Holds.esp", "formID": "0x15E3A7" }
  ],
  "includeChildLocations": true
}
```

If `Holds.esp` is absent, those WRLD aliases are logged and ignored while the
vanilla Falkreath location continues to work. If it is installed, either its
preserved vanilla location or either declared WRLD can select the town map.

## 9. Calibrate map markers

Use the browser-based NavigateVR Map Calibrator:

```text
https://lxe97.github.io/navigatevr-map-calibrator/
```

For each hand:

1. Load the exact DDS used by that hand's NIF.
2. Enter the map armor plugin and local FormID.
3. Enter the represented worldspace plugin and local FormID.
4. Add at least three widely separated, non-collinear reference points.
5. Enter each point's Skyrim world X/Y coordinates.
6. Place its texture point on the same landmark.
7. Calibrate.
8. Test additional points that were not used in the solution.
9. Copy the resulting 2x3 matrix into `markers.calibration.left` or
   `markers.calibration.right`.

The affine transform is:

```text
u = a*x + b*y + c
v = d*x + e*y + f
```

`u` and `v` are normalized texture coordinates. Calibration is geographic; it
does not replace the NIF's hand-local transform.

The current Map Markers calibrator exports the nested `markers.calibration`
layout shown above. Preserve every generated number exactly; rounding the
matrix can visibly move markers on tightly cropped maps.

NavigateVR Map Framework and compatible marker presenters read the same
canonical definition:

```text
SKSE/Plugins/NavigateVRMaps/my-map-pack.json
```

## 10. Package the addon

Example release tree:

```text
My NavigateVR Map Pack/
├── My NavigateVR Map Pack.esp
├── meshes/
│   └── NavigateVR/
│       └── MyMapPack/
│           ├── LMyMap.nif
│           └── RMyMap.nif
├── textures/
│   └── NavigateVR/
│       └── MyMapPack/
│           ├── LMyMap.dds
│           ├── LMyMap_n.dds
│           ├── RMyMap.dds
│           └── RMyMap_n.dds
├── SKSE/
│   └── Plugins/
│       └── NavigateVRMaps/
│           └── my-map-pack.json
├── Scripts/
├── Source/
│   └── Scripts/
└── SEQ/
```

Do not ship:

- Build logs
- PDB files in the normal user download
- Duplicate definitions in consumer-specific directories
- Temporary donor overrides
- A second framework DLL

## 11. Test the finished pack

### Static tests

- ESP parses and is ESL-flagged
- No missing masters or dangling FormLinks
- ARMO → ARMA → NIF links are correct
- NIF texture paths exist
- Left NIF uses LHnd/slot 45
- Right NIF uses RHnd/slot 44
- DDS files have valid dimensions, compression, and mipmaps
- JSON parses
- Every plugin/FormID pair resolves to the expected record type
- Required ownership form is a MISC
- SEQ and compiled Papyrus fragments are present when applicable

### In-game tests

- Acquire each ownership item naturally
- Confirm each acquisition has a fallback when reasonable
- Draw in the correct exterior worldspace
- Test both hands
- Stow and redraw repeatedly
- Change worldspaces, then stow and redraw
- Enter an interior after the correct exterior has been observed
- Load directly inside an interior
- Remove the ownership item and verify fallback behavior
- Test without the map-marker mod installed
- Test with only the release pack enabled, with donor/test overrides disabled
- Inspect `NavigateVRMapFramework.log`

## 12. City map example

The five enclosed vanilla cities have exact child worldspaces:

| City | Worldspace |
|---|---|
| Whiterun | `01A26F:Skyrim.esm` |
| Solitude | `037EDF:Skyrim.esm` |
| Riften | `016BB4:Skyrim.esm` |
| Windhelm | `01691D:Skyrim.esm` |
| Markarth | `016D71:Skyrim.esm` |

A five-city pack needs:

- Five left and five right texture pairs
- Five left and five right NIFs
- Ten ARMA records
- Ten ARMO records
- Five JSON map entries
- Up to five ownership MISC records
- Ten calibration matrices

Reusing NavigateVR's existing hold-map ownership items may be more compatible
than adding five new purchase systems. Test this design against users who do
not own every hold map.

Open-city compatibility can use the same version-2 location and optional-WRLD
matching described above. Confirm the overhaul's authored `LCTN` or `WRLD`
records rather than copying a generated DynDOLOD or Occlusion override.

## Release checklist

- [ ] Unique mod and asset paths
- [ ] Final plugin filename
- [ ] ESPFE compacted before external references were published
- [ ] Check for Errors clean
- [ ] Both hands tested
- [ ] Ownership acquired and enforced
- [ ] Interior behavior tested
- [ ] No test texture or JSON override wins in the mod manager
- [ ] JSON schema validated
- [ ] Marker calibration tested with unused points
- [ ] Compiled scripts and SEQ included
- [ ] Requirements, permissions, and credits documented
