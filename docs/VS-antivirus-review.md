# Online preview antivirus report — October 25, 2026

Status: under investigation. A false positive has not been confirmed.

A user reported Microsoft Defender detecting `Trojan:Script/Wacatac.B!ml` in `BaldrVS-Online-Chat-Test (1).zip` on their Shadow PC. The reported affected item is the entire archive; the triggering EXE, DLL or archive content has not been identified. The current online-branch binary download is paused pending review. Earlier Git commits and previously shared pinned URLs can still contain the reported build; do not use those as an alternative download while this report remains unresolved.

The affected ready-check build was published from source commit `bd50419`. Its ZIP contains the online launcher, AutoPunch helper, four licenses, three TSV name lists, instructions and a checksum manifest. It contains no original game executable, equipment catalog, player profile or chat transcript.

| File | SHA-256 |
| --- | --- |
| `BaldrVS-Online-Chat-Test.zip` | `974DEEEC36FE12EB6900FFDD3BBA45AC0C06981BFC72BDB0F698B03D3A46ED00` |
| `BaldrVSLauncherOnline.exe` | `C7C1B29DE440CEB950B3F78B6E89E23B4E706CCE64DCBFB5BB4EFDBF43150A8C` |
| `BaldrAutoPunch.dll` | `EE88E2CB332ABF3B2DE38DDB8A77030FEF4C22A283F6ADED5A1A1CBB1977E462` |

## Checks completed

- The repository ZIP matches the published build, and every member matches its checksum manifest. This verifies package integrity, not safety.
- AutoPunch, GekkoNet and Detours checkouts have the expected pinned revisions and no local changes.
- Both distributed binaries are unsigned. This fact does not establish the cause of the alert.
- Source and import inspection is consistent with the intended setup: the launcher starts/closes its own local game child, uses HTTPS for public-address discovery and loads the AutoPunch helper into its own process. AutoPunch uses Detours to intercept that process's Winsock functions. Hooking is a possible contributor to heuristic detection; the triggering feature is not known.
- Microsoft Defender on the development PC reported no threats in isolated copies of the archive, launcher and helper, both as a folder scan and individual custom scans. Real-time protection remained enabled. Security intelligence version was `1.461.192.0`, last updated October 24, 2026. A separate exclusion-status check returned access denied, so this local scan cannot establish exclusion status or rule out a cloud-PC-specific result.

No protection settings were changed, quarantined files restored, exclusions added, or replacement binaries produced to work around the detection. The source review and local scan do not prove the cloud detection is incorrect.

## Review path

The exact ZIP and component hashes, source commit and detection name should accompany a [Microsoft Security Intelligence file-analysis submission](https://www.microsoft.com/en-us/wdsi/filesubmission). Microsoft provides this route for suspicious files and suspected incorrect classifications. No submission or vendor determination has been obtained yet. The local review notes and scan/import logs are retained for that submission; no private game files are needed.

The public stable local-VS release was not changed by this online-preview report. The report does not make a safety determination about other files or releases.
