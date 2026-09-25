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

## Day 2: 2026-09-25

**Tried:** shortlisted three GrabCAD cars (Jeep Wrangler Rubicon, Jaguar Mark 2, Honda Civic Type-R) against format, part structure
and license; chose the Jeep. Test-imported the STEP with Datasmith defaults into a scratch level.

**Broke:** the first import showed only a tube-frame chassis, upside down. The downloaded file was a different GrabCAD model
(Goat Built IBEX chassis), not the Jeep.
**Learned:** check the file name / folder before blaming the importer. The import itself (hierarchy, materials) worked.

**Broke:** doors, hood and tailgate are one body (`Split1[2]`). *Split* by Mesh Topology / Vertex Overlap / Material ID gives
"1 of 1 Input Meshes cannot be Split"; by PolyGroup over-splits (~3 pieces per door).
**Learned:**
- A CAD *part* can hold many *bodies*; Datasmith imports each body as its own mesh. Hiding a part in a viewer hides all its bodies,
  so "one part" does not mean "one mesh". Counting `MANIFOLD_SOLID_BREP` entities in the STEP showed 41 bodies in 16 parts.
- Datasmith gives each CAD face its own PolyGroup, so PolyGroup split yields surface patches, not physical parts.
- "Cannot be Split" means the mesh is one connected piece: the panel seams are modelled as lines, not gaps.
  Plan for Day 4: PolyGroup split, then Merge per door.
- The wheel part has 5 instances: the tailgate spare must be excluded from wheel spin.

**Decision:** GrabCAD models are non-commercial and need author credit + link. Raw CAD stays out of git; whether imported car meshes
go in the public repo is still open (see ASSETS.md).
