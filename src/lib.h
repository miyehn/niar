#define MACOS

#include <iostream>
#include <fstream>
#include <vector>

#ifdef MACOS
#include "glew/glew.h"
#endif

#include "SDL2/SDL.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
