# V11c: restore 1P's move list in local VS

The user reported an empty left move list while the right list remained visible. Read-only inspection found 15 native P1 rows at x=0, y=530, mode=1, but the primary manager's visibility byte at +48 was zero. Native HUD hide (`0x4C1A40`) clears this byte; show (`0x4C1690`) sets it. The P2 manager was separately initialized visible, so the two lists disagreed.

V11c temporarily sets the native visibility byte only while rendering a row that belongs to the current primary manager in eligible two-human Training VS. It restores the exact original byte after the native renderer returns. Unrelated rows and other game modes delegate unchanged. Existing menu/object visibility remains under native GUI control. No save files are edited.

Retains V11b controls, HUD, countdown and rounds, and includes the five-byte native Training stage-selector unlock described in Training-stage-unlock.md. V12's failed shared-selector hooks are absent.

Installed separately: `E:\GIGA\BALDRSKY_DIVEX\BaldrSky.VSPrototype.v11c.exe`

SHA-256: `42AF4F00C5F5DF743E75512A8E04289DBCECD6DBC5F430E1B97B745898969460`

Length: 4,667,904 bytes. Built from exact V6 source through the verified append-helper builder; all base-file modifications are checked against explicit allowed ranges. Existing V11b and stage-only builds remain untouched.

`test_vs_v11c.ps1` passes existing scoring, countdown, round reset, cursor and P2 tracker integration mocks plus regressions for a hidden native P1 row, restoration of its visibility byte, unrelated row delegation and non-VS mode delegation.

Live visual validation is pending. Close the running build, launch V11c, enter Training, and check both move lists during combat, after a mecha change, and after F1/reset or re-entry. Launch V11b to revert.

The standalone ImGui launcher DLL is in development. Its host and menu compile, but Start Battle is disabled in the current preview until native auto-start is validated. Do not treat the launcher preview as a playable release.
