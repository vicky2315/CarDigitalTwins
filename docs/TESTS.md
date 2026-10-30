# Tests

Automated tests: what exists, how to run it, and what is planned. Bugs the tests find go in [BUGS.md](BUGS.md).

## How to run

**Python (trip generator):** needs `pip install pytest` once. From the project folder:
```
python -m pytest Tools/TripGenerator/tests -v
```

**Python (relay):** needs `pip install -r Tools/Relay/requirements.txt` once (`websockets`). `python -m pytest Tools/Relay/tests -v`,
or `python -m pytest Tools -v` for all Python tests. R6 opens a local WebSocket on a free port (127.0.0.1 only, no firewall prompt).
Runs in a few seconds and doesn't need Unreal. A test marked `xfail` flags an open bug from BUGS.md: it is expected to fail, and
it turns into an error (`XPASS(strict)`) once the bug is fixed, as a reminder to remove the marker.

**Unreal (C++ automation):** sections 2, 3, 7 and 8 below (41 tests, all pass 2026-10-30). Headless:
`UnrealEditor-Cmd.exe CarDigitalTwins.uproject -ExecCmds="Automation RunTests CarDigitalTwins; Quit" -nullrhi -unattended -log`
`"-ini:EditorPerProjectUserSettings:[/Script/UnrealEd.EditorLoadingSavingSettings]:RestoreOpenAssetTabsOnRestart=NeverRestore"`.
The `-ini` override matters: without it the editor reopens the asset editors left open last session, and under `-nullrhi` that
crashes (`GetRestoredDimensions is not expected to be called on this platform`) before any test runs. It can run while the normal
editor is open. Needs a compiled editor, so don't run it while another session is building.

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
| P4 | Validator rejects bad input | Missing header key, wrong key order, `seq` gap, off-grid time, negative speed, gear 7, throttle 101, fuel −1, missing tyre key, unknown `openings` bit, unknown `driveMode` |
| P5 | Physics helpers | `wheel_rpm_from_speed`, `engine_rpm_for_gear` against hand-computed values; `ease_toward` independent of step size |
| P6 | Incident timing (SPEC §2.2) | Coolant warning ~91 s, critical ~119 s; RR tyre warning ~148 s, never critical; door FL 173 s, hood 178 s; only RR leaks |
| P7 | Physical sanity | Speed ≥ 0; rpm 0 only while parked; gear 0 when stopped; odometer never decreases; throttle and brake never both on; steer within ±40°; fuel never rises; phase order |
| P8 | Output format | One frame per line, LF endings, reads back with `json.load` equal to the generated trip |
| P9 | Engine derate (T1, SPEC §2.5) | `driveMode` Normal without a command; `set_engine_derate` returns the next seq and is idempotent; derate at Critical (119.1 s): speed ≤ 50.5 km/h, coolant < 112 °C within 20 s then stays ≥ 105 °C while running; rpm cap 2500 reached and held while accelerating; switching off restores 90 km/h |

## 2. Telemetry (Unreal C++, `CarDigitalTwins.Telemetry.FileReceiver`): written 2026-10-23

**Run 2026-10-23 (office laptop, headless): U1–U9 all pass.** `Source/CarDigitalTwins/Tests/FileTelemetryReceiverTests.cpp`. U1 reads
the committed `Data/Trips/trip_sample.json`; the rest write small trips to `Saved/Automation/TelemetryTests/` and delete them afterwards.
U3's rejection log lines are declared as expected, so they don't fail the run. Test names must not contain a dot (Unreal splits the
path there). The subsystem side (receiver chosen from settings, one `OnTelemetryUpdated` per sample) is still under §6 "Later".

