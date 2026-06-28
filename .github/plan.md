# Post-Bindless Raster Cleanup Plan

## Current State

- glTF raster shaders read `GpuMaterial` from bindless resources.
- `Mesh` stores surface binding state under `mesh.surface`.
- Deferred and simple renderers use renderer-local glTF pipeline caches keyed by
  `MeshSurface`-derived state.
- Raster renderers no longer use material-name lookup during frame rendering.
- The raster `GltfMaterial` wrapper hierarchy has been removed.
- `GltfMaterialInfo` remains for scene loading and path tracer BSDF creation.

## Remaining Work

1. Run interactive validation with Vulkan validation enabled.
2. Check scene hot reload:
   - glTF textures unregister/register correctly
   - bindless material slots are released/recreated correctly
   - `mesh.surface` data updates after reload
   - no stale pipeline/layout/descriptor validation errors
3. Check shader hot reload on:
   - `geometry.frag`
   - `translucency_lit.frag`
   - `simple_gltf.frag`
   - shared glTF bindless include
4. Check visual behavior:
   - deferred opaque still renders albedo/normal/ORM/emissive correctly
   - deferred translucent still sorts and blends correctly
   - simple renderer still renders glTF meshes
   - single-sided and double-sided materials use the expected culling

## Optional Later Cleanups

- Decide whether `TMP_BINDLESS_DEBUG` should stay hardcoded to `1` or move to a
  config/build flag.
- Revisit stale historical notes in `.github/bindless.md` if documentation drift
  becomes confusing.
- Consider splitting pure geometry from surface binding if `Mesh` starts serving
  non-surface interpretations such as volume/raymarch use cases.

## Done Criteria

- `ellyn` builds.
- Vulkan shaders compile.
- Runtime validation is quiet for deferred/simple glTF raster rendering.
- Scene and shader hot reload work without stale bindless resources.
- Any skipped runtime checks are documented before moving to RTGI/material work.
