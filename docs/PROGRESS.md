# Progress: Vehicle Digital Twin (Roadmap v2)

Last updated: 2026-10-30
Current day: **Wrap-up** (decided 2026-10-30). Items 1 and 3–6 done and pushed 2026-10-30; item 7 changed to a zip backup. Left, all on the personal
laptop (from 2026-11-04): the backup, item 2 (build page F, the dashboard on the car), then the demo recording and the README GIF. Days 1–10
complete; Day 11 build page A–E done; Day 13 Pixel Streaming runs locally.

**Wrap-up (decided 2026-10-30): ship as-is.** The project is a **vehicle telemetry twin with a remote-operations dashboard**. Omniverse /
OpenUSD is a separate future project. Everything not in the list below is under "Follow-ons (not built)" at the end of this file.

| # | Item | Done when |
|---|------|-----------|
| 1 | Commit all open work (done 2026-10-30) | V1–V10 tests, ring-target fix, blend-alpha refactor, `Tools/PixelStreaming/` on `main`; doc fixes |
| 2 | Build page F (personal laptop, from 2026-11-04) | Dashboard runs in PIE against `relay.py --source live`; part rings and callouts correct on the car; four cameras framed; lights retuned |
| 3 | Status evaluator tests (done 2026-10-30: S1–S11 pass) | SPEC §3 thresholds, enter/clear hysteresis and worst-of covered by UE automation tests, all passing |
| 4 | Measured numbers (2026-10-30: PERFORMANCE.md; car figures from Day 3, re-measure with the dashboard in item 2) | Frame time, end-to-end latency (generator → dashboard), triangles / draw calls, test count in a short PERFORMANCE.md, measured figures only |
| 5 | Demo video, 60–90 s (script done 2026-10-30: DEMO_SCRIPT.md; recording after item 2) | Overheating Warning → Critical; connection drop and recovery; derate command round trip visible on the dashboard |
| 6 | README (done 2026-10-30 except the GIF, which comes from the recording after item 2) | GIF, one-line description, architecture diagram, 3–4 measured figures, how to run; Decisions, tests, credits, known issues; no TODOs |
| 7 | Asset safety (changed 2026-10-30: no private asset repo, a dated zip backup instead; personal laptop, 2026-11-04, before anything else) | Dated zip of `Content/Jeep/` (incl. `BP_VehicleTwin`) and the car levels on an external drive or cloud storage; refreshed after item 2 |

Unverified items to check in item 2 or list as known issues in the README: `UViewModelSubsystem` engine tick in PIE; camera blends;
studio lighting; `CarRoot` facing warning; doubled outline at PIE start; BUG-002 (open, parked).

**Earlier direction (2026-10-07, option 3, superseded by the wrap-up):** one backend, two front ends: the remote ops view first, then an
in-car HMI view (Phase H) on the same core. Phase H is now a follow-on.

## Checkpoint (2026-10-30, office laptop `DESKTOP-MRMGNPL`, RTX 4060 Ti)

Commits `231fed4` … `01b5353` on `main`, plus this checkpoint commit, all pushed. Working tree: local `DefaultGame.ini`
(`TelemetrySource=WebSocket`; committed default stays File) kept out of commits; `docs/review/` (progress review PDF) untracked.

**Done today:** wrap-up plan written into this file (seven items, follow-ons). Item 1: blend-alpha refactor, V1–V10, part-ring signal
and anchor set from C++, Pixel Streaming notes, doc fixes, startup map `L_TwinStudio`. Item 3: status evaluator tests S1–S11. Audit
fixes: editor scripts check every save and print MANUAL lines; relay ignores a non-UTF-8 binary frame (R11). Item 4: PERFORMANCE.md
(5.0 ms median frame without the car, 2.9 ms latency, 101 tests: 41 UE + 60 Python, all pass). Item 5: DEMO_SCRIPT.md. Item 6: README
rewritten, no TODOs left in README or docs. Item 7: no private asset repo; after the outside review, a dated zip backup on 2026-11-04 instead.

**Resume (personal laptop, from 2026-11-04):**
0. **Back up the car first:** zip `Content/Jeep/` (incl. `BP_VehicleTwin`) and `Content/Maps/L_VehicleTwin*`, `ImportTestMap*` into
   `CarDigitalTwins-Jeep-2026-11-04.zip` on an external drive or cloud storage (editor closed). Refresh it after item 2 (wrap-up item 7).
