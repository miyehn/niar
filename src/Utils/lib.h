#pragma once

#include <cmath>
#include <algorithm>
#include <fstream>
#include <vector>
#include <string>
#include <memory>
#include <stdexcept>
#include <mutex>

#include "glew/glew.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_vulkan.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/quaternion.hpp"

#include "Utils/myn/Log.h"

using namespace glm;
using namespace myn;

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

// other macros
#define INF std::numeric_limits<float>::infinity()
#define EPSILON 0.001f
#define PI 3.14159265359f
#define HALF_PI 1.57079632679f
#define ONE_OVER_PI 0.31830988618f
#define TWO_PI 6.28318530718f
#define ONE_OVER_TWO_PI 0.15915494309f

#define CONST_PTR(T, NAME) \
	public: \
	static T* NAME(); \
	private: \
	static T* NAME##_value; \
	public:

//------------------ generic helper(s) --------------------

// TODO: make more robust
extern quat quat_from_dir(vec3 dir); 

extern std::string s3(vec3 v);
