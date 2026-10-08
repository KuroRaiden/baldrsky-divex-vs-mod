# Baldr Sky DiveX — patch report

Date: October 8, 2026

## Modified executable

`E:\GIGA\BALDRSKY_DIVEX\BaldrSky.exe`

Two reversible patches were applied: an ISO launch check patch and a mode-selection unlock patch. No third-party patch executable was run.

## Exact changes

Offsets below are hexadecimal **file offsets**, not runtime addresses.

| Patch | File offset | Original byte | Patched byte |
| --- | --- | --- | --- |
| ISO launch check | `0x2742C2` | `74` | `EB` |
| Mode lock flag | `0x1D86E7` | `01` | `00` |
| Mode lock flag | `0x1D86F4` | `01` | `00` |
| Mode lock flag | `0x1D86F9` | `01` | `00` |

The mode patch initializes the three mode-selection lock flags to zero. Analysis showed that these flags control both the padlock display and selection blocking. It enables the six main mode buttons; it does not unlock every item, character, scenario, or stage within those modes.

After the first two patches, the executable had four changed bytes relative to the untouched original. The subsequent character-selector patch is documented below. Its length remains **4,649,984 bytes**.

## Backups and SHA-256 hashes

### Untouched original

Path: `E:\GIGA\BALDRSKY_DIVEX\BaldrSky.original.F53227BC.bak`

SHA-256: `F53227BCFCEF83173E590477F030518F1E07026D8F917AF057B805481E1A1402`

This backup was created before either patch and preserved unchanged.

### ISO patch only / before mode unlock

Path: `E:\GIGA\BALDRSKY_DIVEX\BaldrSky.before-mode-unlock.ABE88705.bak`

SHA-256: `ABE88705FF4F7D4860D8A91F5CAEA54BF4E52A0C2C582FA867A78F491EE73F19`

This backup preserves the ISO patch and was created before the three mode-unlock changes.

### Executable after mode unlock, before character unlock

Path: `E:\GIGA\BALDRSKY_DIVEX\BaldrSky.exe`

SHA-256: `7BDB29BD2864294C023B9BB78A7030FBCE27339107A1A3B93DB628E8E0928BB0`

All three files have a length of **4,649,984 bytes**.

## Verification and testing

- Initial inspection confirmed the executable existed and byte `74` was present at file offset `0x2742C2`.
- Windows file-version and product-version fields were empty. Version 1.00 was not independently confirmed. The first patch was applied after the user explicitly instructed proceeding with the verified byte change.
- The ISO patch was verified to differ from the original by exactly one byte, with unchanged file length.
- User screenshots showed the opening movie and title menu. ISO-unmounted status was not independently inspected by the agent.
- The mode patch was developed using a separate analysis copy of the executable. It was verified to differ from the ISO-patched backup by exactly three bytes, with unchanged file length and the ISO byte still `EB`.
- A subsequent user screenshot showed all three padlocks removed.
- The user reported: **“all modes worked.”** This confirms successful mode-launch testing by the user.
- Both backups were verified unchanged during patch verification.

## Reversing the patches

Close the game before restoring a backup. Copy the chosen backup over `E:\GIGA\BALDRSKY_DIVEX\BaldrSky.exe`:

- To undo only the mode unlock while keeping the ISO patch, restore `BaldrSky.before-mode-unlock.ABE88705.bak`.
- To undo both patches, restore `BaldrSky.original.F53227BC.bak`.

Keep the backup files after restoring. No save-file edits were made by the agent; the game may update its own saves during normal use.

## Subsequent character-selector unlock patch

The user requested unlocking all characters. Five calls to the ownership-state query in the character selector were replaced with `31 C0 90 90 90` (`xor eax, eax` followed by three NOPs). This makes these selector checks return the owned state (zero), covering selection-state lookup, purchase handling, character details, hover rendering, and portrait rendering. The global ownership query and save files were not modified.

| File offset | Before | After |
| --- | --- | --- |
| `0x14B1FD` | `E8 3E 52 F0 FF` | `31 C0 90 90 90` |
| `0x14B7A8` | `E8 93 4C F0 FF` | `31 C0 90 90 90` |
| `0x14CF88` | `E8 B3 34 F0 FF` | `31 C0 90 90 90` |
| `0x14ECE6` | `E8 55 17 F0 FF` | `31 C0 90 90 90` |
| `0x14F045` | `E8 F6 13 F0 FF` | `31 C0 90 90 90` |

Verification: exactly **25 changed bytes** relative to the pre-character-unlock executable; unchanged length of **4,649,984 bytes**; ISO patch and three mode-unlock bytes preserved. In-game character selection and combat testing are pending. This patch targets the existing character selector; it does not add two-player controls or establish that every other mode-specific roster is unlocked.

Current executable: `E:\GIGA\BALDRSKY_DIVEX\BaldrSky.exe`

Current SHA-256: `0A34F07F2698D8105E593AF4C2A09698807604DFFE2DD18E0B193FDBD6A00F02`

New verified backup: `E:\GIGA\BALDRSKY_DIVEX\BaldrSky.before-character-unlock.7BDB29BD.bak`

Backup SHA-256: `7BDB29BD2864294C023B9BB78A7030FBCE27339107A1A3B93DB628E8E0928BB0`

To undo only the character-selector patch, close the game and copy this new backup over `BaldrSky.exe`. It retains both earlier patches.
