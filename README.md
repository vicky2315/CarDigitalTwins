# Vehicle Digital Twin

A single car in Unreal Engine 5.7 driven by recorded / streamed telemetry, with a live dashboard, measured latency and browser delivery via Pixel Streaming.

> Work in progress. See [PROGRESS.md](docs/PROGRESS.md).

## What and why
_TODO (Day 16)_

## Architecture
_TODO: diagram (receiver → subsystem → actor / ViewModel → dashboard → Pixel Streaming)._ See [SPEC.md](docs/SPEC.md).

## How to run
### File mode
_TODO (Day 7)_

### Relay mode
_TODO (Day 10)_

## Performance highlights
_TODO: from [PERFORMANCE.md](docs/PERFORMANCE.md) (Day 12)._

## Assets and licenses
See [docs/ASSETS.md](docs/ASSETS.md). Raw CAD files are not committed.

## Roadmap
- Optional one-off cloud deployment (Pixel Streaming currently local only; see [docs/COSTS.md](docs/COSTS.md))
- MQTT receiver behind `ITelemetryReceiver`
- Phase B: fleet → SUMO traffic → Cesium city
- Phase C: OpenUSD export + Omniverse Kit extension on the same relay stream
