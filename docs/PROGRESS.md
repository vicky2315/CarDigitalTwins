# Progress: Vehicle Digital Twin (Roadmap v2)

Last updated: 2026-10-07
Current day: **Day 8: Bind telemetry to the car** (Day 7 done 2026-10-07).
Days 1–7 complete. Day 11 MVVM rewrite paused before step 2 (machine-independent, can continue on either laptop).

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
- [x] Body paint MI with `StatusColor` (`M_CarPaint` / `MI_CarPaint`, 25 `Paint` meshes, `StatusBlend` + editor preview)
- [x] `Trim` paint group (black parts) built; `MI_CarPaintBlack` reparented to `M_CarPaint`, duplicate master deleted
- [x] Mapping table in SPEC.md (§1, incl. paint groups and materials)
- [x] `BP_VehicleTwin` moved to `/Game/Jeep/Blueprints` (local only)
- [ ] (Optional, later) Door / hood / tailgate animation using the split meshes (pivots and part list: CHECKPOINTS.md, 2026-09-27)

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
- [ ] Wheel spin, interpolation, status colour, doors
- [ ] Overheating incident visibly turns the car amber → red
- Depends on the UI design track (below): if the dashboard shows which signal is in warning, the status evaluator must report
  per-signal status, not only the overall one. Check the track's §8 before writing `FVehicleStatusEvaluator`.

### Day 9: Python WebSocket relay
- [ ] `relay.py` with `--rate`, `--loop`, `--drop-percent`, `--pause-after`
- [ ] Verified with a CLI client

### Day 10: WebSocket receiver in UE
- [ ] `UWebSocketTelemetryReceiver`, state machine, backoff reconnect
- [ ] Dropped-message count, receive latency
- [ ] File ↔ WebSocket switch via one setting

### Day 11: MVVM dashboard (custom MVVM, option B)
- [x] Design decided (two ViewModels, 10 Hz dashboard, formatting in widgets, BP read + one event; SPEC.md §5)
- [ ] Custom ViewModel framework (field-enum notifications, dirty flags, per-frame flush)
- [ ] `UVehicleTelemetryViewModel` + dashboard widgets (screens designed in the UI design track below, SPEC.md §8)
- [x] Data-flow diagram in SPEC.md (§6)
- [ ] (Comparison) Same dashboard on Epic's MVVM plugin

**Core rewrite, step by step (started 2026-09-28, office laptop).** A first version was written in one go, built and passed
9 automation tests, then parked on the local-only branch `mvvm-reference` (commit `9a9abe8`, not pushed, exists only on the
office laptop). We now rebuild it on `main` one step at a time, with an explanation per step. Compare: `git diff main mvvm-reference`.

- [x] 1. `Build.cs`: module root on the include path (`PublicIncludePaths.Add(ModuleDirectory)`), so `#include "MVVM/..."` works
- [ ] 2. `FViewModelFieldMask` (`MVVM/ViewModelFieldMask.h`): uint64 bitmask, one bit per field ← **next: explained, not written yet**
- [ ] 3. `UViewModelBase` part 1: dirty bits + `Flush` (multicast delegate)
- [ ] 4. `UViewModelBase` part 2: `SetField` (compare-before-set, float tolerance)
- [ ] 5. `UVehicleTelemetryViewModel`: field enum, getters, `ApplySample`
- [ ] 6. Automation tests (`CarDigitalTwins.MVVM.Core`)
- [ ] 7. `Subscribe` + `FViewModelSubscription` (RAII unsubscribe)
- [ ] 8. `UViewModelSubsystem`: creates/holds ViewModels, once-per-frame flush
- Then: `UConnectionViewModel`, `UViewModelWidget` base + BP helper library, formatting helpers, debug fake-sample command, first dashboard widget.

### Day 12: Profiling and latency
- [ ] Insights trace; Invalidation/Retainer before/after
- [ ] Custom vs Epic MVVM cost comparison
- [ ] Latency parts (a) and (b); stress test
- [ ] PERFORMANCE.md

### Day 13: Pixel Streaming locally
- [ ] Streams to local browser; camera controls; latency part (c)

### Day 14: Cloud deployment (design only, $0 path, decided 2026-09-25)
- [ ] Deployment design doc in COSTS.md: VM choice, ports/security group, TURN (coturn) plan, auto-shutdown script design, start-on-demand flow
- [ ] Local demo checklist for interviews (launch order, screen-share setup)
- [ ] (Future option) Deploy once, measure, tear down: see "Future options" below

### Day 15: Polish and demo video
- [ ] Camera presets, lighting, scripted demo run, video + GIF

