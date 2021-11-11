cmake_minimum_required(VERSION 3.1)
project("niar")

set(CMAKE_CXX_STANDARD 20)
# set(CMAKE_CXX_FLAGS "-O3")

if(WIN32)
	set(TASKSYS_O ${CMAKE_BINARY_DIR}/tasksys.obj)
	set(ISPC_CXX_COMPILER cl)
endif(WIN32)

if(APPLE)
	set(TASKSYS_O ${CMAKE_BINARY_DIR}/tasksys.o)
	set(ISPC_CXX_COMPILER g++)
endif(APPLE)

find_package(Threads)
find_package(Vulkan REQUIRED)
message(STATUS "Vulkan Found = ${Vulkan_FOUND}")
message(STATUS "Vulkan Include = ${Vulkan_INCLUDE_DIR}")
message(STATUS "Vulkan Lib = ${Vulkan_LIBRARY}")

set(SRC
	src/Engine/main.cpp
	src/Render/Vulkan/Vulkan.cpp
	src/Scene/Scene.cpp
    src/Scene/Camera.cpp
	src/Engine/SceneObject.cpp
	src/Render/Mesh.cpp
    src/Scene/GrassField.cpp
	src/Engine/Config.cpp
	src/Scene/Light.cpp
	src/Scene/AABB.cpp
	src/Render/Materials/Material.cpp
    src/Pathtracer/Primitive.cpp
	src/Pathtracer/Pathtracer.cpp
	src/Pathtracer/BSDF.cpp
	src/Pathtracer/PathtracerLight.cpp
	src/Pathtracer/BVH.cpp
	${CMAKE_BINARY_DIR}/pathtracer_kernel.o
	${TASKSYS_O}
	${CMAKE_SOURCE_DIR}/include/imgui/imgui.h
	${CMAKE_SOURCE_DIR}/include/imgui/imgui.cpp
	${CMAKE_SOURCE_DIR}/include/imgui/imgui_demo.cpp
	${CMAKE_SOURCE_DIR}/include/imgui/imgui_tables.cpp
	${CMAKE_SOURCE_DIR}/include/imgui/imgui_draw.cpp
	${CMAKE_SOURCE_DIR}/include/imgui/backends/imgui_impl_sdl.cpp
	${CMAKE_SOURCE_DIR}/include/imgui/backends/imgui_impl_vulkan.cpp
	${CMAKE_SOURCE_DIR}/include/imgui/imgui_widgets.cpp
	# include/openfbx/ofbx.cpp
	src/Render/Vulkan/VulkanMemoryAllocatorImpl.cpp
	src/Utils/myn/CVar.cpp
	src/Utils/myn/Misc.cpp
    src/Render/Vulkan/PipelineBuilder.cpp
	src/Render/Vulkan/RenderPassBuilder.cpp
	src/Render/Vulkan/Buffer.cpp
	src/Render/Vulkan/DescriptorSet.cpp
	src/Utils/myn/Color.cpp
	src/Render/Texture.cpp
    src/Utils/StbImageImpl.cpp
	src/Utils/TinyGLTFImpl.cpp
	src/Render/Vulkan/SamplerCache.cpp
	src/Render/Vulkan/ShaderModule.cpp
	src/Render/Vulkan/VulkanUtils.cpp
	src/Render/Renderers/DeferredRenderer.cpp
	src/Render/Vulkan/ImageCreator.cpp
    src/Render/Materials/DeferredLighting.cpp
	src/Render/Materials/DeferredBasepass.cpp
	src/Engine/DebugUI.cpp
	src/Engine/DebugUI.h
	src/Utils/myn/RenderDoc.cpp
    src/Scene/SceneLoad_Assimp.cpp
	src/Scene/SceneLoad_glTF.cpp src/Render/Materials/DeferredBasepassGlTF.cpp src/Render/Materials/DeferredBasepassGlTF.h)

if(APPLE)
	add_executable(niar ${SRC})
	add_definitions(-DMACOS)
	target_link_libraries(niar
		${CMAKE_THREAD_LIBS_INIT}
		${CMAKE_SOURCE_DIR}/lib/libMoltenVK.dylib
		${CMAKE_SOURCE_DIR}/lib/libGLEW.a 
		${CMAKE_SOURCE_DIR}/lib/SDL2.framework 
		${CMAKE_SOURCE_DIR}/lib/libzlibstatic.a
		${CMAKE_SOURCE_DIR}/lib/libIrrXML.a
		${CMAKE_SOURCE_DIR}/lib/libassimp.a
		${CMAKE_SOURCE_DIR}/lib/libconfig++.a
	)
