# AGENTS.md — niar

Guidance for AI agents working in this repo. The **codebase is the source of truth**;
when this file disagrees with the code, trust the code and fix this file.

## Project Overview

**niar** is a C++20 Vulkan rendering playground. It carries several rendering paths —
simple unlit forward, PBR deferred G-buffer, a hardware ray-tracing (RTX) pipeline, and
a multi-threaded CPU path tracer — plus a physically-based
sky atmosphere shared by the deferred path and the path tracer.

The **current project direction is ReSTIR GI**, built incrementally on top of the
deferred renderer. The durable roadmap lives in
[.docs/path-to-restir.md](docs/path-to-restir.md) — read it before touching GI
code. Recent milestones done: naive one-bounce diffuse RTGI, minimal + direct-lit
secondary-hit shading, temporal accumulation (ping-pong history, motion-vector
reprojection, depth-based history rejection), and TAA (jitter baked into the projection
matrix, neighborhood-clamp anti-ghosting). The remaining path is reservoir-first: the
next milestone is **Milestone 4 — the reservoir data model / sample buffer** (the
enabling refactor before ReSTIR temporal and spatial reservoir reuse); a classical
spatial denoiser now comes at the *end*, over the resolved ReSTIR output.

## Build System

CMake (≥ 3.21) + Ninja + **vcpkg** for dependencies. `CMakePresets.json` defines the
presets; the primary one is **`debug-dynamic`** (dynamic linking, DLLs copied next to
the executable). Static and release variants also exist.

Requires `VCPKG_ROOT` and `VULKAN_SDK` environment variables. Dependencies come from
vcpkg (`vcpkg.json`); `shaderc_shared` comes from the Vulkan SDK.

```powershell
cmake --preset debug-dynamic          # configure (first time / when CMakeLists changes)
cmake --build build/debug-dynamic     # build all targets
```

On Windows, if the MSVC environment is not already loaded, wrap the build in the VS
developer environment:

```powershell
$cmd = 'call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 && cmake --build build/debug-dynamic --target ellyn -j 14'
cmd.exe /d /s /c $cmd
```

### Build Targets

Three executables, gated by the `GRAPHICS_DISPLAY` compile define:

- **`ellyn`** — interactive Vulkan GUI app (`src/Ellyn.cpp`), `GRAPHICS_DISPLAY=1`.
- **`asz`** — headless CLI path tracer that renders to file (`src/Aszelea.cpp`),
  `GRAPHICS_DISPLAY=0`. Options via cxxopts: `-w -h -o --scene --spp`.
- **`vin`** — CPU shader simulator (`src/Vincent.cpp`), `GRAPHICS_DISPLAY=0`. Prototypes
  and debugs GLSL logic on the CPU (originally built for the sky atmosphere).

Build a single target with `--target ellyn` (or `asz` / `vin`), e.g.
`cmake --build build/debug-dynamic --target ellyn -j 14`.

`GRAPHICS_DISPLAY` gates all Vulkan, SDL2, and ImGui code — always check it when editing
rendering code that must also compile into `asz`/`vin`.

**There are no test or lint commands.** Verify changes by building the affected targets
(`ellyn` for rendering work, plus `asz`/`vin` when shared code changes) and, when
feasible, a runtime smoke check (e.g. toggling the relevant renderer / RTX on-off).

### Running

Binaries resolve assets, shaders, and config through the compile-time `ROOT_DIR` define
(the source tree), so they can run from any working directory.

- `ellyn` — WASD + E/Q move the camera, LMB-drag rotates, ESC quits. Renderer and
  debug options live in the ImGui panels. Path-tracer controls appear via
  `PathtracerController` in the scene tree when the path tracer is the active renderer.
- `asz -w 200 -h 150 -o output.png` — render a frame to file.

## Shaders

Shaders live in `shaders/` (`.vert .frag .comp .rgen .rchit .rmiss`, plus `.glsl`
includes). They are **compiled at runtime via `shaderc` inside the app** — not by an
external build step. The old `scripts/compile_vulkan_shaders.sh` / `glslc` custom target
is **deprecated** (both the script and the CMake target are commented out).

