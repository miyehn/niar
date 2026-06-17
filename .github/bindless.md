# Path to Bindless Materials

This roadmap moves `niar` toward a bindless material/resource model without turning the renderer into a broad engine rewrite. The target is a shared material data path that raster, deferred RTGI, hardware ray tracing, and a possible future Forward+ renderer can all consume.

The immediate motivation is Milestone 2 of RTGI: ray hits need to map from instance and primitive identity to material data and textures. If bindless texture access is expected to become necessary anyway, it is reasonable to build that foundation now instead of creating a temporary material lookup path that will be discarded.

Assumption for estimates: evenings and weekends, with debugging time for Vulkan feature flags, descriptor indexing validation errors, hot reload edge cases, and shader/compiler quirks.

## Direction

Build bindless as shared renderer infrastructure, not as an RTGI-only shortcut:

- keep `SceneAsset` as the owner of loaded textures, mesh buffers, BLAS objects, and asset-scoped lifetime
- add a GPU-facing bindless registry that assigns texture/material indices while resources are alive
- expose material constants and texture indices through storage buffers
- expose sampled textures through descriptor arrays with descriptor indexing
- make raster materials consume material indices before deleting the old per-material descriptor path
- make RTGI hit shading consume the same material/texture tables

The useful mental model is:

- assets own resource objects
- bindless registries own descriptor slots and GPU indices
- scene/renderer code owns per-frame binding, synchronization, and table upload
- shaders receive integer IDs and index into global tables

## Non-Goals

- Do not build a render graph.
- Do not redesign the whole asset system.
- Do not make every Vulkan descriptor bindless immediately.
- Do not remove `SceneAsset` resource ownership.
- Do not make bindless texture lifetime independent from asset lifetime.
- Do not implement sparse residency, virtual texturing, or streaming.
- Do not require ReSTIR reservoirs before the bindless material path is useful.
- Do not solve all glTF material features in the first pass.
- Do not make Forward+ a prerequisite.

## Constraints In The Current Code

- `SceneAsset` already owns asset-scoped texture objects in `asset_textures`, plus mesh buffers and BLAS resources. This is a good lifetime boundary for bindless registration and unregistration.
- `Texture::texturePool` currently maps names to texture objects. Bindless should not rely only on names for shader access; shaders need compact integer indices.
- `GltfMaterialInfo` currently stores texture names. Bindless material data should eventually store texture indices, while keeping names useful for loading, hot reload, and debug UI.
- `GltfMaterial` currently creates one descriptor set per material: material UBO plus albedo, normal, ORM, and emissive textures.
- `DescriptorSetLayout::addBinding` currently creates single-descriptor bindings only. Bindless needs descriptor counts greater than one and descriptor indexing flags.
- The descriptor pool is currently small and fixed. Bindless needs larger descriptor counts and likely update-after-bind pool/layout flags.
- Vulkan device creation currently enables ray query and buffer device address, but not descriptor indexing features.
- TLAS instance custom index is currently the temporary TLAS instance order. RTGI needs that index to map to durable scene instance data.

## Milestone 0: Vocabulary And Limits

Estimated time: 1-2 evenings.

Goal: define exactly what "bindless" means for `niar` before changing descriptor code.

Tasks:

- Define maximum bindless counts for the first version, such as max sampled textures and max materials.
- Decide whether the first texture table is per-scene, global, or hybrid.
- Decide whether default textures occupy permanent slots.
- Define invalid/fallback indices for white, black, and default normal.
- Define `GpuMaterial` with scalar factors and texture indices.
- Define `GpuSceneInstance` with material index and geometry lookup data.
- Document which material texture types are in v1: albedo, normal, ORM, emissive.

Done when:

- there is one written shader-facing layout for material and texture indices
- fallback texture behavior is explicit
- the ownership boundary between `SceneAsset` and bindless registry is clear

Recommendation:

- Start with global permanent slots for default textures.
- Use asset-scoped registration for glTF textures.
- Keep the first texture table fixed-size and simple.

## Milestone 1: Vulkan Descriptor Indexing Support

Estimated time: 1-2 weekends.

Goal: make the Vulkan wrapper capable of binding arrays of sampled textures.

Tasks:

