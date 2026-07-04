# Path to ReSTIR GI

This roadmap keeps `niar` moving toward ReSTIR GI without turning every adjacent
improvement into a renderer rewrite. The target is not production-quality path
tracing. The target is a debuggable, incremental ray-traced indirect lighting
path that grows from the current deferred renderer, RTX shadows, shader hot
reload, and bindless glTF material infrastructure.

Assumption for estimates: evenings and weekends, with occasional debugging
rabbit holes.

## Current State

The first RTGI foundation is already in place:

- deferred remains the primary visibility path
- RTGI has a renderer-owned indirect lighting texture
- RTGI runs as a compute pass
- stochastic diffuse rays are generated from visible G-buffer surfaces
- misses sample the current sky/environment path
- committed hits resolve scene instance, material, geometry, UV, albedo, and
  emissive data
- committed hits reconstruct secondary-hit position and vertex normal
- RTGI secondary-hit shading returns emissive plus direct-lit outgoing radiance
  toward the primary surface
- secondary-hit direct lighting uses the shared point/directional light buffers
  and RT shadow visibility against the current TLAS
- scene and shader hot reload have been smoke-tested with the bindless raster
  material path
- raster glTF shaders read `GpuMaterial` records from the global bindless
  material table
- RTGI reads shared bindless material and geometry tables for hit shading
- deferred opaque, deferred translucent, and simple forward glTF raster paths
  use the bindless material/texture table
- raster renderers no longer need per-material glTF descriptor sets or
  per-frame material lookup by string

The next missing bridge is temporal: the one-ray signal needs accumulation and
history rejection before it can become a stable lighting buffer.

## Direction

Use the existing deferred renderer as the primary visibility path for now:

- rasterize G-buffer as usual
- use G-buffer position, normal, albedo, roughness, metallic, and depth as RTGI
  inputs
- use the existing TLAS/ray query infrastructure as the first ray traversal path
- use the shared bindless `GpuMaterial` table for glTF material data
- write indirect lighting into a separate texture
- composite indirect lighting into deferred lighting
- add temporal/spatial reuse and filtering before attempting full ReSTIR GI

This keeps ReSTIR GI as the next major milestone while avoiding a premature jump
to a full ReSTIR PT renderer.

Forward+ is a possible future renderer milestone, but it is not a prerequisite
for ReSTIR GI. Treat deferred as the current primary visibility path, and
structure RTGI so it is attached to deferred rather than made inseparable from
deferred. The useful investment now is shared scene/view data: camera buffers,
light buffers, TLAS access, material tables, hit geometry tables, history
textures, and indirect lighting outputs that a future Forward+ path could also
consume.

## Non-Goals For This Roadmap

- Do not replace the deferred renderer.
- Do not switch to Forward+ before the first RTGI milestones prove what the GI
  path needs.
- Do not build a render graph yet.
- Do not broaden bindless beyond current glTF material needs unless RTGI proves
  a concrete need.
- Do not redesign descriptor allocation unless the current code becomes painful.
- Do not add async compute or multi-queue scheduling.
- Do not implement ReSTIR PT or ReSTIR PT Enhanced.
- Do not solve all material hit shading before minimal hit color works.

## Milestone 0: RTGI Workspace

Status: complete enough to move on.

Goal: create a stable place for GI work without tangling it into every part of
deferred lighting.

Implemented:

- RTGI-owned indirect lighting texture
- RTGI compute path
- indirect lighting composite into deferred lighting
- enough debug/toggle plumbing to inspect the path
- shader hot reload smoke-tested for the RTGI path

Keep in mind:

- Full resolution is acceptable for v0 because it avoids upsampling
  distractions.
- Lower resolution can come later once the algorithm is alive.
- Keep the indirect lighting texture and any RTGI state separate enough that a
  future Forward+ renderer could sample the same output.

## Milestone 1: Naive One-Bounce Diffuse RTGI

Status: complete enough to move on.

Goal: prove that `niar` can shoot stochastic indirect rays from visible surfaces
and produce a noisy but plausible indirect lighting buffer.

Implemented:

