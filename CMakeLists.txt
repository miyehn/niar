cmake_minimum_required(VERSION 3.1)
project("niar")

set(CMAKE_CXX_STANDARD 11)

find_package(OpenGL)
find_package(Threads)

add_executable(niar
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
  src/Pathtracer/Primitive
  src/Pathtracer/Pathtracer
  src/Pathtracer/BSDF
	src/Pathtracer/PathtracerLight
  )

include_directories(include src src/Pathtracer)

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
