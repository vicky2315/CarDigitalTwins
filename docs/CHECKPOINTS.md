# Checkpoint archive

Newest first. Live checkpoint is in [PROGRESS.md](PROGRESS.md).

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
