# Baldr Sky DiveX local VS mod

Private collaboration workspace for KuroRaiden and armonte. Goal: two human players using two controllers, based on **Survival → Training**.

## Current status

- Mode unlock: user confirmed all six modes launch.
- Character-selector unlock: static byte verification passed; gameplay validation pending.
- VS prototype: build and disassembly checks passed; v2 user testing confirms Player 2 can attack without crashes and select their Training roster mecha. It assigns the Training opponent to controller slot 1 using the existing player-assignment routine.
- V5 adds shared camera framing. Training health, resets, and menus remain unchanged. Experimental V7 adds best-of-three rounds; a dedicated VS menu remains future work.

## Revised prototype v2

The first VS prototype crashed during Player 2 attacks. V2 guards the two observed AI-state dereferences and unlocks the existing Training enemy roster. See [v2 findings and testing](docs/VS-v2-crash-and-roster.md). On October 8, 2026, the user confirmed Player 2 attacks no longer crash and the Player 2 roster can be selected. The user also confirmed Training reset and leaving/re-entering Training work. Every roster entry and broad attack coverage remain unverified. Use the v2 builder on the exact v1 executable.

## Start here

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