- The shader registry is `ShaderModuleAsset::_shaderModuleDefs[]` in
  `src/Assets/ShaderModuleAsset.cpp`. Adding a new shader means adding an entry there
  (path, entry function, `ShaderStage`, optional defines).
- `#include`s are resolved by `TrackingIncluder` (relative to the including file first,
  then the `shaders/` root) and recorded for **hot reload** — editing a shader or any of
  its includes recompiles it live when `Debug.AutoHotReload = 1`.
- `shaders/cshared/` holds headers shared between C++ and GLSL (see ABI note below).

## Architecture

### CPU/GPU Shared ABI (`shaders/cshared/`)

`cshared_common.h` (and `lights.h`) are included by **both** C++ and GLSL. The C++ side
wraps the structs in `namespace glm` and enforces layout with `static_assert`s on
`sizeof` / `alignof` / `offsetof`. `ViewInfo`, `GpuMaterial`, `GpuSceneInstanceRecord`,
and `GpuGeometryRecord` all live here.

**When you change any shared struct, update both the C++ static_asserts and the GLSL
declaration together, and keep the 16-byte alignment / padding correct.** This is the
most common source of silent GPU corruption. Descriptor-set and push-constant indices
are also defined here (`DSET_FRAMEGLOBAL/INDEPENDENT/BINDLESS`, `GLTF_MODEL_MATRIX_PUSH_*`).

### Renderer Strategy Pattern

`src/Render/Renderers/Renderer.h` is the abstract base. Concrete renderers —
`SimpleRenderer`, `DeferredRenderer`, `RayTracingRenderer`, and `Pathtracer` — are
polymorphic and switchable at runtime through ImGui. They are instantiated in
`Ellyn.cpp`. `DeferredRenderer` is the primary path and the host for GI/TAA work.

### Renderer Components (`src/Render/RendererComponents/`)

Reusable, renderer-owned building blocks that hold frame context and own their
synchronization:

- **`GI`** — RTGI compute pass. Owns the indirect-lighting texture, ping-pong history
  textures, and per-pixel sample-count texture. Consumes G-buffer + TLAS + shared light
  and bindless tables; composites into deferred lighting.
- **`TAA`** — temporal AA resolve compute pass with ping-pong history. Note jitter is
  baked into the projection matrix upstream; `globalFrameIndex` (not the frame-in-flight
  ring index) drives the ping-pong toggle.
- **`SceneTlas`** — shared top-level acceleration structure for RTX shadows and GI.
- **`SkyAtmosphereRender`** — sky-atmosphere LUTs and rendering.

### Bindless Resources (`src/Render/BindlessResources.h`)

`BindlessResources` (singleton, `Instance`) owns the global bindless descriptor set:
a bindless Texture2D array plus a material table (`GpuMaterial`) and a geometry-record
table (`GpuGeometryRecord`), both stored in device buffers looked up by index /
buffer-device-address. All raster glTF paths (deferred opaque, deferred translucent,
simple forward) and the RTGI hit shading read from these shared tables — there are no
longer per-material glTF descriptor sets or per-frame material lookups by string.

### Scene Hierarchy

`SceneObject` (`src/Scene/SceneObject.hpp`) is the base scene node (transforms,
parent/child). `Scene` subclasses it as the root. Traversal helpers are BFS; world
transforms are computed lazily via `object_to_world()`.

### Asset & Hot Reload System

`Asset` (`src/Assets/Asset.h`) is the base pooled asset; assets follow a singleton-pool
pattern, track file mtimes, and auto-reload when `Debug.AutoHotReload = 1`. Concrete
assets include `SceneAsset` (glTF), `ShaderModuleAsset`, `EnvironmentMapAsset`, and
`ConfigAsset`. `ConfigAsset` wraps libconfig++ with a typed `lookup<T>("Key.Subkey")`;
the global config singleton is `extern ConfigAsset* Config;`.

### Vulkan Abstraction (`src/Render/Vulkan/`)

