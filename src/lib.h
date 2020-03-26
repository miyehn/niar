#pragma once
#define MACOS

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

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

using namespace glm;

// for showing last path node, see: https://stackoverflow.com/questions/8487986/file-macro-shows-full-path
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
// for color formatting, see: https://stackoverflow.com/questions/2616906/how-do-i-output-coloured-text-to-a-linux-terminal
#define LOG(message) std::cout << "\033[32m[" << __FILENAME__ << ": " << std::to_string(__LINE__) << "]\033[0m " << message << std::endl;
#define WARN(message) std::cout << "\033[33m[" << __FILENAME__ << ": " << std::to_string(__LINE__) << "]\033[0m " << message << std::endl;
#define ERR(message) std::cout << "\033[31m[" << __FILENAME__ << ": " << std::to_string(__LINE__) << "]\033[0m " << message << std::endl;
