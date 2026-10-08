# Baldr Sky DiveX — Training VS prototype

This experimental executable uses **Survival → Training** as the base for two-controller play. Gameplay testing is pending.

## Launch and test

1. Close the current game.
2. Connect both controllers before starting the prototype. The original running game did not detect controller two after it was plugged in.
3. Run `E:\GIGA\BALDRSKY_DIVEX\BaldrSky.VSPrototype.exe`.
4. Open Survival → Training. Use Training's existing character and opponent selection.
5. Check controller one moves only your fighter; controller two moves only the opponent. Check both can attack and take damage.
6. Test Training reset, changing the opponent, leaving Training, and entering it again.

The camera, Training menu, health behavior, fighter selection, and reset rules remain those of Training. There is no new VS menu, round scoring, or online play in this prototype. Gameplay and controller isolation must be tested before treating it as a working VS mode.

## What the prototype changes

The existing Training opponent-spawn call is redirected to a small wrapper. The wrapper forwards the seven original spawn arguments and retains the returned fighter. In Survival's context (game mode `1`), it then calls the game's existing player-assignment routine with controller index `1` (the second controller). Other game-mode contexts skip reassignment.

Reverse-engineering findings:

- `0x743240`: input initialization creates controller slots `0` and `1`.
- `0x74FF40`: per-frame input updates maintain separate button states for both slots.
- `0x61E390`: fighter input reads either controller state based on its controller-slot field.
- `0x6205C0`: the human-input predicate checks that the fighter control-kind field is zero.
- `0x629330`: existing player-assignment routine switches a fighter to human control, assigns controller index, and registers the fighter in the corresponding player slot.
- `0x5FB8C0`: Training battle setup creates the main fighter and opponent.

Addresses in the findings above are virtual addresses, not file offsets.

## Exact file modifications

Base executable SHA-256: `0A34F07F2698D8105E593AF4C2A09698807604DFFE2DD18E0B193FDBD6A00F02`

| File offset | Modification |
| --- | --- |
| `0x1FAF99` | Replace spawn call bytes `E8 32 1A FE FF` with `E8 14 CA 1D 00` |
| `0x3D79B2` | Put a 62-byte wrapper in previously zero executable-section padding |
| `0x200` | Increase `.text` virtual size from 4,027,826 to 4,027,904 bytes, covering the existing raw padding |

Wrapper bytes:

```text
55 8B EC FF 75 20 FF 75 1C FF 75 18 FF 75 14 FF 75 10
FF 75 0C FF 75 08 E8 01 50 E0 FF 83 C4 1C 50 83 3D
1C AA 86 00 01 75 11 85 C0 74 0D 8B 88 58 01 00 00
6A 01 E8 43 0D E5 FF 58 5D C3
```

Static verification confirmed correct call targets, argument forwarding, conditional branches, and balanced stack handling. The byte diff is confined to those three regions: **65 changed bytes**, unchanged file length of **4,649,984 bytes**. All prior ISO, mode, and character-selector patches are preserved.

Prototype SHA-256: `969C4CA6DBD933A2FDD2C075C790B52202D0A357DAC1E5DC493A4D027EFFC502`

## Reversibility and current test evidence

`BaldrSky.exe` remains unchanged. To use the previous game behavior, close the prototype and launch `E:\GIGA\BALDRSKY_DIVEX\BaldrSky.exe`. All prior backups remain available.

A read-only inspection of the original running game confirmed Survival mode `1`, Training battle mode `2`, main fighter human-controlled on slot `0`, and the opponent AI-controlled with no controller slot. Only controller slot `0` was initialized in that running session, including after a second controller was connected.

The prototype has not yet passed an in-game two-controller test. Both executables use the game's normal save location; normal game activity may update those saves.