1. `git stash` the local `DefaultEngine.ini` (`EditorStartupMap=L_VehicleTwin`) and `DefaultGame.ini`, pull, then `git stash pop` and
   keep only `TelemetrySource=WebSocket` (the committed startup map is now `L_TwinStudio`). Build; headless `CarDigitalTwins`: 41 pass.
2. Item 2, build page F (https://claude.ai/artifact/AtxBFvjTzngZuMV9Z5k87u): F1 MPC on `BP_VehicleTwin`, F2 PIE against
   `relay.py --source live --loop --rate 20`, F3 frame the four cameras + retune the lights. Check on the car: rings round and on the
   hood / RR wheel (`b980e77` sets their signal and anchor from C++), callouts placed, Critical banner reads "Engine derate requested
   automatically", Disconnected → retry → Live when the relay restarts.
3. Re-measure frame time with the car (PERFORMANCE.md "How these were measured"; no `-csvGpuStats` on the command line).
4. Record the demo (DEMO_SCRIPT.md, dry run first), cut the README GIF into `docs/media/`, link it in the README in place of the
   GIF comment. `*.gif` is in Git LFS (`.gitattributes`): check that GitHub shows it in the README.
5. Move each unverified item below that checks out from the README "Known issues" to done.
6. Rule while switching laptops: `.uasset`/`.umap` can't merge; never edit the same asset on both before push + pull.

**Unverified (README "Known issues"):** `UViewModelSubsystem` engine tick in PIE; camera blends; studio lighting; `CarRoot` facing
warning; doubled outline at PIE start; BUG-002 (open, parked). Also not yet seen on the car: `WBP_PartRing` brush, callout collapse.

**Not in the wrap-up (audit follow-ups, left as they are):** `relay_client` Ctrl+C summary; U9 own setup + `wss://`; trip generator test
not importing `conftest`; refactor step (b) (cached anchors, widget refreshes gated on the field mask, callout outline fade).

**Not in git (local only, personal laptop):** `Content/Jeep/` (incl. `BP_VehicleTwin` with callout anchors), `Content/Maps/` levels except
`L_TwinStudio`, raw CAD in `RawCAD/`. The office laptop has no car.

## Phase A: Single-car digital twin

### Day 1: Foundation
- [x] UE 5.7 C++ project (`CarDigitalTwins`) compiles and opens
- [x] Plugins enabled: DatasmithCADImporter, PixelStreaming2, ModelViewViewModel (kept for comparison, see SPEC.md §UI architecture)
- [x] Git repo initialised with Unreal `.gitignore` and LFS for `*.uasset` / `*.umap`
- [x] `docs/` created: README, SPEC, LEARNING_LOG, COSTS, ASSETS, PROGRESS
- [x] First commit pushed to a remote (github.com/vicky2315/CarDigitalTwins)
- [x] COSTS.md: two monthly estimates (2 h/day and always-on)
- [x] Check-your-understanding questions answered

### Day 2: Source the CAD model
- [x] Shortlist 2–3 CAD files (link, format, size/parts, license, separate wheels/doors)
- [x] Review shortlist
- [x] One file chosen (2010 Jeep Wrangler Rubicon); license and source recorded in ASSETS.md
- [x] Test import with Datasmith defaults (structure checked: doors/hood/tailgate share one body)

### Day 3: Datasmith import and tessellation
- [x] Two tessellation settings compared (triangle counts, visuals)
- [x] Triangle budget set and justified (setting A, ~1.4 M tris drawn/frame; see SPEC.md §7)
- [x] Settings and counts logged in LEARNING_LOG (screenshots not taken)

### Day 4: Cleanup, pivots and USD-ready naming
**Scope cut (option A, 2026-10-04):** CAD splitting/merging is technical-artist work, not the programming focus. The car is built from
the untouched Datasmith import (`ImportA`); doors/hood/tailgate don't animate, their `openings` state is shown on the dashboard only.
The split front doors etc. stay in `/Game/Jeep/Cleaned` for the optional door animation later.
- [x] `BP_VehicleTwin`: Harvest Components from a fresh `ImportA` placement in `L_VehicleTwin`, all components Movable,
      `CarRoot` (rotation X −90, Z 0), `WheelHub_FL/FR/RL/RR` with `Rim_*` / `Tyre_*` (`Combine3` / `Combine4`), spare unhubbed
- [x] Component tags (`Wheel.*`, `Tyre`, `Paint`) set and saved
- [x] `AVehicleTwinActor` C++ base (tag lookup, hub auto-centring, spin/steer, `StatusColor`, editor preview), BP reparented
- [x] Wheel spin axis correct in the editor preview (2026-10-04: axle from left/right wheel centres, spin about the wheel centre
      without moving the meshes; `CarRoot` yaw 90 so the car faces +X)
- [x] Left-side `Rim`/`Tyre` names and `Tyre` tag swapped back (all four wheels radius 43.5 cm)
- [x] Wheel pivots: solved in C++ (pivot = tyre bounds centre in hub space, hubs stay where authored); shared wheel meshes untouched
- [x] Body paint MI with `StatusColor` (`M_CarPaint` / `MI_CarPaint`, 8 `Paint` components, `StatusBlend` + editor preview;
      count from `vehicle_twin_recipe.json`, see SPEC.md §1)
- [x] `Trim` paint group (black parts) built; `MI_CarPaintBlack` reparented to `M_CarPaint`, duplicate master deleted
- [x] Mapping table in SPEC.md (§1, incl. paint groups and materials)
- [x] `BP_VehicleTwin` moved to `/Game/Jeep/Blueprints` (local only)
- [ ] Door / hood / tailgate animation using the split meshes: scheduled as Phase H, H2 (pivots and part list: CHECKPOINTS.md, 2026-09-27)

### Day 5: Telemetry schema
- [x] Fields with units, trip.json format, stream message format, status thresholds in SPEC.md
- [x] `FVehicleTelemetry` compiles (office laptop)

### Day 6: Trip generator (Python)
- [x] `trip_generator.py` with phases, fixed seed, incidents (`Tools/TripGenerator/`, overheating + RR slow puncture, hood opens)
- [x] Plot/sanity check (speed/gear, rpm, coolant, tyres; same seed → identical file)
- [x] Sample trip committed (`Data/Trips/trip_sample.json`)
- [x] Check-your-understanding questions answered (LEARNING_LOG)

### Day 7: ITelemetryReceiver + file receiver
- [x] `ITelemetryReceiver`, `UFileTelemetryReceiver`, `UTelemetrySubsystem` (GameInstance subsystem, C++-only `UINTERFACE`,
      subsystem polls the receiver each tick), plus `FRecordedTripFile` and `UTelemetrySettings` (Project Settings > Vehicle Telemetry:
      trip path, playback speed multiplier)
- [x] PIE logs 10 frames/s, looping (checked 2026-10-07: first-frame values, 10× loop with `seq` wrap, incidents, pause/stop)
- Day 8 notes: subscribe to `UTelemetrySubsystem::OnTelemetryUpdated`, interpolate `GetPreviousTelemetrySample` →
  `GetLatestTelemetrySample`; on a trip loop `SampleTimeS` drops 189.9 → 0, treat that as a jump, not a blend.

### Day 8: Bind telemetry to the car
- [x] Wheel spin, interpolation, status colour (doors on the 3D car moved to Phase H, H2; `openings` stays on the dashboard until then)
- [x] Overheating incident visibly turns the car amber → red (as a pulsing outline, not paint: SPEC.md §1 "Status outline")
- Step 1 written 2026-10-07 (not built yet): `FVehicleStatusEvaluator` reports per-signal status plus overall, so both front ends
  can show which signal is in warning (SPEC.md §3).
- Step 2 done 2026-10-07: `UTelemetrySubsystem` evaluates every sample, `GetCurrentVehicleStatusReport()`, logs overall status
  changes. Built and verified in PIE at 10×: Warning at 91 s, Critical at 119 s (coolant).
- Step 3 done 2026-10-08: `AVehicleTwinActor` follows telemetry (previous → latest blend, loop snaps) and shows the status as a
  glowing outline (custom stencil + `PP_VehicleStatusOutline`, amber slow pulse / red fast pulse), `r.CustomDepth=3`. Material made
  by `Tools/UnrealEditor/create_status_outline_material.py` (Python Editor Script Plugin, editor only). Verified in PIE at 10×.
  The material is assigned in `BP_VehicleTwin`, which is local-only (personal laptop). Custom depth cost goes to Day 12 profiling.

### Day 9: Python WebSocket relay
- [x] `relay.py` with `--rate`, `--loop`, `--drop-percent`, `--pause-after` (+ `--pause-duration`, `--drop-seed`, `--trip`, `--port`)
      2026-10-08: `Tools/Relay/relay.py`, `websockets==15.0.1` (`Tools/Relay/requirements.txt`). Broadcast to every client, starts at
      the first connection, sends `hello` first, logs messages from clients (return path for T1).
- [x] Verified with a CLI client: `Tools/Relay/relay_client.py` (gaps, loops, latency, stall warning); pytest R1–R6 pass. Run at
      100 msg/s, 5% drop, 2 s pause: gaps reported, "No data for 1 s" during the pause, latency 0–2 ms.

### Day 10: WebSocket receiver in UE
- [x] `UWebSocketTelemetryReceiver`, state machine, backoff reconnect (2026-10-08; SPEC.md §4)
- [x] Dropped-message count, receive latency (`FTelemetryConnectionStatus`, in the once-a-second summary log)
- [x] File ↔ WebSocket switch via one setting (Project Settings > Vehicle Telemetry > Telemetry Source; committed default File)
- [x] PIE checks with the relay (personal laptop, 2026-10-08): Live at 10/s, dropped 0; relay stopped → Disconnected, retries
      0.4 → 0.9 → 2.3 s, restarted → Live with dropped still 0; `--pause-after` → Stale → Live; `--drop-percent 10` raises dropped.
      Receive latency ~18–30 ms avg (includes up to one frame of queue wait; Python client sees 0–2 ms). Max stuck at ~1050 ms
      from a PIE-start hitch: reset the max per window before the dashboard shows it (Day 11).
- [ ] `can't connect: no details` wording for empty backend errors: built? not checked in a log yet (cosmetic)
- Later: a send path for commands (T1) goes on this receiver; the relay already logs client messages.

### Twin completion (after Day 10, before Day 11; added 2026-10-07)
Two-way data: commands go back to the vehicle and its reported state comes back in telemetry.
- [x] T1. Feedback loop: a command from UE goes back to the source and changes what the vehicle does. Example: "Limp mode" when
      coolant is critical → UE sends it over the WebSocket → relay passes it to a vehicle simulator → next samples show rpm capped.
      Needs a two-way relay (Day 9) and a send path in the WebSocket receiver (Day 10); follows the "commands go down" rule (SPEC.md §5).
      2026-10-09: option A "engine protection derate request" (SPEC.md §2.5). Done: contract, simulator + tests (P9).
      2026-10-10: relay `--source live` + `commandAck`, `relay_client.py --send-derate-after`, tests R7–R10 (pytest 59 pass).
      2026-10-10: UE side written (`EVehicleDriveMode`, `VehicleCommand.h`, `SendVehicleCommand`/`PollVehicleCommandAcks`,
      subsystem pending acks + 2 s timeout + auto derate + `Twin.EngineDerate`). Built and checked in PIE against `relay.py --source live`: done.
- [x] T3. Command write-up in SPEC.md: done as §2.5 with T1.
- Prediction: dropped from this project (2026-10-30), listed under "Follow-ons (not built)".

### Day 11: MVVM dashboard (custom MVVM, option B)
- [x] Design decided (two ViewModels, 10 Hz dashboard, formatting in widgets, BP read + one event; SPEC.md §5)
- [x] Custom ViewModel framework (field-enum notifications, dirty flags, per-frame flush): steps 1–8 below
- [ ] `UVehicleTelemetryViewModel` + remote ops dashboard widgets (SPEC.md §8): built (build page A–E); F, playing it on the car, is
      wrap-up item 2
- [x] Data-flow diagram in SPEC.md (§6)
- Comparison with Epic's MVVM plugin: follow-on, not built

**Core rewrite, step by step (started 2026-09-28, office laptop).** A first version was written in one go, built and passed
9 automation tests, then parked on the local-only branch `mvvm-reference` (commit `9a9abe8`, not pushed, exists only on the
office laptop). We now rebuild it on `main` one step at a time, with an explanation per step. Compare: `git diff main mvvm-reference`.

- [x] 1. `Build.cs`: module root on the include path (`PublicIncludePaths.Add(ModuleDirectory)`), so `#include "MVVM/..."` works
- [x] 2. `FViewModelFieldMask` (`MVVM/ViewModelFieldMask.h`): uint64 bitmask, one bit per field
- [x] 3. `UViewModelBase` part 1: dirty bits + `Flush` (multicast delegate)
- [x] 4. `UViewModelBase` part 2: `SetField` (compare-before-set, float tolerance)
- [x] 5. `UVehicleTelemetryViewModel`: field enum, getters, `ApplyTelemetrySample`
- [x] 6. Automation tests (`CarDigitalTwins.MVVM.Core`, M1–M9): all 9 pass (2026-10-23, personal laptop, headless; TESTS.md §7)
- [x] 7. `Subscribe` + `FViewModelSubscription` (RAII unsubscribe): built, M9 passes (2026-10-23)
- [x] 8. `UViewModelSubsystem`: creates/holds ViewModels, once-per-frame flush; M10–M11 pass (2026-10-23, office laptop). Engine tick
      not yet seen in PIE (checked in wrap-up item 2)
- [ ] 9. Dashboard (decided 2026-10-10, D1–D7 on the mock: four views with camera blends, HMI rules, dark studio level).
      Mock: https://claude.ai/artifact/TsApXH75XFbcd2muKzLx3m · build page: https://claude.ai/artifact/AtxBFvjTzngZuMV9Z5k87u
      C++ built: `UConnectionViewModel`, `UVehicleEventLogViewModel`, `UDashboardViewModel`, command records on
      `UTelemetrySubsystem`, `Dashboard/` (settings, game mode, player controller with SetViewTargetWithBlend), car callout anchors +
      studio floor scroll, 22 widget classes in `UI/`, icons in `Art/UI/Icons/`. Build page: A, B, D, E1–E11 done (2026-10-14); UI fixes
      done (2026-10-23); F to do.
      Backlog (added 2026-10-23):
      - [x] **Part rings on the car.** Placed and pulsing on the car (2026-10-23, personal laptop); round ring brush to re-check in item 2. Since `e784edc` the callout layer also sets each copy's signal and anchor from C++. Before: they showed only in the top-left corner; cause: Ring Signal / Anchor Tag never set
            on the five ring copies in `WBP_CarCalloutLayer` (fix steps in the checkpoint above; code now hides and logs unplaceable rings). By design, `UTwinPartRingWidget` is hidden at Play (Collapsed) until its part leaves Normal:
            the hood ring shows when coolant ≥ 105 °C or rpm is off (the live sim crosses 105 °C ~86 s into each loop), and the RR wheel ring
            shows when the tyre drops below 180 kPa (~153 s). Check first: let PIE run past 90 s, then confirm the `WBP_PartRing` copies are inside
            `CalloutCanvas` with `RingSignal` + anchor tag set and the tag exists on `BP_VehicleTwin`. If the mock shows rings while everything is
            Normal, decide whether to add a dim idle ring.
      - [x] **Text overflow** (2026-10-23): `WBP_MarqueeText` scrolls text wider than its cap; top bar link detail uses it.
      - [ ] **Studio lighting looks off** (next session): judge and retune the `L_TwinStudio` lights on the paint together with F3 camera framing.

### Day 12: Profiling and latency (wrap-up item 4)
- [x] Frame time (5.0 ms median, studio + dashboard, no car), latency relay send → game thread (2.9 ms avg), test count (101), car
      triangles / draw calls (Day 3: 1.4 M / ~460), all in PERFORMANCE.md (2026-10-30, office machine)
- [ ] Frame time with the car and the dashboard together: personal laptop, item 2
- Insights deep dive, Invalidation/Retainer before/after, stress test, Epic MVVM comparison: follow-ons, not built

### Day 13: Pixel Streaming locally
- [ ] Runs locally in a browser: started 2026-10-28 (`Tools/PixelStreaming/`: setup notes, `start_stream.ps1` launching Wilbur, the
      relay and the game). Camera control = the dashboard's view tabs and keys 1–4, forwarded by the stream (decided 2026-10-28).
      Anything beyond "runs locally" is a follow-on.

### Day 14: Cloud deployment
- Follow-on, not built: COSTS.md already answers the cost question ($0 decision backed by real pricing).

### Day 15: Polish and demo video (wrap-up item 5)
- [ ] Camera presets, lighting, scripted demo run, video + GIF
- [x] Demo video script, 60–90 s: [DEMO_SCRIPT.md](DEMO_SCRIPT.md) (2026-10-30, reordered after the review): overheating Warning →
      Critical → auto derate round trip (ack, `driveMode` EngineDerate, rpm and speed capped) → back to Warning → link drop and
      recovery; all at 1×, jump cuts instead of 2×
- [ ] Record and edit it (personal laptop, after item 2)

### Day 16: Documentation and portfolio (wrap-up item 6)
- [x] README: one screen with diagram, measured figures and how to run; Decisions, tests, known issues, follow-ons, credits
- [ ] README GIF and demo video link (after the recording, item 5)
- [x] Positioning: a vehicle telemetry twin with a remote-operations dashboard. Don't lead with "digital twin"; never "predictive"
- [x] Portfolio fixes (2026-10-08, done in the README except the GIF): README first screen = GIF + architecture diagram (simulator → relay → `ITelemetryReceiver` →
      twin + dashboard) + 3–4 measured numbers from Day 12 (latency, frame time, dropouts survived). Reviewers can't get the meshes
      (license), so the video carries the demo; README gives "download STEP from GrabCAD + run the rebuild script" (needs BUG-002
      fixed). Never publish a packaged build (it redistributes the meshes). Present the car as a vehicle model, not anything official
      from Jeep. Show the $0 cost decision (COSTS.md) as a design choice. For game-studio applications lead with C++ / rendering.
- Real data clip: follow-on, not built.

## Phase H: In-car HMI view (follow-on, not built; added 2026-10-07)
Same receivers, subsystem, status evaluator and ViewModels as the twin; a second front end for the driver instead of a remote operator.
Shows the architecture separates data from presentation. HMI priorities differ from the ops view: readable at a glance, fixed car-screen
resolution, low latency, day/night themes, minimal distraction.
- [ ] H1. HMI design (SPEC.md §9): instrument cluster (e.g. 1920×720) and centre display; tell-tales from the per-signal status;
      3D car view showing doors/tyres; day/night theme; HTML wireframe. Same process as the UI design track.
- [ ] H2. Door / hood / tailgate animation on the 3D car (brings back the Day 4 optional item): finish the rear doors, use the split
      meshes in `/Game/Jeep/Cleaned` (part list and pivots: CHECKPOINTS.md, 2026-09-27), driven by `openings`. The signature HMI feature.
- [ ] H3. HMI view in UE: cluster + centre display widgets on the existing ViewModels, own camera for the 3D car view, switchable with
      the ops view.
- [ ] H4. HMI commands: e.g. "open tailgate" / lights from the centre display → command path from T1 → vehicle simulator → state
      comes back in the samples (the screen never animates on its own, SPEC.md §5).
- [ ] H5. Fixed 60 fps budget at cluster resolution; profile like Day 12, framed as embedded-hardware limits.
- [ ] H6. HMI demo video + README section.

## Testing (parallel track, started 2026-10-07)
Written in a separate session alongside the feature days. Plan and how to run: [TESTS.md](TESTS.md); bugs found: [BUGS.md](BUGS.md).
- [x] 1. Trip generator pytest suite, P1–P8 (`Tools/TripGenerator/tests/`); found BUG-001 (`--rate` 3/7 fail validation, fixed 2026-10-07)
- [x] 2. Telemetry automation tests U1–U9: all pass (2026-10-23, office laptop, headless; TESTS.md §2)
- [x] 3. `AVehicleTwinActor` automation tests V1–V10 (stand-in actor, no Jeep assets): all pass (2026-10-23, office laptop; TESTS.md §3)
- [x] 4. Roadmap tests as days land: relay R1–R10, MVVM M1–M11, status evaluator S1–S11 (2026-10-30, wrap-up item 3; TESTS.md §8)

## UI design (parallel track, added 2026-10-07; superseded)
Done in practice as the dashboard mock and build page (Day 11, step 9); SPEC.md §8 describes the dashboard as built. The steps below
are kept as a record.

Can run in a separate session alongside Days 8–10; design only, no UMG or C++. Architecture is already decided (SPEC.md §5–6:
two ViewModels, 10 Hz dashboard, formatting in widgets, notify-then-pull); what's missing is the screens themselves. Doing it before
Day 8 matters because the screens decide backend details (see step 3).

**Scope (direction decided 2026-10-07):** this track designs the **remote ops (digital twin) view** first, SPEC.md §8. The in-car
HMI view is Phase H (H1, SPEC.md §9) and can be designed later by the same kind of session. Both share the ViewModels, so §8 must not
put layout-specific data in them. Per-signal status already exists (`FVehicleStatusReport`, SPEC.md §3).

- [ ] 1. Ask the user first: (a) answered 2026-10-07: purpose is a portfolio piece that also works as a real monitoring screen, digital
      twin first; confirm how dense it should be. (b) still open: placement, full-screen overlay on the 3D car or a side panel next to it?
      Also ask whether to show the T1 command (e.g. limp mode) in the first design.
- [ ] 2. Write **SPEC.md §8 "Dashboard screens"**: screen/panel list; every widget mapped to a ViewModel field
      (`UVehicleTelemetryViewModel` from §2.1 fields + derived status §3; `UConnectionViewModel`: state, latency, drops); the four data
      states per panel (no data yet, live, stale, error, see §4); controls (pause, playback speed, file ↔ WebSocket source, §5
      commands); target resolution and aspect for Pixel Streaming (Day 13). Plus an HTML wireframe of the layout.
- [ ] 3. Feed back into the plan and note it under the affected days: per-signal status from `FVehicleStatusEvaluator` (Day 8);
      command functions on `UTelemetrySubsystem` (none yet; playback speed is only read at Play start); fields the Day 10 receiver
      must track for `UConnectionViewModel`.

Constraints to respect: doors/hood/tailgate don't animate in Phase A, `openings` is shown on the dashboard only (Day 4 scope cut;
the animation comes back in Phase H, H2); the body
paint is red (`MI_CarPaint`), so a red Critical colour barely shows on the car, the dashboard should carry status clearly on its
own; leave room for the Showroom mode idea (Optional / later), which would add a web panel over the Pixel Stream.