| # | Test | Checks |
|---|------|--------|
| U1 | Load the real sample | 1900 frames, 10 Hz, first frame non-zero (catches misspelt JSON keys, which convert silently to 0) |
| U2 | Field mapping | One frame with distinct values in all 14 fields lands in the right `FVehicleTelemetry` members |
| U3 | Rejection | Missing file, invalid JSON, schemaVersion 2 / missing, no frames, `rateHz` 0, non-increasing `sampleTimeS` |
| U4 | Playback timing | 60 fps for just under 1 s (59 polls) → frames 0–9, in order, no `seq` gaps, never two in one poll |
| U5 | Hitch | One 4.95 s poll → 50 frames, none skipped; the next poll carries on at frame 50 |
| U6 | Looping | Last frame held one period, wrap to frame 0, 3 loops = 3 × N frames, 100 loops at 60 fps = 100 × N (no drift) |
| U7 | Speed multiplier | ×10 → 100 frames/s; ≤ 0 delivers nothing; change mid-playback |
| U8 | Lifecycle | Poll before start / after stop delivers nothing; double stop safe; restart from frame 0 |
| U9 | Helpers | `GetLoopDurationSeconds` with rate 0, `IsOpen` per bit, display name, commands refused with a reason, configured settings usable (trip file exists, ranges) |

## 3. `AVehicleTwinActor` (Unreal C++, `CarDigitalTwins.Vehicle.TwinActor`): written 2026-10-23

**Run 2026-10-23 (office laptop, headless): V1–V10 all pass.** `Source/CarDigitalTwins/Tests/VehicleTwinActorTests.cpp` on
`AVehicleTwinTestActor` (`Tests/VehicleTwinTestActor.h`): a stand-in car from engine shapes with the real tags (four hubs, cylinder
tyres scaled to 43.5 cm and offset from their hubs, Paint/Trim meshes, `CalloutAnchor.Hood`, one child actor), so no Jeep assets are
needed. Paint and outline use `MI_CarPaint`, `MI_CarPaintBlack` and `PP_VehicleStatusOutline` from `/Game/Materials`. Each test spawns
the car in its own empty game world; with no game mode, play is started through `AWorldSettings::NotifyBeginPlay`. The spec is a
`friend` of `AVehicleTwinActor` to read the wheels and call the setup steps; the V9 blend maths moved into the static
`ComputeTelemetryBlendAlpha` (it now also returns 1 for an interval of 0 instead of dividing by it).

| # | Test | Checks |
|---|------|--------|
| V1 | Wheel setup | 4 tagged hubs → 4 wheels in FL, FR, RL, RR order, only the front steer, axle +Y; missing `Wheel.RR` → 3 + warning, RL without partner falls back to the actor's Y |
| V2 | Spin maths | 36 km/h for 0.1 s = 100 cm / 43.5 cm in degrees on every wheel; positive spin moves the top of the tyre forward; 100 s at 100 km/h stays in 0–360 without losing angle |
| V3 | Steer | +20° turns the front of the FL tyre right by 20°; rear hub untouched |
| V4 | Pivot | Tyre centres stay within 0.05 cm over 12 steps of spin and ±25° steer, with the tyre offset from its hub (regression for `9f64240`) |
| V5 | Setup idempotent | Re-running setup mid-spin returns hubs to the authored pose; same radius and centre; spin back to 0 |
| V6 | Tyre radius | 43.5 cm from the cylinder's vertices; a sphere component (no vertex data) falls back to its bounds radius |
| V7 | Status colour | Blend 2 → 1 on the body, 0.5 on Trim (group scale), −1 → 0; a Paint-tagged slot without `StatusColor` untouched + warning; setup again reuses the instances |
| V8 | Status outline | Warning/Critical put every mesh, child actor included, in custom depth with stencil 1; Normal takes them out; same status twice touches no mesh; no outline material → nothing turned on |
| V9 | Telemetry blend | Alpha 0 → 0.5 → 1 (held) over the arrival interval; trip loop and interval 0 snap to latest; arrival interval from the world clock, three samples in one frame keep the last real interval |
| V10 | Callout anchors | `Wheel.FR` = FR tyre centre (not the hub), still right while spinning; `CalloutAnchor.Hood` = its component; unknown tag → false |

## 4. Relay (Python, pytest): R1–R6 done 2026-10-08, R7–R10 (commands, T1) 2026-10-10, R11 2026-10-30

