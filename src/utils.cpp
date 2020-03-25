#include "utils.hpp"

int unifLoc(uint shaderID, std::string uniformName) {
  int location = glGetUniformLocation(shaderID, uniformName.c_str());
  if (location < 0) {
    std::cout << "unable to find location for uniform: " << uniformName << std::endl;
    exit(1);
  }
  return location;
}

uint newShaderProgram(std::string vertPath, std::string tescPath, std::string tesePath, std::string fragPath) {
  
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

