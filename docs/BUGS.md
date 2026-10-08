# Bugs

Known bugs, found by tests or by hand. Each bug has an ID that the test flagging it refers to. Each entry is written so that someone
new (or another Claude session) can fix it without the original context: where it is, why it happens, the fix options that were
tried, the recommended one, and how to verify.

**When a bug is fixed:** set its status to Fixed (date, commit), remove the `xfail` marker from its test, run the tests
(docs/TESTS.md), and leave the entry here as a record.

| ID | Status | Area | Summary |
|----|--------|------|---------|
| BUG-001 | Fixed (2026-10-07, not committed yet) | Trip generator | `--rate` values that don't divide 1000 evenly fail validation |
| BUG-002 | Open (low priority) | Car rebuild tooling | Status outline doesn't show on `BP_VehicleTwin_Rebuilt` |

---

## BUG-001: `--rate` values that don't divide 1000 evenly fail validation

**Status:** Fixed 2026-10-07 with option A (shared `SAMPLE_TIME_DECIMALS` + `SAMPLE_TIME_GRID_TOLERANCE_S`), not committed yet.
Found 2026-10-07 by test P3. Verified: `xfail` removed, 39 passed, committed 10 Hz sample unchanged. The sections below describe the
code before the fix.

### Symptom
```
python Tools/TripGenerator/trip_generator.py --rate 3 --out <temp folder>/t.json
ValueError: frame 1: sampleTimeS 0.333 is off the 3 Hz grid
```
Same for 7 Hz (`0.143`) and any other rate that doesn't divide 1000 evenly. Rates that do (1, 2, 4, 5, 8, 10, 20, 25, 40, 50, 100 …)
work, which is why the default 10 Hz and the committed sample are fine.

### Where
`Tools/TripGenerator/trip_generator.py` (line numbers as of commit `56f5440`):
- **Line 226**, `generate_trip`: the frame is written with `"sampleTimeS": round(trip_time_s, 3)`, i.e. rounded to whole milliseconds.
- **Line 241**, `generate_trip`: `trip_time_s = seq * delta_s` (exact grid time; this part is correct).
- **Lines 258 and 264**, `validate_trip`: `expected_step_s = 1.0 / trip["rateHz"]`, then
  `if abs(frame["sampleTimeS"] - frame_index * expected_step_s) > 1e-6: raise ValueError(...)`.

### Cause
The generator rounds `sampleTimeS` to 3 decimals, but the validator compares it with the exact grid time and allows only 1e-6 s of
error. At 3 Hz frame 1 is 0.3333… s, written as 0.333: off by 3.3e-4 s, so validation fails. Rounding to 3 decimals can be off by up
to 0.5 ms (5e-4), and the tolerance has to allow for that. `main()` calls `validate_trip` before `write_trip`, so nothing is written.

### Impact
Low today: only the 10 Hz sample is used. It blocks trips at other rates, which the Day 9 relay (`--rate`) and the Day 12 stress tests
may want.

**The Unreal side is not affected.** `UFileTelemetryReceiver` (`Source/CarDigitalTwins/Telemetry/FileTelemetryReceiver.cpp`) only
requires `sampleTimeS` to increase from frame to frame. It delivers a frame once playback time reaches its `sampleTimeS`, so 0.5 ms of
rounding is less than one game frame of delay. The loop length comes from `frames / rateHz`, not from `sampleTimeS`.

### Fix options (both tried on scratch copies, 2026-10-07)
Each was checked at 3, 7, 10, 30, 60 and 100 Hz. All rates generated and validated, and the 10 Hz output stayed byte-identical to
the committed `Data/Trips/trip_sample.json`, so the sample does not need regenerating with either option.

| Option | Change | Result | Trade-off |
|--------|--------|--------|-----------|
| **A (recommended)** | Validator tolerance matches the rounding: half a millisecond | All rates pass; 3 Hz file 175,819 bytes | Times stay readable (`0.333`); validator allows up to 0.5 ms of error |
| B | Generator rounds to 6 decimals instead of 3 | All rates pass; 3 Hz file 176,959 bytes | Times like `0.333333`; files at odd rates are slightly larger |

