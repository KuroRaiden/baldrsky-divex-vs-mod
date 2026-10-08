# V11: movement during a three-second round countdown

Each round begins with 3, 2, 1 and GO in the window title. Both players may move and dash during preparation; normal weapons and Force Crash selection return no attack until GO. The 90-second round timer starts after the 180-tick preparation period. GO remains in the title for one second. First entry, automatic rounds, F1 setup and detected roster/clock resets use the countdown.

This build retains V10's independent native range markers, roster selection, dual-controller combat and conservative camera. The countdown currently appears in the window title, not in the arena.

## Implementation and exact checks

Built from the exact preserved V6 image, SHA-256 `7035444F3ABD89077ABAF487F11282AF38FA114F967D98CF07CA32E45DC7D739`. The builder checks that hash and all expected hook targets, then creates a separate executable. It never overwrites its source or an existing output.

Five existing round/cursor vtable hooks remain. Sixteen native tail jumps to the human weapon/Force Crash selector at VA `0x61A280` are redirected to a scoped wrapper. Their opcode and original relative destination are verified before modification:

```text
62A45A 62A46E 62A49A 62B298 62B2AF 62B2DB 62B302 62C38F
62C3C4 62C3EE 62DBB6 62DBC9 62DBF5 62E618 62E629 62E652
```

Outside the countdown or for another fighter/context, the wrapper delegates to the unchanged original selector. Native movement/dash logic runs before these selector tails. During preparation, only the A/B/C attack bits (`0x700`) are removed from the two fighters' current/buffered inputs and native 64-frame attack history. Direction bits and the `0x800` movement modifier remain. This prevents countdown button combinations from being retained for Force Crash. Holding an attack at GO may activate it once combat becomes legal.

The helper adds no imports. The appended `.vsround` section is 12,288 bytes; 98 existing bytes differ from V6, confined to checked PE metadata, five vtable pointers and sixteen relative destinations. Output length: 4,663,808 bytes.

V11 SHA-256: `459CE643AE8EF3D2BFD505FA5E02D3C703C9BA96F2541820D752708B065FFDB7`.

Installed separately at `E:\GIGA\BALDRSKY_DIVEX\BaldrSky.VSPrototype.v11.exe`. V10 and the original executable are preserved.

## Validation

`scripts/test_vs_v11.ps1` passes scoring and owned-code integration mocks: 3/2/1 and the exact 180-tick unlock boundary, both fighters blocked before GO and delegated after GO, unrelated fighter delegation, attack-history cleanup with movement bits preserved, no scoring during preparation, timeout exactly at countdown plus 90 seconds, fresh countdown after KO/F1 setup, pause-byte regression, removed-fighter KO detection, roster/clock resets, first-to-two scoring and V10 cursor lifecycle/range regressions.

Mocks verify helper logic and unchanged movement fields, not actual controller movement or native rendering. Live V11 gameplay remains pending. V10 was still running when V11 was copied; it was not stopped or modified.

## Live test and rollback

1. Close V10 and launch the separately named V11 executable with both controllers connected.
2. Enter Survival → Training. Watch the title count 3, 2, 1, GO.
3. Move/dash both players during preparation. Try A/B/C and Force Crash combinations; neither fighter should attack before GO. Repeat with attacks held at GO and with buttons released before GO.
4. Confirm both players can attack after GO, then check that a KO starts another countdown and preserves score. Check timeout, F1 restart, roster changes, pause/resume and exit/re-entry.
5. Check that pausing freezes preparation and that normal attacks work outside local VS Training.

Rollback: close V11 and launch V10. No save edit, original replacement or live-process patch is needed.

Build: run `scripts/build_vs_v11.ps1 -SourcePath <exact-V6-file>`; the output is created beside the builder. Copy only your locally built executable beside your own installed game. Do not commit game binaries, assets, saves or analysis databases.
