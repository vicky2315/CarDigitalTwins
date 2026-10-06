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

### Tests
`python -m pytest Tools/TripGenerator/tests` (needs `pip install pytest`). Full list and Unreal automation tests: [TESTS.md](docs/TESTS.md);
known bugs: [BUGS.md](docs/BUGS.md).

## Performance highlights
_TODO: from [PERFORMANCE.md](docs/PERFORMANCE.md) (Day 12)._

## Assets and licenses
**Vehicle model:** [2010 Jeep Wrangler Rubicon](https://grabcad.com/library/2010-jeep-wrangler-rubicon-1) by **Kostiantyn Abramov**,
from the GrabCAD Community Library. Used for non-commercial purposes under the GrabCAD Community terms. All credit for the original
model goes to the author.

The CAD files and the Unreal meshes converted from them are **not included** in this repository. To run the project, download the model
from the link above and follow the import steps in [docs/ASSETS.md](docs/ASSETS.md).

This is a non-commercial portfolio project and is not affiliated with or endorsed by Jeep or Stellantis.

## Roadmap
- Optional one-off cloud deployment (Pixel Streaming currently local only; see [docs/COSTS.md](docs/COSTS.md))
- MQTT receiver behind `ITelemetryReceiver`
- Phase B: fleet → SUMO traffic → Cesium city
- Phase C: OpenUSD export + Omniverse Kit extension on the same relay stream
