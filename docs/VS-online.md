# Online VS development

Work is isolated on the `online-vs` branch. The current public release remains the working local VS launcher. **This is a connection/replay research build, not playable online combat.**

## Current implementation

The online launcher retains the existing mecha, equipment, stage, music and set controls. Create link code / Host produces one `BALDR1-...` code; the other peer pastes it and selects Join with link code. Copy code copies the complete code. A code carries an IPv4 address, UDP port and randomly generated 128-bit room key, encoded with a version prefix and a CRC for typing-error detection. It is an address-bearing invitation, not a short code resolved by a matchmaking server. It is not encryption or authenticated identity. Anyone given the code can attempt the session. A new code generates a new key.

Internet code creation retrieves the host's public IPv4 address over HTTPS from `api.ipify.org`. Discovery runs off the UI thread and has request timeouts. Advanced allows a manual advertised IPv4 override or LAN address. Two peers behind one NAT may need the LAN address override if that router cannot hairpin. IPv6-only connections and a matchmaking/relay fallback are not implemented. AutoPunch's own service is used for NAT traversal; public address discovery is not a gameplay relay.

Both peers currently select the same match settings and explicit loadouts. A bounded handshake checks the wire/input schema, opposite player roles, room key, endpoint, plugin SHA-256, canonical move-ID/type catalog SHA-256, mechas, stage, set, music, seed and all 28 equipment slots before GekkoNet receives gameplay-test packets. Local default sentinels are rejected. Native mecha IDs are not translated through display labels. Automatic exchange of each player's independent choice is a later lobby step.

The connection test uses real UDP and the actual GekkoNet session API, with an eight-frame prediction window, two-frame local input delay, complete save/load requests and desync checks. Its simulation is a small standalone deterministic test model. It never reads or writes native combat through predicted inputs. Both peers must match each other's final checkpoint and the expected replay checksum. A successful connection therefore does not claim native game rollback works. Online Start Battle stays disabled until a verified native bridge exists; turning off Online setup retains local startup behavior.

## Dependencies

Dependencies are built for Win32 and pinned to inspected upstream revisions:

