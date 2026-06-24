# Path to Bindless Materials

This roadmap adds bindless material infrastructure to `niar` and applies it to
the renderer paths that already use glTF materials. RTGI Milestone 2 is the
immediate motivation, but the bindless path should be useful without RTGI:
raster materials should stop owning duplicate texture descriptor sets and
instead consume the same GPU material records and texture table as ray-hit
shading.

The target is narrower than a completely bindless renderer, but complete for
the current glTF material use case:

- one global fixed-size table of combined 2D image samplers
- optional bindless registration on `Texture2D`
- asset-owned texture lifetime
- GPU material records containing bindless texture indices
- deferred, simple, and translucent glTF raster shaders using those records
- GPU instance and geometry records for ray-hit lookup
- textured diffuse and emissive evaluation in `rtgi_generate.comp`

The work order is intentionally hybrid:

1. build the shared bindless texture and material infrastructure
2. prove it against the deferred opaque geometry path
3. use it for RTGI Milestone 2
4. migrate SimpleRenderer and translucent materials
5. remove the remaining legacy glTF descriptors

This avoids maintaining an RTGI-only material system, but also avoids making
every secondary raster path a prerequisite for the next visible GI result.

Assumption for estimates: evenings and weekends. The individual phases should
also be small enough to land as separate commits.

## Chosen V1 Design

### Texture ownership

`SceneAsset`, `EnvironmentMapAsset`, and other resource owners continue to own
their `Texture2D` objects. A texture can optionally register itself with the
global bindless table when constructed and unregister itself when destroyed.
This makes the owning asset transitively own the registration.

Registration must remain optional. G-buffer images, depth, GI output, and other
renderer-owned images should not consume sampled-texture slots unless requested.
`Texture2D` must continue exposing its Vulkan image and image view for barriers,
attachments, storage descriptors, transfers, and non-bindless paths.

### Shared bindless material set

Reserve set 2 for renderer-wide bindless material resources:

```glsl
layout(set = 2, binding = 0) uniform sampler2D BindlessTextures[1024];

layout(set = 2, binding = 1, std430) readonly buffer MaterialTable {
    GpuMaterial Materials[];
};
```

Each entry is a combined `(VkImageView, VkSampler)` descriptor. The caller
provides the sampler configuration during texture construction or registration.
glTF sampler objects are ignored for now; imported material textures use the
current default sampler.

Use a free-list for slot allocation. Do not scan the whole array for every
registration.

Use a small shared owner, called `BindlessResources` in this roadmap, for set 2.
It composes:

- a texture table responsible for binding 0, texture slots, and generations
- a material table responsible for binding 1 and material indices
- the descriptor set and layout used by raster and RTGI pipelines

`BindlessResources` does not own the underlying textures. Assets continue to
own texture objects and source material data.

### Handles

Use a CPU-side typed handle:

```cpp
struct BindlessTexture2DHandle
{
    uint32_t index = InvalidBindlessIndex;
    uint32_t generation = 0;
};
```

The generation detects stale CPU handles after slot reuse. Shaders receive only
the validated `index`; GPU-side generation checking is not needed for v1.

Do not include a runtime resource-type field in the handle yet. A typed
`BindlessTexture2DHandle` cannot accidentally be used as a future cube or
storage-image handle.

### Unregistration and synchronization

V1 may call `waitDeviceIdle()` before clearing/reusing a descriptor slot and
before the texture destroys its image view. This is slow but matches the
existing hot-reload workflow and makes lifetime correctness explicit.

Deferred destruction and fence/submit-serial retirement can replace this later.
Do not implement an arbitrary "wait N frames" policy without also retaining the
old image and image view for the same period.

### Defaults

Register permanent default textures first and reserve known indices:

- white: missing albedo and ORM
- black: missing emissive and empty descriptor slots
- default normal: missing normal map

Material creation chooses the semantically correct fallback. A generic lookup
failure returning black is not sufficient for all material channels.

### Shader access

Put bindless declarations and helpers in one shared GLSL include:

