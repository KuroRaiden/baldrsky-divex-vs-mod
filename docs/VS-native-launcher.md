# Launcher with normal DLL loading

Use `BaldrVSLauncher.Native.exe` in the game folder. It keeps the same mouse/keyboard dropdowns and starts `BaldrSky.VSNative.exe`, a separate build derived from the exact V11c executable. Windows loads `BaldrVSPlugin.dll` through a normal import. The new game entry point calls `StartVSPlugin` after DLL initialization, preserves startup registers and flags, and returns to the original entry point only after successful setup.

The launcher saves a validated 32-byte selection record to `BaldrVS-start.bin`, starts the child normally and reads `BaldrVS-status.bin` for readiness. It no longer uses remote threads, cross-process memory reads/writes, remote allocation or thread suspension. It verifies the exact game/plugin hashes and rejects an existing game instance. The plugin installs the VS startup/render hooks in its own process before the game loop. DllMain does not start threads or patch code.

This replaces the split v2 launcher quarantined as `Behavior:Win32/DefenseEvasion.A!ml`. No quarantined program was restored; protection settings were not changed. The standard launcher and V11c executable are preserved.

The split-screen renderer retains the user-confirmed divider, independent backgrounds and merging. Direct Training startup omitted music initialization: the native plugin now starts the first valid track in the game's loaded Survival playlist via its native BGM player at `0x751CA0`, only if no BGM player exists. Normal game audio settings still apply. It adds no audio assets.

## Build and install

1. Run `scripts/build_native_game.ps1` with `-SourcePath` pointing to verified V11c and `-OutputPath` pointing to `launcher-native/BaldrSky.VSNative.exe`. The script requires V11c SHA256 `42AF4F00C5F5DF743E75512A8E04289DBCECD6DBC5F430E1B97B745898969460` and preserves existing outputs.
2. Run `scripts/build_launcher_native.cmd` with the absolute Dear ImGui checkout path. It generates separate game/plugin hash constants, builds the launcher and plugin, and runs the protocol/host rejection and split-screen rule tests.
3. Run `scripts/test_native_game.ps1` with source, output and launcher paths. It checks preservation of original game bytes outside the specified PE header changes, new section bounds, return to the original entry point, and absence of injection imports in the launcher.
4. Install the three generated files beside the existing catalogs and game data. The source-only repository excludes game binaries. The distribution includes `Install-Native-Launcher.ps1` and `build_native_game.ps1` to derive the separate game executable locally from the user's V11c copy. Close the game and native launcher before installing a matching pair.

The original game, V11c, standard launcher and game data remain usable. The new build writes two disposable settings/status files; normal game routines may also write game settings or saves. Keep one game instance at a time.

## Verification

The build, protocol bounds, DLL exports, unrelated-host rejection, split/merge rules, clipping and PE preservation checks pass. The first native startup reached Survival Training with both human-controlled fighters assigned to controller slots 0/1. Read-only diagnostics observed both split passes and merging without doubling the battle clock. The user confirmed the battle, both controllers and the split-screen visuals. They reported missing music; the native playlist music fix was then added. Audible confirmation of that final fix is pending.

## Rollback

Close the native game and launch the standard launcher or V11c. The separate native launcher, native game, plugin, `BaldrVS-start.bin` and `BaldrVS-status.bin` can be archived afterward. Preserve the standard launcher/helper, original game backups, catalogs and all game data.
