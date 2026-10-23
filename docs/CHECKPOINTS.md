# Checkpoint archive

Newest first. Live checkpoint is in [PROGRESS.md](PROGRESS.md).

## Checkpoint (2026-10-23, office laptop `DESKTOP-MRMGNPL`, RTX 4060 Ti)

Commits: `43338c3` (marquee text), `93af806` (part ring fixes), `0dcfa78` (view tab pip) on `main`, plus this checkpoint commit, all pushed.
Working tree: `DefaultEngine.ini` and `DefaultGame.ini` (local `TelemetrySource=WebSocket`; committed default stays File) kept out of commits.

**Where I stopped:** text overflow fixed: new `UTwinMarqueeTextWidget` / `WBP_MarqueeText` (SizeBox > ScrollBox > TextBlock, caps at
`MaxVisibleWidth`, scrolls longer text with holds at both ends); top bar link detail uses it (receiver URL while Connecting). Built and
checked. Part rings: seen only in the top-left corner, also on the personal laptop with the Jeep. Cause found in `WBP_CarCalloutLayer`:
the five ring copies (`RingHood`, `RingTyreFL/FR/RL/RR`) never had **Ring Signal** set (all Engine), and their Anchor Tag is most likely
the default `CalloutAnchor.Hood`, which `BP_VehicleTwin` doesn't seem to carry, so the ring can't be placed and stays at its designer
position. Code fix in `93af806` (builds, office laptop; not seen running): unplaceable rings are hidden, Play start logs each ring's signal and anchor
(`LogTwinCallouts`), one warning per ring that can't be placed. `WBP_CarCalloutLayer` now stretches in `WBP_TwinOpsHUD`.

**Resume (personal laptop):**
1. Pull, build, open the editor.
2. `WBP_CarCalloutLayer` › each ring copy › Part Ring: `RingHood` Engine / `CalloutAnchor.Engine`; `RingTyreFL` Tyre Front Left / `Wheel.FL`;
   `RingTyreFR` / `Wheel.FR`; `RingTyreRL` / `Wheel.RL`; `RingTyreRR` / `Wheel.RR`. Set on the instances, not in `WBP_PartRing` Class Defaults.
3. Relay `python Tools/Relay/relay.py --source live --loop --rate 20` + PIE: Output Log `Part ring ...` lines show the settings; no
   "can't be placed" warning; > 90 s hood ring pulses amber on the car (coolant ≥ 105 °C), ~153 s RR wheel ring.
4. Build page F (https://claude.ai/artifact/AtxBFvjTzngZuMV9Z5k87u): F1 (Telemetry Source WebSocket, MPC on `BP_VehicleTwin`;
   `L_TwinStudio` already references `TwinOpsGameMode`), F2 PIE against the relay, F3 frame the four cameras + retune the studio lights.
5. Other overflowing labels: reuse `WBP_MarqueeText`, or for text that changes every refresh use Overflow Policy Ellipsis in a capped SizeBox.
6. Rule while switching laptops: `.uasset`/`.umap` can't merge; never edit the same asset on both before push + pull.

**Unverified:** ring diagnosis read from the binary asset, not seen in the editor; whether `BP_VehicleTwin` has a `CalloutAnchor.Hood` tag;
part rings never seen on the car; callout anchor places not checked on the car; camera blends; `UViewModelSubsystem` engine tick seen in PIE
(M10–M11 test it by hand, pass). Check whether still open: UMG float font sizes, BackgroundBlur Corner Radius, Custom node LWC warning. Older: SPEC §1 25
`Paint` meshes vs 8 in the BP; Day 4 `CarRoot` facing warning; doubled outline with editor preview at PIE start; Day 10 "can't connect: no
details" wording.

**Open decisions:** idle (dim) ring when all Normal, if the mock shows one; showroom mode (after H1); GrabCAD license / mesh redistribution
(ASSETS.md); private asset repo for `Content/Jeep/`; Day 8 LinkedIn clip; BUG-002 parked. Pending: add T1 content to the Vehicle Twin Field
Guide; clean up unused font faces and duplicate `.ttf` folders in `/Game/Art/UI/Fonts/` (4 faces used); build page fixes: B3 label is
`Uncompressed (RGBA8)` in 5.7, asset folders on the page (`/Game/UI/...`) differ from the project (`/Game/Art/UI/`, `/Game/Blueprints/UI/`).

**Not in git (local only, personal laptop):** `Content/Jeep/` (`ImportA`, `Cleaned`, `Blueprints/BP_VehicleTwin` with the callout anchors,
`Rebuilt/`), `Content/Maps/` levels except `L_TwinStudio`, raw CAD in `RawCAD/`. The office laptop has no car: `L_TwinStudio` loads without
`BP_VehicleTwin`, so rings and callouts can't be checked here.

## Checkpoint (2026-10-23, personal laptop, RTX 3070 Ti Laptop, host `VigneshSuresh`)