Low-level wrappers: `Vulkan` (instance/device/swapchain), `Buffer` (`VmaBuffer` via VMA),
`DescriptorSet` / `DescriptorSetLayout`, `Pipeline`, `RenderPassBuilder`, `ImageCreator`,
`SamplerCache`, `ShaderBindingTable`, `VulkanUtils`. Validation layers
(`VK_LAYER_KHRONOS_validation`) are on in debug builds (`Debug.ValidationLayer`).
RenderDoc integration via `src/Utils/myn/RenderDoc.h` (`Debug.RenderDoc`; incompatible
with the validation layer and ray tracing — use NSight for RTX).

### Pathtracer (`src/Pathtracer/`)

Tile-based multi-threaded CPU path tracer with a BVH (`BVH.hpp`) and BSDFs (`BSDF.hpp`). 
Config-driven via `config/pathtracer.ini` (hot
reloaded). `.inl` files (`PathtracerCore.inl`, `PathtracerBufferOperations.inl`) hold
inline implementations.

## Configuration (`config/`)

- **`global.ini`** — loaded **once at startup**, not reloadable (scene source,
  environment map, `Debug.*` flags: `RenderDoc`, `ValidationLayer`, `AutoHotReload`,
  `CollapseSceneTree`, `PresentMode`).
- **`deferred.ini`** — **hot-reloadable**; holds the deferred renderer's `gi:` and
  `taa:` sub-sections (`enabled`, `maxSampleCount`, `historyDepthRejectionThreshold`,
  `historyWeight`, `clampExpansion`). (The roadmap still calls this `config/gi.ini`; the
  actual file is `deferred.ini`.)
- **`pathtracer.ini`**, **`skyAtmosphere.ini`** — hot-reloadable tuning.

Read values with `Config->lookup<T>("Key.Subkey")`. Add a config option only for choices
genuinely supported at runtime; when the project direction makes one path mandatory,
delete the old option and fold to the active path rather than keeping dead branches.

## Key Conventions

### Guiding Principles

Prefer code that makes ownership and data flow visible at the call site. Keep
abstractions small, remove redundant state, and place responsibilities at the layer that
has the relevant context.

- Remove unused abstractions, stale branches, and redundant state left behind by older
  architecture.
- Prefer direct code over thin helpers when the helper has one caller and does not
  clarify a real phase boundary.
- When a function asserts a programmer invariant, proceed directly with the
  implementation. Do not repeat the asserted condition as an early return or fallback
  check immediately afterward.
- Keep explanatory comments written by the user. When refactoring the code they
  describe, move or adapt those comments to the new structure instead of deleting them.
  Remove one only when it is no longer accurate or useful, and say why when you do.
- Do not add an extra blank line at the end of a file.

### State, Ownership, and Lifetime

Prefer lean class/struct declarations, especially in headers. Public surfaces should
express intended use without exposing implementation details.

- Expose only the members callers need; keep implementation state private/protected.
- Use const getters, pointers, and references when callers only need read/query/bind
  access.
- Store only durable ownership, externally observable state, or cleanup tokens that must
  survive construction (e.g. keep GPU/material-table indices needed for release, but not
  temporary texture-handle or GPU-record staging arrays after upload).
- Keep construction scratch local to the loading phase that needs it. Before adding a
  member, check whether it is read after init or needed for ownership/lifetime cleanup.
- Do not store transient call inputs such as `VkCommandBuffer` on persistent objects —
  pass them through the function that uses them.
- Keep implementation-only helper types out of the public namespace (`.cpp`-local types
  or private nested types).

### Interfaces and Helpers

- Do not expose parameters when all valid callers pass the same value.
- Inline small glue helpers that only obscure a single call site.
- Keep a helper/abstraction when it is reused, owns a meaningful responsibility, or marks
  a phase boundary that makes the caller easier to understand.
- Put responsibilities at the layer that owns the context: render components that own
  frame context own synchronization and pass ordering; low-level shader wrappers focus
  on binding and dispatch.

### Compute Shaders

