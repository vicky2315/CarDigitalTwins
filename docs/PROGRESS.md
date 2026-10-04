# Progress: Vehicle Digital Twin (Roadmap v2)

Last updated: 2026-10-04
Current day: **Day 6: trip generator** (Day 4 option A done 2026-10-05).
Days 1–5 complete. Day 11 MVVM rewrite paused before step 2 (machine-independent, can continue on either laptop).

## Checkpoint (2026-10-05, personal laptop, RTX 3070 Ti Laptop)

**Where I stopped:** Day 4 done. Wheels roll, body (`Paint`) and black trim (`Trim`) take the status colour, materials cleaned up
(one master `M_CarPaint`), `BP_VehicleTwin` moved to `/Game/Jeep/Blueprints`. Next: Day 6 trip generator.

Summary of Day 4 (details in SPEC.md §1 and LEARNING_LOG):
- Option A: car built from the untouched Datasmith import; `BP_VehicleTwin` harvested from `ImportA`, parts are child actors.
- `AVehicleTwinActor`: tag lookup through child actors, wheel spin/steer about the measured wheel centre, paint groups with status colour,
  editor previews (wheels, status colour).
- Not checked in a Play log after the last build: per-group log lines and the `CarRoot` facing warning. Check on the next Play.

**Resume:**
1. Day 6: `trip_generator.py` (phases, fixed seed, incidents), plot/sanity check, sample trip committed. No editor needed.
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
- [ ] (Optional, later) Door / hood / tailgate animation using the split meshes

### Day 5: Telemetry schema
- [x] Fields with units, trip.json format, stream message format, status thresholds in SPEC.md
- [x] `FVehicleTelemetry` compiles (office laptop)

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
- [x] Design decided (two ViewModels, 10 Hz dashboard, formatting in widgets, BP read + one event; SPEC.md §5)
- [ ] Custom ViewModel framework (field-enum notifications, dirty flags, per-frame flush)
- [ ] `UVehicleTelemetryViewModel` + dashboard widgets
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
