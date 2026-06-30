# Milestone 2 Review Plan: Minimal Hit Shading

Source roadmap: `.github/path-to-restir.md`, Milestone 2.

Goal: make RTGI ray hits return scene-aware material data instead of the current
placeholder black result, while keeping each step small enough to review as a
focused change.

## Review Chunk 1: Keep Committed Hit Data

Purpose: change the RTGI ray-query path from "hit or miss" to "miss, or a
committed hit with enough identifiers to debug."

Implementation scope:

- Replace the boolean-only `rayMissedScene` result with a compact hit result
  struct in `rtgi_generate.comp`.
- Preserve current behavior for misses: sample sky/environment.
- Preserve current behavior for hits: return black for now.
- Capture committed hit fields that are available directly from ray query:
  instance custom index, primitive index, barycentrics if available, and hit
  distance if useful for debug.
- Add a temporary debug/output mode only if there is already a nearby config or
  shader pattern for selecting RTGI debug output. Otherwise keep the captured
  data local and use the next chunk for visible debug.

Review checklist:

- Miss path is unchanged apart from naming/struct plumbing.
- Hit path still contributes black, so visual output should not materially
  change except for intentional debug output.
- The new hit result has clear invalid values for miss/no-data cases.
- No scene/material binding is introduced yet.

Done when:

- The compute shader can distinguish miss from committed hit and retain the hit
  identifiers needed by later chunks.
- Existing skylight-on-miss behavior still works.

## Review Chunk 2: GPU Scene Instance Record Layout

Purpose: define the durable CPU/GPU bridge from TLAS instance row to scene
material metadata, without changing RTGI shading yet.

Implementation scope:

- Add a GPU scene instance record type in shared C++/GLSL-facing layout.
- Store at least:
  - bindless material index
  - geometry record index or reserved geometry fields
  - padding/reserved fields so the layout can grow without churn
- Add `static_assert`s for size and offsets on the C++ side.
- Keep the record renderer-agnostic; do not bake in current G-buffer packing.

Review checklist:

- Field names describe scene lookup responsibilities, not one current shader.
- Layout alignment is explicit and checked.
- Reserved fields are intentional and documented lightly.
- No TLAS build behavior changes are mixed into this chunk.

Done when:

- There is a stable scene instance record definition ready to be filled in TLAS
  order.

## Review Chunk 3: Build Scene Instance Records In TLAS Order

Purpose: populate the scene instance table in exactly the same order used to
build the TLAS instances.

Implementation scope:

- Build one scene instance record per TLAS instance.
- Fill each record's bindless material index from the mesh/surface already used
  by raster glTF rendering.
- Set each TLAS instance's `instanceCustomIndex` to the matching scene instance
  record row.
- Assert that every record has a valid bindless material index.
- Keep reload/rebuild paths from silently diverging: table rebuild and TLAS
  rebuild should happen from the same source ordering.

Review checklist:

- Record count matches TLAS instance count.
- `instanceCustomIndex` is a table row, not an unrelated mesh/material id.
- Multi-scene or reload paths cannot leave stale scene instance data behind.
- The change does not require texture, vertex, or index lookup yet.

Done when:

- RTGI can use `instanceCustomIndex` as a reliable index into a GPU scene
  instance table.

## Review Chunk 4: Bind Scene Instance Data To RTGI

Purpose: expose the scene instance table to `rtgi_generate.comp` without using
it for material shading yet.

Implementation scope:

- Add the scene instance buffer to the RTGI descriptor set layout.
- Bind the buffer from the GI component or the owner that already has the TLAS
  and scene lifetime context.
- Add the matching GLSL buffer declaration.
- Use the committed hit's instance custom index to read the scene instance
  record.
- Add a debug output path for hit/miss and scene instance row if the current
  debug plumbing supports it.

Review checklist:

- Descriptor layout, C++ binding, and GLSL binding stay aligned.
- The buffer lifetime is at least as long as the RTGI pass that reads it.
- Invalid/miss cases do not read the scene instance buffer.
- The normal visual RTGI result can remain unchanged.

Done when:

- A committed RTGI hit can read the matching scene instance record on the GPU.
- A debug view or shader-side assertion strategy can confirm instance rows.

## Review Chunk 5: Bindless Material Index Debug

Purpose: verify the bridge all the way from ray hit to bindless material index
before shading uses it.

Implementation scope:

- Read `bindlessMaterialIndex` from the scene instance record.
- Add a material-index debug output mode.
- Keep normal RTGI shading unchanged outside the debug mode.
- Exercise at least one scene with multiple materials or multiple objects.