```glsl
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 2, binding = 0) uniform sampler2D BindlessTextures[1024];

vec4 sampleBindlessTexture2D(uint index, vec2 uv)
{
    return texture(BindlessTextures[nonuniformEXT(index)], uv);
}

vec4 sampleBindlessTexture2DLod(uint index, vec2 uv, float lod)
{
    return textureLod(BindlessTextures[nonuniformEXT(index)], uv, lod);
}
```

RTGI has no implicit fragment derivatives, so its first material lookup should
use the explicit-LOD helper. `lod = 0` is acceptable for the first working hit.

## V1 Non-Goals

- Bindless storage images.
- Bindless render targets unless sampled indexing is immediately useful.
- 3D textures or cube textures.
- Separate image and sampler arrays.
- glTF sampler fidelity.
- Variable descriptor counts or unsized descriptor arrays.
- Descriptor update-after-bind.
- Deferred GPU resource destruction.
- GPU-driven draws.
- A general bindless buffer registry.
- Alpha-tested ray traversal.
- Full PBR or normal-map evaluation at the secondary hit.

## Phase 0: Confirm Limits and Interfaces

Estimated time: 1-2 evenings.

Goal: lock the concrete constants and interfaces before changing Vulkan setup.

Tasks:

- Query and log the relevant sampled-image descriptor limits.
- Assert that the selected capacity, initially 1024, is supported.
- Reserve descriptor set 2 for shared bindless textures and materials.
- Add a named `DSET_BINDLESS`/`DSET_MATERIALS` constant for set 2 rather than
  scattering the literal index.
- Define `MAX_BINDLESS_TEXTURES_2D` in one C++ location mirrored by one GLSL
  definition.
- Define `InvalidBindlessIndex`.
- Define `BindlessTexture2DHandle`.
- Decide how `Texture2D` opts into registration.

Suggested constructor shape:

```cpp
struct BindlessTexture2DInfo
{
    bool registerTexture = false;
    VkSamplerCreateInfo samplerInfo = SamplerCache::defaultInfo();
};
```

Alternatively, use an optional pointer/reference to sampler info. Avoid adding
several positional boolean and sampler arguments to every constructor.

Done when:

- the capacity and required device limits are visible at startup
- the handle and optional registration API are agreed upon
- existing non-bindless texture constructors remain easy to call

## Phase 1: Descriptor Array Support

Estimated time: 1-2 weekends.

Goal: create and bind a fixed array of combined image samplers.

Tasks:

- Extend `DescriptorSetLayout::addBinding` to accept `descriptorCount`.
- Include descriptor count in layout-cache behavior, which it already compares.
- Increase or separate the descriptor pool capacity for the bindless set.
- Add a descriptor write helper accepting:
  - destination binding
  - destination array element
  - image view
  - sampler
  - image layout
- Enable `shaderSampledImageArrayNonUniformIndexing` in Vulkan 1.2 features.
- Do not enable runtime arrays, variable descriptor counts, partially-bound
  bindings, or update-after-bind unless validation proves one is necessary.
- Allocate one persistent bindless descriptor set.
- Bootstrap the defaults before any pipeline can use the set:
  - allocate the table and descriptor set
  - create the default texture objects without automatic registration
  - install them into reserved slots explicitly
  - fill every remaining array element with the black descriptor

Suggested ownership:

```cpp
class BindlessResources
{
public:
    static BindlessResources* Instance;

    void init();
    void release();
    const DescriptorSet& descriptorSet() const;
    const DescriptorSetLayout& layout() const;

    BindlessTexture2DHandle addTexture2D(
        VkImageView imageView,
        const VkSamplerCreateInfo& samplerInfo);

    void removeTexture2D(BindlessTexture2DHandle handle);
    uint32_t validate(BindlessTexture2DHandle handle) const;

    uint32_t addMaterial(const GpuMaterial& material);
    void updateMaterial(uint32_t index, const GpuMaterial& material);
    void removeMaterial(uint32_t index);
};
```

The texture-table portion owns descriptor slots, generation counters, and the
free-list. The material-table portion owns the GPU material buffer and material
indices. Neither owns textures, image views, or asset source data.

