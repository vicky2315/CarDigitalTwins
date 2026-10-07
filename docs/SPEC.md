# Specification

## 1. Part mapping table (option A, 2026-10-04)

`BP_VehicleTwin` (`/Game/Jeep/Blueprints`, parent `AVehicleTwinActor`) is harvested from the untouched Datasmith import. C++ finds
parts by **component tag**, never by name or asset path. Mesh names are CAD feature names (`Combine3` = wheel, etc.).

| UE component | Tag | Telemetry field | Behaviour | Future USD prim path |
|--------------|-----|-----------------|-----------|----------------------|
| `CarRoot` | | | Orientation fix for the CAD axes: rotation X −90°, Z 90°, location Z 0. Front = +X, right = +Y | `/Vehicle` |
| `WheelHub_FL` / `_FR` | `Wheel.FL` / `Wheel.FR` | `speedKmh`, `steerDeg` | Spin from speed ÷ measured tyre radius; yaw = steer | `/Vehicle/Wheels/FL`, `/FR` |
| `WheelHub_RL` / `_RR` | `Wheel.RL` / `Wheel.RR` | `speedKmh` | Spin only | `/Vehicle/Wheels/RL`, `/RR` |
| `Rim_XX`, `Tyre_XX` (`Combine3` / `Combine4`, child actors from Harvest) | `Tyre` on the tyre | | Follow their hub | `/Vehicle/Wheels/XX/Rim`, `/Tyre` |
| `Rim_Spare`, `Tyre_Spare` | | | Static (tailgate spare, never spins) | `/Vehicle/Body/SpareWheel` |
| Red body panels (25 meshes, `MI_CarPaint`) | `Paint` | derived status (§3) | Status colour, blend scale 1 | `/Vehicle/Body/Paint` |
| Black parts: bumpers, flares, grille, handles etc. (`MI_CarPaintBlack`) | `Trim` | derived status (§3) | Status colour, blend scale 1 | `/Vehicle/Body/Trim` |
| Doors, hood, tailgate (inside the body mesh) | | `openings` | Dashboard only; animation optional later | `/Vehicle/Body/Door_FL` … |

**Hubs:** added in the Blueprint at 0,0,0 and never moved. On setup `AVehicleTwinActor` resets each hub and its children to the
Blueprint transforms, then measures in hub space: wheel centre = tyre bounds centre, axle = left → right wheel centre, radius = farthest
tyre vertex from the axle. Spin and steer rotate the hub about the wheel centre. Mesh pivots (at the vehicle origin) and the 5× shared
wheel meshes are left as imported.

**Child actors:** Harvest Components wrapped every imported mesh actor in a `ChildActorComponent`. Tags sit on that component; the code
follows it to the child actor's mesh. Body and trim components keep Harvest's default names (`StaticMeshActor_N`), so the tags, not the
names, define the groups. Mirrors, headlights and the inner grille have no paint group yet (cosmetic materials: Day 15).

**Status colour (paint groups):** `PaintGroups` on `AVehicleTwinActor` lists the tags that take the status colour, each with a blend
scale (default `Paint` 1, `Trim` 1). `SetStatusColor(Color, Blend)` sets `StatusColor` and `StatusBlend × scale` on every material slot
of those meshes whose material has a `StatusColor` parameter; other slots are left alone.

**Status outline (shown in play, decided 2026-10-07):** the red body paint hides a red tint, so the derived status (§3) is shown as a
glowing, pulsing outline around the whole car instead: Warning amber at 0.5 Hz, Critical red at 2 Hz, Normal none.
`SetStatusOutline(Status)` sets custom stencil value 1 (`StatusOutlineStencilValue`) on every car mesh (child actors included), but
only while the status isn't Normal, because custom depth draws the car a second time (§7). The actor makes its own unbound
post-process component with a dynamic instance of `PP_VehicleStatusOutline` (`/Game/Materials`, Post Process, Scene Color Before
Bloom) and sets `StatusOutlineColor` and `StatusOutlineGlowIntensity` (HDR, pulsed) every frame. Needs `r.CustomDepth=3` (Enabled with
Stencil). Material: four `CustomStencil` taps `StatusOutlineWidthPixels` apart → outline = neighbour stencil × (1 − centre stencil);
Emissive = `PostProcessInput0` + outline × colour × intensity. `SetStatusColor` stays for later use (e.g. the HMI view).