| # | Test | Checks |
|---|------|--------|
| R1 | Envelope | Telemetry message keys in SPEC §2.3 order, `type`, `schemaVersion` 1, `sentUnixMs`, frame unchanged |
| R2 | Send order | Without `--loop` the frames once; with it, wraps to seq 0 forever |
| R3 | Drop share | 0% never, 100% always, 10% ≈ 1000 of 10000 (seeded) |
| R4 | Client checks | Gap counting, a loop back to seq 0 is not a gap, latency from `sentUnixMs` |
| R5 | Schema version | `load_trip` rejects `schemaVersion` 2 |
| R6 | End to end | Real relay + client on a free port at 200 msg/s: `hello` first, 20 frames in order, no gaps, client hello reaches the relay |
| R7 | Ack envelope | `commandAck` keys in SPEC §2.5 order and values |
| R8 | File vs live | File source rejects with `source is a recorded trip`, −1; live seed 42 without commands = first 50 sample frames; a client hello gets no ack; derate `applied` at the next seq (50), repeat still `applied` at 50; next frame `EngineDerate` |
| R9 | Bad / unknown | String or boolean `commandId` → `commandId` −1 `bad command message`; `schemaVersion` 2 and non-boolean `enabled` → bad message; other `name` → `unknown command`; all `appliedAtSeq` −1 |
| R10 | End to end command | Live relay at 500 msg/s, client sends derate at once: exactly one ack (`commandId` 1, `applied`), first `EngineDerate` frame seq = `appliedAtSeq` |
| R11 | Binary frame | A binary frame that isn't UTF-8 (`b""`) is ignored, not a `UnicodeDecodeError` that drops the client; a derate sent next on the same connection is still answered |

## 5. WebSocket receiver (Unreal C++): planned, Day 10

| # | Test | Checks |
|---|------|--------|
| W1 | Gap counting | `FTelemetryStreamStatistics`: 0,1,4 → 2 dropped; seq back to 0 → not a gap; `Reset` forgets the last seq |
| W2 | Latency stats | Latest / average / max over several messages; average 0 with no messages |
| W3 | Backoff | `ComputeReconnectDelaySeconds`: 0.5, 1, 2, 4, 8, 10, 10 s for attempts 1–7; jitter ±1 → ±20 %; huge attempt number doesn't overflow |
| W4 | Bad URL | `StartReceiving` with `http://…` returns false, state stays Idle |
| W5 | Live relay (manual, PIE) | See PROGRESS Day 10 checks: Live, drop count, Stale on pause, reconnect with backoff, relay started after PIE |

## 7. MVVM core (Unreal C++, `CarDigitalTwins.MVVM.Core`): written 2026-10-10, Day 11 step 6

**Run 2026-10-23 (personal laptop, headless): M1–M9 all pass.** **Run 2026-10-23 (office laptop, headless): M1–M11 all pass.**

`Source/CarDigitalTwins/Tests/ViewModelCoreTests.cpp`. No world or assets needed. Listeners bind to `OnFieldsChanged` directly
until `Subscribe` exists (step 7; M9–M10 below come with it).

| # | Test | Checks |
|---|------|--------|
| M1 | AllFields | 0, 3 and 64 fields give the right bits (64 = all ones, no undefined shift) |
| M2 | Mask operations | `AddField`, `HasField`, `HasFieldIndex` (−1 and 64 are false), `AddFields` merge |
| M3 | Field count | `GetFieldCount` = enum `Count`; `GetAllFields` matches it |
| M4 | First sample | Nothing broadcast before `Flush`; one broadcast with exactly the fields that left their default; empty flush sends nothing |
| M5 | Batching | Three samples, one `Flush`: one broadcast with speed and gear once each, getter has the latest value |
| M6 | Tolerance and drift | Coolant +0.03 °C (tolerance 0.05) not marked and getter keeps 90.00; another +0.03 is reported |
| M7 | Status and drive mode | Coolant Warning + `EngineDerate` mark overall status, coolant status and drive mode only |
| M8 | Change during flush | A listener's change during a broadcast goes out with the next `Flush`, not lost |
| M9 | Subscribe (step 7) | Fires once at once with all fields; RAII handle unsubscribes on destruction |
| M10 | Subsystem flush (step 8) | One `Tick` flushes each ViewModel with changes once (two samples + a view change → one broadcast each, with all changes); idle `Tick` sends nothing; only the changed ViewModel broadcasts |
| M11 | Subsystem lifetime (step 8) | `GetViewModel` returns one ViewModel per class, owned by the subsystem; after `Deinitialize` the old one isn't flushed and a new request makes a new one |

