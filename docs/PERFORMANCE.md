# Performance

Measured figures only. Every number says what was measured, on which machine and when. Figures that need the car are marked; the car
(`Content/Jeep/`) is only on the personal laptop, so they come from there.

## Summary

| Figure | Value | Measured |
|--------|-------|----------|
| Frame time, studio + dashboard (no car) | **5.0 ms median** (199 fps), p99 8.0 ms | Office machine, 2026-10-30 |
| Telemetry latency, relay send → UE game thread | **2.9 ms average**, 0 of 1,431 messages dropped | Office machine, 2026-10-30 |
| Car alone: triangles drawn / draw calls | **1.4 M / ~460** per frame | Personal laptop, 2026-09-26 (Day 3) |
| Automated tests | **101 pass**: 41 Unreal (M1–M11, U1–U9, V1–V10, S1–S11) + 60 Python (P, R) | Office machine, 2026-10-30 |

## Machines

| Name | CPU | GPU | Used for |
|------|-----|-----|----------|
| Office machine (`DESKTOP-MRMGNPL`) | AMD Ryzen 5 7600X | NVIDIA GeForce RTX 4060 Ti 8 GB (the RHI's chosen adapter; the CPU's integrated Radeon is not used) | Frame time, latency, tests |
| Personal laptop (`VigneshSuresh`) | not recorded | NVIDIA GeForce RTX 3070 Ti Laptop | Car triangles and draw calls |

## Frame time (office machine, 2026-10-30)

`L_TwinStudio` (floor, three rect lights, post-process, four view cameras, the Twin Ops dashboard) without the car, UE 5.7.4
Development, standalone game (`-game`), 1920×1080 windowed, uncapped (no vsync or frame cap set; 199 fps median shows none applied), relay streaming the live
trip at 10 Hz. CSV profiler, 20,000 frames; the first 1,200 (load and shader warm-up) left out, leaving 101 s.

| Timing | Median | Mean | p95 | p99 | Max |
|--------|--------|------|-----|-----|-----|
| Frame | 5.02 ms | 5.38 ms | 7.10 ms | 7.98 ms | 28.34 ms |
| Game thread | 1.71 ms | 1.79 ms | 2.44 ms | 2.96 ms | 23.81 ms |
| Render thread | 5.14 ms | 5.37 ms | 7.72 ms | 8.93 ms | 26.77 ms |
| RHI thread | 2.97 ms | 3.13 ms | 4.59 ms | 5.37 ms | 22.89 ms |
| GPU | 4.66 ms | 4.99 ms | 6.58 ms | 7.42 ms | 8.49 ms |

Draw calls 117 median (125 max), primitives drawn 2,060 median (2,552 max): the studio and dashboard without the car. The render thread,
not the GPU or the game thread, sets the frame time here. The game thread, which runs telemetry, status, the ViewModels and the widget
updates, takes 1.7 ms.

## Telemetry latency (office machine, 2026-10-30)

Same run. Latency is part (a) of SPEC.md §4: UE's receive time minus the relay's `sentUnixMs`, both on one clock. It is taken when the
game thread parses the message, so it includes the wait in the socket queue until the next frame (at most one frame, ~5 ms here). With
`--source live` the relay runs the vehicle simulator, so a message is sent as soon as its sample is generated.

- **2.9 ms average** over 1,314 messages in steady state. The once-a-second log keeps a running average since the connection opened,
  so the steady value comes from two log lines: average × count at the end minus at a start point, over the messages in between.
  Start points 10, 20 and 30 s in all give 2.91–2.93 ms.
- **Max 670 ms**, once, in the first second: the game's start-up hitch while the socket had already connected. It stays as the
  connection's max (the running max isn't reset per window).
- **0 dropped** of 1,431 messages, Live the whole run.

## Car: triangles and draw calls (personal laptop, 2026-09-26, Day 3)

Datasmith tessellation setting A (chord 0.2 cm, normal 20°), `stat rhi`, the car alone in a scratch level, before the dashboard existed:
**1.4 M triangles drawn** per frame (all passes: depth prepass, base pass, shadow cascades) and **~460 draw calls**. The finer setting B
measured 2.9 M and ~500. Details in LEARNING_LOG.md (Day 3) and SPEC.md §7.

## Tests (office machine, 2026-10-30)

| Suite | Count | Result |
|-------|-------|--------|
| Unreal automation, `CarDigitalTwins` (headless, `-nullrhi`) | 41 | 41 pass |
| Python, `python -m pytest Tools` | 60 | 60 pass |

What each test checks: TESTS.md.

## How these were measured

**Frame time and latency.** Relay first, then the game:
```
python Tools/Relay/relay.py --source live --loop --rate 10
UnrealEditor.exe CarDigitalTwins.uproject /Game/Maps/L_TwinStudio -game -windowed -ResX=1920 -ResY=1080
  -ExecCmds="r.GPUCsvStatsEnabled 1,csvprofile frames=20000" -log
```
Needs `TelemetrySource=WebSocket` (Project Settings > Vehicle Telemetry). The CSV lands in `Saved/Profiling/CSV/`; columns used:
`FrameTime`, `GameThreadTime`, `RenderThreadTime`, `RHIThreadTime`, `GPUTime`, `RHI/DrawCalls`, `RHI/PrimitivesDrawn`. Latency comes
from the `LogVehicleTelemetry: Telemetry:` lines in the log.

**Don't pass `-csvGpuStats` on the command line** (UE 5.7.4): the game crashes at start with `Assertion failed:
GRHIGlobals.IsRHIInitialized ... IsRayTracingAllowed() may only be called once RHI is initialized`. Start the capture from the console
after start-up instead, as above.
