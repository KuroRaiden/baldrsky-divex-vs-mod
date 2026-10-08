# V11b: separate right-side tracker for Player 2

Adds a native weapon/combo tracker for P2 at x=570 in the game's 800x600 HUD coordinates. It uses the same native rows and fonts as the left panel: weapon names, A/B/C selection highlights, hit counts, damage and total. The native row constructor receives P2's own controller/combat context at fighter+340. The rows therefore read P2's weapon data rather than copying P1's rendered values. Combo display modes 1 (weapon list) and 2 (sequence/history) receive their own native manager; disabling combo information removes the second panel.

V11's three-second movable preparation period, attack gate, 90-second timer and round logic are retained. V10's range markers are retained. The already generated V11 is preserved; this revision is separately named V11b.

## Hooks and ownership

The native fighter update pointer at VA `0x7F02E4`, expected `0x61E390`, is redirected to a wrapper. Only the active human P2 in VS Training receives the second combo manager during that update. Movement and combat still delegate to the original function. This scopes weapon selection, committed-attack highlights and native history updates to the correct manager. The original global manager is restored immediately afterward; the global P1 fighter is not changed during combat updates.

The native tracker-row render pointer at VA `0x7EDF20`, expected `0x5E7430`, is redirected to a wrapper. Only rows owned by the second manager temporarily use P2's player/manager context for rendering. Other rows delegate normally. Both globals are restored after rendering.

The second manager uses native constructor `0x5E5540` and row builder `0x5E5E00`. It is rebuilt for a new P2, a round setup/reset or a combo-view change, and removed on leaving local VS Training. Cleanup disposes each GUI row through its native virtual method before releasing the vector through `0x481D20`. It avoids the ordinary manager reset, which also clears shared attack records.

## Reproducible build

Source: exact V6 SHA-256 `7035444F3ABD89077ABAF487F11282AF38FA114F967D98CF07CA32E45DC7D739`.

Output: `BaldrSky.VSPrototype.v11b.exe`, 4,667,904 bytes.

SHA-256: `08EDA99FF0CEB12118EF3760A5E7B3B79B98290FCFC07A41BD36C81D333B9C0A`.

The builder checks the source hash, seven original vtable targets and sixteen native attack-selector tail destinations. Changes from V6 are confined to checked PE metadata, those hooks and a 16,384-byte appended helper section. 105 existing bytes differ. There are no new imports or companion DLL requirements. Existing executables are not overwritten.

```powershell
.\scripts\build_vs_v11b.ps1 -SourcePath '<exact V6 executable>'
.\scripts\test_vs_v11b.ps1
```

Source, builder, tests and this document may be shared in the private repository. Game executables, assets, saves and analysis databases must remain local.

## Validation and live test

Scoring and owned-code integration tests pass. Additional mocks verify 15 list rows bound to P2's context, x=570, P2 update routing with P1 unchanged, normal P1 delegation, secondary render routing and global restoration, no duplicate creation across ticks, cleanup/rebuild after KO, list/history view changes and removal on exit. Previous countdown, pause-byte, missing-fighter KO, scoring and range-cursor regressions also pass.

These tests do not exercise the real game's rendering, live hit counters or controller inputs. Live V11b validation is pending.

Close the current game, launch `E:\GIGA\BALDRSKY_DIVEX\BaldrSky.VSPrototype.v11b.exe`, and enter Survival → Training with both controllers connected. Confirm the right panel shows P2's selected mecha's weapons. After GO, attack separately with each player and check that selection highlights and hit/damage values update on the correct side. Test both combo display views, a KO reset, F1 restart, P2 roster changes and leaving/re-entering Training. Check that the right panel fits and does not cover the bottom Training controls.

Rollback: close V11b and launch the preserved V11 or V10 executable. The original executable and saves are unchanged.
