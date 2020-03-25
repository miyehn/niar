cmake_minimum_required(VERSION 3.1)
project("niar")

set(CMAKE_CXX_STANDARD 14)

find_package(OpenGL)

add_executable(niar
  src/main 
  src/Program 
  src/utils
  src/Camera
	src/Drawable.hpp
  src/GrassField
  )

include_directories(include)

target_link_libraries(niar
  ${OPENGL_gl_LIBRARY}
  ${CMAKE_SOURCE_DIR}/lib/libGLEW.a 
  ${CMAKE_SOURCE_DIR}/lib/SDL2.framework 
  ${CMAKE_SOURCE_DIR}/lib/stb_image.a 
  ${CMAKE_SOURCE_DIR}/lib/libzlibstatic.a
  ${CMAKE_SOURCE_DIR}/lib/libassimp.a
)
