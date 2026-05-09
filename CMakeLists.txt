cmake_minimum_required(VERSION 3.21)
project("niar" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------- Dependencies ----------

find_package(Threads REQUIRED)
find_package(Vulkan REQUIRED)

find_package(SDL2 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(libconfig CONFIG REQUIRED)
# vcpkg package: vulkan-memory-allocator
# cmake package name may vary; try unofficial-vulkan-memory-allocator if this fails
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
# stb is header-only; vcpkg installs headers and exports Stb_INCLUDE_DIR
find_package(Stb REQUIRED)
# tinyexr is compiled as a static library by vcpkg
find_package(tinyexr CONFIG REQUIRED)
# tinygltf is header-only — vcpkg installs headers directly, no cmake config provided
# nlohmann_json is required because vcpkg's tinygltf rewrites its json include to <nlohmann/json.hpp>
find_package(nlohmann_json CONFIG REQUIRED)
find_package(cxxopts CONFIG REQUIRED)

message(STATUS "Vulkan Found    = ${Vulkan_FOUND}")
message(STATUS "Vulkan Include  = ${Vulkan_INCLUDE_DIRS}")
message(STATUS "Vulkan Library  = ${Vulkan_LIBRARIES}")

# ---------- Source lists ----------

# Files compiled into both ellyn and asz
set(SHARED_SRC
    src/Scene/SceneObject.cpp
    src/Scene/Camera.cpp
    src/Scene/Light.cpp
    src/Scene/AABB.cpp
    src/Render/Materials/GltfMaterialInfo.cpp
    src/Render/Mesh.cpp
    src/Scene/MeshObject.cpp
    src/Assets/Asset.cpp
    src/Assets/SceneAsset.cpp
    src/Assets/ConfigAsset.cpp
    src/Pathtracer/Primitive.cpp
    src/Pathtracer/BSDF.cpp
    src/Pathtracer/PathtracerLight.cpp
    src/Pathtracer/BVH.cpp
    src/Pathtracer/Pathtracer.cpp
    src/Utils/myn/Misc.cpp
    src/Utils/TinyGLTFImpl.cpp
    src/Utils/StbImageImpl.cpp
    src/Assets/EnvironmentMapAsset.cpp
    # Note: TinyExrImpl.cpp is intentionally omitted. vcpkg's tinyexr port
    # compiles the implementation into a static library, so no impl file is needed.
    src/Utils/myn/Sample.cpp
    src/Scene/SkyAtmosphere/SkyAtmosphere.cpp
    src/Utils/myn/ShaderSimulator.cpp
    src/CpuSkyAtmosphere/CpuSkyAtmosphere.cpp
    src/Utils/myn/CpuTexture.cpp)

# ellyn-only (Vulkan GUI) sources
set(ELLYN_SRC
    src/Ellyn.cpp
    src/Render/Vulkan/Vulkan.cpp
    src/Scene/Scene.cpp
    src/Scene/GrassField.cpp
    src/Render/Vulkan/VulkanMemoryAllocatorImpl.cpp
    src/Render/Vulkan/PipelineBuilder.cpp
    src/Render/Vulkan/RenderPassBuilder.cpp
    src/Render/Vulkan/Buffer.cpp
    src/Render/Vulkan/DescriptorSet.cpp
    src/Render/Texture.cpp
    src/Utils/myn/RenderDoc.cpp
    src/Render/Vulkan/SamplerCache.cpp
    src/Render/Vulkan/ShaderModule.cpp
    src/Render/Vulkan/VulkanUtils.cpp
    src/Render/Renderers/DeferredRenderer.cpp
    src/Render/Vulkan/ImageCreator.cpp
    src/Utils/DebugUI.cpp
    src/Render/Materials/GltfMaterial.cpp
    src/Render/DebugDraw.cpp
    src/Render/Renderers/SimpleRenderer.cpp
    src/Scene/PathtracerController.cpp
    src/Scene/Probe.cpp
    # imgui SDL2 backend (compiled here since vcpkg dropped sdl2-binding in imgui 1.90+)
    external/imgui/imgui_impl_sdl2.cpp
    # RTX sources
    src/Render/Vulkan/ShaderBindingTable.cpp
    src/Scene/RtxTriangle.cpp
    src/Render/Renderers/RayTracingRenderer.cpp
    src/Render/Materials/ComputeShader.cpp
    src/Scene/SkyAtmosphere/SkyAtmosphereShaders.cpp
        src/Render/Renderers/Renderer.cpp)

set(ASZELEA_SRC
    src/Aszelea.cpp)

set(VINCENT_SRC
    src/Vincent.cpp
    src/Utils/StbImageImpl.cpp
    src/Utils/myn/ShaderSimulator.cpp
    src/CpuSkyAtmosphere/CpuSkyAtmosphere.cpp
    src/Utils/myn/CpuTexture.cpp)

# ---------- Targets ----------

# Common compile definitions shared across all targets
set(NIAR_COMMON_DEFINES
    ROOT_DIR="${CMAKE_SOURCE_DIR}"
    DEBUG=1
    GLM_ENABLE_EXPERIMENTAL)

# ellyn - interactive Vulkan GUI application
add_executable(ellyn ${ELLYN_SRC} ${SHARED_SRC})
target_include_directories(ellyn PRIVATE
    src
    external/imgui
    external/renderdoc
    ${Vulkan_INCLUDE_DIRS}
    ${Stb_INCLUDE_DIR})
target_link_libraries(ellyn PRIVATE
    Threads::Threads
    Vulkan::Vulkan
    SDL2::SDL2
    glm::glm
    imgui::imgui
    libconfig::libconfig++
    GPUOpen::VulkanMemoryAllocator
    unofficial::tinyexr::tinyexr
    nlohmann_json::nlohmann_json
    cxxopts::cxxopts)
target_compile_definitions(ellyn PRIVATE
    ${NIAR_COMMON_DEFINES}
    GRAPHICS_DISPLAY=1)

# Copy runtime DLLs next to ellyn on Windows (no-op for static triplets)
add_custom_command(TARGET ellyn POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_RUNTIME_DLLS:ellyn>
        $<TARGET_FILE_DIR:ellyn>
    COMMAND_EXPAND_LISTS
    COMMENT "Copying runtime DLLs")

# asz - headless CLI path tracer
add_executable(asz ${ASZELEA_SRC} ${SHARED_SRC})
target_include_directories(asz PRIVATE
    src
    ${Stb_INCLUDE_DIR})
target_link_libraries(asz PRIVATE
    Threads::Threads
    libconfig::libconfig++
    unofficial::tinyexr::tinyexr
    nlohmann_json::nlohmann_json
    cxxopts::cxxopts)
target_compile_definitions(asz PRIVATE
    ${NIAR_COMMON_DEFINES}
    GRAPHICS_DISPLAY=0)

# vin - CPU shader simulator
add_executable(vin ${VINCENT_SRC})
target_include_directories(vin PRIVATE
    src
    ${Stb_INCLUDE_DIR})
target_link_libraries(vin PRIVATE
    cxxopts::cxxopts)
target_compile_definitions(vin PRIVATE
    ${NIAR_COMMON_DEFINES}
    GRAPHICS_DISPLAY=0)

# ---------- Vulkan shaders ----------

set(VULKAN_SHADERS_SRC_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(VULKAN_SHADERS_BIN_DIR ${CMAKE_BINARY_DIR}/spirv)
add_custom_target(shaders ALL
    COMMENT "Compiling Vulkan shaders"
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/compile_vulkan_shaders.sh
        ${VULKAN_SHADERS_SRC_DIR} ${VULKAN_SHADERS_BIN_DIR})