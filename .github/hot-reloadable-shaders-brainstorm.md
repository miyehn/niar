# Brainstorm: Hot-Reloadable Shaders

## What's needed to compile from shader source at runtime

Currently `ShaderModuleAsset` reads a pre-compiled SPIR-V binary and hands it to `vkCreateShaderModule`. To compile from GLSL source at runtime, the two realistic options are:

**Option A — Shell out to `glslc`:**  
Write source to a temp path (or use the source path directly), invoke `glslc` as a subprocess, read back the SPIR-V output. Simple but adds process-spawn overhead on every reload and depends on `glslc` being on `PATH` at runtime.

**Option B — Link against `libshaderc` (recommended):**  
The Vulkan SDK ships `shaderc_combined.lib` / `shaderc_shared.lib` at `C:/VulkanSDK/1.3.216.0/Lib/`. Link it in CMakeLists.txt, then use `shaderc::Compiler::CompileGlslToSpv()` in-process. Fast, no subprocess, and critically: the compiler exposes an **includer interface** (`shaderc::CompileOptions::SetIncluder()`) that intercepts every `#include` resolution — which directly solves the dependency tracking problem (see below).

Shader stage can be inferred from the file extension (`.vert`, `.frag`, etc.) or carried in a `ShaderModuleDef` struct.

---

## Critique of Idea 1 (ShaderFileAsset dependency tree)

**Pros:**
- Fits cleanly into the existing `Asset` system — dependency propagation via `AfterReload` callbacks is already supported.
- Per-file granularity; any change anywhere in the include chain triggers the right reloads.

**Cons:**
- Building the tree requires parsing `#include` directives yourself, which is essentially a preprocessing pass. You'd need to do this before compiling, and keep it up to date as files change (an included file gaining a new `#include` would silently break the tracked graph until you manually rebuild it).
- **Shader variants are awkward.** A single `.glsl` file compiled with different macro sets produces different shader modules, each needing its own `ShaderModuleAsset`. But `ShaderFileAsset` nodes would be shared — the same file node could have callbacks from many `ShaderModuleAsset`s. This is manageable but the tree gets complex fast.
- The `Asset` base's `initialize_or_reload_outdated` only checks `relative_path` (one file). `ShaderModuleAsset` or `ShaderFileAsset` would need to override or extend this behavior.

Overall: elegant in theory, but the `#include` parsing is a non-trivial maintenance burden that duplicates work the compiler already does.

---

## Critique of Idea 2 (preprocessed shaders folder + external monitor)

**Pros:**
- The shader monitoring logic is isolated from the rest of the app — good for maintainability.
- Using `glslc -E` or similar to write preprocessed files to disk is straightforward.
- The existing `Asset` system can watch the preprocessed files unchanged — no core changes needed.

**Cons:**
- Dependency tracking still needs to happen in the external system, so the hard problem isn't eliminated.
- The pipeline has two indirections: source change → external tool updates preprocessed file → `Asset` detects preprocessed file change → reload. More moving parts, more latency.
- The external watcher must be running alongside the app, either as a thread or a separate process. Either way it's extra infrastructure.
- Extra disk writes on every source change.

Overall: the decoupling is real, but the complexity budget is higher than it appears. The hard part (dependency tracking) is pushed into a new system rather than eliminated.

---

## Suggested Idea 3: runtime compilation via `libshaderc` with compiler-driven dependency tracking

This collapses both ideas into one mechanism:

1. **Compile from source using `libshaderc`.** No offline SPIR-V, no subprocess.
2. **Implement a custom `shaderc::IncludeInterface`.** The compiler calls `GetInclude()` for every `#include` it resolves. Your implementation records each included file path — by the end of compilation you have the complete, correct, transitive dependency set for free, without any manual `#include` parsing.
3. **Store the dependency set on `ShaderModuleAsset`.** A `std::vector<std::string>` of all files that contributed to the last successful compile.
4. **Extend the reload check.** The `Asset` base currently calls `get_last_write_time` on a single `relative_path`. Override `initialize_or_reload_outdated` in `ShaderModuleAsset` (or add a virtual `is_outdated()` hook to `Asset`) to also check every file in the dependency set.
5. **Shader variants** are naturally handled: each `ShaderModuleAsset` has its own `ShaderModuleDef` (entry point + macros), compiles independently, and gets its own dependency list. No shared tree to maintain.

The `ShaderModuleDef` struct you sketched fits well here:

```cpp
struct ShaderModuleDef {
    std::string entry_file;       // e.g. "shaders/pbr.frag"
    std::string entry_function;   // e.g. "main"
    std::vector<std::string> defines; // e.g. {"USE_NORMAL_MAP=1"}
};
```

Pool key becomes a hash/string of the full `ShaderModuleDef` rather than just a file path.

**What the CMake change looks like:**
```cmake
find_package(Vulkan REQUIRED)
target_link_libraries(ellyn PRIVATE Vulkan::shaderc_combined)
# or link shaderc_shared and copy the DLL
```

**Trade-offs vs Ideas 1 & 2:**
- No separate watcher process or external preprocessing step.
- Compile errors surface directly in the app (can display in ImGui).
- Compilation at startup is slightly slower than loading pre-built SPIR-V, but negligible for a dev-time tool.
- The offline `compile_vulkan_shaders.sh` script can be retired or kept for release builds only.
