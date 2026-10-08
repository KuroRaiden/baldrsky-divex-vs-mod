# V12: simultaneous shared mecha selection

V12 uses Training's existing **mecha-select / F2 screen** for both controllers. Both players choose from the same 70 visible entries in the same 10-by-7 grid. V11b's rounds, countdown, HUDs, attacks and camera are retained.

## Try it

1. Close V11b. Launch `E:\GIGA\BALDRSKY_DIVEX\BaldrSky.VSPrototype.v12.exe` with both controllers connected.
2. Enter Survival → Training and open mecha select / F2.
3. P1 navigates and confirms using the existing menu controls. A cyan `1P` marker becomes `1P READY` after confirmation.
4. P2 navigates the same grid with controller 2. The mapped A attack button confirms; B releases P2's ready state. A pink `2P` marker appears below P1's marker when both choose the same cell.
5. Either player can confirm first. Their choice stays fixed until the other finishes. Both ready triggers the native selection transition and supplies both mecha IDs to Training. Start Training as usual.
6. Check both chosen mechas, independent attacks, round resets and leaving/re-entering Training.

P1's normal Back action cancels the whole selection screen before joint confirmation. No new choice is committed on cancellation. The native detail preview follows P1; a separate P2 preview is not included. F3's legacy enemy selector remains available but is unnecessary for shared selection.

## Verification

Existing scoring, KO, timeout, pause, countdown, attack gate, range cursor and P2 tracker mocks pass. New shared-selection mocks pass for both confirmation orders, no early commit, exactly one joint commit, ready-choice freeze, grid wrap, cancellation/re-entry, P2 unready, native delegation outside Training and label creation/disposal. Shuffled mock IDs verify that both choices use the mecha field from the same roster table rather than a pilot ID or enemy-roster index.

Static and read-only live V11b inspection established the 70 visible tiles. The backing table contains 99 rows; V12 uses the existing 70 displayed cells for both players. The 29 undisplayed rows are not added.

**Live V12 testing is pending.** Mocks do not establish actual controller navigation, label appearance, equipment application or menu transitions. Research did not modify the running process.

## Patch record

Build: `scripts/build_vs_v12.ps1`, using exact V6 SHA-256 `7035444F3ABD89077ABAF487F11282AF38FA114F967D98CF07CA32E45DC7D739`.
Tests: `scripts/test_vs_v12.ps1`.
V12 SHA-256: `25AB67575CE2726EDC151CC1FE1F03398A3959D51AF203AC925BC3AACDF53D79`.
Length: 4,667,904 bytes. Compared with V6: 118 changed existing bytes, plus 16,384 appended bytes. Builder verifies the source, original hooks, zero new imports and that every changed byte falls in an allowed region. It refuses to overwrite an existing output.

New hooks: main mecha-menu update/close vtable entries at VA 0x7E6D04/0x7E6D0C; original menu-dispatch call destinations at VA 0x79AF9D/0x7AA122. Other menus delegate to native dispatcher 0x7A3D80. Joint confirmation uses that dispatcher for P1 after setting Training's P2 choice at 0x8734E8. Native text actions supply player markers and are disposed on menu close.

No game assets or save files are patched. No proprietary binaries, assets, saves or analysis databases are published.

## Rollback

Close V12 and launch `E:\GIGA\BALDRSKY_DIVEX\BaldrSky.VSPrototype.v11b.exe`. V11b and the untouched original backup are preserved.