- Enable required Vulkan 1.2 descriptor indexing features:
  - `shaderSampledImageArrayNonUniformIndexing`
  - `descriptorBindingPartiallyBound`
  - `runtimeDescriptorArray` if using unsized arrays
  - `descriptorBindingVariableDescriptorCount` if using variable-sized arrays
  - `descriptorBindingUpdateAfterBind` if descriptors may be updated after command buffers bind them
- Add descriptor set layout support for descriptor counts greater than one.
- Add descriptor binding flags through `VkDescriptorSetLayoutBindingFlagsCreateInfo`.
- Add descriptor pool support for large sampled image counts.
- Add helper methods for writing one texture slot and writing a range of texture slots.
- Add debug names for bindless layouts, descriptor sets, and texture slots where useful.

Done when:

- a shader can sample from `sampler2D Textures[N]` using a non-constant index
- validation layers are clean for partially populated fallback slots
- descriptor helper APIs can still serve normal non-bindless descriptors

Likely pain points:

- `nonuniformEXT` requirements in GLSL
- descriptor pool exhaustion
- layout cache equality ignoring binding flags
- update-after-bind rules requiring matching pool and layout flags

Guardrail:

- Prefer fixed-size descriptor arrays first if they simplify setup. Runtime arrays can come later.

## Milestone 2: Bindless Texture Registry

Estimated time: 1-3 weekends.

Goal: assign stable GPU texture indices to live `Texture2D` objects.

Tasks:

- Add a `BindlessTextureRegistry` or similarly small renderer-owned component.
- Reserve slots for `_white`, `_black`, and `_defaultNormal`.
- Register `Texture2D` objects and return `uint32_t` texture indices.
- Unregister asset-owned textures when the asset unloads.
- Keep released slots reusable, but avoid recycling a slot while the GPU might still reference old descriptors.
- Update bindless sampled image descriptors when textures are registered or removed.
- Fill empty slots with `_black` or another explicit fallback texture.
- Add debug logging for slot assignment, release, and fallback use.

Done when:

- loaded glTF textures have bindless texture indices
- hot reload can unload and reload a scene without stale texture descriptors
- released slots do not crash frames that are already in flight

Likely pain points:

- asset hot reload while frames are in flight
- duplicate texture names from multiple glTF assets
- deciding whether a texture object's destructor should unregister itself or whether `SceneAsset` should do it explicitly

Recommendation:

- Let `SceneAsset` continue owning `Texture2D*`.
- Let `SceneAsset` register textures after creation and unregister them before deletion.
- Avoid hiding bindless lifetime changes inside `Texture2D` destructors until the ownership story is proven.

## Milestone 3: Bindless Material Table

Estimated time: 2-4 weekends.

Goal: turn glTF material info into shader-readable material records with texture indices.

Tasks:

- Add a `GpuMaterial` struct shared between C++ and GLSL layout definitions.
- Store base color, emissive, ORM factors, normal strength, alpha clip threshold, flags, and texture indices.
- Build material records from `GltfMaterialInfo`.
- Add a storage buffer containing all active material records.
- Add a material registry that maps material identity to material index.
- Handle duplicate material names and hot-reload versioning explicitly.
- Keep material records renderer-independent where possible.

Done when:

- shaders can read material constants from a storage buffer by material index
- material records contain bindless texture indices for albedo, normal, ORM, and emissive
- default texture indices are used when glTF omits a texture

Likely pain points:

- current `GltfMaterialInfo` is name-keyed, but bindless needs stable indices
- duplicate material names can collide across assets
- CPU-side struct packing must match GLSL `std430`

Guardrail:

- Do not make the material registry responsible for owning textures. It should reference texture indices assigned elsewhere.

## Milestone 4: Scene Instance And Geometry Tables

Estimated time: 2-4 weekends.

Goal: let ray hits and raster draws map scene instances to material and geometry data.

Tasks:

- Add a GPU scene instance table with material index, mesh index, vertex offset, index offset, and transform-related data if needed.
- Make TLAS `instanceCustomIndex` point to a durable scene instance table entry.
- Add mesh/primitive metadata needed to fetch triangle vertices and UVs in shaders.
- Ensure combined vertex/index buffers can be used as storage buffers or buffer references.
- Add storage-buffer usage to vertex and index buffer creation if needed.
- Decide whether geometry lookup uses storage buffers or buffer device addresses first.

