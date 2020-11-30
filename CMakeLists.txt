cmake_minimum_required(VERSION 3.1)
project("niar")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "-O2")

find_package(OpenGL)
find_package(Threads)

set(SRC
	src/main 
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
	build/pathtracer_kernel.o
	build/tasksys.o
	include/openfbx/ofbx.cpp
	)

include_directories(include src)
add_definitions(-DROOT_DIR="${CMAKE_SOURCE_DIR}")
add_definitions(-DCMAKE_EXPORT_COMPILE_COMMANDS=ON)


if(APPLE)
	add_executable(niar ${SRC})
	add_definitions(-DMACOS)
	target_link_libraries(niar
		${CMAKE_THREAD_LIBS_INIT}
		${OPENGL_gl_LIBRARY}
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