**Telemetry binding:** in play `AVehicleTwinActor` subscribes to `UTelemetrySubsystem::OnTelemetryUpdated` only to timestamp arrivals,
then blends previous → latest sample (speed, steer) over the last arrival interval into `UpdateWheels`, one sample behind. A trip
loop (`sampleTimeS` going backwards) snaps. Status is polled from `GetCurrentVehicleStatusReport()`, never blended.

Materials (`/Game/Materials`): one master `M_CarPaint` with `PaintColor`, `StatusColor`, `StatusBlend` (0–1), `Metallic`, `Roughness`;
Base Color = Lerp(`PaintColor`, `StatusColor`, `StatusBlend`). Instances: `MI_CarPaint` (red body), `MI_CarPaintBlack` (black trim:
`PaintColor` ≈ 0.02, `Metallic` 0, `Roughness` 0.5). New colours are instances of `M_CarPaint`, never copies of it.

## 2. Telemetry schema (v1, drafted 2026-09-28)

C++: `FVehicleTelemetry` in `Source/CarDigitalTwins/Telemetry/VehicleTelemetry.h`.

### 2.1 Fields (one sample)
Units live in the name, so a value can't be misread without the docs.

| JSON key | C++ | Type | Unit / range | Used by |
|----------|-----|------|--------------|---------|
| `seq` | `Seq` | int64 | +1 per sample | Dropped-message count (Day 10) |
| `sampleTimeS` | `SampleTimeS` | double | s since trip start | Interpolation (Day 8) |
| `speedKmh` | `SpeedKmh` | float | km/h, ≥ 0 | Wheel spin, dashboard |
| `engineRpm` | `EngineRpm` | float | rpm, 0 = engine off | Dashboard, status |
| `gear` | `Gear` | int32 | −1 R, 0 N, 1..6 | Dashboard |
| `throttlePct` | `ThrottlePct` | float | 0..100 | Dashboard |
| `brakePct` | `BrakePct` | float | 0..100 | Dashboard, brake lights (later) |
| `steerDeg` | `SteerDeg` | float | front road-wheel angle, + = right (UE yaw) | Front wheel yaw |
| `odometerKm` | `OdometerKm` | double | km | Dashboard |
| `coolantTempC` | `CoolantTempC` | float | °C | Status (overheating incident) |
| `fuelPct` | `FuelPct` | float | 0..100 | Dashboard, status |
| `batteryV` | `BatteryV` | float | V | Status |
| `tyreKpa` | `TyreKpa` | object `{fl, fr, rl, rr}` | kPa (gauge) | Status |
| `openings` | `Openings` | int bitmask | `EVehicleOpening`: 1 DoorFL, 2 DoorFR, 4 DoorRL, 8 DoorRR, 16 Hood, 32 Tailgate | Door/hood/tailgate animation |

**Not sent, derived in UE:** wheel angular speed (from `speedKmh` and the tyre radius measured on the mesh, so there is one
source of truth), status (§3), receive time and latency (Day 10).

**Why these choices:**
- **camelCase JSON keys** equal to the C++ property names with the first letter lowered: `FJsonObjectConverter` matches keys
  that way (case-insensitively), so Day 7 needs no hand-written parser. `speed_kmh` would not match.
- **`openings` as a bitmask** instead of six bools: one int, trivial in Python (`DoorFL | Hood`), and avoids the `b` prefix that UE
  bool properties need (`bDoorFL` would have to be the JSON key). Rear-door bits stay unused if the Jeep turns out to be 2-door (Day 4).
- **No position (lat/lon/heading)** in v1: the single car stays in place. Phase B adds them as optional fields (no version bump).
- **`sampleTimeS` is trip time, not wall clock**, so recorded trips replay identically; wall-clock send time sits in the stream envelope.

