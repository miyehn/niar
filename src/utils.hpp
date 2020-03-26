#pragma once
#include "lib.h"

// for showing last path node, see: https://stackoverflow.com/questions/8487986/file-macro-shows-full-path
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
// for color formatting, see: https://stackoverflow.com/questions/2616906/how-do-i-output-coloured-text-to-a-linux-terminal
#define LOG(message) std::cout << "\033[32m[" << __FILENAME__ << ": " << std::to_string(__LINE__) << "]\033[0m " << message << std::endl;
#define WARN(message) std::cout << "\033[33m[" << __FILENAME__ << ": " << std::to_string(__LINE__) << "]\033[0m " << message << std::endl;
#define ERR(message) std::cout << "\033[31m[" << __FILENAME__ << ": " << std::to_string(__LINE__) << "]\033[0m " << message << std::endl;

int unifLoc(uint shaderID, std::string uniformName);

uint newShaderProgram(
    std::string vertPath, 
    std::string tescPath, 
    std::string tesePath, 
    std::string fragPath);

