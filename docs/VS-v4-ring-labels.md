# VS v4: Player 2 ring labels

V3 displayed the magenta P2 ring, confirmed by the user's screenshots, but suppressed the ring's name, enemy count, time, and weapon labels. V4 restores those labels through the existing renderer and supplies the name from P2's selected target. This follows the game's existing target-name behavior; it is not necessarily P2's own mecha name.

Installed executable: `E:\GIGA\BALDRSKY_DIVEX\BaldrSky.VSPrototype.v4.exe`.

## Verification

- Source: v2 SHA-256 `225ACD27E51D2F54E240148A158084D66D41EA0C6A82155CEB1D352020392A3F`.
- V4 SHA-256: `ABE5F4FBB16697810754BBC2FCE431EE3295FF7B28A8440E9746D69D8982730A`.
- File length remains 4,650,496 bytes; exactly 273 bytes differ from v2, confined to the ring vtable pointer, sprite-call hook, section virtual size, and helper padding.
- IDA disassembly checked all 74 render-wrapper instructions and 11 color-helper instructions, branch destinations, call targets, stack cleanup, and restoration of borrowed state.
- Runtime validation is pending. Static checks do not confirm visual correctness or crash-free gameplay.

## Implementation and limits

The wrapper renders P1 normally, then renders the ring with P2's owner, center, and lock state. It preserves display flags, allowing enemy count, shared Training time, and the existing A/B/C weapon labels to appear. Weapon labels still use the game's shared human loadout; this does not implement independent P2 loadout HUD data or a second health/status panel.

For name text, the renderer otherwise reads the singleton P1 lock cursor. The wrapper temporarily supplies a stack-backed name-only cursor view, using the game's character-name copy routine at `0x4AC310`. It restores the original cursor pointer before destroying the temporary string with `0x41A300`. This is a synchronous render-time substitution, not a complete second lock-cursor object.

Render wrapper: `0xCBE030`; color helper: `0xCBE16C`. Helpers occupy 358 bytes within the existing v2 section padding. P2's ring remains magenta. All original ring fields are restored after the extra render pass.

## Build, test, and rollback

Run `scripts/build_vs_v4.ps1` with `-SourcePath` pointing to the exact v2 executable. The builder checks its hash and expected bytes and refuses to overwrite an existing output.

Close the current game, launch the installed v4 executable, and enter Survival → Training with both controllers connected. Check P2's ring for enemy count, time, target-name text, and A/B/C labels. Change the opponent mecha and check that the name updates correctly. Test attacks, F1 reset, and leaving/re-entering Training. Report any missing or incorrect text or crashes.

Rollback by closing v4 and launching `BaldrSky.VSPrototype.v3.exe` or the tested `BaldrSky.VSPrototype.v2.exe`. The prior builds, regular executable, original backup, and saves are preserved.