Create binding 1 as part of the final set 2 layout from the beginning. Point it
at a small valid placeholder material buffer until the real material table is
implemented, so pipeline layouts do not change midway through migration.

Done when:

- a test shader samples two textures selected by a non-constant integer
- all 1024 descriptors are valid
- validation layers are quiet
- ordinary descriptor sets still work unchanged

Likely pain points:

- descriptor pool exhaustion
- placing the Vulkan 1.2 feature struct correctly in the device feature chain
- forgetting `nonuniformEXT`
- trying to create defaults before `BindlessResources` is initialized

## Phase 2: Optional Texture Registration

Estimated time: 1 weekend.

Goal: make bindless registration part of a sampled texture's optional lifetime.

Tasks:

- Add an optional `BindlessTexture2DHandle` to `Texture2D`.
- Register only after image creation, image-view creation, and initial layout
  transitions are complete.
- Store the caller-selected sampler used for the combined descriptor.
- Expose a const getter for the handle or validated shader index.
- On destruction:
  - if registered, ask `BindlessResources` to unregister it
  - `BindlessResources` waits for device idle in v1
  - replace the slot descriptor with black
  - increment its generation
  - return the slot to the free-list
  - then destroy the image view and image
- Keep `resource`, `imageView`, dimensions, and format available to renderer
  code.
- Keep `Texture::texturePool` temporarily, but remove entries when pooled
  textures are destroyed so it cannot return dangling pointers.

Initialization order:

1. Create Vulkan device and descriptor infrastructure.
2. Initialize the empty `BindlessResources` allocation.
3. Create default texture objects without automatic registration.
4. Install defaults into reserved bindless slots.
5. Fill all remaining descriptors with black.
6. Load normal assets.

Shutdown order:

1. Release assets and their registered textures.
2. Destroy default textures.
3. Assert that no non-default bindless registrations remain.
4. Release `BindlessResources`.
5. Destroy the Vulkan device.

Done when:

- a registered texture receives a stable valid index
- a non-registered render target consumes no slot
- destroying a registered texture safely restores and frees its slot
- stale CPU handles fail generation validation
- repeated scene hot reload does not leave stale image views in descriptors

## Phase 3: Register glTF Material Textures

Estimated time: 1-2 weekends.

Goal: make all textures used by current glTF materials addressable through
bindless indices.

Tasks:

- Construct `SceneAsset` image textures with bindless registration enabled.
- Use the default sampler for all imported textures in v1.
- Keep `asset_textures` as the owning collection.
- Build an asset-local mapping from tinygltf image/texture indices to
  `BindlessTexture2DHandle` or validated shader index.
- Stop relying on globally unique image names when building new GPU material
  records.
- Keep the existing name-based `GltfMaterialInfo` fields temporarily while the
  raster shaders and CPU path tracer still consume them.
- Register `EnvironmentMapAsset` only if a bindless consumer needs it. RTGI can
  continue using its existing fixed descriptor for now.

Done when:

- every glTF albedo, normal, ORM, and emissive texture has a valid bindless index
- missing channels map to the appropriate permanent default index
- two assets with duplicate image names do not collide in the new mapping
- scene reload removes old registrations before deleting image views

Guardrail:

- Asset-local glTF indices are the loading identity. Names are debug labels and
  temporary compatibility lookup only.

## Phase 4: GPU Material Table

Estimated time: 1-2 weekends.

Goal: give all renderer paths one GPU representation of glTF material parameters
and bindless texture indices.

Define a std430-friendly record, for example:

```cpp
struct GpuMaterial
{
    glm::vec4 baseColorFactor;
    glm::vec4 emissiveFactorAndClipThreshold;
    glm::vec4 ormAndNormalStrength;
    glm::uvec4 textureIndices; // albedo, normal, orm, emissive
};
```

Tasks:

- Add matching C++ and GLSL definitions.
- Add size/alignment assertions on the C++ struct.
- Build material records directly while `SceneAsset` still has tinygltf
  material and texture-index context.
- Store the material records in asset-local CPU data.
- Upload active records through the material-table portion of
  `BindlessResources`.