Review checklist:

- Debug colors/values make mismatches easy to spot.
- Material index reads are guarded by hit validity.
- The debug path does not depend on albedo texture lookup.

Done when:

- Ray-hit debug output shows stable, expected material indices.
- Scene reloads do not mismatch TLAS instance rows and material records.

## Review Chunk 6: Base Color Factor Hit Shading

Purpose: make hits contribute a simple material-colored diffuse signal using
only `GpuMaterial.baseColorFactor`.

Implementation scope:

- Bind the shared bindless material descriptor set to the RTGI compute pipeline.
- Use the hit's scene instance record to fetch the `GpuMaterial`.
- On ray hit, return `GpuMaterial.baseColorFactor.rgb` as the first approximate
  bounced diffuse result.
- Keep textured albedo, emissive, metallic, and roughness out of this chunk.

Review checklist:

- The bindless set is bound only once the shader actually reads material data.
- Hit shading remains deliberately simple and easy to reason about.
- Miss shading still samples sky/environment.
- Direct lighting and RT shadows remain separate from this RTGI signal.

Done when:

- Simple colored objects affect nearby indirect lighting through the RTGI pass.
- The implementation proves ray hit to material fetch without UV complexity.

## Review Chunk 7: Geometry Record Skeleton

Purpose: introduce the geometry lookup layer needed for primitive and UV work,
without sampling textures yet.

Implementation scope:

- Add a GPU geometry record layout referenced by scene instance records.
- Store enough information to locate index/vertex data for the hit geometry:
  buffer address or buffer index, index offset, vertex offset, index type, and
  any needed stride/layout fields.
- Preserve room for per-primitive material identity if mesh build paths ever
  batch multiple primitives into one acceleration-structure geometry.
- Add C++ layout checks for geometry records.

Review checklist:

- The geometry record describes source geometry, not material shading policy.
- 16-bit and 32-bit index handling are either supported or explicitly asserted.
- This chunk does not require UV interpolation or texture sampling yet.

Done when:

- A hit can identify which geometry record should be used for later vertex/index
  lookup.

## Review Chunk 8: Primitive And Barycentric Debug

Purpose: verify hit geometry identity before reading vertex attributes.

Implementation scope:

- Expose primitive index and barycentric coordinates from the committed hit.
- Add debug outputs for primitive index and barycentric coordinates.
- Confirm values are stable across camera movement and scene reload.

Review checklist:

- Debug output is isolated from normal RTGI shading.
- Primitive index values look plausible on simple meshes.
- Barycentric output changes smoothly across triangles.

Done when:

- The renderer can visually inspect primitive identity and barycentrics for RTGI
  ray hits.

## Review Chunk 9: UV Interpolation Debug

Purpose: read hit triangle vertex data and prove UV interpolation before using
  it for material sampling.

Implementation scope:

- Read the hit triangle's indices from the geometry record.
- Read vertex UVs using the project's actual vertex layout.
- Interpolate UVs from barycentric coordinates.
- Add a UV debug output mode.
- Handle or assert unsupported index formats explicitly.

Review checklist:

- GLSL vertex/index layout matches the C++ vertex/index layout exactly.
- UV debug follows mesh unwraps on known textured assets.
- Degenerate or missing UV cases have a clear fallback or assertion.
- Base color factor hit shading from Chunk 6 still works.

Done when:

- RTGI hit shaders can reconstruct stable UVs for textured meshes.

## Review Chunk 10: Textured Albedo Hit Shading

Purpose: upgrade hit shading from base color factor only to albedo texture times
base color factor.

Implementation scope:

- Sample the material albedo texture through the existing bindless texture table.
- Use explicit LOD 0 for the first implementation.
- Multiply sampled albedo by `GpuMaterial.baseColorFactor.rgb`.
- Keep normal mapping, ORM, and specular response out of this chunk.

Review checklist:

- Texture sampling uses the same bindless material conventions as raster glTF
  shaders where practical.
- Missing albedo texture resolves through the existing default texture path.
- Textured indirect color follows mesh UVs.
- No unrelated material model changes are mixed in.

Done when:

- Textured objects can affect nearby indirect lighting with recognizable albedo
  color.

## Review Chunk 11: Emissive Identification Or Contribution

Purpose: handle emissive materials only after the material and UV bridge is
working.

Implementation scope:

- Read `GpuMaterial.emissiveFactorAndClipThreshold.rgb`.
- If UV lookup is available, sample the emissive texture and multiply by the
  emissive factor.
- Choose the smallest useful behavior:
  - identify emissive hits in a debug mode, or
  - add emissive radiance as hit contribution.