Done when:

- RTGI can get instance index and primitive index from ray query
- RTGI can fetch material index for the hit instance
- RTGI can compute hit UVs or at least fetch per-triangle attributes needed for texture sampling

Likely pain points:

- current vertex/index buffers are created for vertex/index/AS input/device-address usage, not general storage-buffer usage
- multi-primitive glTF meshes need primitive-level material identity
- scene traversal order must match TLAS instance table order

Guardrail:

- Keep the scene instance table as shared infrastructure. Do not bury it inside `GI`.

## Milestone 5: Raster Uses Material Indices

Estimated time: 2-5 weekends.

Goal: migrate raster material shading to the bindless material table without breaking the deferred renderer.

Tasks:

- Pass material index to raster shaders, probably via push constant or per-draw data.
- Bind global material and texture tables in the geometry pass.
- Update `geometry.frag`, `simple_gltf.frag`, and translucent material shaders to read `GpuMaterial`.
- Replace per-material texture descriptors with bindless texture lookup.
- Keep old material descriptor path temporarily behind a small compatibility branch while validating output.
- Remove old per-material descriptor sets once raster and RTGI both use bindless material data.

Done when:

- deferred G-buffer output matches the old material path for common scenes
- shader hot reload still works
- material sorting and pipeline binding still behave sensibly
- material texture changes from asset reload appear through the bindless path

Likely pain points:

- deciding where material index lives for raster draw calls
- alpha clip and translucent behavior using bindless albedo alpha
- normal map sampling and tangent-space reconstruction consistency

Guardrail:

- Do not try to optimize draw call binding yet. The point of this milestone is shared material access, not GPU-driven rendering.

## Milestone 6: RTGI Minimal Hit Shading With Textures

Estimated time: 3-6 weeks.

Goal: make RTGI ray hits return scene-aware material color using the bindless path.

Tasks:

- Extend `rtgi_generate.comp` to retain committed ray query hit information.
- Read instance custom index and primitive index from the committed hit.
- Fetch scene instance, material, and geometry data.
- Interpolate UVs at the hit point.
- Sample bindless albedo texture and multiply by base color factor.
- Optionally sample emissive texture and factor.
- Return simple diffuse albedo/emissive contribution before attempting full BSDF correctness.
- Add debug views for material index, texture index, UV, albedo, and emissive.

Done when:

- colored textured objects affect nearby indirect lighting
- emissive objects can be identified or contribute a simple signal
- missing textures and unloaded slots resolve to defaults instead of crashing

Likely pain points:

- barycentric coordinates from ray query
- coordinate-space mistakes around transformed normals
- alpha-tested geometry versus `gl_RayFlagsOpaqueEXT`
- texture derivatives are unavailable in ray/compute hit shading, so explicit LOD may be needed

Guardrail:

- Keep first RTGI hit shading diffuse and approximate. Bindless texture access is the infrastructure win.

## Milestone 7: Hot Reload And Lifetime Hardening

Estimated time: 2-4 weeks.

Goal: make bindless resources survive normal `niar` iteration.

Tasks:

- Stress test scene hot reload while RTGI is enabled.
- Stress test deleting and reloading a scene with different texture counts.
- Add frame-latency handling for descriptor slot reuse.
- Add asserts or debug UI for live texture slots and material records.
- Add clear fallback behavior for stale material indices during reload.
- Ensure descriptor updates happen at a safe point relative to command buffer recording.

Done when:

- repeated hot reload does not leak descriptors, stale image views, or invalid material indices
- RenderDoc captures show understandable bindless descriptors and material buffers
- validation layers remain quiet during reload and renderer switching

Likely pain points:

- resources deleted while descriptors still reference their image views
- stale material indices inside cached renderer objects
- renderer mode switches while asset reload is in progress

## Milestone 8: Cleanup And Shared Material Model

Estimated time: 2-6 weeks.

Goal: remove temporary compatibility code and make bindless the normal material path.

Tasks:

