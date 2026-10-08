# VS prototype v3: Player 2 circular indicator

Based on tested v2. Visual/gameplay testing is pending.

The large blue circle in the user's screenshot is rendered by `CRangeRing`. Its renderer uses its owner fighter's position, selected target, team, and facing, while its target-name text reads the single global Player 1 cursor. This prototype retains the original Player 1 pass and adds a second pass for Player 2 during Survival Training.

## Implementation

- Replace the `CRangeRing` render vtable pointer (VA `0x7E0E70`) with a wrapper at VA `0xCBE030`.
- Draw the original ring first. If GameMode is 1, BattleMode is 2, and Player 2 exists with human control, temporarily borrow the ring for a second draw using P2's owner pointer, world position, and lock-state flag.
- The original render reads P2's own target at fighter offset `0x14B8` and uses it when choosing target/other-enemy sprite frames.
- Restore owner, position, state, and display flags after the second pass. No permanent fighter/global player pointer changes, extra allocated HUD object, or save changes.
- Hook the ring's sprite draw call at VA `0x4CDE28` through VA `0xCBE159`; tint the P2 circular sprite magenta (`0xFF80FF`). The original texture may affect the observed shade.
- Suppress target-name text and duplicate circular statistics during P2's pass because the existing text references P1's singleton cursor. This does not create a second full `CLockCursor` object or HUD.

339 helper bytes occupy existing v2 `.vsfix` padding at file offset `0x46F430`. Section virtual size increases to 387 bytes within its existing 512 raw bytes. Length remains 4,650,496 bytes; 266 bytes differ from v2, confined to helper padding, virtual size, ring vtable pointer, and sprite call.

Source SHA-256: `225ACD27E51D2F54E240148A158084D66D41EA0C6A82155CEB1D352020392A3F`.

Result SHA-256: `08C20A626A52E3ADF2A1DA28259F72CF3188247B72E8A683032B30EC997F062F`.

## Build and test

```powershell
powershell -File .\scripts\build_vs_v3.ps1 -SourcePath 'E:\GIGA\BALDRSKY_DIVEX\BaldrSky.VSPrototype.v2.exe'
```

Copy generated `BaldrSky.VSPrototype.v3.exe` beside the installed game. Connect both controllers before launch. Enter Survival → Training. Check P1's original ring remains, P2 has a distinct magenta ring, each follows its own fighter/target, and attacks, F3 mecha changes, F1 reset, and leaving/re-entering Training remain functional.

For rollback, close v3 and launch unchanged v2. Visual rendering and independent target indication are not yet user-confirmed.
