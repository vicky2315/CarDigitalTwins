# Specification

## 1. Part mapping table
_TODO (Day 4)_

| UE component | Telemetry field | Behaviour | Future USD prim path |
|--------------|-----------------|-----------|----------------------|

## 2. Telemetry schema
_TODO (Day 5): fields with units, `trip.json` format, stream message format, `schema_version` policy._

## 3. Derived status thresholds
_TODO (Day 5)_

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

**Open questions:** Blueprint exposure of bindings; how conversion (value → text/colour) is expressed.

## 6. MVVM data-flow diagram
_TODO (Day 11)_
