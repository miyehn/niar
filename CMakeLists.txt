cmake_minimum_required(VERSION 3.1)
project("sdl-empt")

set(CMAKE_CXX_STANDARD 14)

find_package(OpenGL)

add_executable(sdl-empt
  src/main 
  src/Program 
  src/Program+Game 
  src/utils
  src/Camera
  src/GameObject.hpp)

target_link_libraries(sdl-empt
  ${OPENGL_gl_LIBRARY}
  ${CMAKE_SOURCE_DIR}/lib/libGLEW.a 
  ${CMAKE_SOURCE_DIR}/lib/SDL2.framework 
  ${CMAKE_SOURCE_DIR}/lib/stb_image.a 
  ${CMAKE_SOURCE_DIR}/lib/libzlibstatic.a
  ${CMAKE_SOURCE_DIR}/lib/libassimp.a
)