## Budget constraints ($0 project)
- No cloud hosting: Pixel Streaming runs locally only (NVENC on either machine).
- Machines: office laptop (RTX 4060 Ti) and personal laptop (RTX 3070 Ti Laptop). Tag every measurement with the machine.
- GitHub LFS free tier (10 GB storage, 10 GB/month bandwidth, rechecked 2026-10-08): keep committed assets lean; large content goes to a Release zip / external link if needed.
- Free tools and assets only (GrabCAD / free marketplace, OBS, DaVinci Resolve, Mosquitto, SUMO, Cesium ion free tier, Omniverse).

## Pending setup
- ~~Private asset repo for `Content/Jeep/`~~ (dropped 2026-10-30, wrap-up item 7). Jeep asset work happens on the personal laptop only.

## Follow-ons (not built)
Out of scope for this project (wrap-up decision, 2026-10-30). Listed so the ideas aren't lost; none of them is planned here.
- **Phase H, in-car HMI view** (H1–H6 above): cluster + centre display on the same ViewModels, door / hood / tailgate animation from
  the split meshes in `/Game/Jeep/Cleaned`, HMI commands, 60 fps budget at cluster resolution.
- **Epic MVVM plugin comparison**: the same dashboard on `ModelViewViewModel`, with a measured cost comparison.
- **Pixel Streaming beyond running locally**: browser-side controls, latency part (c), anything more than the local stream.
- **Cloud deployment and its design doc**: VM, ports, TURN, auto-shutdown, start on demand; or a one-off deploy-measure-terminate
  session (~$2–5, steps in COSTS.md).
- **Real data source**: a racing game's UDP telemetry (Forza "Data Out", BeamNG OutGauge) or an ELM327 OBD-II dongle with
  `python-OBD`, behind `ITelemetryReceiver` without changing anything downstream.
- **MQTT** in place of the WebSocket relay.
- **COVESA VSS alignment**: map the SPEC §2.1 fields to Vehicle Signal Specification names (e.g. `speedKmh` → `Vehicle.Speed`).
- **Showroom mode**: configurator-style mode next to the live view (colour swatches on `M_CarPaint`, camera presets, turntable,
  optional door opening and rim swap); limited by the GrabCAD license and the $0 budget.
- **Phase B** (fleet → SUMO → Cesium) and **Phase C** (OpenUSD → Kit extension). Omniverse / OpenUSD becomes its own project.
- **Prediction**: time to the next threshold from a linear trend over recent samples, exposed through the ViewModel.
