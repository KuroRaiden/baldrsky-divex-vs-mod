# Training stage unlock

Installed separately at `E:\GIGA\BALDRSKY_DIVEX\BaldrSky.VSPrototype.v11b.stages.exe`.
Close the existing game, launch this executable, enter Survival → Training, and open Map Change (F4). Previously locked valid stage entries should now show their previews/names and allow selection. Check a previously locked stage, reset, and re-enter Training. Live selection/loading remains pending user verification.

The original V11b executable and backups are preserved. To undo this build, launch `BaldrSky.VSPrototype.v11b.exe` instead. No save files were edited.

## Exact change

- Source SHA-256: `08EDA99FF0CEB12118EF3760A5E7B3B79B98290FCFC07A41BD36C81D333B9C0A`
- Output SHA-256: `22B241A8FAAF5FAD29C7FE86EE6CAF4F5D95AAC9951798CAC25C41A990C0C933`
- File length: 4,667,904 bytes before and after.
- Exactly five differing bytes, file offset `0xDFC67`, VA `0x4E0867`.
- Before: `E8 14 04 F7 FF` (call native stage clear-record predicate `0x450C80`).
- After: `B0 01 83 C4 04` (return AL=1 and consume the original thiscall stack argument).

The predicate's native `retn 4` was inspected to verify argument cleanup. Only its call inside `CSelectMapItem` is changed; the global clear-record query, all other callers, save ownership and existing valid-map bounds remain intact. The normal unlocked branch populates the name/preview and clears the item's disabled flag.

Read-only inspection of the running V11b found 372 total map records but only 220 native Training entries: IDs 0–220 excluding 9. Keep this native list for the DLL launcher's Stage Select dropdown; do not expose every map record as a Training stage.

Build with `scripts/build_training_stages.ps1`. It requires the exact V11b hash, validates original bytes, maps the VA through PE sections, refuses to overwrite outputs, and checks the complete file diff and unchanged source hash.

## Shared launcher status

The separate ImGui window loaded from a DLL is still being implemented. This stage-unlock executable is an interim V11b build; it does not implement the launcher or automatic VS startup. V12's attempted shared native selector is superseded after crashes and blocked P1 confirmation were reported.