### 2.2 `trip.json` (recorded trip, Day 6 writes, Day 7 reads)
One JSON object: a header plus fixed-rate frames. Each frame is exactly a §2.1 sample.
```json
{
  "schemaVersion": 1,
  "vehicleId": "jeep-01",
  "rateHz": 10,
  "seed": 42,
  "frames": [
    {"seq": 0, "sampleTimeS": 0.0, "speedKmh": 0.0, "engineRpm": 780, "gear": 0, "throttlePct": 0, "brakePct": 100,
     "steerDeg": 0, "odometerKm": 12450.2, "coolantTempC": 88.5, "fuelPct": 72, "batteryV": 14.1,
     "tyreKpa": {"fl": 240, "fr": 241, "rl": 238, "rr": 240}, "openings": 0}
  ]
}
```
**Generator (Day 6):** `Tools/TripGenerator/trip_generator.py` (`--seed`, `--rate`, `--out`, `--plot`) writes
`Data/Trips/trip_sample.json`: 190 s at 10 Hz (1900 frames, ~570 KB), seed 42, one frame per line so git diffs stay readable.
It validates every frame against §2.1 and prints when §3 thresholds are crossed. Phases: idle 10 s → accelerate 20 s → cruise 115 s →
brake 15 s → idle 10 s → parked 20 s (engine off). Incidents: cooling failure at 70 s (coolant warning ~91 s, critical ~119 s, the
driver pulls over); rear-right slow puncture from 40 s (warning ~148 s, ends ~150 kPa, above critical); door FL opens at 173 s, hood at
178 s. rpm uses the Wrangler 6-speed ratios, final drive 4.10 and the 42.2 cm tyre radius measured in UE.

Size check: ~300 bytes/frame × 10 Hz × 30 min ≈ 5.4 MB. Fine to parse at load. If trips get long, switch to JSON Lines (one
frame per line) so the relay can stream instead of loading the whole file.

### 2.3 Stream message (WebSocket relay, Day 9/10)
One JSON object per message, one sample per message:
```json
{"type": "telemetry", "schemaVersion": 1, "vehicleId": "jeep-01", "sentUnixMs": 1790577294123, "frame": { ...§2.1 sample... }}
```
- `type` leaves room for other messages (`"hello"`, `"tripStart"`) without a version bump; unknown types are ignored.
- `sentUnixMs` is stamped by the relay at send time. UE subtracts it from its receive time for latency part (a). Both run on
  the same machine, so clocks agree; across machines this would need clock sync.
- Gaps in `frame.seq` = dropped messages; `seq` going backwards = trip looped (`--loop`), not an error.

### 2.4 `schemaVersion` policy
- Integer, one number (no minor). Current: **1** (`VehicleTelemetrySchemaVersion` in C++).
- **No bump:** adding an optional field. Readers ignore unknown keys and keep defaults for missing ones.
- **Bump:** renaming or removing a field, changing a unit or meaning, changing the envelope.
- UE on mismatch: log one warning, reject the trip / drop the messages, show the connection as errored rather than
  displaying wrong values.

## 3. Derived status thresholds (drafted 2026-09-28)

Computed in UE per sample; overall status = worst of the rows. Numbers are plausible for a 2010 Wrangler (3.8 L V6, 6-speed)
but are **demo values, not manufacturer data**.

| Signal | Warning | Critical | Clear warning when | Notes |
|--------|---------|----------|--------------------|-------|
| `coolantTempC` | ≥ 105 | ≥ 115 | < 102 | Normal running ~90–100. Drives the overheating incident (amber → red, Day 8). |
| `tyreKpa` (any wheel) | < 180 or > 300 | < 140 | ≥ 185 / ≤ 295 | Nominal 240 kPa (35 psi). 180 = 25 % under, the usual TPMS trigger. |
| `fuelPct` | < 15 | < 5 | ≥ 17 | |
| `batteryV` (engine running, rpm > 0) | < 13.0 or > 14.8 | < 12.0 or > 15.5 | 13.2..14.6 | Below ~13 V while running = not charging. Ignored with engine off. |
| `engineRpm` | > 5500 | > 6200 | < 5300 | Near the rev limit. |

**Hysteresis:** a signal enters Warning at the threshold but only clears a few units back (the "clear" column). At 10 Hz with
noisy data, a value hovering at 105 °C would otherwise flip amber/normal several times a second. Critical → Warning uses the
same gap. Not applied to stale data: connection staleness is §4, separate from vehicle health.

