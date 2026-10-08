# V9 pause flag width correction

Live V8 testing confirmed two-win match completion once, then later reported another stuck 0–0 match with a defeated opponent. Read-only inspection of the running V8 process found Training state 0 and a four-byte value of 0x01000000 at 0x874B98. The actual pause flag is the first byte, which was zero. An unrelated adjacent flag was set.

V8's four-byte pause read treated that neighboring flag as a pause and skipped both KO and timeout scoring. V9 changes only the source-level pause check to a volatile unsigned-char read at 0x874B98. Compiler disassembly confirms `cmpb`, replacing the word-sized check. V8's KO-removal and two-name corrections remain included.

The new mock reproduces the failure with pause byte zero and neighboring byte set. It fails on the V8 logic and passes on V9. A true pause-byte value of one still blocks scoring. All existing scoring, removal, reset, and two-name mock checks also pass. Actual gameplay verification is pending.

V9 SHA-256: EC7D2ABBE1BB7638394BF8184A6D76143DDAF00431960B31E6F4A8C517E75D87.
Length: 4,663,808 bytes. Built from preserved exact V6 (7035444F3ABD89077ABAF487F11282AF38FA114F967D98CF07CA32E45DC7D739), with 12,288 appended bytes and 32 changed existing bytes in checked metadata/vtables. Source: src/vs_rounds_v9.c; builder: scripts/build_vs_v9.ps1; tests: scripts/test_vs_v9.ps1.

Close V8 and launch E:\GIGA\BALDRSKY_DIVEX\BaldrSky.VSPrototype.v9.exe. Enter Training, defeat each player in turn, and confirm round awards and resets. Also test timeout and F1 after two wins. V8 remains untouched. No live process memory, saves, or assets were modified.