- stable per-pixel random numbers
- cosine-weighted hemisphere sampling around the G-buffer normal
- one diffuse ray through the current TLAS
- sky/environment lighting on miss
- placeholder black contribution on hit
- direct lighting and RT shadows remain separate from the RTGI signal

Remaining issues can be handled while improving hit shading:

- ray origin bias tuning
- coordinate-space mistakes exposed by new debug views
- random sequence quality
- image layout/barrier issues if the pass schedule changes

## Milestone 2: Minimal Hit Shading

Status: complete enough to move on.

Goal: make ray hits return scene-aware material data instead of the current
placeholder black result.

Implemented:

- committed hit resolves to a scene instance record
- scene instance record resolves to a bindless material index
- scene instance record resolves to a bindless geometry record
- `instanceCustomIndex` maps to the corresponding scene instance table row
- ray-query helper keeps committed hit details, including primitive index and
  barycentrics
- RTGI binds and reads the shared bindless material and geometry tables
- geometry records provide buffer-device-address vertex/index lookup data
- shader-side hit code reconstructs UVs from primitive index and barycentrics
- hit contribution samples albedo with explicit LOD 0 and multiplies by base
  color factor
- emissive materials can contribute a simple emissive signal

Done when:

- bounced rays can return approximate diffuse scene color
- simple colored objects affect nearby indirect lighting
- textured albedo follows mesh UVs once UV lookup is enabled
- emissive objects, if present, can contribute or at least be identified
- scene reloads cannot silently mismatch TLAS instance order and material data

Deferred:

- dedicated ray-hit debug visualizations for instance index, material index,
  primitive index, barycentrics, and UVs; add these later only if needed for
  debugging

Likely pain points:

- current combined vertex/index buffers belong to individual assets, so geometry
  records use buffer-device-address lookups
- GLSL-side vertex/index layout must match the CPU layout exactly
- 16-bit index decoding needs explicit handling
- preserving per-primitive material identity matters if a mesh/asset build path
  ever batches multiple primitives into one acceleration-structure geometry

Guardrails:

- Prefer a compact scene instance/material bridge before attempting high-fidelity
  material evaluation.
- Texture lookup is useful but should not block the first material-colored hit.
- Keep the scene instance and geometry records renderer-agnostic enough that
  deferred RTGI and a future Forward+ renderer could share them.
- Do not encode ray-hit shading around current G-buffer packing.

## Milestone 2.5: Direct-Lit Secondary Hit Shading

Status: complete enough to move on.

Goal: make the raw one-sample RTGI signal physically meaningful enough to
accumulate before temporal/spatial filtering hides the per-sample behavior.

Implemented:

- secondary-hit world position reconstructed from ray origin, direction, and hit
  distance
- secondary-hit normals reconstructed from hit triangle vertex normals and
  barycentrics
- secondary-hit UV/material lookup preserved from Milestone 2
- RTGI binds and reads the same point and directional light buffers as deferred
  lighting
- direct-light BRDF evaluation is shared by deferred, translucent, and RTGI
  lighting paths
- secondary-hit contribution returns emissive plus direct-lit outgoing radiance
  toward the primary surface
- secondary-hit direct lighting uses ray-query shadow visibility through the
  current TLAS
- the deferred composite still treats the RTGI buffer as an incoming indirect
  radiance-like signal for the primary surface, not as fully BRDF-weighted final
  lighting

Deferred:

- MIS, explicit light sampling at the primary surface, recursive bounces, and
  GGX/specular primary-ray sampling
- tangent-space normal maps at secondary hits
- dedicated debug visualization for secondary-hit primitive/material/UV/shadow
  state

Likely pain points:

- raw one-sample output is now more meaningful, but still extremely noisy
- secondary-hit shadow rays add traversal cost before temporal accumulation can
  amortize the noise
- self-shadow bias may still need tuning in real scenes

Guardrails:

- Keep the RTGI buffer contract stable while adding accumulation: miss radiance,
  emissive hit radiance, and direct-lit secondary-hit radiance are all incoming
  signals to be composited at the primary surface.
- Leave higher-fidelity path sampling decisions for later milestones; Milestone
  3 should focus on making the current signal converge.

