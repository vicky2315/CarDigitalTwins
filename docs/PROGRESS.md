# Progress: Vehicle Digital Twin (Roadmap v2)

Last updated: 2026-10-09
Current day: **Twin completion T1** (steps 1–2 of 5 done 2026-10-09), then T2–T3, then Day 11.
Days 1–10 complete. Day 11 MVVM rewrite paused before step 2 (machine-independent, can continue on either laptop).

**Direction (decided 2026-10-07, option 3):** one backend, two front ends. Digital twin first (Phase A + Twin completion: remote
ops view, two-way data, prediction), then expand the same core into an in-car HMI view (Phase H). Aim: get solid at digital twins,
then HMI; the portfolio shows both on one data pipeline.

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
Turns the project from a digital *shadow* (data flows one way) into a digital *twin* (two-way data plus prediction).
- [x] T1. Feedback loop: a command from UE goes back to the source and changes what the vehicle does. Example: "Limp mode" when
      coolant is critical → UE sends it over the WebSocket → relay passes it to a vehicle simulator → next samples show rpm capped.
      Needs a two-way relay (Day 9) and a send path in the WebSocket receiver (Day 10); follows the "commands go down" rule (SPEC.md §5).
      2026-10-09: option A "engine protection derate request" (SPEC.md §2.5). Done: contract, simulator + tests (P9).
      2026-10-10: relay `--source live` + `commandAck`, `relay_client.py --send-derate-after`, tests R7–R10 (pytest 59 pass).
      2026-10-10: UE side written (`EVehicleDriveMode`, `VehicleCommand.h`, `SendVehicleCommand`/`PollVehicleCommandAcks`,
      subsystem pending acks + 2 s timeout + auto derate + `Twin.EngineDerate`). Built and checked in PIE against `relay.py --source live`: done.
