# V10 independent P2 melee-range marker

The user requested the close/out-of-melee-range cue around P1, responding to P2's range. V8/V9 rendered a second name and numeric distance using a temporary cursor copy, but native range markers are separate child animation objects. Copying the text renderer cannot draw those children.

V10 creates a second native CLockCursor targeting P1. Its constructor and update run with P2 as the temporary observer, then restore the global P1 pointer. The game's existing sub_618170 range predicate drives the native close/far animations; no guessed distance threshold or custom circle test is used. The original cursor still responds to P1 and targets P2.

The secondary object owns its name, distance text, and native marker animations, replacing the prior text-only second pass. It registers with the existing GUI pool and uses the game's reference holder. Cleanup runs before explicit and automatic round resets, fighter replacement/removal, and leaving the eligible Training context. Native disposal clears registered reference holders. The old cursor's update and render behavior remains the fallback for other objects.

## Verification

- Source: src/vs_rounds_v10.c; builder: scripts/build_vs_v10.ps1.
- Exact V6 source SHA-256: 7035444F3ABD89077ABAF487F11282AF38FA114F967D98CF07CA32E45DC7D739.
- V10 SHA-256: 4E2BE02FDF92C6B49C7B2C762C88533C3D3A9669C7D25DD6167729F0AB7224FF.
- Length: 4,663,808 bytes; 12,288 appended and 35 changed existing bytes relative to V6, confined to checked metadata and five vtable hooks.
- Existing V9 round, removed-fighter, timeout, reset, and one-byte pause regression tests pass.
- Additional translated-address mock tests pass for native creation under P2, one cursor allocation across updates, distinct text passes, opposite 1P/2P range states, observer restoration, reset/KO disposal and recreation, and leaving Training.
- Disassembly checks confirm constructor 0x4CC310, GUI registration 0x4AC3A0, reference assignment 0x565670, native update 0x4CBD50, and x86 calling conventions. Native disposal 0x78C460 was inspected for its already-disposed guard and reference-holder invalidation.

Mocks validate helper behavior, not actual engine allocation/destruction or visual animation. Live verification is pending. No original backup, V9 executable, assets, or saves are modified.

## Live test

Close V9 and launch E:\GIGA\BALDRSKY_DIVEX\BaldrSky.VSPrototype.v10.exe. Enter Survival Training with two controllers. Move P1 into and out of P2's melee reach; the marker around P1 should switch through the game's native close/far states. Test different mechas/weapons with unequal reach, both names/distances, attacks, KO reset, F1, roster replacement, and leaving/re-entering. Roll back by closing V10 and launching the preserved V9.
