# Assets and Licenses

Raw CAD files are **not** committed (see `.gitignore`). Place them in `RawCAD/` locally and download from the source below.

## CAD shortlist (Day 2)

Selection criteria: Datasmith-importable NURBS format (STEP/IGES/native CAD, not a pre-triangulated mesh), wheels and doors as
separate parts or bodies, a license that allows a public portfolio and demo video, and a moderate part count.

| # | Name | Link | Format | Size / parts | License | Wheels separate? | Doors separate? |
|---|------|------|--------|--------------|---------|------------------|-----------------|
| 1 | 2010 Jeep Wrangler Rubicon (Kostiantyn Abramov, 2018) | [GrabCAD](https://grabcad.com/library/2010-jeep-wrangler-rubicon-1) | STEP, IGES, Parasolid, STL, SolidWorks 2015 assembly | STEP 81 MB (download 410 MB); 16 parts, 41 solid bodies | GrabCAD community (non-commercial, attribute + link) | Yes (`Jeep Wheel` × 5, incl. spare) | Partly: doors, hood and tailgate are one body (`Split1[2]`); needs splitting |
| 2 | Jaguar Mark 2 (Michel Man, 2021) | [GrabCAD](https://grabcad.com/library/jaguar-mark-2-1) | STEP, Parasolid, SolidWorks 2020 assembly | Not downloaded; assembly folder with ~15 numbered parts | GrabCAD community | Likely | Unknown |
| 3 | Honda Civic Type-R 2020 (Fawaz Bukht Majmader, 2021) | [GrabCAD](https://grabcad.com/library/honda-civic-type-r-2020-1) | IGES, SolidWorks assembly (no STEP) | Not downloaded; assembly + wheel assembly + brake caliper/plate | GrabCAD community | Yes (caliper separate, stays static) | Probably not (body is 1–2 parts) |

Rejected: single-body STEP models (Toyota Supra, Kia Sportage, Porsche 911, Tesla Model S) and models with only body + wheels
(AMG GT, Range Rover Velar, VW Polo).

## Chosen model: 2010 Jeep Wrangler Rubicon

- **Author:** Kostiantyn Abramov
- **Link:** https://grabcad.com/library/2010-jeep-wrangler-rubicon-1
- **License:** GrabCAD Community Library terms. Non-commercial use only; public non-commercial use must credit the author and
  link to the original model. Commercial use requires written permission from the author.
- **Why:** real assembly (not a single merged solid), STEP available, separate wheel part, hinge parts for doors / hood / tailgate,
  moderate part count. Boxy panels make pivot setup straightforward.

### Download
1. Log in to GrabCAD and open the link above; click **Download files** (all files).
2. Unzip to `RawCAD/2010 Jeep Wrangler Rubicon/`.
3. Import `Imported CAD/2010 Jeep Wrangler Rubicon - Assembly.STEP` with Datasmith (settings: see LEARNING_LOG Day 3).

### Structure (from the STEP file)
| Part | Instances | Bodies | Notes |
|------|-----------|--------|-------|
| 2010 Jeep Wrangler Rubicon | 1 | 18 | Main body. `Split1[1]` = shell, `Split1[2]` = doors + hood + tailgate as one body |
| Jeep Wheel | 5 | 2 | 4 road wheels + tailgate spare (spare must not spin) |
| Door Hinge | 6 | 1 | |
| Hood Hinge | 2 | 2 | |
| Tail Gate Hinge | 2 | 2 | |
| Windows | 1 | 1 | All glass in one body; door glass must be separated and follow its door |
| Interior Base, lights, grille, logos, gasoline cover | | | Static |

### Known cleanup (Day 4)
- `Split1[2]` is one connected mesh: *Split → By Mesh Topology / By Vertex Overlap* finds nothing to split
  ("1 of 1 Input Meshes cannot be Split"). *Split → By PolyGroup* works but over-splits (~3 pieces per door),
  so plan: PolyGroup split → **Merge** pieces per door / hood / tailgate.
- Same for `Windows`: split per pane, merge door glass with its door (or attach under the door pivot).
- Imported orientation is flipped relative to UE (Z-up); fix in `BP_VehicleTwin`.

## Redistribution policy
Imported car meshes derived from the CAD file are a derivative of a non-commercial, attribution-required model.
_Open decision:_ keep them out of the public repo (rebuild from the steps above) or ask the author for permission.
- 2026-10-03: attribution added to README (needed for public use regardless). A credit alone does not grant redistribution.
- Permission request to the author via GrabCAD: _not sent yet_. When answered, record date and wording here. Yes → commit meshes to the
  main repo; no / no reply → private asset repo (below).

## Syncing work-in-progress assets between machines (suggestion, 2026-09-28, not yet done)
**Problem:** `Content/Jeep/` is gitignored, so edited meshes (door split, pivots) exist only on the machine that made them.
A level pulled onto the other machine opens with empty actors, and re-saving it there can clear the missing references.
Re-importing the STEP per machine loses manual edits.

**Suggested fix (option A):** a separate **private** GitHub repo (e.g. `CarDigitalTwins-Assets`, Git LFS for `*.uasset` / `*.umap`)
cloned into `Content/Jeep/`; the main repo keeps ignoring that folder. Private storage is not public redistribution, so the
open decision above is unaffected.
1. On the personal laptop: `git init` in `Content/Jeep/`, `git lfs track "*.uasset" "*.umap"`, commit the WIP door split, push.
2. On the office laptop: clone into `Content/Jeep/`.
3. Decide where levels that reference the Jeep live (likely the asset repo, since they only load where the Jeep exists).
4. Switching machines: close the editor → commit + push **both** repos → pull both on the other machine.

**Rules until then:** don't save levels that reference Jeep assets on a machine without them; never edit the same `.uasset`
on both machines without syncing in between (binary, no merge). LFS quota is per account (private repos count); check it in
GitHub → Settings → Billing.

Alternatives considered: cloud drive / Syncthing (no history, half-synced files), re-import per machine (loses edits),
making the main repo private (blocks going public without history rewrite).

## Other assets
| Asset | Source | License |
|-------|--------|---------|