- Point set 2 binding 1 at the material storage buffer.
- Give each loaded material a stable material index for the lifetime of the
  loaded scene.
- Put the material index somewhere existing `GltfMaterial` objects can obtain
  it during raster migration.
- Continue producing `GltfMaterialInfo` temporarily for material construction,
  CPU path tracing, and compatibility while raster migration is in progress.
- On hot reload, rebuild the material table after new textures have registered.

Relevant descriptor shape:

- set 2, binding 0: `sampler2D BindlessTextures[1024]`
- set 2, binding 1: `readonly storage buffer GpuMaterial[]`

Done when:

- a compute debug pass can select a material index and display its bindless
  albedo texture
- all material records contain valid texture indices
- albedo, ORM, normal, and emissive defaults are correct
- scene reload rebuilds records without retaining old texture indices

## Phase 5: Prove Bindless In Deferred Opaque

Estimated time: 1-2 weekends.

Goal: make the primary deferred opaque glTF path use set 2 instead of its
per-material UBO and texture descriptors.

This is the proving step for the shared material representation. Deferred opaque
is the primary visibility path, exercises all four current material textures,
and shades the same opaque surfaces RTGI will hit.

Tasks:

- Add a shared GLSL material include containing:
  - `GpuMaterial`
  - set 2 declarations
  - material lookup by index
  - bindless sampling helpers
- Pass `materialIndex` to raster shaders.
- The simplest current path is to extend the existing push constants:
  - keep the model matrix at offset 0
  - add a `uint materialIndex` at offset 64
  - use a vertex-stage range for the matrix and a fragment-stage range for the
    material index
- Add the material-index push-constant range to all glTF pipeline layouts during
  the migration bridge, even if SimpleRenderer and translucency do not consume
  it yet. This keeps the shared `GltfMaterial::setPerDrawParameters` call valid.
- Update `GltfMaterial::setPerDrawParameters` to push both model matrix and
  material index, preferably as two explicit `vkCmdPushConstants` calls with
  the matching offsets and stage flags.
- Add set 2 to `PbrGltfMaterial`'s pipeline layout.
- Bind set 2 for the deferred geometry pass.
- Update `geometry.frag` to consume `GpuMaterial` and bindless textures.
- Replace `materialParams`, `AlbedoMap`, `NormalMap`, `ORMMap`, and
  `EmissiveMap` in the deferred opaque shader with `GpuMaterial` plus bindless
  samples.
- Preserve current alpha clipping, normal-map strength, ORM factors, emissive
  packing, and G-buffer output behavior.
- Compare the old and new paths in representative scenes before deleting the
  deferred opaque bindings.
- Keep the legacy `GltfMaterial::dynamicSet` and material UBO alive because
  SimpleRenderer and translucent materials still use them.
- Stop binding set 3 only for the deferred opaque pipeline once its layout no
  longer declares that set.
- Keep material/pipeline sorting if it remains useful. Bindless material access
  does not require changing draw submission yet.

Potential CPU-side push-constant definitions:

```cpp
constexpr uint32_t GltfModelMatrixOffset = 0;
constexpr uint32_t GltfMaterialIndexOffset = sizeof(glm::mat4);
```

Respect `VkPhysicalDeviceLimits::maxPushConstantsSize` and use explicit offsets
and stage flags in C++ and GLSL. Avoid relying on C++ tail padding for a combined
struct. If extending push constants becomes awkward, use a small per-draw
dynamic buffer instead; do not keep the old per-material texture descriptor set
merely to transport the integer.

Done when:

- deferred G-buffer output matches the old material path
- alpha clipping still works
- deferred opaque glTF draws bind no per-material texture descriptors
- SimpleRenderer and translucent materials remain unchanged and working
- scene hot reload updates deferred material textures through set 2
- shader hot reload still works

Likely pain points:

- pipeline layouts must include set 2 even if set 1 is unused
- push-constant range stage flags and offsets
- material-table index changes during asset reload
- avoiding a generic material bind call that tries to bind set 3 against the new
  deferred opaque pipeline layout

Guardrail:

- Do not combine this with GPU-driven drawing, indirect draws, or removal of
  material sorting. The milestone is shared material lookup, not draw-system
  modernization.

## Phase 6: Ray-Hit Instance and Geometry Data

Estimated time: 2-4 weekends.

Goal: map a committed ray-query hit to a material and interpolated UV.

The required chain is:

```text
committed hit
  -> instanceCustomIndex
  -> GpuSceneInstance
  -> materialIndex
  -> GpuMaterial
  -> bindless texture index

committed hit
  -> primitiveIndex + instance geometry offsets
  -> triangle indices
  -> vertex UVs
  -> barycentric interpolation
```

Suggested records:

```cpp
struct GpuSceneInstance
{
    uint32_t materialIndex;
    uint32_t vertexOffset;
    uint32_t indexOffset;
    uint32_t reserved;
};
```

Add more geometry metadata only when the shader actually needs it.

Tasks:

- Build `GpuSceneInstance` entries in exactly the order used to build TLAS
  instances.
- Set `instanceCustomIndex` to the corresponding table index.
- Preserve a stable table-to-TLAS mapping for the recorded frame.
- Expose vertex and index data to compute shaders:
  - simplest path: add `VK_BUFFER_USAGE_STORAGE_BUFFER_BIT` and bind the
    combined buffers as storage buffers
  - alternative: use buffer device address and GLSL buffer references
- Prefer storage buffers first unless buffer references remove a concrete
  multi-asset binding problem.
- Add instance, vertex, and index buffers to the existing RTGI set 0.
- Bind the shared material set 2 alongside RTGI sets 0 and 1.
- In `rtgi_generate.comp`, retrieve committed:
  - instance custom index
  - primitive index
  - barycentric coordinates
- Fetch the triangle and interpolate UVs.
- Add debug output modes for instance index, material index, primitive index,
  barycentrics, and UVs before sampling textures.

Possible descriptor layout after this phase:

- set 0: existing RTGI resources, then scene instance, vertex, and index storage
  buffers
- set 1: existing sky resources
- set 2, binding 0: global combined sampled texture array
- set 2, binding 1: shared material storage buffer

Done when:

- each ray hit resolves to the correct material index
- a UV checker sampled at the secondary hit follows the mesh UVs
- multi-primitive meshes retain the correct primitive material
- TLAS rebuilds and scene reloads cannot silently reorder data incorrectly

Likely pain points:

- one global vertex/index binding does not naturally cover multiple independent
  asset buffers
- current combined buffers belong to individual assets
- 16-bit index decoding in GLSL
- matching the CPU `Vertex` memory layout in GLSL
- preserving per-primitive material identity

Decision checkpoint:

- If multiple scene assets must be active simultaneously, either consolidate
  their geometry into renderer-owned scene buffers or use device-address-based
  geometry records. Do not create one descriptor binding per asset.

## Phase 7: Resume RTGI Milestone 2

Estimated time: 1-3 weekends after geometry lookup works.

Goal: replace the current hit-is-black result with minimal textured hit shading.

Tasks:

- Change the ray-query helper to retain the committed query instead of returning
  only hit/miss.
- On miss, keep the existing environment/sky result.
- On hit:
  - resolve instance and material
  - interpolate UV
  - sample albedo with `textureLod(..., 0.0)`
  - multiply by base color factor
  - sample emissive and multiply by emissive factor
- Begin with a deliberately simple result:

```glsl
hitRadiance = emissive + debugBounceScale * albedo;
```

- Keep normal map, ORM, direct lighting at the secondary hit, and recursive
  visibility out of the first version.
- Add debug modes for raw albedo, emissive, texture index, and material index.

Done when:

- textured colored objects affect the indirect-lighting buffer
- emissive materials can be identified and contribute
- default textures produce sensible values
- miss lighting remains unchanged
- direct lighting and RT shadows remain unchanged

This is the handoff back to Milestone 2 of `path-to-restir.md`. Continue material
fidelity only as needed. Finish the remaining raster migration before calling
bindless materials v1 complete; RTGI temporal work does not need to wait if the
remaining migration is straightforward and isolated.