Wrap compute dispatches in a small class derived from `ComputeShader`
(`src/Render/Materials/ComputeShader.h`); use `.cpp`-local wrappers when a shader is used
by one component.

- `ComputeShader::dispatch(...)` binds the pipeline + descriptor sets and calls
  `vkCmdDispatch` — no layout transitions, barriers, or submits.
- Pass `VkCommandBuffer` into `dispatch(...)`; don't keep it as shader state.
- Keep descriptor-set pointers on the concrete wrapper as `const DescriptorSet*` when the
  shader only binds/queries them.
- The owning render component handles image layout transitions, barriers, and inter-pass
  ordering.
- Use `Vulkan::Instance->immediateSubmit(...)` for one-time setup only; per-frame compute
  records into the frame command buffer.

### Render Passes and Synchronization

- Keep logically separate rendering/debug work in separate passes when it clarifies
  ownership and toggling. Don't force work into subpasses just to cut object counts.
- Prefer explicit render/compute pass ordering over hidden side effects inside material
  or shader wrappers.
- Put image barriers at the producer/consumer boundary owned by the render component, not
  inside low-level dispatch helpers.
- If a previous pass's outgoing dependency already covers a consumer, don't add a
  redundant consumer-side external dependency.

### Descriptor Sets

Organized by update frequency (indices in `cshared_common.h`):

- **Set 0 — `DSET_FRAMEGLOBAL`**: frame-global data (camera/`ViewInfo`, lights).
- **Set 1 — `DSET_INDEPENDENT`**: pass-/component-specific bindings.
- **Set 2 — `DSET_BINDLESS`**: the global bindless texture / material / geometry tables.
- **Per-object data** (model matrix + bindless material index) goes through **push
  constants** (`GLTF_MODEL_MATRIX_PUSH_*`, `GLTF_MATERIAL_INDEX_PUSH_*`), not a
  descriptor set.

Keep the C++ descriptor layout and the GLSL binding declarations aligned. Avoid fake
shader declarations for reserved bindings unless the resource is intentionally part of
that shader interface. Use `SamplerCache` for samplers: start from
`SamplerCache::defaultInfo()`, override only the fields that matter, and let descriptor
helpers (`DescriptorSet::pointToImageView(...)`) use their default sampler path when the
default behavior is intentional.

### Code Style

- **Header extensions**: `.h` and `.hpp` both used, no strict rule. `.inl` files hold
  inline implementations included at the bottom of headers.
- **Include paths**: `src/` and `include/` are on the include path; use
  project-root-relative paths (`#include "Scene/Scene.hpp"`). `shaders/cshared/` is also
  on the include path for the shared ABI headers.
- **Logging**: use the color-coded macros in `Utils/myn/Log.h`, not raw
  `printf`/`std::cout`.
- **Naming**: types/classes `PascalCase`; namespaces lowercase (`myn`, `myn::sky`);
  functions/methods follow nearby code.
- **Line length**: wrap only when a line significantly exceeds ~120 chars. Don't add
  artificial breaks to short expressions or call arguments that read clearly on one line.

## Key Dependencies

| Library | Purpose |
|---|---|
| Vulkan (via `VULKAN_SDK`) | GPU rendering API |
| shaderc (`shaderc_shared`) | Runtime GLSL → SPIR-V compilation |
| SDL2 | Window + input |
| GLM | Math (vectors, matrices, quaternions) |
| Dear ImGui (+ SDL2 backend in `external/imgui`) | In-engine debug UI |
| TinyGLTF (+ nlohmann-json) | glTF 2.0 scene loading |
| VMA | Vulkan memory allocation |
| libconfig++ | `.ini` config parsing |
| Intel ISPC | Optional SIMD path-tracer kernel |
| stb_image / tinyexr | Image I/O |
| cxxopts | CLI option parsing (`asz`, `vin`) |

## Tooling & Platform

- A Blender addon (`scripts/blender/addons/myn`) assists the glTF export workflow.
- Windows is the primary target. macOS support via MoltenVK exists historically but is
  considered broken/legacy.
