#pragma once
#include "lib.h"

struct Drawable;
struct Mesh;

uint uniform_loc(uint shaderID, std::string uniformName);

uint new_shader_program(
    std::string vertPath, 
    std::string fragPath, 
    std::string tescPath = "", 
    std::string tesePath = "");

std::vector<Mesh*> load_meshes(std::string source);