M10–M11: `Source/CarDigitalTwins/Tests/ViewModelSubsystemTests.cpp` (`CarDigitalTwins.MVVM.Subsystem`). The subsystem is made by hand on a
bare `UGameInstance` (no world, no telemetry source) and the test calls `Tick` in place of the engine tick; the engine registration
itself is checked in PIE.

## 8. Status evaluator (Unreal C++, `CarDigitalTwins.Telemetry.StatusEvaluator`): written 2026-10-30

**Run 2026-10-30 (office laptop, headless): S1–S11 all pass.** `Source/CarDigitalTwins/Tests/VehicleStatusEvaluatorTests.cpp`, on
`FVehicleStatusEvaluator` directly (plain C++, no world or assets). Each threshold is walked up through Warning and Critical and back
down through both clear values, one step either side of every value in SPEC §3, so a `≥` written as `>` (or the reverse) fails at that
step. Each walk also checks that the overall status equals the row under test while every other signal stays healthy. S11 reads the
committed `Data/Trips/trip_sample.json`.

| # | Test | Checks |
|---|------|--------|
| S1 | Healthy | Running car with every signal in range: all rows and overall Normal |
| S2 | Coolant | 104.9 N, 105 W, 114.9 W, 115 C, 112 C, 111.9 W, 102 W, 101.9 N |
| S3 | Tyre low (lowest wheel) | 180 N, 179.9 W, 140 W, 139.9 C, 144.9 C, 145 W, 184.9 W, 185 N (only RR changed) |
| S4 | Tyre high (highest wheel) | 300 N, 300.1 W, 400 W (no Critical side), 295.1 W, 295 N |
| S5 | Fuel | 15 N, 14.9 W, 5 W, 4.9 C, 6.9 C, 7 W, 16.9 W, 17 N |
| S6 | Battery, engine running | Low: 13.0 N, 12.9 W, 12.0 W, 11.9 C, 12.1 C, 12.2 W, 13.1 W, 13.2 N. High: 14.8 N, 14.9 W, 15.5 W, 15.6 C, 15.4 C, 15.3 W, 14.7 W, 14.6 N |
| S7 | Engine rpm | 5500 N, 5500.1 W, 6200 W, 6200.1 C, 6000 C, 5999.9 W, 5300 W, 5299.9 N |
| S8 | Hysteresis | 103 °C is Normal on the way up, Warning on the way down; 113 °C Warning up, Critical down; 90 → 120 → 90 jumps N → C → N |
| S9 | Memory | `ResetStatusMemory` forgets a Warning (103 °C then Normal); engine off ignores 11 V and clears the battery memory (13.1 V Normal after restart) |
| S10 | Worst of | Coolant Warning + fuel Critical → overall Critical, rows keep their own status; two Warnings → Warning; one tyre high + one low → tyre row Critical; `WorseStatus` order |
| S11 | Sample trip | Coolant Warning 88–94 s, Critical 116–122 s; RR tyre Warning 145–151 s, never Critical; fuel, battery and rpm Normal all trip (same windows as P6) |

## 6. Later (with the roadmap)
- Day 7 `UTelemetrySubsystem`: receiver chosen from settings, one `OnTelemetryUpdated` per sample.
- Day 8 end-to-end: sample trip at ×50 turns the car amber, then red.