## Milestone 3: Temporal Accumulation

Status: complete enough to move on.

Goal: convert the noisy one-ray result into a temporally accumulated indirect
signal.

Implemented:

- ping-pong history textures for indirect radiance, plus a per-pixel
  accumulated sample count texture
- same-pixel running-average accumulation, capped at a configurable
  `maxSampleCount` (`config/gi.ini`)
- reprojection via a dedicated motion vector G-buffer attachment
  (`current_uv - prev_uv`, produced from `PrevViewMatrix`/`PrevProjectionMatrix`)
- depth-based history rejection: linearized view-space depth is stored in the
  history alpha channel and compared against the expected reprojected depth,
  with a configurable `historyDepthRejectionThreshold`
- raw vs. converged output can be compared by setting `maxSampleCount: 0` in
  `config/gi.ini` (hot-reloadable): the accumulation formula collapses to the
  one-sample result every frame without any dedicated toggle

Deferred:

- dedicated debug views for history age, rejected pixels, and sample count
  heatmap; optional follow-up work, not required to close the milestone

Done when:

- still camera noise decreases over time
- moving the camera does not leave obviously broken trails everywhere
- disocclusion behavior is visible and debuggable

## Milestone 4: Spatial Filtering

Estimated time: 3-5 weeks.

Goal: add a depth/normal-aware spatial filter so the indirect buffer becomes
usable at low sample counts.

Tasks:

- Add a bilateral or a trous-style filter over indirect radiance.
- Use depth and normal differences to reject unrelated neighbors.
- Preserve enough edge detail to avoid obvious light leaking.
- Add debug modes for raw GI, filtered GI, and filter weights.

Done when:

- one-sample indirect lighting is visibly less noisy
- edges are not destroyed by the filter
- the filter can be disabled to compare raw vs filtered output

Likely pain points:

- overblurring corners
- leaking through thin geometry
- filter order relative to temporal accumulation

## Milestone 5: Reservoir Prototype

Estimated time: 4-8 weeks.

Goal: introduce the ReSTIR-style reservoir data path before trying to match the
paper perfectly.

Tasks:

- Define a compact reservoir struct for indirect samples.
- Store candidate direction, hit distance or hit identity, radiance/throughput,
  weight sum, and sample count.
- Generate one or more local candidates per pixel.
- Resample local candidates into one reservoir.
- Shade from the selected reservoir sample.
- Add debug views for selected direction, weight, sample count, and invalid
  reservoirs.

Done when:

- a pixel can select and reuse a representative indirect sample through
  reservoir math
- output roughly matches the naive RTGI signal in simple scenes
- reservoir state can be inspected visually

Guardrail:

- Treat this as a prototype. Correctness and debug visibility matter more than
  performance.

## Milestone 6: Temporal Reservoir Reuse

Estimated time: 4-8 weeks.

Goal: reuse previous-frame indirect reservoirs when the current pixel can safely
reproject to old data.

Tasks:

- Store previous-frame reservoirs.
- Reproject current pixels to previous-frame reservoir locations.
- Validate temporal reservoirs using depth, normal, material, and roughness.
- Combine current and temporal reservoirs.
- Clamp or reset history when confidence is low.

Done when:

- stable camera views converge faster than the naive temporal accumulator
- camera motion does not create severe ghosting
- temporal reuse can be visualized and disabled independently

## Milestone 7: Spatial Reservoir Reuse

Estimated time: 4-8 weeks.

Goal: resample nearby reservoirs so neighboring pixels share useful indirect
paths.

Tasks:

- Pick a small neighborhood pattern.
- Validate neighbors by depth, normal, material, and roughness.
- Combine spatial reservoirs with current pixel reservoirs.
- Keep a debug mode for accepted/rejected neighbors.
- Compare quality against the non-reservoir temporal/spatial filter path.

Done when:

- adjacent pixels can share useful indirect samples
- noise drops faster than naive spatial filtering
- light leaking is controlled enough for simple scenes

## Milestone 8: ReSTIR GI Polish Pass

Estimated time: 1-3 months.

Goal: turn the prototype into a renderer feature that is pleasant to iterate on.

