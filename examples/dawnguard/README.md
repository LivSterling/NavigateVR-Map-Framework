# NavigateVR — Dawnguard Maps

Adds physical NavigateVR maps for:

- Dayspring Canyon
- The Soul Cairn
- The Forgotten Vale

The maps are selected automatically by NavigateVR Map Framework when the
player draws NavigateVR's **Map of Provinces and Isles** in the corresponding
worldspace.

## Requirements

- Skyrim VR
- Dawnguard
- SKSEVR
- NavigateVR — Equipable Dynamic Compass and Maps
- NavigateVR Map Framework 0.3.1 or later

## Installation

Install the archive with Mod Organizer 2 and enable:

```text
NavigateVR - Dawnguard Maps.esp
```

Keep the addon after NavigateVR in the left pane. Do not allow another test or
donor mod to overwrite the addon JSON or textures unless that override is
intentional.

## Acquiring the maps

The framework enforces ownership of a miscellaneous map item. The hidden ARMO
forms displayed in the player's hands are not the objects being purchased or
picked up.

### Dayspring Canyon

Ask Sorine Jurard for a map while inside Fort Dawnguard. It costs 100 gold.

### Soul Cairn

Valerica's hand-drawn map is placed among her research materials. If the player
misses the placed copy, Valerica can provide another through dialogue.

### Forgotten Vale

A map is placed at Knight-Paladin Gelebor's camp. Gelebor can provide another
through dialogue if the placed copy was missed.

The acquisition topics disappear once the player owns the corresponding map.

## Using the maps

1. Acquire the map's miscellaneous ownership item.
2. Enter the matching worldspace.
3. Equip and draw NavigateVR's Map of Provinces and Isles.
4. Stow and redraw after changing worldspaces.

Both hands are supported.

Inside an interior, the framework uses the last exterior worldspace it observed
on an earlier map draw. Loading a save directly into an interior before drawing
a map in the associated exterior may fall back to NavigateVR's normal map.

## Map markers

The included JSON contains tested left- and right-hand calibration matrices for
all three maps. The selector and compatible marker presenters share this one
definition under `SKSE/Plugins/NavigateVRMaps`. Map Markers itself remains
optional.

## Dialogue and audio

The three acquisition replies intentionally contain only `...`; no custom
voice acting is included. Their result scripts are compiled and grant or sell
the ownership items. Depending on subtitle settings, the silent response may
advance quickly because no silent `.fuz` duration file is included.

## Compatibility

- Uses new records and unique mesh/texture paths.
- Does not replace NavigateVR's Papyrus scripts.
- Does not modify Dawnguard NPC or quest records.
- Adds two placed map references to Dawnguard cells.
- Adds acquisition dialogue through a new Start-Game-Enabled quest.

Another mod can still overwrite the same loose asset paths. In particular,
disable development/donor versions such as `NavigateVR - Soul Cairn by Utru`
when verifying the textures shipped in this archive.

## Troubleshooting

Check:

```text
Documents/My Games/Skyrim VR/SKSE/NavigateVRMapFramework.log
```

Confirm that:

- `dawnguard.json` was loaded
- All three definitions resolved
- The ownership item is present
- The expected Dawnguard worldspace was detected
- No other mod wins the addon JSON or texture paths

## Credits

- NavigateVR and its original map assets/workflow
- LXE97 for Map Markers for NavigateVR and the map calibrator
- Bethesda for Skyrim and Dawnguard
