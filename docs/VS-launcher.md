# Standalone VS launcher — experimental startup fix

The launcher shows shared 1P/2P mecha dropdowns, a stage dropdown, and Start Battle. Both player dropdowns contain the same 99 native Training entries. The stage list contains 220 native Training stages. Connect both controllers before starting.

## Build

Use Visual Studio 2022 Community C++ x86 tools and official Dear ImGui commit `2859f6723c13de3a9661b8087fb7919fa3796813` from https://github.com/ocornut/imgui. Pass that checkout's absolute directory to `scripts/build_launcher.cmd`. The build produces `launcher/BaldrVSLauncher.exe`, `BaldrVSSetup.dll`, and the two catalogs. The helper hash header is generated before compiling the launcher. Always distribute the EXE and DLL from the same build. ImGui's MIT license is included.

## Installation

Install beside `BaldrSky.VSPrototype.v11c.exe`, whose required SHA-256 is `42AF4F00C5F5DF743E75512A8E04289DBCECD6DBC5F430E1B97B745898969460`. Close the game and launcher first. Preserve existing launcher files in a fresh backup directory. The release package's `Install-Launcher.ps1` checks the base hash and package hashes, refuses running instances, creates a unique backup, and copies only the five launcher files. It does not patch the game executable.

Run `BaldrVSLauncher.exe`, select both mechas and a stage, then Start Battle. The launcher verifies the game and helper hashes, creates a new game process, loads the helper, and briefly pauses that child's main thread while installing two verified native CALL replacements. It resumes the thread after initialization. Injection/setup failure closes only that new child.

The helper redirects the input polling call at `0x7AA02B` to wait for native mecha and Training data, then initializes Survival and Training with the selected fighters and map. It also redirects the movie-start call at `0x78251E` to a no-op for this VS session. The game file on disk is unchanged. Native scene clearing/reset and reveal remove startup graphics. The reveal routine requires the scene manager in ECX despite IDA's inferred stdcall signature; the corrected declaration passes it explicitly.

## Verification and remaining checks

Protocol bounds, exported entry points, and rejection of an unrelated executable passed. Prior live tests confirmed GameMode 1, BattleMode 2, human control kind 0 for both players, and controller slots 0/1. Those tests did not establish a clean visual handoff: one showed a black scene; another retained the intro video. An intermediate reveal-call build crashed with access violation; startup diagnostics identified the missing ECX argument and the next build reached Training.

The latest movie-suppression build has passed build/safety checks but is **not yet verified live**. The preceding test child (PID 21764) became windowless and did not exit after termination was requested, leaving the helper DLL locked. Windows restart is required before reinstalling and retesting. The interrupted installation temporarily left the installed EXE and DLL from different builds; hash verification prevents their use. Reinstall the complete package after restart.

Retest Kou versus Masa on Boot Camp first: no intro, terrain and both actors visible, three-second countdown, independent movement, attacks after GO, both move trackers, KO/timeout reset, and new match. Then try another roster pair and stage. Ready status reflects native fighter creation; visual/control confirmation remains necessary. `--smoke` is a diagnostic launch of Kou/Masa/Boot Camp and logs to `launcher-smoke.txt`; it leaves the game running. Startup access violations write `vs-startup-crash.txt` without swallowing the exception.

## Rollback

Close both programs. Restore the five files from the desired launcher backup, or stop using the launcher and run the unchanged V11c executable directly. Existing original game backups and prior builds are preserved. Never restore quarantined experimental executables or disable Windows protection to run them.

Only source, catalogs, scripts, documentation, and the ImGui license belong in this repository. Game binaries, assets, saves, and analysis databases are excluded.
