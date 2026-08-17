# Compatibility examples

`bruma-wyrmstooth.compatibility.json` demonstrates how the framework can make
NavigateVR's existing Bruma and Wyrmstooth map armors participate in modular
worldspace selection. Copy only the definitions you need into an addon's own
JSON file; do not install this example blindly alongside another definition
with the same ID.

The Bruma entry intentionally targets:

```text
BSHeartland.esm:0x0A764B
```

That WRLD has the display name `Tamriel`, but it is not Skyrim's
`Skyrim.esm:0x00003C` Tamriel worldspace. Do not add the Skyrim Tamriel record
as a Bruma alias: doing so would cause the Bruma map to be selected throughout
Skyrim.

Framework 0.3.1 or newer is required when a definition points to one of
NavigateVR's own built-in map armors. Older versions could react to
NavigateVR equipping the same armor and repeatedly submit another equip.
