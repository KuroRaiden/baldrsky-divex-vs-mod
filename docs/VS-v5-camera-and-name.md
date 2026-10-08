# VS v5: shared camera and duplicate target-label fix

The user reported doubled overhead P2 name text in both v3 and v4, and a camera that followed only P2 in v4. V5 builds on v4 and addresses the singleton target-cursor and camera paths. Runtime verification is pending; the observed overlap may have additional causes.

Installed: `E:\GIGA\BALDRSKY_DIVEX\BaldrSky.VSPrototype.v5.exe`.

## Changes

- Hook the per-frame camera-position call at VA `0x5E1413`. During Survival Training with two present fighters and a human P2, use the midpoint of their body centers and adjust zoom from their horizontal and vertical separation. Other contexts call the original position routine.
- Fit center separation plus 200 world units of margin into a nominal 640x400 area of the 800x600 scene. Zoom ranges from 256 (normal scale) down to 16 (1/16 scale), allowing substantially wider framing than the original minimum of 192. At extreme separation this lower limit may still constrain framing. Ring circles and large attack effects are not guaranteed to fit, and Training overlays can cover part of the playfield.
- The camera writes the existing renderer's translation and zoom counters; it does not change fighter coordinates, controller ownership, the camera's owned fighter reference, save data, or combat logic. Framing is recalculated each frame without smoothing; camera shake is bypassed in the local VS path.
- At VA `0x619C88`, P2 target changes skip the singleton P1 target-HUD/cursor recreation block while preserving the fighter's actual target assignment. V4's ring name still reads P2's target independently.
- Wrap the `CLockCursor` render vtable entry at VA `0x7E0C70`. In Survival Training, only the manager's current cursor renders its overhead label; retired fading cursor labels are suppressed. Existing cursor ownership, update, and disposal are unchanged. This targets a traced source of overlapping names, with visual confirmation still required.

## Static verification

- Exact v4 source SHA-256: `ABE5F4FBB16697810754BBC2FCE431EE3295FF7B28A8440E9746D69D8982730A`.
- V5 SHA-256: `030555A23237691ADC8D8C8AED20D7A38735A39438C2052F40C5AA49F7659150`.
- Length: 4,651,520 bytes (1,024 bytes appended). Exactly 16 bytes in the existing v4 file differ, restricted to two hooks, the cursor render pointer, section virtual/raw sizes, and PE code-size accounting.
- 588 helper bytes in the extension of the existing `.vsfix` section; previous v2/v4 helpers are preserved. The section remains within its existing image allocation.
- IDA decoded the emitted instructions and checked branch/call destinations, virtual coordinate calls, stack cleanup, and the original fallback path.
- Installed v4 and v3 hashes were checked and remain unchanged.

Helpers: camera `0xCBE200`, target HUD guard `0xCBE3D8`, cursor render wrapper `0xCBE40E`.

## Build and test

Run `scripts/build_vs_v5.ps1 -SourcePath 'E:\GIGA\BALDRSKY_DIVEX\BaldrSky.VSPrototype.v4.exe'`. The builder checks the exact source hash and expected patch bytes, verifies the permitted differences, and refuses to overwrite its output.

Close the current game, launch v5, and enter Survival → Training with both controllers. Check the overhead P2 name at rest and while changing targets. Move both players in opposite directions horizontally and vertically: the camera should stay between them and zoom out as they separate, then zoom in as they approach. Test attacks, roster changes, F1 reset, and leaving/re-entering Training. Report clipping, abrupt zoom, missing names, or crashes.

Rollback: close v5 and launch a preserved earlier executable. No existing executable or backup is overwritten. No game binaries are uploaded to the repository.
