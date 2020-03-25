#include "Drawable.hpp"

struct Cube : Drawable {
  
  Cube(Camera* camera);
  ~Cube();

  // inherited
  void update(float time_elapsed);
  bool handle_event(SDL_Event event);
  void draw();

  // properties and methods
  const aiScene* scene;
  
  // material / shader
  uint shader;
  uint vbo;
  uint vao;
};

