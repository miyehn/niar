//
//  utils.hpp
//  sdl-empty
//
//  Created by miyehn on 8/20/19.
//  Copyright © 2019 miyehn. All rights reserved.
//

#include "lib.h"

using namespace std;
using namespace glm;

int unifLoc(uint shaderID, string uniformName);

void newVBO(float* buf, size_t size);

uint newVAO(float* vertices, size_t size);

void newEBO(uint* buf, size_t size);

uint newTexture(string imgPath);

uint newShaderProgram(string vertPath, string tescPath, string tesePath, string fragPath);

//---- generic helpers ----