| Project | Revision | License | Role |
| --- | --- | --- | --- |
| [AutoPunch](https://github.com/delthas/autopunch) | `db0b962a8bed74179997e755e851f99e885a10d1` | MIT | Winsock IPv4 NAT traversal |
| [GekkoNet](https://github.com/HeatXD/GekkoNet) | `9da2363d7c5a68cc77f8dfe2da7cf51d3afe3bee` | BSD-2-Clause | Peer input prediction, rollback events and desync detection |
| [Detours](https://github.com/microsoft/Detours) | `64ec135a509884aa60ac6c19b59564f1da9cb2fa` | MIT | AutoPunch's pinned submodule |

GekkoNet is statically linked with `NO_ASIO_BUILD=ON`. The custom nonblocking Winsock `sendto`/`recvfrom` adapter permits AutoPunch to intercept the same socket path. It accepts only the pinned peer after a matching handshake, limits envelope size and receive work per poll, and keeps allocation ownership consistent with GekkoNet's callback contract. The room key filters unrelated traffic; it is sent in the packet envelope and does not establish a secure channel.

The build stages a small adaptation of AutoPunch: DllMain does no network work, `StartAutoPunch` initializes outside the loader lock, and the module is retained for process lifetime. The upstream remote-process loader, injector UI and updater are not built. The DLL hooks only its own process. AutoPunch has no gameplay relay fallback, and this build has not been verified across two Internet connections yet. An Internet host code must not be interpreted as proof of reachability.

## Native replay findings (exact supported v1.00 image)

The user's built-in replay suggestion is the native determinism baseline.

| Native address | Observed behavior |
| --- | --- |
| `0x442F90` | Captures a 2728-byte initial setup; first word is `timeGetTime()` seed. Includes both mecha IDs, stage and 14 native equipment records, then calls the engine's `srand(seed)`. |
| `0x4BFD50` | Copies that setup into replay controller `0x86D3C0`, clears the input vector, enables recording. |
| `0x4BFC40` | Appends one eight-byte `(pressed, held)` input record and increments the replay frame counter. |
| `0x4BFA20` | Restores the next recorded input pair and advances the playback iterator. |
| `0x4BFE60` | Returns the initial setup, resets the playback iterator and enables playback. |
| `0x446100` | Restores initial mecha/stage/equipment settings, rebuilds native scene state and reseeds RNG. |
| `0x5F6830` | Calls native record or playback before the rest of the battle update. The traced input pair is **1P**, globals `0x874BE0` / `0x874BDC`. |
| `0x6763A0` | Engine CRT `srand`; RNG state lives at `engine_getptd()+20`, not in the plugin CRT. |

This replay path reconstructs from initial setup and consumes inputs. It is not yet evidence of arbitrary mid-combat state snapshots. Native playback does not capture the human 2P stream; the online format must retain both players, pressed-edge history, mod round state and equipment context. Replay frame count and the visible battle clock differ during pauses/hitstop; network frames must follow the actual combat-update boundary rather than elapsed seconds.

`native_replay_audit.h` is enabled only in an explicitly built diagnostic DLL. It replaces the single six-byte `timeGetTime` call at `0x442F98` with a fixed seed and hooks the two native record/playback call sites in `0x5F6830`. It records 300 actual combat frames with the native recorder, restarts through native `0x446100`, and replays 300 frames with 1P coming from the native input vector and 2P coming from the supplementary test stream.

The experiment found that native playback restores the combat setup and RNG but retains the global battle clock. Before the explicit clock restore, all 300 playback samples differed only by a constant clock offset; positions, HP, active moves, inputs and RNG already matched. Restoring `0x8738EC` to the recorded starting clock corrected that difference. Two complete runs then passed with every sampled record/playback frame matching, and the two runs' full traces matching each other. This covers one matchup, stage and scripted sequence. It is **not** a complete-state checksum or proof of arbitrary mid-combat rollback restoration. Broader moves, collisions, hitstop, round transitions and resets still need coverage.

Next engine work: capture and replay both real human input streams; expose a safe draw-free combat advancement function; connect Gekko save/load requests to verified native checkpoints or reconstruction; and include dynamic actors, mod round state, input histories, equipment context and engine RNG in restoration and validation. The native replay path provides a proven initial reconstruction baseline, not yet the complete rollback adapter. Audio, rendering and local UI effects need separate handling during resimulation.

## Build and tests

Clone the dependency repositories at the revisions above. AutoPunch's Detours submodule URL uses SSH upstream; it can be overridden to `https://github.com/microsoft/Detours.git` before initializing. No global Git configuration change is needed.

Run `scripts/build_online.ps1` with `-GekkoPath`, `-AutoPunchPath`, `-ImGuiPath` and `-NativeBuild` pointing to the verified existing local native build. Requires Visual Studio 2022 C++ tools, CMake and Git. Output is `online-build/`; it is ignored by Git and is not a public release. Place local game catalogs beside the launcher for UI testing. It does not bundle original game assets.

The build runs `test_online.exe`: wire bounds, 20,000 malformed-packet samples, room/version rejection, match changes, endpoint validation, link-code round trips/typos/truncation and 600 actual Gekko stress frames with 591 loads. `scripts/test_online_peers.ps1` starts two local peers, joins through one code and verifies their final checkpoints. Add `-AutoPunch` to load the requested traversal helper in both peers. Direct and AutoPunch loopback tests pass. This verifies helper compatibility, not router traversal.

For reproducible impaired UDP tests, use `-LagMs 60 -JitterMs 20 -LossPercent 5`. The test-only Python proxy affects only loopback ports 24764/24765 through 24766, includes 2% duplication and natural jitter reordering, and exits after the test. Both peers passed at 60 ms one-way delay, 20 ms jitter, 5% loss and 2% duplication after fixing completion-packet retransmission. It does not modify the operating system's networking configuration. Python 3 is required for that test.

Native audit: build the explicit `NativeReplayAudit` CMake target, then run `scripts/test_native_replay.ps1 -GameDirectory <owned-game-directory>`. It refuses a running game/launcher, checks the native executable hash, backs up the plugin/settings in memory and restores them in `finally`. It generates inputs and exits its own test processes; do not install that diagnostic DLL as a normal plugin. Audit CSVs remain local and are ignored.

## Clumsy plan

[Clumsy](https://github.com/jagt/clumsy) is the independent Windows network-condition test path requested by the user. Its WinDivert driver needs an administrator token; the current agent process does not have one. No driver has been installed or security setting changed. `scripts/start_clumsy_online_test.ps1 -ClumsyPath <official-clumsy.exe>` prepares 60 ms delay and 5% loss for 30 seconds when run from an administrator terminal. It refuses an already running Clumsy session. The prepared test filter is:

`outbound and ip and udp and ip.DstAddr == 127.0.0.1 and (udp.SrcPort == 24764 or udp.SrcPort == 24765) and (udp.DstPort == 24764 or udp.DstPort == 24765)`

Test direct baseline first, then 60 ms delay per direction, 20 ms jitter where supported, 1% and 5% loss, duplicates/reordering, and a short disconnect. Record both peers' checksums, rollback count, stalls and shutdown result. For the second-PC test, substitute only the selected peer IP and gameplay port. Do not use Clumsy's broad default all-traffic filter. Clumsy and two-PC Internet results are not yet verified.