- Keep this separate from direct-light sampling or full ReSTIR light selection.

Review checklist:

- Emissive behavior is documented as either debug-only or contributing radiance.
- Non-emissive materials stay unchanged.
- The change does not imply full emissive area-light sampling yet.

Done when:

- Emissive objects can be identified by RTGI hits, or can contribute a simple
  radiance term if enabled.

## Review Chunk 12: Milestone 2 Cleanup And Guardrails

Purpose: finish the milestone by tightening invariants and removing temporary
scaffolding that is no longer useful.

Implementation scope:

- Audit descriptor bindings, scene instance lifetime, and reload behavior.
- Remove stale debug modes only if they have been superseded and are not useful
  for future GI work.
- Rename temporary structs/functions now that their responsibilities are clear.
- Add comments only around non-obvious layout, indexing, or ray-query behavior.
- Update `.github/path-to-restir.md` status if Milestone 2 is complete.

Review checklist:

- No stale temporary names hide the final data flow.
- Scene reload cannot silently mismatch TLAS instance order, scene instance
  records, and material data.
- Future milestones can consume the instance/material/geometry bridge without
  depending on G-buffer packing.

Done when:

- Milestone 2's done conditions are met:
  - bounced rays can return approximate diffuse scene color
  - simple colored objects affect nearby indirect lighting
  - ray-hit debug views show expected instance and material indices
  - textured albedo follows mesh UVs once UV lookup is enabled
  - emissive objects, if present, can contribute or at least be identified
  - scene reloads cannot silently mismatch TLAS instance order and material data

## Milestone 2.5: Direct-Lit Secondary Hit Shading

Purpose: make the one-bounce RTGI sample physically more meaningful before
temporal accumulation starts hiding raw one-sample behavior. Keep this milestone
small: shade the secondary hit with emissive plus direct lighting, but do not add
MIS, reservoir sampling, recursive bounces, primary-surface specular sampling,
or temporal/spatial reuse.

Working model:

- Primary visible surfaces still sample one cosine-weighted diffuse ray.
- Misses still return sky/environment radiance.
- Committed secondary hits reconstruct hit position, normal, UV, and material.
- Secondary hit contribution returns outgoing radiance toward the primary
  surface: simple emissive plus direct lighting at the hit point.
- The deferred composite can continue applying the primary surface albedo to the
  RTGI buffer; do not silently change the RTGI output contract to final
  BRDF-weighted primary-surface lighting in this milestone.

## Review Chunk 13: Reconstruct Secondary Hit Surface Data

Purpose: turn the committed ray hit into the minimum surface data needed for
direct lighting at the hit point.

Implementation scope:

- Use `rayOrigin + rayDir * hitT` to reconstruct the secondary hit world
  position.
- Read hit triangle vertex positions and normals through the existing bindless
  geometry record path.
- Interpolate vertex normals using committed barycentric coordinates.
- Transform the interpolated normal to world space. If the current TLAS/scene
  instance data does not expose the needed transform cleanly, add the minimum
  renderer-agnostic transform data to the scene instance record or a companion
  table.
- Keep UV reconstruction and albedo/emissive lookup from Milestone 2 intact.
- Keep tangent-space normal maps out of this chunk; use vertex normals only.

Review checklist:

- World-space hit position matches the same ray origin and direction used for
  traversal.
- Normal interpolation follows the same triangle index and barycentric data as
  UV interpolation.
- Transform handling is explicit; no hidden dependency on current G-buffer
  packing or raster draw order.
- Invalid geometry or generated/AABB intersections fall back clearly instead of
  reading triangle vertex data.

Done when:

- `rtgi_generate.comp` can build a secondary-hit shading input containing world
  position, world normal, UV, material, and outgoing direction back toward the
  primary surface.

## Review Chunk 14: Share Direct-Light Inputs With RTGI

Purpose: expose the same point and directional light buffers used by deferred
lighting to the RTGI compute pass.

Implementation scope:

- Add RTGI descriptor bindings for the existing point-light and
  directional-light buffers, or move the declarations into a shared frame-global
  binding path if that is cleaner.
- Keep binding numbers and GLSL declarations aligned with the existing
  `ViewInfo.NumPointLights` and `ViewInfo.NumDirectionalLights` fields.
- Prefer extracting layout-free lighting helper math from `lighting_common.glsl`
  over including it directly if its current descriptor declarations conflict
  with `rtgi_generate.comp`.
- Do not add new light types, area-light sampling, or MIS.

Review checklist:

