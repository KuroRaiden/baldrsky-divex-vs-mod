# Baldr Sky DiveX local VS mod

Private collaboration workspace for KuroRaiden and armonte. Goal: two human players using two controllers, based on **Survival → Training**.

## Online VS development

The separate `online-vs` branch is building a two-player online launcher with AutoPunch, GekkoNet and shareable link codes and ImGui player chat. Its current build tests connections and rollback with a standalone simulation; **native online combat is not ready**. The game's built-in replay is the determinism baseline, with two identical 600-frame native audit runs so far. See [online implementation, replay findings and testing](docs/VS-online.md). The public latest release remains the local VS build.

## Compatibility

This mod and launcher target the original **BALDR SKY DiveX version 1.00** executable. Compatibility with later game patches is **not guaranteed**. The build and installer require the exact supported executable hashes; other versions may be rejected or behave incorrectly.

## Current status

The **Native VS Launcher** is the current launcher. It uses normal DLL loading, rotating position-based split cameras, FT2/FT3/FT5/Unlimited, music selection, a 3× visual countdown, bottom-center gold seconds timer and independent equipment editors for both players. Finishing a finite set closes the game while leaving the launcher open. Equipment loadouts and named presets are saved per player and mecha; 2P's leftover CPU dash/Force Crash bonuses are removed. See [native launcher setup, equipment and verification](docs/VS-native-launcher.md). Earlier injection-launcher notes below are historical.

V11c is the working Training VS base. The user confirmed stage selection and both weapon trackers work. It supports two controllers, independent mechas, both HUDs and range indicators, first-to-two rounds, and a three-second countdown with movement enabled.

A standalone Dear ImGui launcher now provides the same 99-entry Training roster for both players and 220 Training stages. It starts the exact V11c executable and injects a small setup helper into only that new process. The UI is part of the EXE. The game file stays unchanged.

The launcher remains experimental. Earlier live tests reached Training with both controller slots assigned but the intro video blocked the display. The latest fix suppresses the native movie-start call and resets/reveals the Survival scene; it builds and passes protocol/export/host-rejection checks. The matching pair has now been installed. The user confirmed that the test showed the battle without the intro and that they closed it themselves; the user also confirmed that Start Battle in the visible launcher goes straight into VS. Launcher navigation uses mouse and keyboard only; in-game two-controller support is unchanged. See [launcher build, installation, testing and rollback](docs/VS-launcher.md).

The failed V12 shared native selector is superseded. Use V11c and the separate launcher workflow.

## Revised prototype v2

The first VS prototype crashed during Player 2 attacks. V2 guards the two observed AI-state dereferences and unlocks the existing Training enemy roster. See [v2 findings and testing](docs/VS-v2-crash-and-roster.md). On October 8, 2026, the user confirmed Player 2 attacks no longer crash and the Player 2 roster can be selected. The user also confirmed Training reset and leaving/re-entering Training work. Every roster entry and broad attack coverage remain unverified. Use the v2 builder on the exact v1 executable.

## Start here

V11c is the current base: see [V11c tracker restoration](docs/VS-v11c-primary-tracker.md). V12's native selector failed live testing and should not be installed. Earlier notes below are historical.

V11b adds a separate native P2 weapon/combo tracker on the right, with P2's weapon data, highlights and native hit/damage counters. Tracker routing/lifecycle and previous regression mocks pass; live V11b testing is pending. See [V11b testing](docs/VS-v11b-P2-tracker.md). Build from exact V6 with scripts/build_vs_v11b.ps1.

V11 adds a three-second countdown in the window title before each round, with movement enabled and weapons/Force Crash blocked until GO. The 90-second timer begins at GO. Scoring, countdown, attack-gate and cursor mocks pass; live V11 testing is pending. See [V11 testing](docs/VS-v11-countdown.md). Build from exact V6 with scripts/build_vs_v11.ps1.

V10 adds a second native range-marker cursor around P1, driven by P2's melee-range state. The user confirmed both native range indicators visually. See [V10 testing](docs/VS-v10-independent-range-marker.md). Build from exact V6 with scripts/build_vs_v10.ps1.

V9 corrects an oversized pause-flag read that could block KO and timeout scoring in V8. The exact neighboring-flag regression and existing mock tests pass; live validation is pending. See [V9 fix](docs/VS-v9-pause-flag.md). Build with scripts/build_vs_v9.ps1 from exact V6.

V8 corrects V7's missed KO after fighter removal and adds the missing second overhead name. Regression tests pass; live verification is pending. See [V8 fixes and testing](docs/VS-v8-KO-and-names.md). Use scripts/build_vs_v8.ps1 against exact V6.

V7 adds first-to-two rounds, equal 10,000 health, and a 90-second timer, with score and results in the window title. Scoring and mock integration tests pass; live gameplay is pending. See [V7 rules and test checklist](docs/VS-v7-rounds.md). Build from exact V6 with scripts/build_vs_v7.ps1. V6 retains the conservative camera zoom limit.

V3's magenta Player 2 ring is visible in user screenshots. V4 restores its circular labels. Experimental v5 addresses overlapping overhead names reported in v3/v4 and adds midpoint camera tracking with wider dynamic zoom for both players. V5 runtime testing failed with black gaps and repeated scene elements. V6 clamps zoom to the original minimum while retaining midpoint tracking; runtime testing is pending and full framing at large separation remains unresolved. See [V6 correction](docs/VS-v6-camera-containment.md). See [v5 details and testing](docs/VS-v5-camera-and-name.md) and [v4 ring labels](docs/VS-v4-ring-labels.md). Earlier builds remain available for rollback.

Read [patch history](docs/BaldrSky-patch-report.md) and [prototype research and testing](docs/BaldrSky-VS-prototype.md).

Use your own local installation. No game executables, assets, saves, or analysis databases are stored here.

Build from the exact previously patched executable (SHA-256 `0A34F07F2698D8105E593AF4C2A09698807604DFFE2DD18E0B193FDBD6A00F02`):

```powershell
powershell -File .\scripts\build_vs_prototype.ps1 -SourcePath 'E:\GIGA\BALDRSKY_DIVEX\BaldrSky.exe'
```

The builder checks the source hash and expected bytes and writes a separate prototype beside the script. It preserves the source. Copy the generated `BaldrSky.VSPrototype.exe` beside your installed game before launching it. Both controllers must be connected before startup.

`scripts/inspect_training.ps1` performs read-only inspection of the running game. Run exactly one Baldr Sky executable and enter Survival → Training first. This inspection is specific to the analyzed binary layout.

## Collaboration

armonte: please review the spawn wrapper and controller assignment, then test independent movement/attacks, damage, reset, opponent changes, and leaving/re-entering Training. Record the source/prototype hashes, controller models, and results in issues. Initial v2 attack and roster-selection tests passed according to the user; broader testing remains open.

Use branches and pull requests for changes. Include expected original bytes, resulting hashes, and rollback instructions for any patch. Never commit game binaries or saves.







An experimental distance-triggered vertical split-screen launcher is available separately. V2 adds a divider and per-view background camera/parallax handling, confirmed by the user. Windows subsequently blocked its launcher executable during packaging; binary distribution is blocked. See [split-screen build and verification](docs/VS-split-screen.md).



The new [native-loading launcher](docs/VS-native-launcher.md) replaces injection with normal DLL imports. Its startup, controllers and split-screen behavior were confirmed by the user. The user also confirmed that the native build music fix works.


