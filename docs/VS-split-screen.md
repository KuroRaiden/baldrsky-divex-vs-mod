# Experimental distance-triggered split-screen

Launch `BaldrVSLauncher.SplitScreen.v2.exe` beside the existing game installation. It uses its own `BaldrVSSetup.SplitScreen.v2.dll` and the existing roster/stage catalogs and exact V11c base. The standard launcher/helper and game executable remain unchanged. Use one game instance and connect both controllers before launching.

When the V6 shared camera would request zoom below its safe 192/256 floor, the prototype renders two vertical views: 1P on the left and 2P on the right. The trigger is horizontal center separation above 653 pixels or vertical separation above 333 pixels, derived from V6's 640-by-400 fit area and 200-pixel margin. It merges when both separations fall to at most 580 horizontally and 280 vertically. Hysteresis prevents repeated switching at the boundary. Menus, transitions and missing fighters return to the native shared view.

## Implementation

The helper replaces two verified calls in the newly launched child's memory, while its main thread is paused during initialization:

- `0x787937`: native scene-render call to `0x740150`, redirected to a wrapper that temporarily uses native 256/256 scale when split and restores both 28-byte zoom-counter records afterward.
- `0x740E8F`: world-render call to `0x73F6A0`, redirected to a wrapper that draws the sorted native world list in two clipped viewports using each fighter's body center.

The HUD is still rendered once by the native scene renderer. Simulation, input and the battle clock are not called by the split wrappers. Each object's own clip rectangle is intersected with its viewport. Each pass temporarily sets the root camera counters at +992/+1020, so background routines that query the camera directly use the same position as that viewport. Both complete 28-byte records are restored before the native HUD draws. Layer scroll ratios are applied around each viewport origin, including the right half, to retain native parallax. The original surface clip is restored after both passes. A four-pixel black divider is drawn with the surface FillRect method (vtable+120) while split. `split_screen_status` exposes split state, scene-frame count and both world-pass counts for read-only diagnostics. This is a separate experiment; broad native draw-object compatibility is not established.

## Build and verification

Run `scripts/build_launcher_split.cmd` with the absolute Dear ImGui checkout path used by the regular launcher and `v2` as the second argument. Output goes to the ignored `launcher-split` directory. The helper hash header is generated separately. The launcher remains mouse/keyboard only.

The split/merge threshold, hysteresis, disabled-state reset, extreme-coordinate and viewport-intersection checks pass. Protocol/export checks and rejection of an unrelated host also pass. The live startup test reached Survival Training, created both human-controlled fighters, and advanced the battle clock. The v1 live counters observed both world passes and split/merge transitions without doubling the battle clock. User feedback requested an obvious divider and independent background scrolling. V2 builds and passes the same checks; its separate launcher/helper pair is installed. The user confirmed that V2 shows the divider, scrolls each background with its player, and merges when the players come together. During packaging, Windows blocked the V2 launcher executable and it disappeared from the installed folder. The confirmed source remains available, but V2 distribution is blocked; no quarantined file was restored and no security setting was changed.

Test by moving both players beyond the camera range, then back together. Check terrain, actors, effects, range rings and overhead labels in both halves; the HUD/trackers should remain single. Check attacks, round resets, stage changes and menus, and watch for black gaps or frame-rate loss. A second full world pass increases rendering work. Current transitions switch immediately; blending remains a future refinement.

## Rollback

Close the test game and launch the standard `BaldrVSLauncher.exe`. The experiment can be removed by deleting only `BaldrVSLauncher.SplitScreen.v2.exe` and `BaldrVSSetup.SplitScreen.v2.dll` after closing it. Keep the standard launcher/helper, catalogs, V11c and all game data.