- [ ] T2. Prediction: estimate time to the next threshold from recent samples (e.g. "coolant critical in ~25 s", "RR tyre at
      180 kPa in ~6 min"); exposed through the ViewModel so both front ends can show it.
- [ ] T3. Write both up in SPEC.md (command message format next to §2.3; prediction method and its limits).

### Day 11: MVVM dashboard (custom MVVM, option B)
- [x] Design decided (two ViewModels, 10 Hz dashboard, formatting in widgets, BP read + one event; SPEC.md §5)
- [ ] Custom ViewModel framework (field-enum notifications, dirty flags, per-frame flush)
- [ ] `UVehicleTelemetryViewModel` + remote ops dashboard widgets (screens designed in the UI design track below, SPEC.md §8).
      The in-car HMI view is a second set of widgets on the same ViewModels (Phase H): keep ViewModels free of layout assumptions.
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
- [ ] Demo video 60–90 s (decided 2026-10-08, $0 portfolio path): live stream → overheating amber → red → connection drop and
      recovery → command back to the car (T1) → prediction ("coolant critical in ~25 s", T2). First 30 s must carry it.

### Day 16: Documentation and portfolio
- [ ] README complete; portfolio section (demo video, no live link); runnable in < 15 min
- [ ] Frame it as a digital twin (physical entity, virtual entity, two-way data thread, prediction); list the HMI view as the next phase
- [ ] Portfolio fixes (2026-10-08): README first screen = GIF + architecture diagram (simulator → relay → `ITelemetryReceiver` →
      twin + dashboard) + 3–4 measured numbers from Day 12 (latency, frame time, dropouts survived). Reviewers can't get the meshes
      (license), so the video carries the demo; README gives "download STEP from GrabCAD + run the rebuild script" (needs BUG-002
      fixed). Never publish a packaged build (it redistributes the meshes). Present the car as a vehicle model, not anything official
      from Jeep. Show the $0 cost decision (COSTS.md) as a design choice. For game-studio applications lead with C++ / rendering.
- [ ] Real data clip: at least one short clip with a real source (CARLA, free, open source; or a racing game's telemetry output if
      already owned), see "Future options: Real data source". Finishing Day 16 matters more than adding scope.

## Phase H: In-car HMI view (after Phase A; added 2026-10-07)
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
- [ ] 2. Telemetry automation tests U1–U9 (needs Day 7 committed)
- [ ] 3. `AVehicleTwinActor` automation tests V1–V7 (synthetic actor, no Jeep assets)
- [ ] 4. Roadmap tests as days land (status thresholds Day 8, relay Day 9 done: R1–R6, MVVM Day 11)

## UI design (parallel track, added 2026-10-07)
Can run in a separate session alongside Days 8–10; design only, no UMG or C++. Architecture is already decided (SPEC.md §5–6:
two ViewModels, 10 Hz dashboard, formatting in widgets, notify-then-pull); what's missing is the screens themselves. Doing it before
Day 8 matters because the screens decide backend details (see step 3).

**Scope (direction decided 2026-10-07):** this track designs the **remote ops (digital twin) view** first, SPEC.md §8. The in-car
HMI view is Phase H (H1, SPEC.md §9) and can be designed later by the same kind of session. Both share the ViewModels, so §8 must not
put layout-specific data in them. Per-signal status already exists (`FVehicleStatusReport`, SPEC.md §3).

- [ ] 1. Ask the user first: (a) answered 2026-10-07: purpose is a portfolio piece that also works as a real monitoring screen, digital
      twin first; confirm how dense it should be. (b) still open: placement, full-screen overlay on the 3D car or a side panel next to it?
      Also ask whether to show the T2 prediction ("critical in ~25 s") and the T1 command (e.g. limp mode) in the first design.
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
- [ ] **Private asset repo for `Content/Jeep/`** (needs personal laptop, WIP door split lives there; **do next**: the Day 4 work was
      already lost once and only survived in autosaves, 2026-10-03): see ASSETS.md
      "Syncing work-in-progress assets between machines". Until done, Jeep asset work happens on the personal laptop only.

## Future options
- [ ] **One-off cloud deployment** (~$2–5): AWS Budget alert at $5 first → launch g4dn.xlarge on-demand for a 3–4 h session →
      TURN + auto-shutdown tested from mobile data → real numbers in COSTS.md → terminate the VM and delete the EBS volume.
- [ ] **Real data source** (added 2026-10-07): a new receiver or relay input so the demo isn't only simulated. Free: a racing game's
      UDP telemetry output (e.g. Forza "Data Out", BeamNG OutGauge: speed, rpm, gear, pedals, steering; likely no tyre pressure or
      doors) bridged into the relay. ~$10–20: ELM327 OBD-II dongle + `python-OBD` (standard PIDs: speed, rpm, coolant, throttle,
      fuel, module voltage). Either plugs in behind `ITelemetryReceiver` without changing anything downstream.
- [ ] **COVESA VSS alignment** (added 2026-10-07): map the §2.1 fields to their Vehicle Signal Specification names in SPEC.md and the
      README (e.g. `speedKmh` → `Vehicle.Speed`). Industry vocabulary for both the twin and the HMI.

## Optional / later
- [ ] MQTT upgrade
- [ ] Phase B (fleet → SUMO → Cesium)
- [ ] Phase C (OpenUSD → Kit extension → side-by-side demo)
- [ ] **Showroom mode** (idea 2026-10-05, *not decided*; overlaps Phase H's centre display and H2 door animation, decide after H1):
      car-configurator style mode next to the live twin, after Day 13 (~2–3 days).
      Toggle Live Twin ↔ Showroom; web panel over the Pixel Stream (browser → UE messages) with body/trim colour swatches (`PaintColor`
      on `M_CarPaint`); camera presets + turntable (overlaps Day 15); optional door/hood opening with the split meshes in
      `/Game/Jeep/Cleaned`; optional rim swap. Limits: no public link at $0 (local + video only; a public version would need a
      glTF + three.js viewer, separate project) and the GrabCAD license (sharing the model publicly needs the author's permission).