endif(APPLE)

if(WIN32)
	add_executable(niar WIN32 ${SRC})
	add_definitions(-DWINOS)
	add_definitions(-DGLEW_STATIC)
	target_link_libraries(niar
		${CMAKE_THREAD_LIBS_INIT}
		${Vulkan_LIBRARY}
		${CMAKE_SOURCE_DIR}/lib/glew32s.lib 
		${CMAKE_SOURCE_DIR}/lib/SDL2x64.lib
		${CMAKE_SOURCE_DIR}/lib/stb_image.a 
		${CMAKE_SOURCE_DIR}/lib/zlibstaticd.lib
		${CMAKE_SOURCE_DIR}/lib/IrrXMLd.lib
		${CMAKE_SOURCE_DIR}/lib/assimp-vc142-mtd.lib
		${CMAKE_SOURCE_DIR}/lib/libconfig++.lib
	)
	set_target_properties(niar PROPERTIES LINK_FLAGS /SUBSYSTEM:CONSOLE)
	configure_file(lib/SDL2.dll ${niar_BINARY_DIR} COPYONLY)
	configure_file(lib/libconfig++.dll ${niar_BINARY_DIR} COPYONLY)
endif(WIN32)

# include_directories(include src)
target_include_directories(niar PUBLIC SRC include ${Vulkan_INCLUDE_DIR})
target_include_directories(niar PUBLIC include/SDL2)
target_include_directories(niar PUBLIC include/imgui)
add_definitions(-DROOT_DIR="${CMAKE_SOURCE_DIR}")
add_definitions(-DCMAKE_EXPORT_COMPILE_COMMANDS=ON)

add_definitions(-DDEBUG=1)


#-------- building ispc --------
# tasksys.o
set(TASKSYS_SRC ${CMAKE_SOURCE_DIR}/include/ispc/tasksys.cpp)
add_custom_command(
	OUTPUT ${TASKSYS_O}
	COMMAND ${ISPC_CXX_COMPILER} -c -m64 -O3 -Wall ${TASKSYS_SRC} -lm -lpthread -D_DEBUG -D_ITERATOR_DEBUG_LEVEL=2 -D_ALLOW_RUNTIME_LIBRARY_MISMATCH -o ${TASKSYS_O}
	DEPENDS ${TASKSYS_SRC}
	COMMENT "generating tasksys.o"
)

# pathtracer_kernel.o
set(KERNEL_SRC ${CMAKE_SOURCE_DIR}/src/Pathtracer/pathtracer_kernel.ispc)
set(KERNEL_INCLUDE ${CMAKE_SOURCE_DIR}/src/Pathtracer/pathtracer_kernel_utilities.h)
set(KERNEL_H ${CMAKE_SOURCE_DIR}/src/Pathtracer/pathtracer_kernel_ispc.h)
set(KERNEL_O ${CMAKE_BINARY_DIR}/pathtracer_kernel.o)
add_custom_command(
	OUTPUT ${KERNEL_H} ${KERNEL_O}
	COMMAND ispc -O2 --target=avx2-i32x8 --arch=x86-64 ${KERNEL_SRC} -h ${KERNEL_H} -o ${KERNEL_O}
	DEPENDS ${KERNEL_SRC}#${KERNEL_INCLUDE}
	COMMENT "generating pathtracer_kernel.o"
)
# build
add_custom_target(ispc
	DEPENDS ${TASKSYS_O} ${KERNEL_H} ${KERNEL_O}
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
add_dependencies(niar ispc)

#-------- vulkan shaders --------
set(VULKAN_SHADERS_SRC_DIR ${CMAKE_SOURCE_DIR}/shaders/vulkan)
set(VULKAN_SHADERS_BIN_DIR ${CMAKE_SOURCE_DIR}/build/spirv)
add_custom_target(shaders ALL
	COMMENT "compiling Vulkan shaders"
	COMMAND ${CMAKE_SOURCE_DIR}/scripts/compile_vulkan_shaders.sh ${VULKAN_SHADERS_SRC_DIR} ${VULKAN_SHADERS_BIN_DIR}
)