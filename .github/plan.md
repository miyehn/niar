# Bindless Phase 1: Descriptor Array and Registry

## Goal

Create the persistent set-2 bindless descriptor set, register the existing
default textures through the normal slot allocator, implement texture slot
allocation, and prove non-uniform indexed sampling with a debug startup compute
test.

Do not add automatic `Texture2D` registration or asset integration yet.

## 1. Core Infrastructure Header

Status: complete.

`src/Render/BindlessResources.h` defines:

- fixed capacity and invalid index
- typed generational texture handles
- future texture registration settings with an explicit required sampler
- `BindlessResources` singleton lifecycle and texture APIs
- descriptor set/layout accessors
- descriptor-pool ownership
- slot occupancy, generation, and free-list state

The implementation must preserve this behavior:

- `addTexture2D(...)` returns a handle containing slot index and generation
- `removeTexture2D(...)` waits for device idle, replaces the released descriptor
  with a valid filler descriptor, increments the generation, and returns the
  slot to the free-list
- `validate(...)` treats invalid and stale handles as errors; it does not
  silently return a default texture

## 2. Descriptor Wrapper Support

Status: complete.

- Extend `DescriptorSetLayout::addBinding(...)` with a defaulted
  `descriptorCount = 1`.
- Keep descriptor count in descriptor-layout cache identity.
- Add a descriptor image-array writer accepting:
  - binding
  - first array element
  - descriptor type
  - a contiguous collection of `VkDescriptorImageInfo`
- Refactor `pointToImageView(...)` to use the array writer.
- Add descriptor allocation from a caller-provided `VkDescriptorPool`.
- Keep the existing shared static pool unchanged.

## 3. BindlessResources Implementation

Add `src/Render/BindlessResources.cpp` and include it in `ellyn`.

Initialization:

- Create a dedicated descriptor pool with capacity for:
  - 1024 combined image samplers
  - one storage-buffer descriptor
  - one descriptor set
- Create set 2 with:
  - binding 0: 1024 combined image samplers, fragment and compute visibility
  - binding 1: one storage buffer, fragment and compute visibility
- Allocate one persistent descriptor set from the dedicated pool.
- Create a 16-byte zeroed placeholder storage buffer and bind it at binding 1.
- Initialize all slot generations to zero and all slots as free.
- Register `_white`, `_black`, and `_defaultNormal` through the same
  `addTexture2D(...)` path used by ordinary textures.
- Store the returned handles on the corresponding `Texture2D` objects.
- After `_black` is registered, fill every still-unused descriptor with its
  combined image sampler so the fixed array is fully valid.
- Do not require particular indices for any default texture.

Texture operations:

- `addTexture2D(...)` pops a free slot, obtains the sampler through
  `SamplerCache`, writes the combined image sampler, marks the slot occupied,
  and returns its current generation.
- Every registration call site must provide a `VkSamplerCreateInfo`; callers
  that want the standard behavior must explicitly pass
  `SamplerCache::defaultInfo()`.
- Exhaustion is a hard error.
- `removeTexture2D(...)` validates the handle, waits for device idle, writes the
  current filler descriptor, marks the slot free, increments generation, and
  returns it to the free-list.
- Invalid, stale, double-removed, and out-of-range handles are hard errors.
- The default textures use normal handles but remain alive until shutdown
  because `Texture2D` owns their lifetime.

Release:

- Require all ordinary asset registrations to be free.
- Unregister the default textures through `Texture2D` before destroying the
  descriptor pool.
- Destroy the dedicated descriptor pool.
- Release the placeholder material buffer.
- Clear the singleton and free-list state.
- Do not destroy texture objects; assets/default-texture ownership remains
  unchanged.

## 4. Vulkan Feature Enablement

- Enable `shaderSampledImageArrayNonUniformIndexing` in the existing Vulkan 1.2
  logical-device feature chain.
- Do not enable partially-bound descriptors, runtime arrays, variable
  descriptor counts, or update-after-bind.
- Retain the Phase 0 suitability check requiring support for this feature and
  the fixed 1024-entry table.

## 5. Shader Interface

Add `shaders/bindless_resources.glsl`:

```glsl
#ifndef _BINDLESS_RESOURCES
#define _BINDLESS_RESOURCES

#extension GL_EXT_nonuniform_qualifier : require

layout(set = DSET_BINDLESS, binding = 0)
uniform sampler2D BindlessTextures[MAX_BINDLESS_TEXTURES_2D];

vec4 sampleBindlessTexture2D(uint index, vec2 uv)
{
    return texture(BindlessTextures[nonuniformEXT(index)], uv);
}

vec4 sampleBindlessTexture2DLod(uint index, vec2 uv, float lod)
{
    return textureLod(BindlessTextures[nonuniformEXT(index)], uv, lod);
}

#endif
```

Do not inject hard-coded default-texture indices. Material records receive
validated indices from the handles stored by the default `Texture2D` objects.

## 6. Debug Startup Self-Test

Add `shaders/bindless_self_test.comp` to the shader registry.

The debug-only test:

- binds set 2
- dispatches two invocations
- receives the runtime white and black handle indices through a small test input
  buffer or push constants
- samples both through `sampleBindlessTexture2DLod(...)`
- writes the results to a host-readable storage buffer
- inserts a compute-to-host memory barrier
- reads the buffer on CPU and checks approximately white and black

Supporting buffer changes:

- Add `VmaBuffer::readData(...)`.
- Support mapped `VMA_MEMORY_USAGE_GPU_TO_CPU` allocations with random host
  access.
- Invalidate the VMA allocation before CPU reads.

Failure is a debug assertion/error. Success logs one concise initialization
message.

## 7. Startup and Shutdown Wiring

Startup order in `Ellyn.cpp`:

1. Create Vulkan.
2. Compile shaders.
3. Create default texture objects.
4. Create the empty `BindlessResources` descriptor infrastructure.
5. Register the default textures and retain their handles on `Texture2D`.
6. Fill unused descriptors with the registered black texture.
7. Run the debug self-test.
8. Load assets and renderers normally.

Shutdown order:

1. Wait for device idle.
2. Destroy renderers.
3. Release/delete assets.
4. Unregister the default textures.
5. Release `BindlessResources`.
6. Destroy Vulkan.

Adjust default-texture cleanup so their bindless registrations are explicitly
removed before the bindless descriptor pool is destroyed. The texture objects
remain responsible for their own images and views.

## 8. Verification

- Build `ellyn`, `asz`, and `vin`.
- Launch `ellyn` with validation enabled.
- Confirm:
  - all 1024 texture descriptors are populated
  - binding 1 references a valid placeholder buffer
  - the debug compute self-test passes
  - no descriptor or synchronization validation errors occur
  - existing renderers behave unchanged
- Exercise add/remove in the debug test:
  - allocated slots need not have predetermined indices
  - removal restores black
  - reuse returns the same index with an incremented generation
  - stale-handle validation is an error
- Touch `bindless_resources.glsl`, refocus the app, and verify dependent shaders
  hot reload successfully.

## Explicit Non-Goals

- No changes to `Texture2D` constructors.
- No automatic texture registration.
- No `SceneAsset` or material migration.
- No update-after-bind or deferred destruction.
- No variable descriptor counts.
- No bindless storage images, cube maps, or render targets.
