#include "lib.h"

using namespace glm;

int unifLoc(uint shaderID, std::string uniformName);

uint newShaderProgram(
    std::string vertPath, 
    std::string tescPath, 
    std::string tesePath, 
    std::string fragPath);

