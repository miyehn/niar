# Copilot Instructions for niar

## Project Overview

**niar** is a C++20 Vulkan rendering playground with multiple rendering paths: simple unlit forward, PBR deferred G-buffer, hardware ray tracing (RTX), and a CPU multi-threaded path tracer with optional SIMD via Intel ISPC.

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

There are three executable targets, named after original characters:
- **`ellyn`** — Interactive Vulkan GUI application (`src/Ellyn.cpp`), compiled with `GRAPHICS_DISPLAY=1`
- **`asz`** — Headless CLI path tracer that writes to file (`src/Aszelea.cpp`), `GRAPHICS_DISPLAY=0`
- **`vin`** — CPU shader simulator (`src/Vincent.cpp`), `GRAPHICS_DISPLAY=0`
- **`ispc`** — Custom CMake target that compiles the ISPC pathtracer kernel (`src/Pathtracer/pathtracer_kernel.ispc`)

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

### Pathtracer
`src/Pathtracer/` contains a tile-based multi-threaded CPU path tracer with a BVH (`BVH.hpp`), BSDF (`BSDF.hpp`), and an optional ISPC SIMD kernel. Config-driven via `config/pathtracer.ini` (hot reloaded at runtime).

## Key Conventions

### Naming
- Types/classes: `PascalCase`
- Functions and methods: `snake_case`
- Namespaces: lowercase (`myn`, `myn::sky`)
- Shader files: `.vert`, `.frag`, `.comp`, `.rgen`, `.rchit`, `.rmiss`

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

### Descriptor Set Layout Convention
Vulkan descriptor sets are organized by update frequency:
- Set 0: Frame-global data (camera, lights)
- Set 1–2: Material-specific
- Set 3: Per-object (model matrix UBO)

### Config Files
`config/global.ini` is loaded once at startup. `config/pathtracer.ini` and `config/skyAtmosphere.ini` hot-reload during execution. Use the `Config->lookup<T>("Key.Subkey")` pattern to read values.

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
