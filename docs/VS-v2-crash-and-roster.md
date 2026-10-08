# VS prototype v2: Training roster and attack crash

## Findings

Windows Application Error events for the v1 prototype recorded access violations (`0xc0000005`) at module offsets `0x23A239` and `0x23A25E`. These map to instructions at VA `0x63A239` and `0x63A25E` in `sub_63A140`.

Those instructions dereference the fighter's AI state pointer at fighter offset `0x183C`, reading/writing its direction cache at `+0x28`. Human-controlled fighters can lack this state. This provides a concrete explanation for the reported attack crash, but a live register dump was not captured.

## Changes

- Detour the cache read: preserve the original AI cache read when the pointer exists; otherwise use direction `+1` when the geometric result is tied. Preserve flags and stack depth.
- Detour the cache write: preserve the original write when the pointer exists; otherwise skip it. Replay `mov al,1` and preserve flags/stack depth.
- Replace Training enemy selector's unlock predicate call with `mov al,1; add esp,4`, preserving the original thiscall argument cleanup. This exposes entries in the existing Training roster; it does not invent new fighters or edit save ownership.
- Add a 512-byte executable `.vsfix` section for the helpers. Existing ISO, main-mode, character, and Player 2 assignment patches are retained.

| File offset | Before | After |
| --- | --- | --- |
| `0x239633` | `8B 93 3C 18 00 00` | `E9 C8 3D 68 00 90` |
| `0x23965E` | `89 41 28 B0 01` | `E9 B8 3D 68 00` |
| `0xDD10A` | `E8 91 2E F7 FF` | `B0 01 83 C4 04` |

Additional PE metadata changes: section count, size of code, size of image, and new section header at file offset `0x298`. Helper section file offset `0x46F400`, RVA `0x8BE000`. Read helper VA `0xCBE000`; write helper VA `0xCBE01B`.

Source SHA-256: `969C4CA6DBD933A2FDD2C075C790B52202D0A357DAC1E5DC493A4D027EFFC502`.

Result SHA-256: `225ACD27E51D2F54E240148A158084D66D41EA0C6A82155CEB1D352020392A3F`.

Length: 4,649,984 → 4,650,496 bytes. Exactly 32 original-file bytes changed, confined to the three patch regions and PE metadata; 512 bytes appended. Builder verifies source hash, expected bytes, available section-header padding, and allowed diff regions.

## Build, test, rollback

```powershell
powershell -File .\scripts\build_vs_v2.ps1 -SourcePath 'E:\GIGA\BALDRSKY_DIVEX\BaldrSky.VSPrototype.exe'
```

The builder leaves the source untouched and refuses to overwrite an existing v2 output. Copy the generated executable beside the installed game. Close other game instances, connect both controllers before startup, and launch `BaldrSky.VSPrototype.v2.exe`.

Enter Survival → Training. Press F3 (Enemy Change), inspect all roster pages, select previously locked mechas, and test Player 2 movement/attacks. Test attacks in both directions, multiple attack types, reset, opponent changes, and leaving/re-entering Training. The reported crash sites are guarded; other faults and individual mecha compatibility still require gameplay testing.

User validation on October 8, 2026: "2P can attack without crashes" and "2P roster can be selected too". These confirm the reported attack crash no longer occurs in the tested case and Player 2 roster selection works. The user subsequently confirmed Training reset and leaving/re-entering Training also work, including Player 2 control and attacks as described in the test instructions. Every roster entry, damage behavior, and extended attack coverage remain unverified. To roll back, close v2 and launch the unchanged regular executable or v1 prototype. No save files were modified by the patch builder.
