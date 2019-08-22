//
//  Program+Game.cpp
//  sdl-empty
//
//  Created by miyehn on 8/20/19.
//  Copyright © 2019 miyehn. All rights reserved.
//

#include "Program.hpp"

using namespace std;
using namespace glm;

void Program::setup() {
    cout << "hello from SDL." << endl;
    
    shader = newShaderProgram(
                              "./shaders/grass.vert",
                              "./shaders/grass.tesc",
                              "./shaders/grass.tese",
                              "./shaders/grass.frag");
    glUseProgram(shader);
    
    vao = newVAO(vertices, 24);
    setVertexAtrribPointers();
    
    glPatchParameteri(GL_PATCH_VERTICES, 1);
    
    cout << "done with setup." << endl;

    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile("../media/example.obj", aiProcess_Triangulate);
    if (!scene) cout << "import failed" << endl;

}

void Program::draw() {
    
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);
    
    mat4 transformation = getTransformation(vec3(0.0f, 0.0f, 0.0f));
    glUniformMatrix4fv(unifLoc(shader, "transformation"), 1, GL_FALSE, value_ptr(transformation));
    glDrawArrays(GL_PATCHES, 0, 6);
    
}

void Program::typedKey(SDL_Keycode key) {
    if (key == SDLK_UP) {
        cameraPosition.y += 0.1f;
    } else if (key == SDLK_DOWN) {
        cameraPosition.y -= 0.1f;
    } else if (key == SDLK_LEFT) {
        cameraPosition.x -= 0.1f;
    } else if (key == SDLK_RIGHT) {
        cameraPosition.x += 0.1f;
    }
}

void Program::setVertexAtrribPointers() {
    // set attrib pointer for position.
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
}

