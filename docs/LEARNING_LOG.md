# Learning Log

What I tried, what broke, what I learned.

## Day 1: 2026-09-25

**Tried:** created the C++ project on UE 5.7, enabled DatasmithCADImporter, PixelStreaming2 and ModelViewViewModel.

**Broke:** debugging with the `UnrealBuildTool` run configuration built successfully but never launched the editor.
**Learned:** UnrealBuildTool is Epic's C# build program; debugging it just builds UBT. To launch the editor, run the
`CarDigitalTwinsEditor` target in `DevelopmentEditor | Win64` (or `DebugGame Editor` for accurate stepping through my own code).

**Decision:** custom MVVM (option B) instead of Epic's MVVM plugin; plugin kept for a Day 12 comparison. See SPEC.md §5.
Epic's MVVM is two layers:
- `FieldNotification` (engine runtime module): `INotifyFieldValueChanged`, `FieldNotify` specifier, per-field IDs and delegates.
- `ModelViewViewModel` plugin (Beta in 5.7): `UMVVMViewModelBase`, compiled bindings (`UMVVMViewClass`), `UMVVMView` widget extension,
  binding modes, conversion functions, ViewModel resolvers.

**Notes:** PixelStreaming2 is the current plugin in 5.7; the older PixelStreaming plugin is legacy.

**Decision:** $0 budget, no cloud hosting. Keeping a g4dn.xlarge (Windows, Mumbai) available is ~$60/month at 2 h/day or
~$415/month always on; the cost comes from keeping it available, not from a single test. Pixel Streaming stays local (RTX 3070 Ti Laptop
has NVENC); Day 14 becomes a deployment design. A one-off deploy-measure-terminate session (~$2–5) is kept as a future option.
See COSTS.md.
