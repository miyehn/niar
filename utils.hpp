#include "lib.h"

using namespace std;
using namespace glm;

int unifLoc(uint shaderID, string uniformName);

uint newShaderProgram(string vertPath, string tescPath, string tesePath, string fragPath);

uint newVFShaderProgram(string vertPath, string fragPath);

uint newComputeShaderProgram(string shaderPath);

uint newTexture1D(void* data, GLsizei num_floats);
