# Path to ReSTIR GI

This roadmap keeps `niar` moving toward ReSTIR GI without turning every adjacent improvement into a renderer rewrite. The target is not production-quality path tracing. The target is a debuggable, incremental ray-traced indirect lighting path that can grow from the current deferred renderer, RTX shadows, and shader hot reload work.

Assumption for estimates: evenings and weekends, with occasional debugging rabbit holes.

## Direction

Use the existing deferred renderer as the primary visibility path for now:

- rasterize G-buffer as usual
- use G-buffer position, normal, albedo, roughness, metallic, and depth as RTGI inputs
- use the existing TLAS/ray query infrastructure as the first ray traversal path
- write indirect lighting into a separate texture
- composite indirect lighting into deferred lighting
- add temporal/spatial reuse and filtering before attempting full ReSTIR GI

This keeps ReSTIR GI as the next major milestone while avoiding a premature jump to a full ReSTIR PT renderer.

Forward+ is a possible future renderer milestone, but it is not a prerequisite for ReSTIR GI. Treat deferred as the current primary visibility path, and structure RTGI so it is attached to deferred rather than made inseparable from deferred. The useful investment now is shared scene/view data: camera buffers, light buffers, TLAS access, material tables, history textures, and indirect lighting outputs that a future Forward+ path could also consume.

## Non-Goals For This Roadmap

- Do not replace the deferred renderer.
- Do not switch to Forward+ before the first RTGI milestones prove what the GI path needs.
- Do not build a render graph yet.
- Do not implement bindless as a prerequisite.
- Do not redesign descriptor allocation unless the current code becomes painful.
- Do not add async compute or multi-queue scheduling.
- Do not implement ReSTIR PT or ReSTIR PT Enhanced.
- Do not solve all material hit shading before the first GI ray works.

## Milestone 0: RTGI Workspace

Estimated time: 1-2 weekends.

Goal: create a stable place for GI work without tangling it into every part of deferred lighting.

Tasks:

- Add an RTGI toggle and debug mode selection.
- Add a separate indirect lighting texture, probably full resolution first for simplicity.
- Add clear ownership for RTGI resources in `DeferredRenderer` or a small helper owned by it.
- Add debug views for the indirect texture, ray hit/miss, hit distance, and normal-facing term.
- Keep the first version in the graphics/deferred flow unless a compute pass is clearly simpler.

Done when:

- the renderer can allocate, clear, and display an indirect lighting texture
- shader hot reload works for the RTGI shader path
- RTGI can be toggled without affecting the existing direct lighting path

Notes:

- Full resolution is acceptable for v0 because it avoids upsampling distractions.
- Lower resolution can come later once the algorithm is alive.
- Keep the indirect lighting texture and any RTGI state separate enough that a future Forward+ renderer could sample the same output.

## Milestone 1: Naive One-Bounce Diffuse RTGI

Estimated time: 2-4 weekends.

Goal: prove that `niar` can shoot stochastic indirect rays from visible surfaces and produce a noisy but plausible indirect lighting buffer.

Tasks:

- Generate stable per-pixel random numbers.
- Sample a cosine-weighted hemisphere direction around the G-buffer normal.
- Trace one diffuse ray through the current TLAS.
- Return sky/environment lighting on miss.
- Return a simple placeholder value on hit, such as normal/debug color, constant bounce color, or emissive-only contribution.
- Composite the result into deferred lighting behind a debug toggle.

Done when:

- visible surfaces receive noisy indirect light
- misses sample the current sky/environment path
- the result is stable enough to inspect frame-to-frame
- direct lighting and RT shadows still work unchanged

Likely pain points:

- ray origin bias
- coordinate-space mistakes
- random sequence correlation
- image layout/barrier mistakes
- deciding where the RTGI pass should live relative to the deferred subpasses

## Milestone 2: Minimal Hit Shading

Estimated time: 3-6 weeks.