**Critical clears at** (same gap, derived 2026-10-07): coolant < 112; tyre ≥ 145; fuel ≥ 7; battery ≥ 12.2 / ≤ 15.3; rpm < 6000.
Each row keeps its own ≥ / > exactly as in the table. The tyre row is judged on the lowest wheel (low side) and the highest wheel
(high side). Code: `FVehicleStatusEvaluator` (`Source/CarDigitalTwins/Telemetry/VehicleStatusEvaluator.h`), one status per row
plus the overall (worst) status in `FVehicleStatusReport`.

## 4. Connection states
_TODO (Day 10): Idle → Connecting → Live → Stale → Disconnected; stale threshold; backoff._

## 5. UI architecture: custom MVVM (decided 2026-09-25)

**Decision:** build our own lightweight MVVM framework (option B) instead of relying on Epic's
ModelViewViewModel plugin. The plugin stays enabled so the same dashboard can be built on it
for a measured comparison on Day 12.

**Why:**
- Learn the mechanism (observable fields, subscription, change propagation), not just the editor tooling.
- Telemetry updates ~15 fields together at 10 Hz; batching notifications into one flush per
  frame is a deliberate fit for that pattern.
- Gives PERFORMANCE.md a custom-vs-Epic comparison.

**Design direction (to be detailed on Day 11):**
- ViewModel base: fields identified by an enum; setters compare old/new and mark a field dirty.
- One change event per ViewModel carrying the set of dirty fields, flushed once per frame by the subsystem.
- Widgets bind in C++ (subscribe in `NativeConstruct`, unsubscribe in `NativeDestruct`) and never hold actor pointers.
- Data flow: `ITelemetryReceiver` → `UTelemetrySubsystem` → ViewModel → widgets.

**Decisions (2026-09-28):**
- **Two ViewModels.** `UVehicleTelemetryViewModel`: vehicle data, read-only, changes per sample. `UConnectionViewModel`:
  connection state, latency, drops, and commands (pause, playback speed, source switch).
- **Dashboard updates per sample (10 Hz)**, not per frame. Only the 3D car interpolates every frame (Day 8).
- **Formatting in widgets** via shared helpers (`FormatSpeed`, `StatusToColor`); ViewModels hold plain numbers and enums.
- **Blueprint: read + one event.** Subscribing/binding is C++; Blueprint gets getters, commands and one
  `OnViewModelChanged` event.

**Mechanics:**
- Change event carries only a field mask (which fields changed); widgets pull values through getters ("notify, then pull").
- Float fields have a per-field tolerance; changes smaller than it don't mark the field dirty.
- Subscribing fires the callback once immediately with all fields set, so widgets never start blank.
- Subscriptions are RAII handles (`FViewModelSubscription`); destroying the handle unsubscribes.
- `UViewModelSubsystem` (GameInstance subsystem) creates ViewModels on first request, keeps them alive and flushes dirty
  ones once per frame. Fields set during a flush are delivered on the next frame.
- Setters are game-thread only.
- Commands go down (widget → ViewModel → subsystem); the resulting state comes back up the normal path. Widgets never update
  their own display optimistically.

## 6. MVVM data-flow diagram

```
 JSON ─▶ Receiver ─▶ FVehicleTelemetry ─▶ UTelemetrySubsystem ──OnTelemetryUpdated──┬──▶ Car actor (interpolates every frame)
                                              ▲                                     │
                                              │ commands                            ▼
 UViewModelSubsystem ──creates/holds/flushes──▶ ViewModels (telemetry, connection) ──OnFieldsChanged(mask)──▶ Widgets
                                              ▲                                                               │
                                              └──────────────── commands (pause, speed, source) ◀─────────────┘
```
Arrows into a box = that box subscribes to / is called by the arrow's source. The telemetry subsystem knows no UI class.

## 7. Rendering budget (decided 2026-09-26)

**Tessellation:** Datasmith setting A for all parts: Chord Tolerance 0.2 cm, Max Edge Length off, Normal Tolerance 20°, Stitching Heal.

**Budget:** car ≈ 1.4 M triangles drawn per frame, ≈ 460 draw calls (RTX 3070 Ti Laptop, `stat rhi`). The finer setting doubled
triangles with no visible gain except slightly smoother wheel rims. The GPU must also run Pixel Streaming encode and the dashboard,
so triangles are kept low. Draw calls are the bigger cost for this car and are reduced by Day 4 merges.
Exceptions are made per part with Datasmith Retessellate (see LEARNING_LOG Day 3).