Commit: checkpoint commit after `6bf70a8` on `main`, pushed (UI fix `.uasset`s, PROGRESS/TESTS updates).
Working tree: `DefaultEngine.ini` (startup map `L_VehicleTwin`) and `CarDigitalTwins.uproject.DotSettings.user` kept out of commits; `ImportTestMap` and `L_VehicleTwin` untracked on purpose.

**Where I stopped:** UI issues from 2026-10-14 fixed (layout adjustments to match the mock; 12 dashboard WBPs in this commit). MVVM automation
tests run headless: M1–M9 all pass (TESTS.md §7; headless needs the `RestoreOpenAssetTabsOnRestart=NeverRestore` override, TESTS.md top).
Part rings not seen on the car yet: by design hidden until a part leaves Normal (Day 11 backlog has the timings and checks).

**Resume (personal laptop):**
1. Rings: relay `python Tools/Relay/relay.py --source live --loop --rate 20` + PIE, wait > 90 s → hood ring pulses amber (coolant ≥ 105 °C),
   ~153 s RR wheel ring. Not there → check `WBP_PartRing` copies in `CalloutCanvas` (`RingSignal`, anchor tag) and the tag on `BP_VehicleTwin`.
2. Build page F (https://claude.ai/artifact/AtxBFvjTzngZuMV9Z5k87u): F1 Telemetry Source WebSocket, GameMode Override `TwinOpsGameMode`
   on `L_TwinStudio`, MPC on `BP_VehicleTwin`; F2 PIE against the relay; F3 frame the four cameras.
3. Studio lighting looks off: retune `L_TwinStudio` lights on the paint together with F3 (Day 11 backlog).
4. Rule while switching laptops: `.uasset`/`.umap` can't merge; never edit the same asset on both before push + pull.

**Unverified:** part rings never seen running; callout anchor places not checked on the car; camera blends; `UViewModelSubsystem` once-per-frame
flush (M10 not written). Check whether still open after the UI fixes: UMG float font sizes, BackgroundBlur Corner Radius, Custom node LWC
warning. Older: SPEC §1 25 `Paint` meshes vs 8 in the BP; Day 4 `CarRoot` facing warning; doubled outline with editor preview at PIE start;
Day 10 "can't connect: no details" wording.

**Open decisions:** idle (dim) ring when all Normal, if the mock shows one; showroom mode (after H1); GrabCAD license / mesh redistribution
(ASSETS.md); private asset repo for `Content/Jeep/`; Day 8 LinkedIn clip; BUG-002 parked. Pending: add T1 content to the Vehicle Twin Field
Guide; clean up unused font faces and duplicate `.ttf` folders in `/Game/Art/UI/Fonts/` (4 faces used); build page fixes: B3 label is
`Uncompressed (RGBA8)` in 5.7, asset folders on the page (`/Game/UI/...`) differ from the project (`/Game/Art/UI/`, `/Game/Blueprints/UI/`).

**Not in git (local only, personal laptop):** `Content/Jeep/` (`ImportA`, `Cleaned`, `Blueprints/BP_VehicleTwin` with the callout anchors,
`Rebuilt/`), `Content/Maps/` levels except `L_TwinStudio`, raw CAD in `RawCAD/`.

## Checkpoint (2026-10-14, personal laptop, RTX 3070 Ti Laptop, host `VigneshSuresh`)

Commit: checkpoint commit after `923e645` on `main`, pushed (includes `DefaultGame.ini` with F1 `HudWidgetClass` = `WBP_TwinOpsHUD`).
Working tree: `DefaultEngine.ini` (startup map `L_VehicleTwin`) and `CarDigitalTwins.uproject.DotSettings.user` kept out of commits; `ImportTestMap` and `L_VehicleTwin` untracked on purpose.

**Where I stopped:** build page Part E finished: E6 `WBP_PowertrainPanel` (in `91b533b`), E7–E11 `WBP_TyresPanel`, `WBP_BodyPanel`,
`WBP_SidePanel`, `WBP_CarCalloutLayer`, `HUD/WBP_TwinOpsHUD` in `923e645`. Some UI issues seen at the end of the session, not written down yet.
Build page updated: E11 canvas slot fields named as the Details panel shows them (Offset vs Position/Size per anchor), E10 slot anchors
stay top-left (C++ sets alignment, size, position), E11 waiting-panel frame rows spelled out.

**Resume (personal laptop):**
1. ~~Write down the UI issues from 2026-10-14 here, then fix them.~~ Done 2026-10-23: small layout adjustments to match the mock.
2. Session Frontend › Automation `CarDigitalTwins.MVVM` → expect 9 green (M1–M9).
3. Build page F (https://claude.ai/artifact/AtxBFvjTzngZuMV9Z5k87u): F1 settings (HUD Widget Class done and committed; Telemetry Source WebSocket, GameMode Override
   `TwinOpsGameMode` on `L_TwinStudio`, MPC on `BP_VehicleTwin`), F2 PIE against `python Tools/Relay/relay.py --source live --loop --rate 20`,
   F3 frame the four cameras and judge the lights on the paint.
4. Rule while switching laptops: `.uasset`/`.umap` can't merge; never edit the same asset on both before push + pull.

**Unverified:** tests M1–M9 not run on either machine; no widget seen running in PIE (HUD, callouts, camera blends). Callout anchor places
not checked on the car. Still open: UMG float font sizes, BackgroundBlur Corner Radius, Custom node LWC warning; light and camera values not
judged with the car. Older: SPEC §1 25 `Paint` meshes vs 8 in the BP; Day 4 `CarRoot` facing warning; doubled outline with editor preview at
PIE start; Day 10 "can't connect: no details" wording.

**Open decisions:** showroom mode (after H1); GrabCAD license / mesh
redistribution (ASSETS.md); private asset repo for `Content/Jeep/`; Day 8 LinkedIn clip; BUG-002 parked. Pending: add T1 content to the
Vehicle Twin Field Guide; clean up unused font faces and duplicate `.ttf` folders in `/Game/Art/UI/Fonts/` (4 faces used); build page fixes:
B3 label is `Uncompressed (RGBA8)` in 5.7, asset folders on the page (`/Game/UI/...`) differ from the project (`/Game/Art/UI/`, `/Game/Blueprints/UI/`).

**Not in git (local only, personal laptop):** `Content/Jeep/` (`ImportA`, `Cleaned`, `Blueprints/BP_VehicleTwin` with the callout anchors,
`Rebuilt/`), `Content/Maps/` levels except `L_TwinStudio`, raw CAD in `RawCAD/`.

## Checkpoint (2026-10-13, personal laptop, RTX 3070 Ti Laptop, host `VigneshSuresh`)

Commit: `91b533b` on `main`, pushed. Working tree: editor-changed Config `.ini` files and `CarDigitalTwins.uproject.DotSettings.user`
kept out of commits; the other `Content/Maps/` levels untracked on purpose.

**Where I stopped:** dashboard WBPs D1–D10 and E1–E5 done and pushed in `91b533b` (editor built here with the new C++ parents).
B done: fonts `F_Barlow` / `F_BarlowCondensed` and the 7 icons, at `/Game/Art/UI/` (not `/Game/UI/`); WBPs at `/Game/Blueprints/UI/`
(parts in `Dashboard/Parts/`). A6 car placed in `L_TwinStudio`; A7 three callout anchors added to `BP_VehicleTwin` by
`Tools/UnrealEditor/create_callout_anchors.py`. Next on the build page: E6.

**Resume (office PC):**
1. `git pull` (LFS fetches fonts and WBPs). Editor closed, build `CarDigitalTwinsEditor` (first office build after the log fix), then
   Session Frontend › Automation `CarDigitalTwins.MVVM` → expect 9 green (M1–M9). `L_TwinStudio` will warn about missing
   `BP_VehicleTwin`: expected.
2. Build page E6–E11 (PowertrainPanel, TyresPanel, BodyPanel, SidePanel, CarCalloutLayer, TwinOpsHUD):
   https://claude.ai/artifact/AtxBFvjTzngZuMV9Z5k87u. Panel frame = Background Blur › Border per panel (E0), no separate widget.
   Commit per part, **push before going home**.
3. Personal laptop: F (settings, play against the relay), frame the four cameras by eye (F3), judge the lights on the paint.
4. Rule while switching laptops: `.uasset`/`.umap` can't merge; never edit the same asset on both before push + pull.

**Unverified:** tests M1–M9 not run on either machine; no widget seen running in PIE. Callout anchor places not checked on the car.
Still open: UMG float font sizes, BackgroundBlur Corner Radius, Custom node LWC warning; light and camera values not judged with the car.
Older: SPEC §1 25 `Paint` meshes vs 8 in the BP; Day 4 `CarRoot` facing warning; doubled outline with editor preview at PIE
start; Day 10 "can't connect: no details" wording.

**Open decisions:** showroom mode (after H1); GrabCAD license / mesh redistribution (ASSETS.md); private asset repo for
`Content/Jeep/`; Day 8 LinkedIn clip; BUG-002 parked. Pending: add T1 content to the Vehicle Twin Field Guide; clean up the unused
font faces and duplicate `.ttf` folders in `/Game/Art/UI/Fonts/` (only 4 faces are used); build page fixes: B3 label is
`Uncompressed (RGBA8)` in 5.7, asset folders differ from the page.

**Not in git (local only, personal laptop):** `Content/Jeep/` (`ImportA`, `Cleaned`, `Blueprints/BP_VehicleTwin` now with the callout
anchors, `Rebuilt/`), `Content/Maps/` levels except `L_TwinStudio`, raw CAD in `RawCAD/`.

## Checkpoint (2026-10-12, office laptop `DESKTOP-MRMGNPL`, RTX 4060 Ti)

Commit: `493f78f` on `main`, pushed (`5ee045f` is the build fix before it). Working tree: clean apart from editor-changed Config
files, kept out of commits; the other `Content/Maps/` levels untracked on purpose.

**Where I stopped:** Part A done except the car-dependent steps, in `L_TwinStudio` (in git). A3 `M_StudioFloor` made by script.
A4 floor placed (its Z was off, fixed by hand). A5 lights + unbound manual-exposure volume by
`Tools/UnrealEditor/create_studio_lighting.py` (look checked on the empty floor only). A8 four tagged cameras by
`Tools/UnrealEditor/create_studio_cameras.py` (framing not judged without the car). Both scripts update by label: change the constants
at the top and re-run. Build fix: `LogVehicleTelemetry` was defined in three `.cpp` files and collided in the unity build; now one
`TelemetryLog.h` and a single define in `TelemetrySubsystem.cpp`.

**Resume:**
1. Part C with the editor closed: build `CarDigitalTwinsEditor` (a build after the log fix is not confirmed yet); open the editor,
   Session Frontend › Automation `CarDigitalTwins.MVVM` → expect 9 green (M1–M9). Part D's parent classes only exist after this build.
2. Parts B, D, E of the build page (fonts, icons, the WBPs): https://claude.ai/artifact/AtxBFvjTzngZuMV9Z5k87u. Commit per part.
   **Push before going home.**
3. Personal laptop (needs `BP_VehicleTwin`, `Content/Jeep/`, gitignored): A6 place the car, A7 callout anchors, then F; frame the four
   cameras by eye (F3) and judge the lights on the paint (tune `create_studio_lighting.py`, re-run).
4. Rule while switching laptops: `.uasset`/`.umap` can't merge; never edit the same asset on both before push + pull. Once the car
   is placed in `L_TwinStudio` (A6, personal laptop), the office copy will show a missing-asset reference: expected.

**Unverified:** dashboard C++ compiled before the log fix, but tests M1–M9 and every widget have not run; still open:
UMG float font sizes, BackgroundBlur Corner Radius, Custom node LWC warning. Light and camera values are calculated, not seen with the car.
Older: SPEC §1 25 `Paint` meshes vs 8 in the BP; Day 4 `CarRoot` facing warning; doubled outline with editor preview at PIE
start; Day 10 "can't connect: no details" wording.

**Open decisions:** showroom mode (after H1); GrabCAD license / mesh redistribution (ASSETS.md); private asset repo for
`Content/Jeep/`; Day 8 LinkedIn clip; BUG-002 parked. Pending: add T1 content to the Vehicle Twin Field Guide.

**Not in git (local only, personal laptop):** `Content/Jeep/` (`ImportA`, `Cleaned`, `Blueprints/BP_VehicleTwin`, `Rebuilt/`),
`Content/Maps/` levels except `L_TwinStudio`, raw CAD in `RawCAD/`.

## Checkpoint (2026-10-12, personal laptop, RTX 3070 Ti Laptop, host `VigneshSuresh`)

Commit: the handoff commit `7b8b536` on `main`, pushed. Working tree: Config `DefaultGame.ini` (source = WebSocket) and
`DefaultEngine.ini` changed by the editor, kept out of commits; the other `Content/Maps/` levels untracked on purpose.

**Where I stopped:** handing off to the office laptop. Part A1 done: `L_TwinStudio` created, empty, now **in git** (first level in
git; the other maps stay local). A2 done: `MPC_TwinStudio` (scalar `GroundScrollCm` = 0) at `/Game/Twin/Studio/Materials/` (a
`Materials` subfolder, unlike the build page). A3 not done: `Tools/UnrealEditor/create_studio_floor_material.py` written, not run.

**Resume (office laptop):** git pull, Part C build + M1–M9, A3 script, Parts B, D, E; A4–A8 and F need `BP_VehicleTwin`.

**Unverified:** floor material script never run; dashboard C++ compiles but nothing has run. Part A camera/light values calculated,
not seen.

## Checkpoint (2026-10-10, personal laptop, RTX 3070 Ti Laptop, host `VigneshSuresh`)

Commit: `24a1503` on `main` (+ this checkpoint fix), pushed. Working tree: clean except Config `DefaultGame.ini` (source =
WebSocket) and `DefaultEngine.ini`, changed by the editor and kept out of commits; `Content/Maps/` untracked on purpose.

**Where I stopped:** T1 done end to end (`33e884c`, PIE checked), T2 moved to optional (`a8d8f7d`); both pushed. Day 11 steps 2–5
committed (`6c65e00`..`861ee81`). Dashboard designed and decided (D1–D7, dark studio level), mock and build page published; all C++
for steps 6–8 and the dashboard written; it builds (one fix: `Units = "px"` removed). Starting Parts A–F next session.

**Resume:**
1. Build page Parts A + B with the editor open (studio level `L_TwinStudio`, MPC + floor material, lights, 4 tagged cameras, 3
   anchors in `BP_VehicleTwin`, Barlow fonts, icons): https://claude.ai/artifact/AtxBFvjTzngZuMV9Z5k87u
2. Part C is built and committed: open the editor, Session Frontend `CarDigitalTwins.MVVM` → expect 9 green (M1–M9, not run yet).
3. Parts D–F: the 21 WBPs, settings, PIE vs `relay.py --source live --loop --rate 20`; commit per part, push. Mock for comparison:
   https://claude.ai/artifact/TsApXH75XFbcd2muKzLx3m

**Unverified:** dashboard C++ compiles but nothing has run (tests M1–M9, any widget); still open: UMG float font sizes,
BackgroundBlur Corner Radius, Custom node LWC warning. Camera/light start values in Part A are calculated, not seen. Older: SPEC §1 25 `Paint` meshes vs 8 in the BP; Day 4
`CarRoot` facing warning; doubled outline with editor preview at PIE start; Day 10 "can't connect: no details" wording.

**Open decisions:** showroom mode (after H1); GrabCAD license / mesh redistribution (ASSETS.md); private asset repo for
`Content/Jeep/`; Day 8 LinkedIn clip; BUG-002 parked. Pending: add T1 content to the Vehicle Twin Field Guide (links to the dashboard pages added).

**Not in git (local only, personal laptop):** `Content/Jeep/` (`ImportA`, `Cleaned`, `Blueprints/BP_VehicleTwin`, `Rebuilt/`),
`Content/Maps/` levels (the new `L_TwinStudio` will live there too), raw CAD in `RawCAD/`. `mvvm-reference` is now on origin.

## Checkpoint (2026-10-09, personal laptop, RTX 3070 Ti Laptop, host `VigneshSuresh`)

Commit: `bc69473` on `main`, pushed. Working tree: T1 steps 1–2 uncommitted (`docs/SPEC.md`, `docs/TESTS.md`,
`Tools/TripGenerator/trip_generator.py`, its tests, regenerated `Data/Trips/trip_sample.json`); `Content/Maps/` untracked on purpose.

**Where I stopped:** Day 9 (`9854b4a`) and Day 10 (`bc69473`) done and pushed. T1 (engine derate request, option A) steps 1–2 done:
SPEC §2.1 `driveMode`, §2.5 `command` / `commandAck`; `VehicleSimulator` class with `set_engine_derate()` (rpm 2500, 50 km/h, coolant
under 112 °C in ~14 s); sample differs only by `"driveMode": "Normal"`; pytest 51 pass (P4 case + P9). Step 3 (relay) not started.

**Resume:**
1. T1 step 3, `Tools/Relay/relay.py`: `--source live|file` (live runs `VehicleSimulator` in real time, new simulator on `--loop`);
   `handle_client_message` applies `engineDerate`, `commandAck` to the sender only, file mode rejects "source is a recorded trip";
   `relay_client.py --send-derate-after SECONDS`; tests R7–R10; update SPEC §2.3 relay bullet + TESTS §4. Expect all pytest green.
2. T1 steps 4–5 (C++): `EVehicleDriveMode` + `DriveMode` in `FVehicleTelemetry`; `FVehicleCommand`,
   `ITelemetryReceiver::SendVehicleCommand()` (file receiver refuses), parse `commandAck`; subsystem pending ack (2 s timeout),
   auto derate on first Critical coolant (`bAutoEngineDerateOnCriticalCoolant`), console `Twin.EngineDerate 1/0`. User builds + PIE:
   red ~119 s → command → ack applied → `EngineDerate` → `Critical -> Warning`.
3. Then tick T1, update the "Vehicle Twin Field Guide" artifact (scratchpad file, same path keeps the URL), commit + push; T2, T3, Day 11.

**Unverified:** SPEC §1 says 25 `Paint` meshes, BP has 8 (check the `group Paint` PIE log line). Day 4 `CarRoot` facing warning not
looked at. Doubled outline with the editor preview ticked at PIE start, not tested. Day 10 "can't connect: no details" wording.

**Open decisions:** UI design track questions; showroom mode (after H1); mesh redistribution / GrabCAD license (ASSETS.md); private
asset repo for `Content/Jeep/` (deferred). Still to do: Day 8 LinkedIn clip (car not selected). BUG-002 parked (`docs/BUGS.md`).

**Not in git (local only, personal laptop):** `Content/Jeep/` (`ImportA`, `Cleaned` 1128 split meshes, `Blueprints/BP_VehicleTwin`,
`Rebuilt/`), `Content/Maps/` levels, raw CAD in `RawCAD/`. MVVM reference branch `mvvm-reference` only on the office laptop.

## Checkpoint (2026-10-08, personal laptop, RTX 3070 Ti Laptop, host `VigneshSuresh`)

Commit: `929f8d7` on `main`, pushed. Working tree: `Content/Maps/` (untracked on purpose) and the uncommitted car rebuild scripts in
`Tools/UnrealEditor/` (`export_vehicle_twin_recipe.py`, `rebuild_vehicle_twin_from_cad.py`, `vehicle_twin_recipe.json`).

**Where I stopped:** Day 8 done and pushed. Side task started: rebuild the car from the STEP file plus a recipe exported from the
hand-built `BP_VehicleTwin`, so `Content/Jeep/` can be recreated. Export works (113 components, 72 meshes; tags `Paint` 8, `Trim` 22,
`Tyre` 4, one each `Wheel.*`). Rebuild ran: `/Game/Jeep/Rebuilt/ImportA` + `BP_VehicleTwin_Rebuilt`, looks mostly the same, but
**Preview Status Outline doesn't show on the rebuilt car** although its Status Outline Material is set.

**Resume:**
1. ~~Rebuilt car outline~~ parked 2026-10-08 as BUG-002 (low priority experiment); scripts + recipe committed.
2. Day 9: `relay.py` (`--rate`, `--loop`, `--drop-percent`, `--pause-after`), verify with a CLI client.
3. Record the Day 8 LinkedIn clip (car not selected: the editor selection outline is yellow too).

**Unverified:** SPEC.md §1 says 25 `Paint` meshes, the BP has 8 `Paint` components: check the original car's `group Paint` PIE log
line and fix SPEC. Day 4 Play log (`CarRoot` facing warning) not looked
at. Outline with the editor preview left ticked at PIE start: possible doubled outline, not tested.

**Open decisions:** UI design track questions (ops view placement, show T1/T2 in the first design). Showroom mode (decide after H1).
Redistribution of the imported meshes / GrabCAD license (ASSETS.md). Private asset repo for `Content/Jeep/` still pending (the rebuild
scripts don't cover the 1128 hand-split door meshes).

**Not in git (local only, personal laptop):** `Content/Jeep/` (`ImportA`, `Cleaned` with the 1128 split meshes for H2 door animation,
`Blueprints/BP_VehicleTwin`, new `Rebuilt/` with `BP_VehicleTwin_Rebuilt`), `Content/Maps/L_VehicleTwin.umap`, `ImportTestMap.umap`,
the side-by-side test level if saved, raw CAD in `RawCAD/`. Leftovers safe to delete: empty `Content/Maps/_GENERATED/vigne/`, empty
root `Jeep/Cleaned/`. MVVM reference branch `mvvm-reference` exists only on the office laptop.

## Checkpoint (2026-10-08, personal laptop, RTX 3070 Ti Laptop, host `VigneshSuresh`)

Commit: `1f8d9b0` on `main`, 2 commits not pushed (plus this checkpoint). Working tree clean except `Content/Maps/` (untracked on purpose).

**Where I stopped:** Day 8 done. Step 2: `UTelemetrySubsystem` evaluates every sample, `GetCurrentVehicleStatusReport()` (`1406469`).
Step 3: `AVehicleTwinActor` follows telemetry (previous → latest blend, loop snaps) and shows status as a pulsing amber/red outline
(custom stencil + `PP_VehicleStatusOutline`, made by `Tools/UnrealEditor/create_status_outline_material.py`) (`1f8d9b0`). Both
verified in PIE at 10×: Warning 91 s, Critical 119 s.

**Resume:**
1. Push `main` (2 commits + checkpoint).
2. Record the Day 8 demo clip for LinkedIn: PIE at 10×, car **not selected** (the editor selection outline is yellow too), wheels
   rolling → amber outline (~91 s) → fast red pulse (~119 s), Output Log `Vehicle status:` line in shot.
3. Day 9: `Tools/` relay `relay.py` (`--rate`, `--loop`, `--drop-percent`, `--pause-after`) streaming SPEC.md §2.3 messages;
   verify with a CLI client.
4. Private asset repo for `Content/Jeep/` (Pending setup): `BP_VehicleTwin` (now holding the outline material assignment) exists
   only on this laptop.

**Unverified:** Day 4 Play log: per-group log lines and the `CarRoot` facing warning (`LogVehicleTwin`) still not looked at.
Editor preview of the outline (`bPreviewStatusOutline`) left ticked when starting PIE: possible doubled outline, not tested.

**Open decisions:** UI design track questions (ops view placement, show T1/T2 in the first design). Showroom mode (decide after H1).
Redistribution of the imported meshes / GrabCAD license (ASSETS.md).

**Not in git (local only, personal laptop):** `Content/Jeep/` (`ImportA`, `Cleaned` with the 1128 split meshes for H2 door animation,
`Blueprints/BP_VehicleTwin`), `Content/Maps/L_VehicleTwin.umap`, `ImportTestMap.umap`, raw CAD in `RawCAD/`. `M_GlossyTest` is no
longer in `Content/` (gone since the last checkpoint). Leftovers safe to delete: empty `Content/Maps/_GENERATED/vigne/`, empty root
`Jeep/Cleaned/`. MVVM reference branch `mvvm-reference` exists only on the office laptop.

## Checkpoint (2026-10-07, personal laptop, RTX 3070 Ti Laptop, host `VigneshSuresh`)

Commit: the checkpoint commit after `ed324e1` on `main`, pushed. Working tree clean except `Content/Maps/` (untracked on purpose).
Day 8 step 1 (`Telemetry/VehicleStatusEvaluator.h/.cpp`, SPEC.md §3 Critical clear values) is committed but **not built yet**.

**Where I stopped:** Day 7 committed (`cfdd9f0`). Roadmap changed to one backend, two front ends: digital twin first (Twin
completion T1–T3 after Day 10), then in-car HMI (Phase H) (`ed324e1`). UI design track added for a separate session (`612d3e4`).
Day 8 step 1 written: `FVehicleStatusEvaluator` (per-signal status + overall, hysteresis per SPEC.md §3), not built yet.

**Resume:**
1. Day 8 step 2: in `UTelemetrySubsystem`, run `FVehicleStatusEvaluator` on every new sample, keep the latest
   `FVehicleStatusReport`, add `GetCurrentVehicleStatusReport()`, log one line when the overall status changes.
2. Close the editor and build (new USTRUCT, Live Coding won't do). PIE at 10×: status log goes Warning ~91 s, Critical ~119 s.
3. Day 8 step 3+: car subscribes in `BeginPlay`/`EndPlay`, lerps previous → latest (loop = snap) into `UpdateWheels`, polls status for
   `SetStatusColor` with warning/critical colour properties (pick a Critical colour that shows on the red body paint).
4. Private asset repo for `Content/Jeep/` (Pending setup): all Jeep assets incl. `BP_VehicleTwin` exist only on this laptop.

**Unverified:** `VehicleStatusEvaluator` not compiled yet. Day 4 Play log: per-group log lines and the `CarRoot` facing warning
(`LogVehicleTwin`) still not looked at.

**Open decisions:** UI design track questions (ops view placement, show T1/T2 in the first design). Showroom mode (decide after H1).
Redistribution of the imported meshes / GrabCAD license (ASSETS.md).

**Not in git (local only, personal laptop):** `Content/Jeep/` (`ImportA`, `Cleaned` with the 1128 split meshes for H2 door animation,
`Blueprints/BP_VehicleTwin`), `Content/Maps/L_VehicleTwin.umap`, `ImportTestMap.umap`, raw CAD in `RawCAD/`, `M_GlossyTest` in
`Content/CADImports/Materials/`. Leftovers safe to delete: empty `Content/Maps/_GENERATED/vigne/`, empty root `Jeep/Cleaned/`.
MVVM reference branch `mvvm-reference` exists only on the office laptop.

## Checkpoint (2026-10-07, personal laptop, RTX 3070 Ti Laptop, host `VigneshSuresh`)

Commit: `e9abe2f` on `main`, pushed. Working tree: Day 7 not committed yet (7 new files in `Source/CarDigitalTwins/Telemetry/`,
`CarDigitalTwins.Build.cs`, this file, `Config/DefaultGame.ini`); `Content/Maps/` untracked on purpose.

**Where I stopped:** Day 7 done and checked in PIE (10 samples/s, first-frame values, 10× loop with `seq` wrap, incidents,
pause/stop), not committed. Testing track runs in a separate session: trip generator suite + BUG-001 fix committed (`e9abe2f`).
Old checkpoints moved to [CHECKPOINTS.md](CHECKPOINTS.md).

**Resume:**
1. Project Settings > Vehicle Telemetry: set `PlaybackSpeedMultiplier` back to 1 (`Config/DefaultGame.ini` still says 10 from the
   loop test). Then commit Day 7 ("Complete Day 7: …") and push; the testing session's U1–U9 need it committed.
2. Day 8: bind telemetry to `AVehicleTwinActor`: subscribe to `UTelemetrySubsystem::OnTelemetryUpdated`, interpolate previous →
   latest sample (loop = jump), wheel spin from `SpeedKmh`, status colour from SPEC.md §3 with hysteresis. Done when the
   overheating incident turns the car amber → red. Start with `/prime telemetry binding`.
3. Private asset repo for `Content/Jeep/` (Pending setup): all Jeep assets incl. `BP_VehicleTwin` exist only on this laptop.

**Unverified:** Day 4 Play log: per-group log lines and the `CarRoot` facing warning (`LogVehicleTwin`) still not looked at. Check on
the next Play.

**Open decisions:** Showroom mode (Optional / later). Redistribution of the imported meshes / GrabCAD license (ASSETS.md).

**Not in git (local only, personal laptop):** `Content/Jeep/` (`ImportA`, `Cleaned` with the 1128 split meshes for the optional
door animation, `Blueprints/BP_VehicleTwin`), `Content/Maps/L_VehicleTwin.umap`, `ImportTestMap.umap`, raw CAD in `RawCAD/`,
`M_GlossyTest` in `Content/CADImports/Materials/`. Leftovers safe to delete: empty `Content/Maps/_GENERATED/vigne/`, empty root `Jeep/Cleaned/`.
MVVM reference branch `mvvm-reference` exists only on the office laptop.

## Checkpoint (2026-10-05, personal laptop, RTX 3070 Ti Laptop)

**Where I stopped:** Day 6 done (`e482163`, pushed): `Tools/TripGenerator/trip_generator.py` + `Data/Trips/trip_sample.json`
(190 s, 10 Hz, overheating + RR slow puncture, hood opens). Day 4 done before it (`e7db864`). Next: Day 7 file receiver.
Discussed the final output and a configurator ("Showroom mode") idea, not decided yet (see Optional / later).

Summary of Day 4 (details in SPEC.md §1 and LEARNING_LOG):
- Option A: car built from the untouched Datasmith import; `BP_VehicleTwin` harvested from `ImportA`, parts are child actors.
- `AVehicleTwinActor`: tag lookup through child actors, wheel spin/steer about the measured wheel centre, paint groups with status colour,
  editor previews (wheels, status colour).
- Not checked in a Play log after the last build: per-group log lines and the `CarRoot` facing warning. Check on the next Play.

**Resume:**
1. Day 7: `ITelemetryReceiver`, `UFileTelemetryReceiver` reading `Data/Trips/trip_sample.json` (SPEC.md §2.2), `UTelemetrySubsystem`; PIE logs 10 frames/s, looping. Day 6 (trip generator) done.
2. Private asset repo for `Content/Jeep/` (see Pending setup): `BP_VehicleTwin` now lives there too and exists only on this laptop.

**Not in git (local only):** all of `Content/Jeep/` (incl. `BP_VehicleTwin`), `L_VehicleTwin`, `ImportTestMap`.

## Checkpoint (2026-09-27, personal laptop, RTX 3070 Ti Laptop)

**Where I stopped:** Day 4 in progress, mid-way through steps 1–2.

**Recovered 2026-10-03:** the meshes below were never saved to disk (Modeling Mode wrote them to `/Game/Maps/_GENERATED/vigne/`,
only the level was saved). Restored from editor autosaves of 2026-09-27 21:26; see LEARNING_LOG "Unsaved Modeling Mode meshes".

Done so far (Modeling Mode default names, currently in `/Game/Maps/_GENERATED/vigne/` → to be moved to `/Game/Jeep/Cleaned`):
- `Door_FL`, `Door_FR` — PolyGroup split → Merge done, pivot set on hinge edge (front edge, vertical axis).
- `Hood` — split/merge done, pivot set on rear edge (near cowl, horizontal axis).
- `Tailgate` — split/merge done, pivot set on hinge-side edge (vertical axis); spare wheel kept as a separate mesh, not merged into the tailgate.
- `Windows` — split and merged into per-door glass (`Glass_FL`/`Glass_FR`, rear doors still pending) plus one combined `Glass_Static` for windshield/rear/fixed panes. No pivot needed for glass.
- Only **4** merged meshes exist (`Combined_04D78D0B`, `_17A9DBE6`, `_44391FCA`, `_FFA1BC57`), not one per part above; the rest are
  still loose `Split1_*` (body) / `Shell1_*` (glass) pieces in the level. Identify which part each `Combined_*` is during the rename step.

Not done yet:
- `Door_RL`, `Door_RR` — rear doors not split/merged/pivoted at all.
- Road wheel pivots (hub centre, all 4) and spare wheel pivot check (leave as imported).
- Step 3 (`<Part>_<Position>` renaming + component tags) — not started for anything above; current asset names are still the Modeling Mode defaults.

**Local-only state (not in git, only on the personal laptop):**
- Raw CAD: `RawCAD/2010 Jeep Wrangler Rubicon/Imported CAD/2010 Jeep Wrangler Rubicon - Assembly.STEP`
- Chosen import (tessellation A): `Content/Jeep/ImportA/` (ignored). Car B import deleted.
- Cleaned/split meshes so far: 1128 assets in `Content/Maps/_GENERATED/vigne/` (**not** ignored: don't commit). Move them in the
  Content Browser to `/Game/Jeep/Cleaned` (ignored) + Fix Up Redirectors, then set Modeling Mode's asset location to `/Game/Jeep/Cleaned`.
- Root `Jeep/Cleaned/` folder is an empty leftover from a 2026-09-26 save to a disk path; safe to delete.
- Test material `M_GlossyTest` in `Content/CADImports/Materials/` (ignored).
- `Content/Maps/ImportTestMap.umap` holds car A; left uncommitted on purpose (scratch level that references ignored meshes).
  Plan: commit a proper `Maps/L_VehicleTwin` once `BP_VehicleTwin` is placed.
- The office PC has an older import; re-download the model there if needed (see ASSETS.md).

**Superseded 2026-10-04 by the option A scope cut (see Day 4 below); kept for the optional door animation.**

**Resume with Day 4, in this order:**
0. Move the recovered meshes to `/Game/Jeep/Cleaned` (above) and set up the private asset repo (Pending setup) so they exist in two places.
1. `Door_RL`, `Door_RR`: PolyGroup split → Merge, then Edit Pivot on the hinge edge (same as the front doors).
2. Road wheel pivots to hub centre (all 4); confirm spare wheel pivot is untouched.
3. Rename everything done so far to `<Part>_<Position>` + add component tags.
4. `AVehicleTwinActor` C++ + `BP_VehicleTwin` (fix flipped orientation here).
5. Body paint MI with `StatusColor`. 6. Mapping table in SPEC.md §1.

**Open:** redistribution of imported meshes (ASSETS.md); optional Day 3 wireframe screenshots.
