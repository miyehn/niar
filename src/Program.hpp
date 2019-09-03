#include <chrono>
#include "Camera.hpp"
#include "GameObject.hpp"

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
    
    SDL_Window* window;
    SDL_GLContext context;
    TimePoint previous_time;

    Camera camera;
    vector<GameObject*> objects;
    
    // implementation of these decides game behavior.
    void setup();
    void update(float time_elapsed);
    void draw();

};
