#pragma once
#define MACOS

#include <iostream>
#include <math.h>
#include <algorithm>
#include <fstream>
#include <vector>
#include <string>
#include <memory>
#include <stdexcept>

#ifdef MACOS
#include "glew/glew.h"
#endif

#include "SDL2/SDL.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "libconfig/libconfig.h++"

using namespace glm;

// from: https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf
template<typename ... Args>
std::string string_format( const std::string& format, Args ... args ) {
  size_t size = snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
  if( size <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
  std::unique_ptr<char[]> buf( new char[ size ] ); 
  snprintf( buf.get(), size, format.c_str(), args ... );
  return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}

// for showing last path node, see: https://stackoverflow.com/questions/8487986/file-macro-shows-full-path
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

// for color formatting, see: https://stackoverflow.com/questions/2616906/how-do-i-output-coloured-text-to-a-linux-terminal
#define LOGF(...) std::cout << "\033[32m[" << __FILENAME__ << ": " << std::to_string(__LINE__) << "]\033[0m " << \
  string_format(__VA_ARGS__) << std::endl;
#define LOG(message) std::cout << "\033[32m[" << __FILENAME__ << ": " << std::to_string(__LINE__) << "]\033[0m " << \
  message << std::endl; 

#define WARNF(...) std::cout << "\033[33m[" << __FILENAME__ << ": " << std::to_string(__LINE__) << "]\033[0m " << \
  string_format(__VA_ARGS__) << std::endl;
#define WARN(message, ...) std::cout << "\033[33m[" << __FILENAME__ << ": " << std::to_string(__LINE__) << "]\033[0m " << \
  message << std::endl;

#define ERRF(...) std::cout << "\033[31m[" << __FILENAME__ << ": " << std::to_string(__LINE__) << "]\033[0m " << \
  string_format(__VA_ARGS__) << std::endl;
#define ERR(message) std::cout << "\033[31m[" << __FILENAME__ << ": " << std::to_string(__LINE__) << "]\033[0m " << \
  message << std::endl;

//---- pathtracer specific
#define TRACEF(...) std::cout << "\033[1;35m[Pathtracer] " << string_format(__VA_ARGS__) << "\033[0m" << std::endl;
#define TRACE(message) std::cout << "\033[1;35m[Pathtracer] " << message << "\033[0m" << std::endl; 

//-------- gl_errors.hpp, found in Jim's base code --------
#define STR2(X) # X
#define STR(X) STR2(X)

inline void gl_errors(std::string const &where) {
  GLenum err = 0;
  while ((err = glGetError()) != GL_NO_ERROR) {
    #define CHECK( ERR ) \
      if (err == ERR) { \
        std::cerr << "WARNING: gl error '" #ERR "' at " << where << std::endl; \
      } else

    CHECK( GL_INVALID_ENUM )
    CHECK( GL_INVALID_VALUE )
    CHECK( GL_INVALID_OPERATION )
    CHECK( GL_INVALID_FRAMEBUFFER_OPERATION )
    CHECK( GL_OUT_OF_MEMORY )
    CHECK( GL_STACK_UNDERFLOW )
    CHECK( GL_STACK_OVERFLOW )
    {
      std::cerr << "WARNING: gl error '" << err << "'" << std::endl;
    }
    #undef CHECK
  }
}
#define GL_ERRORS() gl_errors(__FILE__  ":" STR(__LINE__) )
//---------------------------------------------------------

// other macros
#define INF std::numeric_limits<float>::infinity()
#define EPSILON 0.001f
#define PI 3.14159265359f
#define ONE_OVER_PI 0.31830988618f
#define TWO_PI 6.28318530718f
#define ONE_OVER_TWO_PI 0.15915494309f
typedef std::chrono::time_point<std::chrono::high_resolution_clock> TimePoint;