- Remove old per-material descriptor allocation for glTF materials.
- Fold duplicate material parameter structs into the shared `GpuMaterial` definition.
- Make material debug UI inspect bindless material records and texture slots.
- Document shader set/binding conventions for bindless resources.
- Reassess descriptor set layout organization after raster and RTGI both use the path.
- Reassess whether material/instance tables should move into a broader scene GPU data component.

Done when:

- glTF materials have one primary GPU representation
- raster and RTGI use the same material constants and texture indices
- bindless descriptor code is small, named, and isolated enough to maintain

## Suggested Descriptor Set Shape

Keep the existing update-frequency convention, but add a shared scene/material set:

- Set 0: frame-global data, G-buffer inputs where relevant, lights, TLAS
- Set 1: independent feature data such as sky
- Set 2: bindless scene data
- Set 3: per-object or legacy dynamic data during migration

Possible set 2 layout:

- binding 0: `storage buffer GpuMaterial[]`
- binding 1: `storage buffer GpuSceneInstance[]`
- binding 2: `storage buffer GpuMesh[]` or geometry metadata
- binding 3: `sampler2D BindlessTextures[]`
- binding 4: optional vertex data buffer
- binding 5: optional index data buffer

Exact bindings can change, but the important rule is that material texture access lives in one shared set instead of per-material descriptor sets.

## Shader Sketch

```glsl
#extension GL_EXT_nonuniform_qualifier : require

struct GpuMaterial {
    vec4 baseColorFactor;
    vec4 emissiveFactor_clipThreshold;
    vec4 orm_normalStrengths_flags;
    uvec4 textureIndices; // albedo, normal, orm, emissive
};

layout(set = 2, binding = 0, std430) readonly buffer MaterialTable {
    GpuMaterial Materials[];
};

layout(set = 2, binding = 3) uniform sampler2D BindlessTextures[];

vec4 sampleAlbedo(uint materialIndex, vec2 uv)
{
    GpuMaterial material = Materials[materialIndex];
    uint textureIndex = material.textureIndices.x;
    return texture(BindlessTextures[nonuniformEXT(textureIndex)], uv) * material.baseColorFactor;
}
```

For RTGI, use `textureLod(..., 0.0)` or an explicit LOD policy until there is a better roughness/distance-aware choice.

## Asset Ownership Notes

`SceneAsset` is already close to the right model:

- it creates glTF textures as part of scene loading
- it stores the resulting `Texture2D*` objects in `asset_textures`
- it deletes those textures in `release_resources`
- it owns combined mesh buffers and BLAS collections in the same asset lifetime

Bindless should build on that:

- register each asset texture with the bindless texture registry after creation
- store returned texture indices where material building can use them
- unregister texture slots before deleting `Texture2D` objects
- update material records after texture indices are assigned
- keep default textures registered globally for fallback use

This preserves hot-reload friendliness while giving shaders stable integer handles for the duration of a loaded asset.

## ReSTIR GI Impact

Bindless helps ReSTIR GI once rays need real hit shading:

- local candidates can evaluate textured diffuse albedo
- emissive hits can become real candidates
- temporal and spatial validation can use material IDs
- reservoir debug views can show selected material and texture identity

Bindless does not solve:

- ray bias
- noise
- temporal reprojection
- reservoir weighting
- denoising
- motion vectors
- alpha-tested ray traversal correctness

The right success criterion is not "the renderer is modern." The right success criterion is that both raster and RTGI can ask the same question: given a material index and UV, what material response do I get?

## Recommended Next Commit

Start with a small foundation commit:

- add a bindless roadmap document
- add descriptor layout support for descriptor counts and binding flags
- enable descriptor indexing features
- create a fixed-size bindless sampled texture set
- register default textures into permanent slots
- add one tiny debug shader or compute path that samples a texture by integer index

Do not migrate glTF materials in the same commit. First prove that indexed texture sampling works cleanly under validation.

## Reading Order

Suggested order, from most immediately useful to more architectural:

1. Vulkan descriptor indexing feature and layout rules.
2. `GL_EXT_nonuniform_qualifier` and non-uniform resource indexing.
3. NVIDIA/AMD examples of bindless texture arrays in Vulkan.
4. glTF material texture/sampler model.
5. Ray query hit attribute and barycentric coordinate access.
6. GPU-driven rendering and meshlet material tables, later reference only.
