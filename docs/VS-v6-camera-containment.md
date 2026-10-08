# V6 camera containment correction

V5 runtime testing failed: user screenshots show black gaps and repeated terrain and mechas at wide zoom. Static patch validation did not establish rendering correctness.

V6 rebuilds from the unchanged V4 executable. It retains V5 midpoint camera positioning and target/cursor guards, but clamps zoom to 192/256, the minimum in the original camera routine. V5 allowed zoom down to 16/256; read-only inspection observed 62/256 during the broken scene. The precise renderer constraint remains under investigation.

This is a containment correction, not a complete solution for keeping both players visible at arbitrary separation. No fighter positions, controls, saves, or Training rules are changed. Runtime rendering and name behavior remain unverified.

## Build verification

- Source SHA-256: ABE5F4FBB16697810754BBC2FCE431EE3295FF7B28A8440E9746D69D8982730A
- V6 SHA-256: 7035444F3ABD89077ABAF487F11282AF38FA114F967D98CF07CA32E45DC7D739
- Length: 4,651,520 bytes; 1,024 appended bytes and 16 changed pre-existing bytes relative to V4.
- Builder validates source hash, intercepted bytes, and permitted modification ranges.
- Verified unsigned zoom-floor comparison uses full-width immediate 192, avoiding signed imm8 interpretation.

## Test and rollback

Close V5. Launch E:\GIGA\BALDRSKY_DIVEX\BaldrSky.VSPrototype.v6.exe with both controllers connected. Enter Survival Training, move together and apart, attack, reset, change opponent, and leave/re-enter. Check for black gaps, repeated mechas, camera centering, and doubled names. Very distant players may leave the frame at the zoom limit.

All earlier executables remain untouched. Close V6 and launch V4 to roll back. Full wide framing needs further renderer investigation.
