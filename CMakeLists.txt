cmake_minimum_required(VERSION 3.1)
project("niar")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-O3")

find_package(OpenGL)
find_package(Threads)

set(SRC
	src/main 
	src/Vulkan.inl
	src/Scene
	src/Program 
	src/Camera
	src/Drawable
	src/Mesh
	src/Shader
	src/GrassField
	src/Input
	src/Light
	src/Utils
	src/Texture
	src/Materials
	src/Pathtracer/Primitive
	src/Pathtracer/Pathtracer
	src/Pathtracer/BSDF
	src/Pathtracer/PathtracerLight
	src/Pathtracer/BVH
	build/pathtracer_kernel.o
	build/tasksys.o
	# include/openfbx/ofbx.cpp
	)

include_directories(include src)
add_definitions(-DROOT_DIR="${CMAKE_SOURCE_DIR}")
add_definitions(-DCMAKE_EXPORT_COMPILE_COMMANDS=ON)

add_definitions(-DDEBUG=1)

if(APPLE)
	add_executable(niar ${SRC})
	add_definitions(-DMACOS)
	target_link_libraries(niar
		${CMAKE_THREAD_LIBS_INIT}
		${OPENGL_gl_LIBRARY}
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
		${OPENGL_gl_LIBRARY}
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

#-------- building ispc --------
# tasksys.o
set(TASKSYS_SRC ${CMAKE_SOURCE_DIR}/include/ispc/tasksys.cpp)
set(TASKSYS_O ${CMAKE_BINARY_DIR}/tasksys.o)
add_custom_command(
	OUTPUT ${TASKSYS_O}
	COMMAND g++ -c -m64 -O3 -Wall ${TASKSYS_SRC} -lm -lpthread -o ${TASKSYS_O}
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