**Recommended: option A, with one shared constant**, so the rounding and the tolerance can't drift apart again. Near the other
constants at the top of the file:
```python
SAMPLE_TIME_DECIMALS = 3                   # sampleTimeS is written in whole milliseconds
```
Line 226:
```python
"sampleTimeS": round(trip_time_s, SAMPLE_TIME_DECIMALS),
```
`validate_trip`, replacing the `1e-6` check (the extra 1e-9 absorbs floating-point error at exactly half a millisecond):
```python
sample_time_tolerance_s = 0.5 * 10 ** -SAMPLE_TIME_DECIMALS + 1e-9
...
if abs(frame["sampleTimeS"] - frame_index * expected_step_s) > sample_time_tolerance_s:
```
The validator still catches real mistakes: frames are 1/rateHz apart (at most 1000 Hz, ≥ 1 ms), so a frame at the wrong grid
position is off by far more than 0.5 ms. The P4 test `move_sample_time_off_grid` (0.15 s instead of 0.1 s) still fails validation.

### Verify the fix
1. In `Tools/TripGenerator/tests/test_trip_generator.py`, remove the `@pytest.mark.xfail(...)` line above
   `test_rate_that_does_not_divide_1000_validates`. Optionally widen its rates, e.g. `[3, 7, 30, 60]`.
2. `python -m pytest Tools/TripGenerator/tests -v` → expect **39 passed**, no xfail. If the marker is left in, those tests
   report `XPASS(strict)` and fail: that is the reminder to remove it.
3. `test_seed42_reproduces_committed_sample_byte_for_byte` must still pass. If it fails, the change altered the 10 Hz output: don't
   regenerate the sample to make it pass, find out why.
4. Update this entry: Status → Fixed (date, commit); update the table at the top; update the P3 row in docs/TESTS.md.

---

## BUG-002: Status outline doesn't show on `BP_VehicleTwin_Rebuilt`

**Status:** Open, low priority (found 2026-10-08). The rebuild was an experiment to see whether the car can be recreated from the
STEP file plus a recipe; nothing depends on it. The hand-built `BP_VehicleTwin` is still the car used everywhere.

### Symptom
`Tools/UnrealEditor/rebuild_vehicle_twin_from_cad.py` creates `/Game/Jeep/Rebuilt/ImportA` and `BP_VehicleTwin_Rebuilt` from
`vehicle_twin_recipe.json` (written by `export_vehicle_twin_recipe.py`: 113 components, 72 meshes, tags `Paint` 8, `Trim` 22,
`Tyre` 4, one each `Wheel.*`). The rebuilt car looks mostly the same, but ticking **Preview Status Outline** on a placed instance
shows no outline, although its Class Defaults show `Status Outline Material` = `PP_VehicleStatusOutline`. Not yet tried in PIE.

### Where
- `Source/CarDigitalTwins/Vehicle/VehicleTwinActor.cpp`: `SetStatusOutline`, `EnsureStatusOutlinePostProcess`,
  `CollectStatusOutlineMeshComponents` (SPEC.md §1 "Status outline").
- `Tools/UnrealEditor/rebuild_vehicle_twin_from_cad.py`, `build_blueprint`: sets `status_outline_material` on the generated
  class's default object after compiling; copies names, hierarchy, transforms, tags, meshes and materials, sets mobility Movable,
  nothing about Nanite or render settings.

### What the code rules out
`CollectStatusOutlineMeshComponents` takes every `UPrimitiveComponent` (child actors included) with no filter, so the rebuilt car's
plain `StaticMeshComponent`s should be collected the same way as the original's Harvest child actors.

### Suspects
1. The outline is in two halves: meshes write custom stencil 1, the post-process draws around it. Unknown which half fails.
2. `EnsureStatusOutlinePostProcess` only logs a missing/wrong material from BeginPlay; the editor preview fails silently.
3. Mesh asset or component settings differ from the original import (Nanite, Render CustomDepth Pass, visibility of a parent).
4. Preview flag is per placed instance (`EditAnywhere`): it must be ticked on the rebuilt car's own instance.

### Debug steps (no build needed for 1–2)
1. Both cars in one level, Preview Status Outline ticked **only on the rebuilt car** (each car's unbound post-process outlines every
   stencil-1 pixel in the scene, so with both ticked the original can draw the rebuilt car's outline and the test lies).
2. Viewport → View Mode → Buffer Visualization → Custom Stencil. Rebuilt car visible there → post-process half (check PIE log for
   `StatusOutlineMaterial is not set` / not a Post Process material). Not visible → mesh half (compare one rebuilt mesh asset and
   component with its `ImportA` twin).
3. Optional: log `"%s: status outline on %d mesh components"` in BeginPlay to compare both cars by one number.

### Verify the fix
Rebuilt car shows the amber pulse in the editor preview and in PIE at 10× (Warning ~91 s, Critical ~119 s), same `LogVehicleTwin`
lines as the original.