Tasks:

- Add quality/performance presets.
- Add half-resolution mode and upsampling if useful.
- Improve hit shading fidelity.
- Improve denoising and history rejection.
- Add scene/material stress tests.
- Add GPU timing markers around RTGI passes.
- Document debug modes and known limitations.

Done when:

- RTGI is a believable renderer mode rather than a fragile experiment
- it survives camera movement and common scenes
- the debug tooling is good enough to diagnose bad frames

## Possible Future Milestone: Forward+

Estimated time if pursued: 2-4 months for a basic useful Forward+ renderer,
longer if it includes broad renderer cleanup.

Forward+ is worth considering if the renderer starts needing a more flexible
raster front-end:

- many dynamic lights where clustered/tiled culling would help both deferred and
  forward shading
- better translucent lighting support
- material models that do not fit comfortably into fixed G-buffer channels
- MSAA-friendly shading
- a desire to share more material evaluation code between raster and ray-hit
  shading

Forward+ is not worth doing just because it is more modern. It should solve a
concrete pain point discovered while building RTGI.

If pursued, the likely sequence is:

- factor light lists and camera/view data into renderer-independent buffers
- add clustered or tiled light culling as shared infrastructure
- build a simple Forward+ path that draws opaque meshes first
- add translucent support after opaque Forward+ works
- keep deferred working until Forward+ can match the scenes used for RTGI
  testing
- make RTGI consume shared view/material/TLAS data rather than deferred-specific
  internals

## Forward+ Reassessment Checkpoints

Checkpoint A: after Milestone 2.

Question: is material/hit shading awkward because scene instance or geometry data
is too tied to the current deferred/raster path?

Default answer should still be no. First try shared scene instance and geometry
tables. Consider Forward+ only if a shared material model clearly wants a new
raster path too.

Checkpoint B: after Milestone 4.

Question: are transparency, G-buffer limitations, or many-light handling limiting
the scenes that can demonstrate RTGI?

If yes, this is the first realistic point to plan Forward+ as a parallel or next
major milestone.

Checkpoint C: after Milestone 6 or 7.

Question: is the ReSTIR GI path stable enough that renderer architecture, rather
than GI correctness, is now the bottleneck?

If yes, Forward+ may be a good modernization milestone. If reservoir validation,
reprojection, and denoising are still unstable, stay focused on GI.

## Overall Timeline From Here

Optimistic path:

- minimal material-colored RTGI hits: 2-3 weeks
- temporal/spatial denoised RTGI: 2-3 months
- first ReSTIR-style reservoir prototype: 3-5 months
- recognizable ReSTIR GI: 5-8 months

Normal path:

- minimal material-colored RTGI hits: 3-6 weeks
- temporal/spatial denoised RTGI: 3-4 months
- first ReSTIR-style reservoir prototype: 5-7 months
- recognizable ReSTIR GI: 8-12 months
- optional Forward+ milestone, if reassessment says it is useful: add 2-4
  months

Rabbit-hole path:

- 12+ months, mostly due to hit geometry lookup, reprojection, denoising, and
  reservoir correctness issues.

## Recommended Next Commit

Start Milestone 3 with the smallest temporal accumulation path:

- add ping-pong indirect lighting history textures
- keep a per-pixel accumulated sample count or validity mask
- reproject by stable screen position first, without making motion vectors a
  prerequisite
- reject history using depth and normal differences
- expose a simple on/off comparison between raw one-sample RTGI and accumulated
  RTGI

That turns the now scene-aware RTGI signal into something that can converge
while keeping motion vectors and more advanced rejection as follow-up slices.
The input signal now includes sky/environment misses, emissive secondary hits,
and shadowed direct lighting at secondary-hit surfaces.

## Reading Order

Suggested order, from most relevant to most ambitious:

1. ReSTIR GI: path resampling for real-time path tracing.
2. ReSTIR DI / RTXDI: direct-light reservoir sampling.
3. GRIS: the generalized math behind ReSTIR.
4. ReSTIR PT: whole-path reuse.
5. ReSTIR PT Enhanced: future reference, not an implementation target for this
   roadmap.
