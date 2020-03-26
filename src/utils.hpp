#pragma once
#include "lib.h"

struct Drawable;

int unifLoc(uint shaderID, std::string uniformName);

uint new_shader_program(
    std::string vertPath, 
    std::string fragPath, 
    std::string tescPath = "", 
    std::string tesePath = "");

int load_meshes(std::string source, Drawable* root);