## Phase 8: Finish Raster Migration and Remove Legacy Descriptors

Estimated time: 1-3 weekends.

Goal: move the remaining glTF raster consumers onto the shared material set and
remove the duplicated per-material GPU representation.

Tasks:

- Add set 2 to `SimpleGltfMaterial` and `PbrTranslucentGltfMaterial` pipeline
  layouts.
- Bind set 2 in:
  - `SimpleRenderer`
  - the deferred translucency pass
- Update:
  - `simple_gltf.frag`
  - `translucency_lit.frag`
- Preserve simple-renderer output, alpha behavior, normal mapping, ORM factors,
  emissive shading, and translucent blending.
- Confirm all glTF raster shaders now read `GpuMaterial`.
- Remove the old set 3 glTF material declarations.
- Remove `GltfMaterial::dynamicSet`.
- Remove the per-material material-parameter UBO.
- Remove or simplify `bindMaterialDescriptors` for glTF materials.
- Remove name-based texture lookup from GPU raster material construction where
  it is no longer needed. Keep it only for CPU consumers or debug compatibility.
- Reassess material sorting after the migration, but retain it unless removing
  it has a concrete benefit.

Done when:

- deferred, simple, and translucent output matches the old path
- no glTF raster pipeline allocates or binds per-material texture descriptors
- `GpuMaterial` is the sole GPU material representation for glTF materials
- scene and shader hot reload work across all migrated paths
- deleting the legacy descriptors does not affect RTGI

Guardrail:

- This phase removes migration scaffolding. It should not expand into a general
  material-system rewrite or GPU-driven renderer.

## Suggested Commit Sequence

1. `bindless: add fixed sampled texture descriptor array`
2. `bindless: add optional Texture2D registration`
3. `assets: register glTF material textures`
4. `materials: upload GPU material table`
5. `materials: migrate deferred opaque to bindless materials`
6. `rt: add scene instance material mapping`
7. `rt: expose triangle geometry and interpolate hit UV`
8. `rtgi: sample bindless material textures at ray hits`
9. `materials: migrate simple and translucent shaders`
10. `materials: remove per-material glTF descriptors`

Each commit should build `ellyn`. Shader-related commits should also compile all
Vulkan shaders and run with validation layers enabled.

## Validation Checkpoints

Before starting RTGI-specific instance and geometry work:

- Startup reports the selected capacity and device limits.
- Default indices are stable and documented.
- Every descriptor array element contains a valid descriptor.
- Texture registration exhaustion fails loudly.
- Double unregistration and stale-handle use assert in debug builds.
- Slot reuse increments generation.
- Registered texture destruction waits for GPU idle before destroying its view.
- Scene hot reload works repeatedly with validation enabled.
- Duplicate image names across assets do not affect GPU material lookup.
- RenderDoc shows the expected image/sampler in selected descriptor slots.
- Deferred opaque output matches the old path.
- Deferred opaque glTF materials no longer bind their per-material texture set.

Before considering the RTGI Milestone 2 bindless handoff complete:

- Ray hits resolve the expected instance and material.
- Barycentric UV interpolation follows the hit mesh.
- RTGI debug views correctly show instance, material, UV, albedo, and emissive.
- Textured albedo and emissive hits contribute to the indirect buffer.

Before calling bindless materials v1 complete:

- SimpleRenderer output matches the old path.
- Translucent output and blending match the old path.
- No glTF raster path allocates or binds per-material texture descriptors.
- `GpuMaterial` is the sole GPU representation used by glTF raster shaders.
- Hot reload works across deferred, simple, translucent, and RTGI paths.

## Follow-Up Work After V1

These are useful after raster and RTGI both consume the v1 material path:

- remove legacy name-based texture lookup from asset/material compatibility code
- honor glTF sampler objects
- use separate image and sampler arrays if useful
- add bindless cube textures
- allow sampled render targets to opt into the same 2D table
- add a separate bindless storage-image table
- replace `waitDeviceIdle()` with fence/submit-serial retirement
- add better explicit LOD selection for ray-hit texture sampling
- support alpha-tested ray traversal
- consider bindless or device-address-based general buffer access
