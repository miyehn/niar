# Bindless Phase 2: Optional Texture Registration

## Summary

Complete `Texture2D` ownership of optional bindless registrations. Existing
call sites remain unregistered by default; Phase 3 will explicitly opt glTF
textures in.

## API and Lifecycle Changes

- Extend every public `Texture2D` constructor with a final defaulted
  `const BindlessTexture2DInfo& bindlessInfo = {}` parameter.
- Keep `registerTexture = false` as the default. When true, require callers to
  explicitly provide a valid `VkSamplerCreateInfo`, such as
  `SamplerCache::defaultInfo()`.
- Retain `getBindlessHandle()`; the sampler configuration is consumed during
  registration and does not need to be stored afterward.
- Add one private registration helper shared by the constructors:
  - Do nothing when registration is disabled.
  - Assert that `BindlessResources` exists and the sampler create info is valid.
  - Register only after image creation, upload, mip generation, layout
    transition, and image-view creation finish.
- Do not change existing render-target, environment-map, or glTF call sites in
  this phase.

## Destruction and Pool Cleanup

- In `Texture2D::~Texture2D()`, unregister first when the handle is valid, then
  destroy the image view. The base `Texture` destructor continues destroying
  the image afterward.
- Reorder `unregisterBindless()` so an invalid handle returns before asserting
  that `BindlessResources` exists. Registered textures must assert the registry
  exists, unregister, then clear their handle.
- Keep default textures manually registered by `createDefaultTextures()` and
  explicitly unregistered by `unregisterDefaultTextures()`; their later
  destructors should see invalid handles and do nothing.
- In `Texture::~Texture()`, erase every `texturePool` entry whose value equals
  `this` before destroying the image. This avoids dangling pooled pointers
  without storing an additional pool key and does not remove a newer texture
  that overwrote the same name.
- Preserve the existing shutdown order: assets and registered textures first,
  defaults next, then `BindlessResources`, then Vulkan.

## Temporary Verification

- Extend the `TMP_BINDLESS_DEBUG` startup checks with constructor-lifetime
  coverage:
  - Create a nonregistered 1x1 texture and confirm its handle remains invalid.
  - Create a registered 1x1 texture using `SamplerCache::defaultInfo()` and
    confirm its handle validates.
  - Delete it and confirm its pool entry is removed.
  - Create another registered texture and confirm the freed index is reused
    with an incremented generation.
  - Delete it and confirm cleanup leaves no additional occupied slot.
- Build `ellyn`, `asz`, and `vin`.
- Launch `ellyn` with validation enabled and confirm:
  - the startup bindless self-test passes;
  - no descriptor lifetime or synchronization errors occur;
  - all renderer-owned textures remain unregistered;
  - graceful shutdown reaches an empty bindless registry.
- Treat actual registered `SceneAsset` reload testing as Phase 3, when glTF
  textures begin opting in.

## Assumptions

- No automatic registration based on texture type or usage.
- No glTF, environment-map, material, shader, or renderer migration in this
  phase.
- No partial binding, update-after-bind, deferred destruction, or removal of
  the black filler policy.
- Existing constructor calls remain source-compatible because the new argument
  is final and defaulted.
