# Online VS development

Work is isolated on the `online-vs` branch. The current public release remains the working local VS launcher. **This is a connection/replay research build, not playable online combat.**

## Current implementation

The online launcher retains the existing mecha, equipment, stage, music and set controls. Create link code / Host produces one `BALDR1-...` code; the other peer pastes it and selects Join with link code. Copy code copies the complete code. A code carries an IPv4 address, UDP port and randomly generated 128-bit room key, encoded with a version prefix and a CRC for typing-error detection. It is an address-bearing invitation, not a short code resolved by a matchmaking server. It is not encryption or authenticated identity. Anyone given the code can attempt the session. A new code generates a new key.

Internet code creation retrieves the host's public IPv4 address over HTTPS from `api.ipify.org`. Discovery runs off the UI thread and has request timeouts. Advanced allows a manual advertised IPv4 override or LAN address. Two peers behind one NAT may need the LAN address override if that router cannot hairpin. IPv6-only connections and a matchmaking/relay fallback are not implemented. AutoPunch's own service is used for NAT traversal; public address discovery is not a gameplay relay.

The connection/chat test runs without game files or an equipment catalog. Its bounded handshake checks the wire/input schema, opposite player roles, room key, endpoint and fixed test-simulation manifest before GekkoNet receives test packets. The selected mechas, stage and loadouts do not affect this standalone simulation and need not match. A future native battle bridge must agree on the actual game/plugin hashes, catalog, seed and both players' settings/loadouts before combat; a compile-time gate prevents enabling native combat through the current test manifest.

The connection test uses real UDP and the actual GekkoNet session API, with an eight-frame prediction window, two-frame local input delay, complete save/load requests and desync checks. Its simulation is a small standalone deterministic test model. It never reads or writes native combat through predicted inputs. Both peers must match each other's final checkpoint and the expected replay checksum. A successful connection therefore does not claim native game rollback works. Online Start Battle stays disabled until a verified native bridge exists; turning off Online setup retains local startup behavior.

## Cloud PC setup and player names

