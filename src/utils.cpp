//
//  utils.cpp
//  sdl-empty
//
//  Created by miyehn on 8/20/19.
//  Copyright © 2019 miyehn. All rights reserved.
//

#include "utils.hpp"

using namespace std;
using namespace glm;

int unifLoc(uint shaderID, string uniformName) {
    int location = glGetUniformLocation(shaderID, uniformName.c_str());
    if (location < 0) {
        cout << "unable to find location for uniform: " << uniformName << endl;
        exit(1);
    }
    return location;
}

void newVBO(float* buf, size_t size) {
    // create vbo (vertex buffer object)
    uint vbo;
    glGenBuffers(1, &vbo);
    // bind object (buffer)
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    // set options (buffer data)
    glBufferData(GL_ARRAY_BUFFER, size, buf, GL_STATIC_DRAW);
}

uint newVAO(float* vertices, size_t size) {
    uint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    
    // create VBO for current VAO
    newVBO(vertices, size * sizeof(float));
    
    return vao;
}

void newEBO(uint* buf, size_t size) {
    // create ebo (element buffer object)
    uint ebo;
    glGenBuffers(1, &ebo);
    // bind it
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    // set buffer data
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, buf, GL_STATIC_DRAW);
}

uint newTexture(string imgPath) {
    
    // load texture
    int img_w, img_h, n;
    unsigned char* data = stbi_load(imgPath.c_str(), &img_w, &img_h, &n, 0);
    if (n != 3) exit(1);
    if (data) { // if loading image successful, create a texture
        cout << "successfully loaded img " << img_w << "x" << img_h << ", " << n << " bit.\n";
        
        // create texture
        uint texture;
        glGenTextures(1, &texture);
        
        // bind it
        // binding texture also automatically sets the sampler2D uniform in fragment shader
        glBindTexture(GL_TEXTURE_2D, texture);
        
        // texture settings
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // generate texture and its mipmap from data
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img_w, img_h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        // after having texture, delete original data
        stbi_image_free(data);
        return texture;
    } else {
        cout << "unable to load image.\n" << endl;
        exit(1);
    }
}

uint newShaderProgram(string vertPath, string tescPath, string tesePath, string fragPath) {
    
    // load vertex shader file
    std::ifstream vIfs(vertPath);
    std::string vContent( (std::istreambuf_iterator<char>(vIfs) ),
                         (std::istreambuf_iterator<char>()     ));
    const char* vertexShaderFile = vContent.c_str();
    
    // load TCS file
    std::ifstream tcIfs(tescPath);
    std::string tcContent( (std::istreambuf_iterator<char>(tcIfs) ),
                          (std::istreambuf_iterator<char>()     ));
    const char* tescShaderFile = tcContent.c_str();
    
    // load TES file
    std::ifstream teIfs(tesePath);
    std::string teContent( (std::istreambuf_iterator<char>(teIfs) ),
                          (std::istreambuf_iterator<char>()     ));
    const char* teseShaderFile = teContent.c_str();
    
    // load fragment shader file
    std::ifstream fIfs(fragPath);
    std::string fContent( (std::istreambuf_iterator<char>(fIfs) ),
                         (std::istreambuf_iterator<char>()     ));
    const char* fragmentShaderFile = fContent.c_str();
    
    const GLchar** vertexShaderSource = { &vertexShaderFile };
    const GLchar** tescShaderSource = { &tescShaderFile };
    const GLchar** teseShaderSource = { &teseShaderFile };
    const GLchar** fragmentShaderSource = { &fragmentShaderFile };
    
    //---- vertex shader ----
    
    // create shader
    uint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    
    // attach to source
    glShaderSource(vertexShader, 1, vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    // check if compilation successful
    int  success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "vertex compile failed: \n" << infoLog << std::endl;
        exit(1);
    }
    
    //---- TCS ----
    
    // create shader
    uint TCS = glCreateShader(GL_TESS_CONTROL_SHADER);
    
    // attach to source
    glShaderSource(TCS, 1, tescShaderSource, NULL);
    glCompileShader(TCS);
    
    // check if compilation successful
    glGetShaderiv(TCS, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(TCS, 512, NULL, infoLog);
        std::cout << "TCS compile failed: \n" << infoLog << std::endl;
        exit(1);
    }
    
    //---- TES ----
    
    // create shader
    uint TES = glCreateShader(GL_TESS_EVALUATION_SHADER);
    
    // attach to source
    glShaderSource(TES, 1, teseShaderSource, NULL);
    glCompileShader(TES);
    
    // check if compilation successful
    glGetShaderiv(TES, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(TES, 512, NULL, infoLog);
        std::cout << "TES compile failed: \n" << infoLog << std::endl;
        exit(1);
    }
    
    //---- fragment shader ----
    
    // create shader
    uint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    
    // attach to source
    glShaderSource(fragmentShader, 1, fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    // check if compilation successful
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "fragment compile failed: \n" << infoLog << std::endl;
        exit(1);
    }
    
    //---- create shader program ----
    uint newProgram = glCreateProgram();
    glAttachShader(newProgram, vertexShader);
    glAttachShader(newProgram, TCS);
    glAttachShader(newProgram, TES);
    glAttachShader(newProgram, fragmentShader);
    glLinkProgram(newProgram);
    
    // check for success
    glGetProgramiv(newProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(newProgram, 512, NULL, infoLog);
        std::cout << "shader link failed: \n" << infoLog << std::endl;
        exit(1);
    }
    
    // delete shaders
    glDeleteShader(vertexShader);
    glDeleteShader(TCS);
    glDeleteShader(TES);
    glDeleteShader(fragmentShader);
    
    return newProgram;
    
}
