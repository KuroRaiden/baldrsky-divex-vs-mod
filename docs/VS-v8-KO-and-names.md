# V8 KO reset and second overhead name

V7 live testing failed: the user defeated P2 but the title stayed 0–0. Read-only inspection found both the global P2 pointer and Training opponent pointer cleared. V7 required both fighters to exist before checking a KO, so it returned without awarding the round. Its mock had tested zero health but not actual fighter removal.

V8 continues result checking for an established Training match when a fighter disappears. A missing current fighter counts as defeated; a missing pair draws. Retired fighter addresses are never dereferenced. The normal Training setup recreates both fighters and the existing controller reassignment remains in place. A positive replacement pointer still starts a fresh match for roster changes.

The current HP field is offset 5656. Offset 5652 is a scale/count, not a raw maximum HP value; live inspection observed 1. V8 sets that field to 1 and current HP to 10,000, comparing health against the agreed 10,000-point starting amount. V7 incorrectly wrote 10,000 to both fields. This corrects the maximum-field interpretation; the current-HP address was already correct.

Manual battle-clock rewind is detected before the original update, so its next tick cannot hide the reset. Explicit setup clears the native Training menu state when starting the fresh match.

The user also reported a missing overhead 1P name. The singleton native lock cursor normally labels only its target. V8 renders the original current cursor, then uses a stack copy to render the opposite fighter's name and distance. The copy borrows font/opacity resources, creates and destroys its own name string, and restores the global distance observer before returning. It does not create a second live cursor or retain fighter references. Retired cursors still skip the extra pass.

## Build and checks

- Builder: scripts/build_vs_v8.ps1; source: src/vs_rounds_v8.c.
- Exact source is preserved V6, SHA-256 7035444F3ABD89077ABAF487F11282AF38FA114F967D98CF07CA32E45DC7D739.
- V8 SHA-256 A2175A86B8A06076075659C6E580D5B293BB52BEB96B625CCEC5929584A9458F.
- Length 4,663,808 bytes; 12,288 appended and 32 changed existing bytes relative to V6. Changes are checked section metadata and four vtable entries.
- Scoring and translated-address mock integration tests passed, including removed P1, removed P2, both removed, respawned pointers, retained scores, corrected HP scale, clock rewind, two name passes, owned-string disposal, and observer restoration.
- Compiler disassembly checks confirmed native render/name/destructor calls and stack cleanup.

These checks do not confirm real game lifecycle or visual rendering. Both reported fixes remain pending live verification. V6 and V7 executables remain untouched; no saves or assets were modified.

## Live test

Close V7 and launch E:\GIGA\BALDRSKY_DIVEX\BaldrSky.VSPrototype.v8.exe. Enter Survival Training with both controllers connected. Confirm both names, defeat P2, then defeat P1. Each KO should award one point and recreate the same selected mechas with fresh health. Test two-win completion, F1, timeout, and roster changes. Roll back by closing V8 and launching V6.
