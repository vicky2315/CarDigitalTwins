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

1) Why .uasset files need Git LFS: partly right

You're right that the files can be large. The deeper reason is that they're binary, and that matters even when a file is small:

- Git can't diff or merge binary files. With a .cpp file, Git stores the lines that changed. With a .uasset, every save is an unreadable blob, so Git stores a full new copy each time.
- Git keeps every version forever, and every clone downloads the whole history. Save a 50 MB mesh 20 times and the repo holds about 1 GB for that one asset, even though the current file is only 50 MB.
- What LFS changes: Git stores only a small pointer file, and the real files live on the LFS server. A clone downloads only the versions it checks out.
- Merge conflicts: because two people's edits to a .uasset can't be merged, LFS also offers file locking, where one person checks the file out and others wait. Studios rely on this.

Game analogy: saving a full-world snapshot every time you autosave, instead of saving only what changed. The save folder grows even if the world itself never gets bigger.

2) Why Pixel Streaming needs NVENC

Here's what happens on every frame:
1. UE renders the frame on the GPU.
2. The frame is compressed into video (H.264, H.265 or AV1). Sending raw 1080p frames would need about 3 Gbps, and video brings that down to about 10 Mbps.
3. WebRTC sends the compressed video to the browser.

Step 2 has to happen 30–60 times a second, in real time. There are two ways to do it
- Encode on the CPU: it's heavy, it competes with your game thread, and the finished frame first has to be copied from GPU memory to the CPU, which adds latency.
- Encode with NVENC: NVENC is a separate hardware block on NVIDIA GPUs built only foe frame straight from GPU memory and barely affects rendering or latency.

Game analogy: recording gameplay with OBS. The x264 (CPU) encoder drops your FPS, whhadowPlay, costs almost nothing. Pixel Streaming is the same thing running as a livestream.

A correction to the roadmap: "NVIDIA specifically" is stronger than it needs to be. I believe Pixel Streaming can also use AMD's hardware encoder (AMF) and has software codec fallbacks, but I
haven't checked this for 5.7. You can confirm it in the PixelStreaming2 plugin settilly about is hardware encoding, and NVIDIA is simply the most common andbest-supported option, especially on cloud GPUs like the T4. Your RTX 3070 Ti has NVENC, so you're covered either way.


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