### Day 16: Documentation and portfolio
- [ ] README complete; portfolio section (demo video, no live link); runnable in < 15 min

## Testing (parallel track, started 2026-10-07)
Written in a separate session alongside the feature days. Plan and how to run: [TESTS.md](TESTS.md); bugs found: [BUGS.md](BUGS.md).
- [x] 1. Trip generator pytest suite, P1–P8 (`Tools/TripGenerator/tests/`); found BUG-001 (`--rate` 3/7 fail validation, fixed 2026-10-07)
- [ ] 2. Telemetry automation tests U1–U9 (needs Day 7 committed)
- [ ] 3. `AVehicleTwinActor` automation tests V1–V7 (synthetic actor, no Jeep assets)
- [ ] 4. Roadmap tests as days land (status thresholds Day 8, relay Day 9, MVVM Day 11)

## UI design (parallel track, added 2026-10-07)
Can run in a separate session alongside Days 8–10; design only, no UMG or C++. Architecture is already decided (SPEC.md §5–6:
two ViewModels, 10 Hz dashboard, formatting in widgets, notify-then-pull); what's missing is the screens themselves. Doing it before
Day 8 matters because the screens decide backend details (see step 3).

- [ ] 1. Ask the user first, these shape everything: (a) purpose: portfolio demo video (reads at a glance) or functional monitoring
      tool (dense, every value, alert list)? (b) placement: full-screen overlay on the 3D car, or a side panel next to it?
- [ ] 2. Write **SPEC.md §8 "Dashboard screens"**: screen/panel list; every widget mapped to a ViewModel field
      (`UVehicleTelemetryViewModel` from §2.1 fields + derived status §3; `UConnectionViewModel`: state, latency, drops); the four data
      states per panel (no data yet, live, stale, error, see §4); controls (pause, playback speed, file ↔ WebSocket source, §5
      commands); target resolution and aspect for Pixel Streaming (Day 13). Plus an HTML wireframe of the layout.
- [ ] 3. Feed back into the plan and note it under the affected days: per-signal status from `FVehicleStatusEvaluator` (Day 8);
      command functions on `UTelemetrySubsystem` (none yet; playback speed is only read at Play start); fields the Day 10 receiver
      must track for `UConnectionViewModel`.

Constraints to respect: doors/hood/tailgate don't animate, `openings` is shown on the dashboard only (Day 4 scope cut); the body
paint is red (`MI_CarPaint`), so a red Critical colour barely shows on the car, the dashboard should carry status clearly on its
own; leave room for the Showroom mode idea (Optional / later), which would add a web panel over the Pixel Stream.

## Budget constraints ($0 project)
- No cloud hosting: Pixel Streaming runs locally only (NVENC on either machine).
- Machines: office laptop (RTX 4060 Ti) and personal laptop (RTX 3070 Ti Laptop). Tag every measurement with the machine.
- GitHub LFS free tier (~1 GB storage, ~1 GB/month bandwidth): keep committed assets lean; large content goes to a Release zip / external link if needed.
- Free tools and assets only (GrabCAD / free marketplace, OBS, DaVinci Resolve, Mosquitto, SUMO, Cesium ion free tier, Omniverse).

## Pending setup
- [ ] **Private asset repo for `Content/Jeep/`** (needs personal laptop, WIP door split lives there; **do next**: the Day 4 work was
      already lost once and only survived in autosaves, 2026-10-03): see ASSETS.md
      "Syncing work-in-progress assets between machines". Until done, Jeep asset work happens on the personal laptop only.

## Future options
- [ ] **One-off cloud deployment** (~$2–5): AWS Budget alert at $5 first → launch g4dn.xlarge on-demand for a 3–4 h session →
      TURN + auto-shutdown tested from mobile data → real numbers in COSTS.md → terminate the VM and delete the EBS volume.

## Optional / later
- [ ] MQTT upgrade
- [ ] Phase B (fleet → SUMO → Cesium)
- [ ] Phase C (OpenUSD → Kit extension → side-by-side demo)
- [ ] **Showroom mode** (idea 2026-10-05, *not decided*): car-configurator style mode next to the live twin, after Day 13 (~2–3 days).
      Toggle Live Twin ↔ Showroom; web panel over the Pixel Stream (browser → UE messages) with body/trim colour swatches (`PaintColor`
      on `M_CarPaint`); camera presets + turntable (overlaps Day 15); optional door/hood opening with the split meshes in
      `/Game/Jeep/Cleaned`; optional rim swap. Limits: no public link at $0 (local + video only; a public version would need a
      glTF + three.js viewer, separate project) and the GrabCAD license (sharing the model publicly needs the author's permission).
