# V7 best-of-three rounds prototype

October 8, 2026. Experimental implementation; live game validation pending.

## Rules and behavior

- First to two round wins; 90 seconds per round (5,400 native 60 Hz battle ticks).
- Both fighters start with 10,000 maximum and current health.
- KO awards the opponent a round. Double KO is a draw.
- At timeout, remaining health percentages determine the winner. Equal percentages draw.
- Draws replay without awarding either player a win.
- Automatic round reset uses the game's original Training setup, preserving existing roster selections and controller-assignment patches.
- The window title shows score, remaining time, and the previous round result. This is the initial match display; no new in-game HUD art is included.
- At two wins, the winner appears in the title and the native Training menu opens. F1 reset is intended to start a new match. A roster replacement or detected battle-clock rewind also starts a fresh match.
- There is no inter-round pause/countdown yet; rounds reset immediately.

## Integration

Exact V6 source hash: 7035444F3ABD89077ABAF487F11282AF38FA114F967D98CF07CA32E45DC7D739.

The builder appends a `.vsround` section at VA 0xCBF000 and changes three CPractice vtable entries: update at 0x7EEE54 (original 0x5F8CC0), setup at 0x7EEF28 (original 0x5FB8C0), and Training health recovery at 0x7EEF40 (original 0x5F8D50). Recovery is suppressed only for the two controlled fighters in Survival Training. Other contexts delegate to the original functions.

Current/max health fields are fighter offsets 5656/5652. Clock source is 0x8738EC, which the original circular TIME display converts using 60 ticks per second. Native reset recreates fighters; the VS spawn wrapper already assigns the opponent to controller 1.

The helper is compiled from owned source with installed Clang/LLD, with no CRT dependency or new imports. It calls the game's existing sprintf and SetWindowTextA import for the title. Only the executable contains the appended helper; no companion DLL is required at runtime.

## Verification

V7 SHA-256: 81B7620B6ECFBA7C2160D077C962BAADAB8B95EF77168582491CE0647B58C1FA.

Length: 4,663,808 bytes. Relative to V6: 12,288 appended bytes; 30 changed existing bytes, confined to the section metadata and three checked vtable entries. Source hash and expected original entries were checked. V6 remains untouched.

Scoring tests passed for KO, double KO, timeout wins, ties, differing maximum-health percentages, and invalid maximum-health handling.

Mock integration tests passed for equal starting health, recovery suppression/delegation, score retention across automatic resets, timeout draws, double KO, first-to-two completion, native menu state, clock rewind, and roster replacement. The mock uses translated addresses to avoid Windows' low-address allocations. It validates the controller logic and call conventions, not actual game lifecycle behavior. Compiler disassembly was checked for original-call targets and stdcall recovery cleanup.

Run scripts/test_vs_rounds.ps1 to repeat these tests with Clang, LLD, and the Windows SDK. Run scripts/build_vs_v7.ps1 against exact V6 to build the separate executable. These scripts preserve existing output files.

## Live test checklist

Close any other game build, then launch E:\GIGA\BALDRSKY_DIVEX\BaldrSky.VSPrototype.v7.exe with both controllers connected. Enter Survival > Training.

1. Confirm the title displays 0–0, 01:30 and both health bars start full.
2. Attack until a KO. Confirm one point, fresh health, and the same two mechas/controllers after reset.
3. Let a round expire with unequal health, then test equal-health timeout.
4. Win twice. Confirm the winner title and Training menu. Test F1 starting a fresh 0–0 match.
5. Test opponent selection, manual reset, leaving/re-entering, and paused menus.

The game may recover/destroy fighters at another lifecycle stage not represented in the mock. Report any missing KO award, premature timeout, lost controller assignment, incorrect menu behavior, or stale health display. These remain unverified until tested live.

Rollback: close V7 and launch the preserved V6 executable. No original backups, saves, assets, or earlier builds are changed. The private repository stores source and documentation only.

Live update: V7 failed to award a KO after P2 removal. Its maximum-HP interpretation also needed correction. See VS-v8-KO-and-names.md; V7 remains available for reproduction.
