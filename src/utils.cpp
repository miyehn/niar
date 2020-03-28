#include "utils.hpp"
#include "Drawable.hpp"
#include "Mesh.hpp"

uint uniform_loc(uint shaderID, std::string uniformName) {
  uint location = glGetUniformLocation(shaderID, uniformName.c_str());
  if (location < 0) {
    std::cout << "unable to find location for uniform: " << uniformName << std::endl;
  }
  return location;
}

uint new_shader_program(std::string vertPath, std::string fragPath, std::string tescPath, std::string tesePath) {

  bool tessellate = tescPath.length() > 0 && tesePath.length() > 0;

  uint newProgram = glCreateProgram();
  uint vertexShader, fragmentShader, TCS, TES;

  // load vertex shader file
  std::ifstream vIfs(vertPath);
  std::string vContent( (std::istreambuf_iterator<char>(vIfs) ),
                       (std::istreambuf_iterator<char>()     ));
  const char* vertexShaderFile = vContent.c_str();
  const GLchar** vertexShaderSource = { &vertexShaderFile };
  
  // load fragment shader file
  std::ifstream fIfs(fragPath);
  std::string fContent( (std::istreambuf_iterator<char>(fIfs) ),
                       (std::istreambuf_iterator<char>()     ));
  const char* fragmentShaderFile = fContent.c_str();
  const GLchar** fragmentShaderSource = { &fragmentShaderFile };
  
  //---- vertex shader ----
  
  // create shader
  vertexShader = glCreateShader(GL_VERTEX_SHADER);
  
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
  
  //---- fragment shader ----
  
  // create shader
  fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  
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
  
  glAttachShader(newProgram, vertexShader);
  glAttachShader(newProgram, fragmentShader);
  

  if (tessellate) {

    // load TCS file
    std::ifstream tcIfs(tescPath);
    std::string tcContent( (std::istreambuf_iterator<char>(tcIfs) ),
                          (std::istreambuf_iterator<char>()     ));
    const char* tescShaderFile = tcContent.c_str();
    const GLchar** tescShaderSource = { &tescShaderFile };
    
    // load TES file
    std::ifstream teIfs(tesePath);
    std::string teContent( (std::istreambuf_iterator<char>(teIfs) ),
                          (std::istreambuf_iterator<char>()     ));
    const char* teseShaderFile = teContent.c_str();
    const GLchar** teseShaderSource = { &teseShaderFile };

    //---- TCS ----
    
    // create shader
    TCS = glCreateShader(GL_TESS_CONTROL_SHADER);
    
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
    TES = glCreateShader(GL_TESS_EVALUATION_SHADER);
    
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
  
    glAttachShader(newProgram, TCS);
    glAttachShader(newProgram, TES);
  }

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
  glDeleteShader(fragmentShader);
  if (tessellate) {
    glDeleteShader(TCS);
    glDeleteShader(TES);
  }
  
  return newProgram;
}

std::vector<Mesh*> load_meshes(std::string source) {
  // import mesh from source
  Assimp::Importer importer;
  const aiScene* scene = importer.ReadFile(source,
      aiProcess_GenSmoothNormals |
      aiProcess_CalcTangentSpace |
      aiProcess_Triangulate |
      aiProcess_JoinIdenticalVertices |
      aiProcess_SortByPType);
  if (!scene) {
    ERR(importer.GetErrorString());
  }

  // access and create mesh drawables from imported source
  std::vector<Mesh*> meshes;
  for (int i=0; i<scene->mNumMeshes; i++) {
    aiMesh* mesh = scene->mMeshes[i];
    if (mesh) meshes.push_back(new Mesh(mesh));
  }

  LOGF("loaded %d meshes", meshes.size());

  // importer seems to automatically handle memory release for scene
  return meshes;
}