Goal: make ray hits return scene-aware lighting data instead of pure debug values.

Tasks:

- Map ray hits back to mesh instance and primitive identity.
- Provide enough geometry/material data for simple hit evaluation.
- Start with diffuse albedo only.
- Add emissive return if the material data already makes that easy.
- Keep texture lookup optional for the first working version.

Done when:

- bounced rays can return approximate diffuse scene color
- simple colored objects affect nearby indirect lighting
- emissive objects, if present, can contribute or at least be identified

Likely pain points:

- current material data may be organized around raster draw calls rather than ray hits
- texture/material indirection may tempt a bindless detour
- instance custom index needs a durable mapping to mesh/material data

Guardrail:

- If material lookup starts becoming a major project, use a temporary compact material table first. ReSTIR GI needs a working signal more than it needs perfect material fidelity at this stage.
- Prefer material/instance data layouts that can serve raster, deferred RTGI, and a future Forward+ path. Avoid encoding ray hit shading entirely around current G-buffer packing.

## Milestone 3: Temporal Accumulation

Estimated time: 3-5 weeks.

Goal: convert the noisy one-ray result into a temporally accumulated indirect signal.

Tasks:

- Add ping-pong history textures for indirect radiance.
- Add a history validity mask or accumulated sample count.
- Start with simple history reuse.
- Add camera/depth/normal rejection.
- Add motion-vector reprojection later if necessary.
- Add debug views for history age, rejected history, and accumulated sample count.

Done when:

- still camera noise decreases over time
- moving the camera does not leave obviously broken trails everywhere
- disocclusion behavior is visible and debuggable

Likely pain points:

- `niar` may not currently have motion vectors
- camera cuts and animated objects need history rejection
- temporal accumulation can hide bugs until movement exposes them

Guardrail:

- Do not make motion vectors a prerequisite for the first accumulation pass. Start simple, then replace the reprojection logic when the lack of motion vectors becomes the limiting factor.
- If motion vectors become necessary, consider whether they should be produced as shared view data rather than as a deferred-only attachment.

## Milestone 4: Spatial Filtering

Estimated time: 3-5 weeks.

Goal: add a depth/normal-aware spatial filter so the indirect buffer becomes usable at low sample counts.

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

Goal: introduce the ReSTIR-style reservoir data path before trying to match the paper perfectly.

Tasks:

- Define a compact reservoir struct for indirect samples.
- Store candidate direction, hit distance or hit identity, radiance/throughput, weight sum, and sample count.
- Generate one or more local candidates per pixel.
- Resample local candidates into one reservoir.
- Shade from the selected reservoir sample.
- Add debug views for selected direction, weight, sample count, and invalid reservoirs.

Done when:

- a pixel can select and reuse a representative indirect sample through reservoir math
- output roughly matches the naive RTGI signal in simple scenes
- reservoir state can be inspected visually

Likely pain points:

- PDF/weight mistakes
- invalid samples that look plausible
- mixing visibility, BRDF, and target function terms incorrectly

Guardrail:

- Treat this as a prototype. Correctness and debug visibility matter more than performance.

## Milestone 6: Temporal Reservoir Reuse

Estimated time: 4-8 weeks.

Goal: reuse previous-frame indirect reservoirs when the current pixel can safely reproject to old data.

Tasks:

- Store previous-frame reservoirs.
- Reproject current pixels to previous-frame reservoir locations.
- Validate temporal reservoirs using depth, normal, and material checks.
- Combine current and temporal reservoirs.
- Clamp or reset history when confidence is low.

Done when:

- stable camera views converge faster than the naive temporal accumulator
- camera motion does not create severe ghosting
- temporal reuse can be visualized and disabled independently

Likely pain points:

- disocclusion
- hidden bias from over-trusting old reservoirs
- history instability around thin geometry and silhouettes

## Milestone 7: Spatial Reservoir Reuse

Estimated time: 4-8 weeks.