- RTGI and deferred read the same CPU-authored light data for the current frame.
- Descriptor ownership remains in `GI`/deferred renderer code that already owns
  frame context.
- No shader include introduces duplicate or mismatched descriptor declarations.
- Existing deferred lighting output is unchanged.

Done when:

- `rtgi_generate.comp` can iterate current point and directional lights without
  duplicating CPU light upload logic.

## Review Chunk 15: Shadow Rays From Secondary Hits

Purpose: let direct lighting at the secondary hit respect scene visibility.

Implementation scope:

- Add an RTGI-local `shadowFactor` helper or share a descriptor-free helper with
  deferred lighting.
- Trace shadow rays from secondary hit position toward each light.
- Offset the shadow ray origin along the secondary hit normal using the existing
  small ray-bias convention.
- Use `TerminateOnFirstHit`, `SkipClosestHitShader`, and opaque ray flags for
  shadow visibility.
- Use point-light distance as `tMax`; use a large directional-light `tMax`.

Review checklist:

- Shadow rays use the same TLAS already bound for RTGI ray queries.
- Self-shadow acne is controlled by a small normal offset, not by arbitrary
  large bias.
- Point-light shadow rays cannot hit geometry behind the light.
- This chunk does not change the primary indirect-ray sampling distribution.

Done when:

- Direct-light evaluation at secondary hits can be visibly occluded by scene
  geometry.

## Review Chunk 16: Direct-Lit Hit Contribution

Purpose: replace emissive-only or albedo-only secondary hit return values with a
simple physically motivated outgoing radiance estimate.

Implementation scope:

- Build a secondary-hit material record from albedo texture times
  `baseColorFactor`, emissive texture times emissive factor, and ORM values
  already available through `GpuMaterial`.
- Evaluate direct lighting at the secondary hit using `-rayDir` as the outgoing
  direction toward the primary surface.
- Reuse the existing Cook-Torrance-style direct-light math where practical, but
  keep the first implementation local and explicit if sharing would require
  descriptor churn.
- Return `emission + directLightingAtSecondaryHit`.
- Preserve sky/environment-on-miss behavior.
- Keep the primary-surface composite contract unchanged; do not multiply by the
  primary surface BRDF inside RTGI in this chunk.

Review checklist:

- Non-emissive secondary hits are lit by actual scene lights rather than acting
  like emission.
- Emissive secondary hits still contribute even without direct lighting.
- Metallic and roughness affect the secondary hit's direct-light response, but
  primary ray directions remain diffuse cosine samples.
- Energy scale is explainable from light/material inputs, not a debug color or
  hidden arbitrary multiplier.

Done when:

- One-bounce RTGI produces noisy but recognizable direct-lit bounce color from
  secondary surfaces.

## Review Chunk 17: Milestone 2.5 Guardrails And Roadmap Update

Purpose: lock the direct-lit secondary hit behavior as the pre-accumulation
target and keep future work boundaries clear.

Implementation scope:

- Audit the RTGI shader for stale comments implying base color or emissive is
  the whole hit contribution.
- Document the current RTGI buffer contract: incoming indirect radiance-like
  signal for the primary surface, not full primary BRDF-weighted final lighting.
- Update `.github/path-to-restir.md` if Milestone 2.5 is complete.
- Leave MIS, explicit light sampling at the primary surface, GGX primary-ray
  sampling, recursive bounces, and temporal/spatial reuse for later milestones.

Review checklist:

- The code makes it clear which part is secondary-hit shading and which part is
  primary-surface composition.
- The roadmap still points to Milestone 3 temporal accumulation next.
- Debug visualization remains optional and is not required to consider this
  milestone complete.

Done when:

- The raw one-sample RTGI target is physically meaningful enough to accumulate:
  miss radiance, emissive hit radiance, and direct-lit secondary hit radiance all
  work in simple scenes.

## Suggested PR Grouping

If the chunks feel too small as individual PRs, group them this way:

- PR 1: Chunks 1-3, hit data plus scene instance table construction.
- PR 2: Chunks 4-6, RTGI binding, material index debug, base color factor hit
  shading.
- PR 3: Chunks 7-9, geometry records, primitive/barycentric debug, UV debug.
- PR 4: Chunks 10-12, textured albedo, emissive handling, cleanup.
- PR 5: Chunks 13-17, secondary hit surface reconstruction and direct-lit hit
  shading.

Avoid grouping across the main risk boundaries:

- scene instance/TLAS ordering
- descriptor and bindless material binding
- geometry buffer/index decoding
- texture and emissive material evaluation
- secondary-hit direct lighting and shadow visibility
