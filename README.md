# Baldr Sky DiveX local VS mod

Private collaboration workspace for KuroRaiden and armonte. Goal: two human players using two controllers, based on **Survival → Training**.

## Current status

- Mode unlock: user confirmed all six modes launch.
- Character-selector unlock: static byte verification passed; gameplay validation pending.
- VS prototype: build and disassembly checks passed; v2 user testing confirms Player 2 can attack without crashes and select their Training roster mecha. It assigns the Training opponent to controller slot 1 using the existing player-assignment routine.
- Training camera, health, resets, and menus remain unchanged. Round scoring and a VS menu are future work.

## Revised prototype v2

The first VS prototype crashed during Player 2 attacks. V2 guards the two observed AI-state dereferences and unlocks the existing Training enemy roster. See [v2 findings and testing](docs/VS-v2-crash-and-roster.md). On October 8, 2026, the user confirmed Player 2 attacks no longer crash and the Player 2 roster can be selected. The user also confirmed Training reset and leaving/re-entering Training work. Every roster entry and broad attack coverage remain unverified. Use the v2 builder on the exact v1 executable.

## Start here

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
