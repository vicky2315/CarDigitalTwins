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

## Day 3: 2026-09-26

**Tried:** imported the Jeep STEP twice with different Datasmith tessellation settings and compared cost and looks.
Measured on the personal laptop (RTX 3070 Ti Laptop), same scene, the other car hidden.

| Setting | Chord Tolerance | Max Edge Length | Normal Tolerance | Stitching |
|---------|-----------------|-----------------|------------------|-----------|
| A (coarse, default) | 0.2 cm | 0 (off) | 20° | Heal |
| B (fine) | 0.05 cm | 0 (off) | 10° | Heal |

| Measurement | A | B | Change |
|-------------|---|---|--------|
| Triangles drawn per frame (`stat rhi`) | 1.4 M | 2.9 M | ×2.07 |
| Draw calls (`stat rhi`) | 460 | 500 | +9% |
| `Jeep Wheel` mesh triangles (Static Mesh Editor) | 65,000 | 125,169 | ×1.9 (×5 instances ≈ +300 k) |

**Looks:** with a glossy test material (metallic 1, roughness 0.1) B was slightly smoother on the wheel rims and identical everywhere else.
The default matte material hides faceting completely; wireframe (Alt+2), Lighting Only, or a glossy material is needed to see it.

**Learned:**
- `stat rhi` "Triangles drawn" counts every pass (depth prepass, base pass, each shadow cascade), so it is several times the mesh
  triangle count. Stat commands measure the whole view, not the selected actor; `stat none` hides them all.
- Tessellation changes GPU cost (triangles) but not CPU cost (draw calls): it adds triangles inside meshes, not new meshes or
  material sections. The small draw-call difference is most likely camera/culling; use a camera bookmark (Ctrl+1) for repeatable shots.
- For this car the draw calls (~460 for 41 bodies × material sections × passes) matter more than triangles; Day 4 merges address that.
- **Datasmith can retessellate individual meshes** (right-click mesh → Datasmith → Retessellate) from the stored CAD data, so one
  part can get finer settings without re-importing the whole car. Do it before editing a mesh: retessellation rebuilds it and
  discards splits/merges. Tessellation must be settled before Day 4 cleanup for the same reason.

**Decision:** setting A everywhere, including the wheels. B doubles the frame's triangles for no visible gain except slightly smoother
rims, and upgrading only the wheel would cost ≈ 300 k extra triangles (60 k × 5 instances) for a barely visible difference.
If the rims look faceted in the demo video, retessellate just `Jeep Wheel`.

## Editor startup: shader compile time (2026-10-03)

**Tried:** compared this project with another UE 5.7 project (bluetide) whose editor starts much faster on the same machine.

**Broke:** nothing broken, but every editor start here spent a long time compiling shaders before the editor opened.
**Learned:**
- The cause was not a cache trick. Both projects use the same engine and the same shared Zen DDC (`%LOCALAPPDATA%\UnrealEngine\Common\Zen`).
  The fast project has **no `[/Script/Engine.RendererSettings]` section**, so it runs on engine defaults where the expensive features
  are off. This project came from the full template, which turns them on.
- Every enabled rendering feature adds shader permutations to **every material**, so the cost multiplies with material count.
  Engine C++ defaults (checked in the 5.7 source): `r.RayTracing` 0, `r.Substrate` 0, `r.SkinCache.CompileShaders` 0,
  `r.Shadow.Virtual.Enable` 0, `r.GenerateMeshDistanceFields` 0. `r.PathTracing` defaults to 1 but only compiles anything when ray tracing is on.
- `r.RayTracing` is the biggest lever: it pulls in ray tracing, path tracer and hardware Lumen permutations, and forces skin cache shaders on.
- Renderer project settings like these need an editor restart, and the first start after changing them still compiles once.

**Decision:** in `DefaultEngine.ini` turned off `r.RayTracing`, `r.PathTracing`, `r.Lumen.HardwareRayTracing`, `r.Substrate` (car uses
plain Datasmith materials) and `r.SkinCache.CompileShaders` (no skeletal meshes). Kept Lumen (now software), virtual shadow maps and mesh
distance fields (software Lumen needs them). Trade-off: glossy paint reflections are slightly worse without hardware ray tracing, which also
frees GPU for Pixel Streaming (SPEC.md §7). If the demo video needs better reflections, re-enable hardware ray tracing for that capture only.
_Startup time before/after not measured yet._

## Unsaved Modeling Mode meshes (2026-10-03)

**Broke:** opening the project, the split doors/hood/tailgate/glass from Day 4 (2026-09-27) were gone: the level showed actors with no
static mesh.
**Learned:**
- Modeling Mode tools (PolyGroup split, Merge) create **new assets**, by default in `_GENERATED/<user>/` next to the current level, here
  `/Game/Maps/_GENERATED/vigne/`. They were never in `/Game/Jeep/Cleaned` as I assumed.
- **Ctrl+S saves only the current level**, not new or modified assets. The level was saved with 1119 references to meshes that existed only
  in memory; closing the editor without "Save All" dropped them. Use **Save All** (Ctrl+Shift+S) after Modeling Mode work.
- On 2026-09-26 the output folder had been typed as a disk path (`E:\...\Jeep\Cleaned`), which is not a content path: it created an empty
  `Jeep/Cleaned` folder at the repo root and the assets never landed in `Content/`. Asset locations must be `/Game/...` paths.
- Git was not the cause: GitHub Desktop's stash during the 2026-10-01 pull didn't include untracked files (checked the dangling stash commits).
- **Recovery:** the editor autosaves dirty packages to `Saved/Autosaves/<package path>/<Name>_AutoN.uasset`. Copying the newest autosave of
  each package back to `Content/<package path>/<Name>.uasset` (with the editor closed) is what the editor's own "Restore packages" does.
  All 1119 referenced meshes had an autosave from 2026-09-27 21:26; after restoring, **Save All** turns them back into normal saved assets.
- Moving assets must happen **inside the editor** (Content Browser move + Fix Up Redirectors), never in Explorer: the editor rewrites the
  references in the level; a file-system move leaves them pointing at the old path.

**Decision:** Modeling Mode asset location set to `/Game/Jeep/Cleaned`; Save All after every modeling session; set up the private asset repo
(ASSETS.md) so the Jeep work exists somewhere other than one laptop's `Saved/` folder.