Goal: resample nearby reservoirs so neighboring pixels share useful indirect paths.

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

Likely pain points:

- neighbor validation thresholds
- spatial correlation
- bias vs variance tradeoffs
- performance cost of sampling many neighbors

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

Estimated time if pursued: 2-4 months for a basic useful Forward+ renderer, longer if it includes broad material-system cleanup.

Forward+ is worth considering if the renderer starts needing a more flexible raster front-end:

- many dynamic lights where clustered/tiled culling would help both deferred and forward shading
- better translucent lighting support
- material models that do not fit comfortably into fixed G-buffer channels
- MSAA-friendly shading
- a desire to share more material evaluation code between raster and ray-hit shading
- a cleaner path for bindless material and texture lookup

Forward+ is not worth doing just because it is more modern. It should solve a concrete pain point discovered while building RTGI.

If pursued, the likely sequence is:

- factor light lists and camera/view data into renderer-independent buffers
- add clustered or tiled light culling as shared infrastructure
- build a simple Forward+ path that draws opaque meshes first
- add translucent support after opaque Forward+ works
- keep deferred working until Forward+ can match the scenes used for RTGI testing
- make RTGI consume shared view/material/TLAS data rather than deferred-specific internals

Expected impact on ReSTIR GI:

- Helpful if material lookup, transparency, or many-light handling become the limiting problem.
- Distracting if the current issue is still ray generation, history, filtering, or reservoir correctness.
- Best done after the RTGI signal exists, not before.

## Forward+ Reassessment Checkpoints

Checkpoint A: after Milestone 1.

Question: did naive RTGI reveal any deferred-specific blocker?

Default answer should be no. If the main problems are noise, barriers, random sampling, or ray bias, continue with deferred.

Checkpoint B: after Milestone 2.

Question: is material/hit shading becoming awkward because material data is too tied to deferred G-buffer packing or raster draw calls?

If yes, first try a shared material/instance table. Consider Forward+ only if a shared material model clearly wants a new raster path too.

Checkpoint C: after Milestone 4.

Question: are transparency, G-buffer limitations, or many-light handling limiting the scenes that can demonstrate RTGI?

If yes, this is the first realistic point to plan Forward+ as a parallel or next major milestone.

Checkpoint D: after Milestone 6 or 7.

Question: is the ReSTIR GI path stable enough that renderer architecture, rather than GI correctness, is now the bottleneck?

If yes, Forward+ may be a good modernization milestone. If reservoir validation, reprojection, and denoising are still unstable, stay focused on GI.

## Overall Timeline

Optimistic path:

- noisy one-bounce RTGI: 1 month
- temporal/spatial denoised RTGI: 2-3 months
- first ReSTIR-style reservoir prototype: 3-5 months
- recognizable ReSTIR GI: 5-8 months

Normal path:

- noisy one-bounce RTGI: 1-2 months
- temporal/spatial denoised RTGI: 3-4 months
- first ReSTIR-style reservoir prototype: 5-7 months
- recognizable ReSTIR GI: 8-12 months
- optional Forward+ milestone, if reassessment says it is useful: add 2-4 months

Rabbit-hole path:

- 12+ months, mostly due to material lookup, reprojection, denoising, and reservoir correctness issues.

## Recommended Next Commit

Start with Milestone 0:

- add RTGI resource allocation
- add an indirect lighting texture
- add a debug display mode for it
- add config/debug toggles
- do not trace rays yet

The first win should be boring: a visible texture that the renderer owns and can display. Once that path is stable, the first stochastic ray has somewhere clean to land.

## Reading Order

Suggested order, from most relevant to most ambitious:

1. ReSTIR GI: path resampling for real-time path tracing.
2. ReSTIR DI / RTXDI: direct-light reservoir sampling.
3. GRIS: the generalized math behind ReSTIR.
4. ReSTIR PT: whole-path reuse.
5. ReSTIR PT Enhanced: future reference, not an implementation target for this roadmap.
