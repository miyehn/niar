# Copilot Instructions for niar

## Project Overview

**niar** is a C++20 Vulkan rendering playground with multiple rendering paths: simple unlit forward, PBR deferred G-buffer, hardware ray tracing (RTX), and a CPU multi-threaded path tracer with optional SIMD via Intel ISPC.

The current project direction is ReSTIR GI, following
`.github/path-to-restir.md`. Work is currently focused on the bindless material
infrastructure described in `.github/bindless.md` before continuing RTGI
Milestone 2.

## Build System

CMake 3.1+ with Ninja generator, targeting x64-Debug on Windows.

```bash
cmake -B build -G Ninja
cmake --build build
```

Shaders must be compiled separately (this is also a CMake custom target):
```bash
./scripts/compile_vulkan_shaders.sh <shaders_src_dir> <shaders_bin_dir>
```

The shader script uses `glslc --target-env=vulkan1.2 -g -O0` to compile GLSL to SPIR-V.

**No test or lint commands exist.**

## Build Targets

There are three executable targets:
- **`ellyn`** — Interactive Vulkan GUI application (`src/Ellyn.cpp`), compiled with `GRAPHICS_DISPLAY=1`
- **`asz`** — Headless CLI path tracer that writes to file (`src/Aszelea.cpp`), `GRAPHICS_DISPLAY=0`
- **`vin`** — CPU shader simulator (`src/Vincent.cpp`), `GRAPHICS_DISPLAY=0`

To check build success for a specific target:
```powershell
cmake --build build/debug-dynamic --target ellyn -j 14
cmake --build build/debug-dynamic --target asz -j 14
cmake --build build/debug-dynamic --target vin -j 14
```

On Windows, if the compiler environment is not already loaded, run the target build through Visual Studio's developer environment:
```powershell
$cmd = 'call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 && cmake --build build/debug-dynamic --target ellyn -j 14'
cmd.exe /d /s /c $cmd
```

Swap `ellyn` for `asz` or `vin` as needed.

The `#define GRAPHICS_DISPLAY` flag gates all Vulkan, SDL2, and ImGui code. Always check this flag when modifying rendering-related code.

## Architecture

### Renderer Strategy Pattern
`src/Render/Renderers/Renderer.h` defines the abstract base. Concrete implementations (`SimpleRenderer`, `DeferredRenderer`, `RayTracingRenderer`, and `Pathtracer`) are all polymorphic and can be switched at runtime via ImGui. All renderers are instantiated in `Ellyn.cpp`.

### Scene Hierarchy
`SceneObject` (`src/Scene/SceneObject.hpp`) is the base scene node with transforms and parent/child relationships. `Scene` (subclasses `SceneObject`) is the root. Traversal helpers use BFS. Lazy transform computation — call `object_to_world()` to get the world matrix.

### Asset Hot Reload System
`Asset` (`src/Assets/Asset.h`) is the base pooled asset class. It tracks file modification timestamps and reloads automatically when `Debug.AutoHotReload = 1` in `config/global.ini`. Assets follow a singleton pool pattern. `ConfigAsset` wraps libconfig++ and exposes a typed `lookup<T>(cfg_path)` template method. Global config singleton: `extern ConfigAsset* Config;`.

### Vulkan Abstraction
`src/Render/Vulkan/` contains low-level wrappers. VulkanMemoryAllocator (VMA 3.0.1) handles GPU memory. Validation layers (`VK_LAYER_KHRONOS_validation`) are enabled in DEBUG builds. RenderDoc integration available via `src/Utils/myn/RenderDoc.h`.

Use `SamplerCache` for Vulkan sampler setup. When a descriptor needs non-default sampler behavior, start from `SamplerCache::defaultInfo()` and override only the fields that matter, then pass that `VkSamplerCreateInfo` to descriptor helpers such as `DescriptorSet::pointToImageView(...)`. Let descriptor helpers use their default sampler path when the default behavior is intentional.

### Pathtracer
`src/Pathtracer/` contains a tile-based multi-threaded CPU path tracer with a BVH (`BVH.hpp`), BSDF (`BSDF.hpp`), and an optional ISPC SIMD kernel. Config-driven via `config/pathtracer.ini` (hot reloaded at runtime).

## Key Conventions

### Overall

Prefer code that makes ownership and data flow visible at the call site. Keep abstractions small, remove redundant state, and place responsibilities at the layer that has the relevant context.

- Remove unused abstraction left behind by older architecture.
- Do not expose parameters when all valid callers pass the same value.
- Inline helpers that have one caller and do not clarify a meaningful phase boundary.
- Do not add an extra blank line at the end of a file.

Preserve existing explanatory comments written by the user. When refactoring
the code they describe, move or adapt those comments to the new structure
instead of deleting them. Remove one only when it is no longer accurate or
useful, and make that reason explicit when reporting the change.

