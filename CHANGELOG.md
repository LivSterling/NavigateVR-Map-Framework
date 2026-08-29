# Changelog

## 0.3.2

- Moved NavigateVR controller identification into
  `SKSE/Plugins/NavigateVRMapFramework.json`.
- Added EditorID-based lookup for `TRC_WorldMap`, with an owning-plugin check,
  so FormID-compacted NavigateVR installations can be detected safely.
- Retained a configurable plugin-local FormID fallback for customized or
  older installations.
- Made the NavigateVR plugin filename configurable for renamed plugins.
- Removed the compiled controller plugin and FormID fallback; missing or
  invalid framework settings now disable selection with a clear log message.
- Added a machine-readable schema and explicit logging for controller
  resolution.

## 0.3.1

- Prevented recursive equip-event handling while the framework adds, equips,
  unequips, or removes a registered map armor.
- Added a no-op path when NavigateVR has already equipped the exact armor
  selected by the registry. This fixes compatibility definitions that point to
  NavigateVR's built-in Bruma or Wyrmstooth armors.
- Updated the schema, validator, examples, and author guide for the calibrated
  `markers.calibration.left/right` layout used by Map Markers for NavigateVR.
- Standardized selector and marker integrations on one canonical definition
  under `SKSE/Plugins/NavigateVRMaps`.

## 0.3.0

- Added modular discovery of all JSON files under
  `SKSE/Plugins/NavigateVRMaps`.
- Added exact-worldspace selection for registered left/right NavigateVR map
  armors.
- Added optional per-map miscellaneous-item ownership enforcement.
- Added priority handling for multiple definitions targeting one worldspace.
- Added last-exterior-worldspace fallback for interiors.
- Preserved NavigateVR's original Papyrus, drawing, and stowing behavior.
- Disabled unstable live switching across cell/worldspace transitions; maps
  are selected on draw.
- Added structured logging for configuration resolution and runtime selection.
- Added the complete addon-author guide and JSON schema.
