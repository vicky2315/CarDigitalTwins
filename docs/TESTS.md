# Tests

Automated tests: what exists, how to run it, and what is planned. Bugs the tests find go in [BUGS.md](BUGS.md).

## How to run

**Python (trip generator):** needs `pip install pytest` once. From the project folder:
```
python -m pytest Tools/TripGenerator/tests -v
```
Runs in a few seconds and doesn't need Unreal. A test marked `xfail` flags an open bug from BUGS.md: it is expected to fail, and
it turns into an error (`XPASS(strict)`) once the bug is fixed, as a reminder to remove the marker.

**Unreal (C++ automation):** _planned, see sections 2–3 below._ Headless:
`UnrealEditor-Cmd.exe CarDigitalTwins.uproject -ExecCmds="Automation RunTests CarDigitalTwins; Quit" -nullrhi -unattended -log`.
Needs a compiled editor, so don't run it while another session is building.

## Conventions
- Python tests live in `Tools/TripGenerator/tests/`; Unreal tests will live in `Source/CarDigitalTwins/Tests/`, so test work never
  edits the same files as feature work.
- Unreal test names start with `CarDigitalTwins.` (e.g. `CarDigitalTwins.Telemetry.FileReceiver.Looping`).
- Tests don't depend on the Jeep assets (`Content/Jeep/` is local to one laptop): build synthetic actors, write temp trip files.
- Timing windows come from SPEC.md; if SPEC changes, change the test in the same commit.

## 1. Trip generator (Python, pytest): done 2026-10-07

File: `Tools/TripGenerator/tests/test_trip_generator.py`.

| # | Test | Checks |
|---|------|--------|
| P1 | Determinism | Seed 42 reproduces the committed `Data/Trips/trip_sample.json` byte for byte; another seed gives a different trip |
| P2 | Committed sample valid | `validate_trip` passes; 1900 frames, 10 Hz, schemaVersion 1, `jeep-01`, seed 42 |
| P3 | Other rates | 5 Hz and 20 Hz validate with the right frame count; 3 Hz and 7 Hz validate (BUG-001, fixed 2026-10-07) |
| P4 | Validator rejects bad input | Missing header key, wrong key order, `seq` gap, off-grid time, negative speed, gear 7, throttle 101, fuel −1, missing tyre key, unknown `openings` bit |
| P5 | Physics helpers | `wheel_rpm_from_speed`, `engine_rpm_for_gear` against hand-computed values; `ease_toward` independent of step size |
| P6 | Incident timing (SPEC §2.2) | Coolant warning ~91 s, critical ~119 s; RR tyre warning ~148 s, never critical; door FL 173 s, hood 178 s; only RR leaks |
| P7 | Physical sanity | Speed ≥ 0; rpm 0 only while parked; gear 0 when stopped; odometer never decreases; throttle and brake never both on; steer within ±40°; fuel never rises; phase order |
| P8 | Output format | One frame per line, LF endings, reads back with `json.load` equal to the generated trip |

## 2. Telemetry (Unreal C++): planned

| # | Test | Checks |
|---|------|--------|
| U1 | Load the real sample | 1900 frames, 10 Hz, first frame non-zero (catches misspelt JSON keys, which convert silently to 0) |
| U2 | Field mapping | One frame with distinct values in all 14 fields lands in the right `FVehicleTelemetry` members |
| U3 | Rejection | Missing file, invalid JSON, schemaVersion 2 / missing, no frames, `rateHz` 0, non-increasing `sampleTimeS` |
| U4 | Playback timing | 60 fps for 1 s → exactly 10 frames, in order, no `seq` gaps |
| U5 | Hitch | One 5 s poll → 50 frames, none skipped |
| U6 | Looping | Last frame held one period, wrap to frame 0, 3 loops = 3 × N frames, no drift |
| U7 | Speed multiplier | ×10 → 100 frames/s; ≤ 0 delivers nothing; change mid-playback |
| U8 | Lifecycle | Poll before start / after stop delivers nothing; double stop safe; restart from frame 0 |
| U9 | Helpers | `GetLoopDurationSeconds` with rate 0, `IsOpen` per bit, display name, settings defaults |

## 3. `AVehicleTwinActor` (Unreal C++, synthetic actor): planned

| # | Test | Checks |
|---|------|--------|
| V1 | Wheel setup | 4 tagged hubs → 4 wheels; missing tag → 3 + warning |
| V2 | Spin maths | Degrees = speed / radius over time, wraps within 0–360 |
| V3 | Steer | Front only, + = right |
| V4 | Pivot | Wheel centre stays in place while spinning (regression for `9f64240`) |
| V5 | Setup idempotent | Re-running setup mid-spin resets to the authored pose, same radius |
| V6 | Tyre radius | From vertices of a known cylinder; bounds fallback |
| V7 | Status colour | Blend clamp, per-group scale, slots without `StatusColor` untouched, no stacked instances |
| V8 | Status outline | Warning/Critical turn custom depth on with stencil 1 on all meshes (child actors too); Normal turns it off; same status twice touches no mesh |
| V9 | Telemetry blend | Alpha 0 → 1 over the arrival interval; trip loop snaps to latest; several samples in one frame keep the last real interval |

## 4. Later (with the roadmap)
- Day 7 `UTelemetrySubsystem`: receiver chosen from settings, one `OnTelemetryUpdated` per sample.
- Day 8 status thresholds (SPEC §3): table-driven edges, hysteresis, battery ignored with engine off.
- Day 9 relay (pytest): `--drop-percent`, `--loop`, envelope format.
- Day 11 MVVM (`CarDigitalTwins.MVVM.Core`): field masks, tolerance, once-per-frame flush, subscribe fires immediately, RAII unsubscribe.
- Day 8 end-to-end: sample trip at ×50 turns the car amber, then red.