### Header Extensions
`.h` and `.hpp` are both used with no strict rule. `.inl` files hold inline implementations included at the bottom of headers (e.g., `PathtracerBufferOperations.inl`).

### Include Paths
`src/` and `include/` are both on the include path. Use project-root-relative paths:
```cpp
#include "Scene/Scene.hpp"
#include "Render/Vulkan/Vulkan.hpp"
#include "Utils/myn/Log.h"
```

### Logging
Use the macros from `Utils/myn/Log.h` (color-coded terminal output) rather than raw `printf`/`std::cout`.

### Naming
- Types/classes: `PascalCase`
- Functions and methods: follow nearby code
- Namespaces: lowercase (`myn`, `myn::sky`)
- Shader files: `.vert`, `.frag`, `.comp`, `.rgen`, `.rchit`, `.rmiss`

### Class/Struct Members

Prefer lean class/struct declarations, especially in headers. The public surface should make intended use clear without exposing implementation details.
- Expose only the members that callers need. Keep implementation state private or protected.
- Use const getters when callers only need read-only access.
- Store only essential state. Do not add cached fields or boolean flags when the value can be trivially inferred from existing members.
- Do not store transient call inputs, such as `VkCommandBuffer`, on persistent objects. Pass them through the function that uses them.
- Prefer `const` pointers/references for dependencies that are only read, queried for layout, or bound.
- Keep implementation-only helper types out of the public namespace when possible. Prefer `.cpp`-local types, or private nested types when they must stay close to the owning type.
- Avoid thin private helper functions that only obscure a single call site. Fold small glue code into the owning function unless it is reused or clarifies a real phase boundary.

### Compute Shaders

Wrap compute shader dispatches in a small class derived from `ComputeShader`. Use `.cpp`-local wrapper classes when the compute shader is only used by one component.
- `ComputeShader::dispatch(...)` should bind the pipeline, bind descriptor sets, and call `vkCmdDispatch`. It should not do image layout transitions, memory barriers, or command buffer submission.
- Pass `VkCommandBuffer` into `dispatch(...)`; do not keep it as shader object state.
- Keep descriptor set pointers on the concrete shader wrapper, not on the base class. Make them `const DescriptorSet*` when the shader only binds or queries them.
- The render component that owns the surrounding frame context should own synchronization: image layout transitions, memory barriers, and ordering between passes.
- Use `Vulkan::Instance->immediateSubmit(...)` for one-time resource setup only. Per-frame compute work should record into the frame command buffer when it participates in the frame graph.
- Keep shader wrapper setup focused on shader-specific pipeline configuration. Do not expose wrapper classes from headers unless multiple components need them.
- If several compute passes follow the same pattern, make their call sites consistent before adding new abstractions.

### Descriptor Set Layout Convention
Vulkan descriptor sets are organized by update frequency:
- Set 0: Frame-global data (camera, lights)
- Set 1–2: Material-specific
- Set 3: Per-object (model matrix UBO)

Descriptor set layouts may include bindings that are reserved for near-term shader work, but avoid fake shader declarations unless the resource is intentionally part of that shader interface. When a resource is part of the interface, keep the C++ descriptor layout and GLSL binding declarations aligned.

### Render Passes And Synchronization
Keep logically separate rendering/debug work in separate passes when it makes ownership and toggling clearer. Do not force work into subpasses only to reduce object counts.
- Prefer explicit render/compute pass ordering over hidden side effects inside material or shader wrappers.
- Put image barriers at the producer/consumer boundary owned by the render component, not inside low-level dispatch helpers.
- If a previous pass's outgoing dependency already covers a consumer pass, do not add redundant consumer-side external dependencies.
- Framebuffer/render pass objects are cheap enough that clarity and correct attachment ownership should win over aggressive reuse.

### Config Files
`config/global.ini` is loaded once at startup. `config/pathtracer.ini`, `config/skyAtmosphere.ini` and others are hot-reload during execution. Use the `Config->lookup<T>("Key.Subkey")` pattern to read values.

### Config And Branches
Use config options for choices that are genuinely supported at runtime. When the project direction makes one path mandatory, remove the old option and fold code to the active path instead of keeping dead branches, shader defines, or inactive descriptor layouts.

## Key Dependencies

| Library | Purpose |
|---|---|
| Vulkan 1.3.216 | GPU rendering API |
| SDL2 | Window + input |
| GLM | Math (vectors, matrices, quaternions) |
| Dear ImGui | In-engine debug UI |
| TinyGLTF | glTF 2.0 scene loading |
| VMA 3.0.1 | Vulkan memory allocation |
| libconfig++ | `.ini` config parsing |
| Intel ISPC | SIMD pathtracer kernel compilation |
| stb_image / tinyexr | Image I/O |

## Platform Notes

Windows is the primary target (Vulkan SDK at `C:/VulkanSDK/1.3.216.0`). macOS support via MoltenVK exists in CMakeLists.txt but is considered broken/legacy.
