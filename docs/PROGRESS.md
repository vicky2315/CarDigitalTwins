# Progress: Vehicle Digital Twin (Roadmap v2)

Last updated: 2026-09-25
Current day: **Day 3: Datasmith import and tessellation** (Days 1–2 complete)

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
- [ ] Two tessellation settings compared (triangle counts, visuals)
- [ ] Triangle budget set and justified
- [ ] Settings, counts and screenshots logged

### Day 4: Cleanup, pivots and USD-ready naming
- [ ] `AVehicleTwinActor` C++ base + `BP_VehicleTwin`
- [ ] Split `Split1[2]` into doors / hood / tailgate (PolyGroup split → Merge) and door glass out of `Windows`
- [ ] Wheel and door pivots fixed (tailgate spare wheel excluded from spin)
- [ ] `<Part>_<Position>` naming + component tags
- [ ] Body paint MI with `StatusColor`
- [ ] Mapping table in SPEC.md

### Day 5: Telemetry schema
- [ ] Fields with units, trip.json format, stream message format, status thresholds in SPEC.md
- [ ] `FVehicleTelemetry` compiles

### Day 6: Trip generator (Python)
- [ ] `trip_generator.py` with phases, fixed seed, incidents
- [ ] Plot/sanity check; sample trip committed

### Day 7: ITelemetryReceiver + file receiver
- [ ] `ITelemetryReceiver`, `UFileTelemetryReceiver`, `UTelemetrySubsystem`
- [ ] PIE logs 10 frames/s, looping

### Day 8: Bind telemetry to the car
- [ ] Wheel spin, interpolation, status colour, doors
- [ ] Overheating incident visibly turns the car amber → red

### Day 9: Python WebSocket relay
- [ ] `relay.py` with `--rate`, `--loop`, `--drop-percent`, `--pause-after`
- [ ] Verified with a CLI client

### Day 10: WebSocket receiver in UE
- [ ] `UWebSocketTelemetryReceiver`, state machine, backoff reconnect
- [ ] Dropped-message count, receive latency
- [ ] File ↔ WebSocket switch via one setting

### Day 11: MVVM dashboard (custom MVVM, option B)
- [ ] Custom ViewModel framework (field-enum notifications, dirty flags, per-frame flush)
- [ ] `UVehicleTelemetryViewModel` + dashboard widgets
- [ ] Data-flow diagram in SPEC.md
- [ ] (Comparison) Same dashboard on Epic's MVVM plugin

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

## Budget constraints ($0 project)
- No cloud hosting: Pixel Streaming runs locally only (NVENC on either machine).
- Machines: office laptop (RTX 4060 Ti) and personal laptop (RTX 3070 Ti Laptop). Tag every measurement with the machine.
- GitHub LFS free tier (~1 GB storage, ~1 GB/month bandwidth): keep committed assets lean; large content goes to a Release zip / external link if needed.
- Free tools and assets only (GrabCAD / free marketplace, OBS, DaVinci Resolve, Mosquitto, SUMO, Cesium ion free tier, Omniverse).

## Future options
- [ ] **One-off cloud deployment** (~$2–5): AWS Budget alert at $5 first → launch g4dn.xlarge on-demand for a 3–4 h session →
      TURN + auto-shutdown tested from mobile data → real numbers in COSTS.md → terminate the VM and delete the EBS volume.

## Optional / later
- [ ] MQTT upgrade
- [ ] Phase B (fleet → SUMO → Cesium)
- [ ] Phase C (OpenUSD → Kit extension → side-by-side demo)
