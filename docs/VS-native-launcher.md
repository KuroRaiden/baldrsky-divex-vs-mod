# Launcher with normal DLL loading

Use `BaldrVSLauncher.Native.exe` in the game folder. It keeps the same mouse/keyboard dropdowns and starts `BaldrSky.VSNative.exe`, a separate build derived from the exact V11c executable. Windows loads `BaldrVSPlugin.dll` through a normal import. The new game entry point calls `StartVSPlugin` after DLL initialization, preserves startup registers and flags, and returns to the original entry point only after successful setup.

The launcher saves a validated selection record to `BaldrVS-start.bin`, starts the child normally and reads `BaldrVS-status.bin` for readiness. It no longer uses remote threads, cross-process memory reads/writes, remote allocation or thread suspension. It verifies the exact game/plugin hashes and rejects an existing game instance. The plugin installs the VS startup/render hooks in its own process before the game loop. DllMain does not start threads or patch code.

The setup window has FT2, FT3, FT5 and Unlimited match presets, a music selector with Auto and Random choices, and a Close Game button. Finite sets end the game process automatically after the set ends, bypassing the game's confirmation prompt and preventing background threads from lingering. The launcher stays open, displays "Set finished. Choose your next battle.", and enables Start Battle again. Close Game ends the battle started by the launcher and leaves the launcher open.

This replaces the split v2 launcher quarantined as `Behavior:Win32/DefenseEvasion.A!ml`. No quarantined program was restored; protection settings were not changed. The standard launcher and V11c executable are preserved.

The split-screen renderer retains the user-confirmed divider, independent backgrounds and merging. Direct Training startup omitted music initialization: the native plugin now starts the first valid track in the game's loaded Survival playlist via its native BGM player at `0x751CA0`, only if no BGM player exists. Normal game audio settings still apply. It adds no audio assets.

## Build and install

1. Run `scripts/build_native_game.ps1` with `-SourcePath` pointing to verified V11c and `-OutputPath` pointing to `launcher-native/BaldrSky.VSNative.exe`. The script requires V11c SHA256 `42AF4F00C5F5DF743E75512A8E04289DBCECD6DBC5F430E1B97B745898969460` and preserves existing outputs.
2. Run `scripts/build_launcher_native.cmd` with the absolute Dear ImGui checkout path. It generates separate game/plugin hash constants, builds the launcher and plugin, and runs the protocol/host rejection and split-screen rule tests.
3. Run `scripts/test_native_game.ps1` with source, output and launcher paths. It checks preservation of original game bytes outside the specified PE header changes, new section bounds, return to the original entry point, and absence of injection imports in the launcher.
4. Install the three generated files beside the existing catalogs and game data. The source-only repository excludes game binaries. The distribution includes `Install-Native-Launcher.ps1` and `build_native_game.ps1` to derive the separate game executable locally from the user's V11c copy. Close the game and native launcher before installing a matching pair.

The original game, V11c, standard launcher and game data remain usable. The new build writes two disposable settings/status files; normal game routines may also write game settings or saves. Keep one game instance at a time.

## Verification

The split camera assigns its left half to whichever fighter is farther left on the map, and its right half to the other fighter. At split entry it uses the current horizontal order immediately. During a split it retains that order within a 16-pixel horizontal buffer and swaps after a clear crossing. Each half keeps its assigned fighter's camera, terrain/parallax, ring and world effects. Player-specific health bars and move-list HUDs retain their established screen positions. Order/crossing, buffer, reset and extreme-coordinate tests pass; live crossing behavior awaits user confirmation.

2P's range ring flashes yellow for 24 battle ticks (0.4 seconds) when its fighter loses HP, alternating yellow and its normal magenta every six ticks. Further damage restarts the flash. Healing, pointer replacement and clock rewind clear the effect. The color override is scoped to the existing ring sprite call and P2 ownership. Damage-timing tests and a real FT2 startup/render/shutdown test pass; the yellow appearance still needs visual confirmation in combat.

The build, protocol bounds, DLL exports, match-set boundaries, music bounds, unrelated-host rejection, split/merge rules, clipping and PE preservation checks pass. The first native startup reached Survival Training with both human-controlled fighters assigned to controller slots 0/1. Read-only diagnostics observed both split passes and merging without doubling the battle clock. The user confirmed the battle, both controllers and the split-screen visuals. They reported missing music; the native playlist music fix was then added. The user subsequently confirmed that music works in the native build.

## Rollback

The October 9 update adds native hit-counter digits for the three-second countdown, selectable music (Auto, Random and 92 tracks), finite set presets and Unlimited. Movement remains available during countdown while attacks are blocked. Automated set/music boundary tests pass. A dedicated integration-test plugin generated two KOs through the actual countdown/round sequence and verified FT2 completion status and process exit code 0. The production plugin does not generate test KOs. Visual countdown styling and the new menu choices still await user confirmation.

Close the native game and launch the standard launcher or V11c. The separate native launcher, native game, plugin, `BaldrVS-start.bin` and `BaldrVS-status.bin` can be archived afterward. Preserve the standard launcher/helper, original game backups, catalogs and all game data.