Download the [online connection/chat preview](https://github.com/KuroRaiden/baldrsky-divex-vs-mod/raw/refs/heads/online-vs/downloads/BaldrVS-Online-Chat-Test.zip) on **both computers**. Close any older online launcher, extract the entire ZIP into a writable folder and run `BaldrVSLauncherOnline.exe` from that folder. The cloud computer does not automatically receive local workspace files. This package is a development preview on the `online-vs` branch; it does not replace the public stable local-VS release.

The online EXE embeds the 99 mecha names, 220 stage names and 92 music names, with TSV copies also included in the ZIP. Online controls appear even if external catalogs are absent. This fixes the earlier cloud-PC startup that showed only "Roster unavailable" and hid the connection controls. No original game executable, assets or equipment-description catalog is distributed in this preview. Local game startup still requires a separately installed compatible native build; native online battles remain disabled.

Enter **PLAYER NAME** before hosting or joining. Names allow 1–24 printable UTF-8 characters (up to 96 bytes), trim outer spaces, and save to `BaldrVS-player-name.txt` beside the launcher when leaving the field or connecting. If the folder cannot be written, the name remains usable for that session and a notice explains the save failure. This local profile is ignored by Git and excluded from the package. Names are exchanged during the handshake, shown above the peer's chat and attached to messages with P1/P2 labels. A name cannot change during an active connection. Display names are self-chosen labels, not verified identities.

One player selects Create link code / Host and shares the code; the other enters it and selects Join with link code. Leave AutoPunch enabled for an Internet test. The same wire version is required on both PCs. An October 24 user screenshot shows the local and Shadow PC launchers both passing the connection test and exchanging named chat messages with AutoPunch active. This is evidence for that two-PC connection/chat setup; native online combat and broader router coverage remain unverified.

## Ready check

Connecting opens a lobby with chat. The synchronization test stays at frame zero until **both players check Ready (you)** in their own chat pane. A read-only **Peer ready** check shows the other player's state. Players can uncheck before start; the control briefly locks while a ready change is being acknowledged and locks once startup is committed. Each accepted change posts a named system message, such as "Kou is ready." or "Kou is not ready.", in both chat histories. Both peers also receive one start notice. Readiness clears on disconnect and every new connection.

Ready changes use a separate sequenced control channel with acknowledgements and 250 ms retries, so a full chat queue cannot block the ready gate. The host proposes start for the exact two current ready revisions; the guest validates that pair and acknowledges it. A delayed proposal cannot start a lobby after the guest unchecks. Lost start acknowledgements are retried without duplicate notices. Lobby control packets pass the same room/role/endpoint checks as chat. Both-ready starts the current standalone netplay test; it does not enable the unfinished native battle bridge.

## Starting local VS on a second PC

The online preview deliberately disables **Start Battle** while **Online setup** is checked, and explains this beside the button. To play local VS on the cloud PC, transfer the matching `BaldrSky.VSNative.exe`, `BaldrVSPlugin.dll` and `BaldrVSLauncher.Native.exe` from the working PC into the cloud game's folder, together with the mecha/stage/music TSV lists. Run the Native launcher, or uncheck Online setup in the Online launcher. The original `BaldrSky.exe` alone is not the native VS build. Keep the original game and its data files. Missing native files are now named directly in the setup status; startup still verifies the exact supported hashes. This local startup path does not connect the two PCs for gameplay.

## Player chat

The ImGui player-chat pane sits beside the match controls. After peers connect with a link code, either player can type a message and press Enter or Send. Messages show the sender's display name and P1/P2 label, wrap in a scrollable history, and display pending or failed delivery. The transcript follows new messages when already near its bottom. It retains the most recent 100 lines in memory and resets for a new connection; no chat log is written to disk.

Chat uses separate control packets on the existing peer socket, outside Gekko input/state events. Each direction uses ordered sequence numbers, acknowledgements and 250 ms retries; duplicates are acknowledged without appearing twice. Messages allow up to 512 UTF-8 bytes and eight pending sends. Invalid/truncated UTF-8, control characters and oversized messages are rejected. Text is rendered through ImGui's plain-text API. Incoming packets still require the existing room/role/endpoint handshake. The wire protocol is now version 4 for the ready/start controls, so both players need this build; the link-code format remains BALDR1.

After the synchronization test passes, the GUI keeps the peer connection open for chat until Disconnect, peer departure, timeout or launcher close. Hosts wait for the invited player until cancelled. The connection test does not start native combat. Unit tests cover validation, loss/retries, duplicate/out-of-order handling, queue/history bounds and disconnection. Real two-peer tests send three messages each direction, including a maximum-size message and a message after test completion. Direct and AutoPunch tests with 60 ms delay, 20 ms jitter and 5% loss pass without duplicate transcript entries. Run `scripts/test_online_peers.ps1 -Chat` or add `-Chat -AutoPunch -LagMs 60 -JitterMs 20 -LossPercent 5` for that integration check.

## Dependencies

Dependencies are built for Win32 and pinned to inspected upstream revisions:

| Project | Revision | License | Role |
| --- | --- | --- | --- |
| [AutoPunch](https://github.com/delthas/autopunch) | `db0b962a8bed74179997e755e851f99e885a10d1` | MIT | Winsock IPv4 NAT traversal |
| [GekkoNet](https://github.com/HeatXD/GekkoNet) | `9da2363d7c5a68cc77f8dfe2da7cf51d3afe3bee` | BSD-2-Clause | Peer input prediction, rollback events and desync detection |
| [Detours](https://github.com/microsoft/Detours) | `64ec135a509884aa60ac6c19b59564f1da9cb2fa` | MIT | AutoPunch's pinned submodule |

GekkoNet is statically linked with `NO_ASIO_BUILD=ON`. The custom nonblocking Winsock `sendto`/`recvfrom` adapter permits AutoPunch to intercept the same socket path. It accepts only the pinned peer after a matching handshake, limits envelope size and receive work per poll, and keeps allocation ownership consistent with GekkoNet's callback contract. The room key filters unrelated traffic; it is sent in the packet envelope and does not establish a secure channel.

The build stages a small adaptation of AutoPunch: DllMain does no network work, `StartAutoPunch` initializes outside the loader lock, and the module is retained for process lifetime. The upstream remote-process loader, injector UI and updater are not built. The DLL hooks only its own process. AutoPunch has no gameplay relay fallback, and the user has demonstrated the connection/chat test on a local PC and Shadow PC. Broader router compatibility remains unverified. An Internet host code must not be interpreted as proof of reachability.

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

Ready-check verification: `test_online_ready.exe` covers the one-ready gate, unchecking, stale/reordered revisions, malformed controls, lost ready/start acknowledgements, disconnect reset and exactly-once chat notices. It also checks that a normal chat acknowledgement updates its own message when a system notice has the same numeric ID. Two-peer direct and AutoPunch tests with 60 ms delay, 20 ms jitter and 5% loss passed: the lobby remained at frame zero before both checks, the host unchecked/rechecked, both peers saw five unique status notices, and three chat messages in each direction were delivered, including a 512-byte message and post-test chat. The launcher UI was visually checked without game files: ready controls are disabled offline, the online-battle limitation is visible, and switching to local setup names the missing native pair.

Clone the dependency repositories at the revisions above. AutoPunch's Detours submodule URL uses SSH upstream; it can be overridden to `https://github.com/microsoft/Detours.git` before initializing. No global Git configuration change is needed.

Run `scripts/build_online.ps1` with `-GekkoPath`, `-AutoPunchPath`, `-ImGuiPath` and `-NativeBuild` pointing to the verified existing local native build. Requires Visual Studio 2022 C++ tools, CMake and Git. Output is `online-build/`; it is ignored by Git. Roster, stage and music name lists are embedded from `data/`. Native files staged for local development are not part of the connection/chat preview.

Run `scripts/package_online.ps1` after a successful build. Its explicit whitelist includes the online EXE, AutoPunch helper, four dependency licenses, three name lists, instructions and SHA-256 manifest. It excludes game binaries, the native combat plugin, equipment data, local profiles, chat history and diagnostics. The generated ZIP can be copied to another PC without a local game installation for connection/chat testing.

The build runs `test_online.exe`: wire bounds, 20,000 malformed-packet samples, room/version rejection, match changes, endpoint validation, link-code round trips/typos/truncation and 600 actual Gekko stress frames with 591 loads. Chat tests cover UTF-8 names, name lengths, profile stream save/reload and name display snapshots. An EXE startup check (`--verify-catalogs`) verifies all 99/220/92 embedded lists without external TSV files. A separate clean-folder UI test confirmed populated dropdowns, online controls and profile save/reload without game files. `scripts/test_online_peers.ps1` starts two local peers with different names, joins through one code and verifies their final checkpoints; `-Chat` also checks exchanged names and messages. Add `-AutoPunch` to load the requested traversal helper in both peers. AutoPunch/name/chat loopback tests with 60 ms delay, 20 ms jitter and 5% loss pass. This verifies helper compatibility, not router traversal.

For reproducible impaired UDP tests, use `-LagMs 60 -JitterMs 20 -LossPercent 5`. The test-only Python proxy affects only loopback ports 24764/24765 through 24766, includes 2% duplication and natural jitter reordering, and exits after the test. Both peers passed at 60 ms one-way delay, 20 ms jitter, 5% loss and 2% duplication after fixing completion-packet retransmission. It does not modify the operating system's networking configuration. Python 3 is required for that test.

Native audit: build the explicit `NativeReplayAudit` CMake target, then run `scripts/test_native_replay.ps1 -GameDirectory <owned-game-directory>`. It refuses a running game/launcher, checks the native executable hash, backs up the plugin/settings in memory and restores them in `finally`. It generates inputs and exits its own test processes; do not install that diagnostic DLL as a normal plugin. Audit CSVs remain local and are ignored.

## Clumsy plan

[Clumsy](https://github.com/jagt/clumsy) is the independent Windows network-condition test path requested by the user. Its WinDivert driver needs an administrator token; the current agent process does not have one. No driver has been installed or security setting changed. `scripts/start_clumsy_online_test.ps1 -ClumsyPath <official-clumsy.exe>` prepares 60 ms delay and 5% loss for 30 seconds when run from an administrator terminal. It refuses an already running Clumsy session. The prepared test filter is:

`outbound and ip and udp and ip.DstAddr == 127.0.0.1 and (udp.SrcPort == 24764 or udp.SrcPort == 24765) and (udp.DstPort == 24764 or udp.DstPort == 24765)`

Test direct baseline first, then 60 ms delay per direction, 20 ms jitter where supported, 1% and 5% loss, duplicates/reordering, and a short disconnect. Record both peers' checksums, rollback count, stalls and shutdown result. For the second-PC test, substitute only the selected peer IP and gameplay port. Do not use Clumsy's broad default all-traffic filter. Clumsy results are not yet verified. The user has demonstrated a two-PC connection/chat session; native online battles remain in development.
