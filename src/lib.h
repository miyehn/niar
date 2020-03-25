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
