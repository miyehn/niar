#include <vector>
#include <chrono>
#include "Camera.hpp"
#include "GameObject.hpp"

using namespace std;
using namespace glm;
typedef chrono::time_point<chrono::high_resolution_clock> TimePoint;

class Program {
    
public:
    
    Program(string name, int width, int height);
    ~Program();
    void run();
    
private:
    
    // program properties
    string name;
    size_t width;
    size_t height;
    float aspectRatio() { return (float)width / (float)height; }
    
    SDL_Window* window;
    SDL_GLContext context;
    TimePoint previous_time;

    Camera camera;
    vector<GameObject*> objects;
    
    // implementation of these decides game behavior.
    void setup();
    void update(float time_elapsed);
    void draw();
    void setVertexAtrribPointers();
    
    // temporary global storage - might be factored out later.
    vec3 cameraPosition = vec3(0.0f, -2.0f, 2.0f);
    float cameraPitch = radians(45.0f);
    float fov = radians(55.0f);
    float cutoffNear = 0.1f;
    float cutoffFar = 30.0f;
    
    uint shader;
    uint vao;
    
    float vertices[24] = {
        -1.0f, 0.0f, 0.0f, 0.1,
        -0.5f, 0.0f, 0.0f, 0.2,
        -0.5f, 0.5f, 0.0f, 0.3,
        0.0f, 0.0f, 0.0f, 0.4,
        0.5f, 0.0f, 0.0f, 0.5,
        0.5f, 0.5f, 0.0f, 0.5
    };
    
};
