#include "Shader.hpp"

Shader::Shader(const std::string& frag_path) {
	id = glCreateProgram();
	uint vertexShader, fragmentShader;

  LOGF("-- blit shader program \"%s\"", frag_path.c_str());

	int  success;
	char infoLog[512];
	if (quad_vert) vertexShader = quad_vert;
	else {
		// load vertex shader file
		std::ifstream vIfs("../shaders/quad.vert");
		std::string vContent( (std::istreambuf_iterator<char>(vIfs) ),
												 (std::istreambuf_iterator<char>()     ));
		const char* vertexShaderFile = vContent.c_str();
		const GLchar** vertexShaderSource = { &vertexShaderFile };
  
		// create shader
		vertexShader = glCreateShader(GL_VERTEX_SHADER);
		
		// attach to source
		glShaderSource(vertexShader, 1, vertexShaderSource, NULL);
		glCompileShader(vertexShader);
		
		// check if compilation successful
		glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
		if (!success) {
				glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
				std::cout << "vertex compile failed: \n" << infoLog << std::endl;
				exit(1);
		}
		quad_vert = vertexShader;
	}

  // load fragment shader file
  std::ifstream fIfs(frag_path);
  std::string fContent( (std::istreambuf_iterator<char>(fIfs) ),
                       (std::istreambuf_iterator<char>()     ));
  const char* fragmentShaderFile = fContent.c_str();
  const GLchar** fragmentShaderSource = { &fragmentShaderFile };
  
  // create shader
  fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  
  // attach to source
  glShaderSource(fragmentShader, 1, fragmentShaderSource, NULL);
  glCompileShader(fragmentShader);
  
  // check if compilation successful
  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
  if (!success) {
      glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
      std::cout << "fragment compile failed: \n" << infoLog << std::endl;
      exit(1);
  }
  
  glAttachShader(id, vertexShader);
  glAttachShader(id, fragmentShader);
  
  glLinkProgram(id);

  // check for success
  glGetProgramiv(id, GL_LINK_STATUS, &success);
  if (!success) {
      glGetProgramInfoLog(id, 512, NULL, infoLog);
      std::cout << "shader link failed: \n" << infoLog << std::endl;
      exit(1);
  }

  glDetachShader(id, vertexShader);
  glDetachShader(id, fragmentShader);

  glDeleteShader(fragmentShader);
}

Shader::Shader(
    const std::string& vert_path, 
    const std::string& frag_path, 
    const std::string& tesc_path, 
    const std::string& tese_path) {

  LOGF("-- shader program \"%s\", \"%s\"", vert_path.c_str(), frag_path.c_str());

  bool tessellate = tesc_path.length() > 0 && tese_path.length() > 0;

  id = glCreateProgram();
  uint vertexShader, fragmentShader, TCS, TES;

  // load vertex shader file
  std::ifstream vIfs(vert_path);
  std::string vContent( (std::istreambuf_iterator<char>(vIfs) ),
                       (std::istreambuf_iterator<char>()     ));
  const char* vertexShaderFile = vContent.c_str();
  const GLchar** vertexShaderSource = { &vertexShaderFile };
  
  // load fragment shader file
  std::ifstream fIfs(frag_path);
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
      glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
      std::cout << "fragment compile failed: \n" << infoLog << std::endl;
      exit(1);
  }
  
  glAttachShader(id, vertexShader);
  glAttachShader(id, fragmentShader);
  

  if (tessellate) {

    // load TCS file
    std::ifstream tcIfs(tesc_path);
    std::string tcContent( (std::istreambuf_iterator<char>(tcIfs) ),
                          (std::istreambuf_iterator<char>()     ));
    const char* tescShaderFile = tcContent.c_str();
    const GLchar** tescShaderSource = { &tescShaderFile };
    
    // load TES file
    std::ifstream teIfs(tese_path);
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
  
    glAttachShader(id, TCS);
    glAttachShader(id, TES);
  }

  glLinkProgram(id);

  // check for success
  glGetProgramiv(id, GL_LINK_STATUS, &success);
  if (!success) {
      glGetProgramInfoLog(id, 512, NULL, infoLog);
      std::cout << "shader link failed: \n" << infoLog << std::endl;
      exit(1);
  }

  // detach shaders
  glDetachShader(id, vertexShader);
  glDetachShader(id, fragmentShader);
  if (tessellate) {
    glDetachShader(id, TCS);
    glDetachShader(id, TES);
  }
  
  // delete shaders
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);
  if (tessellate) {
    glDeleteShader(TCS);
    glDeleteShader(TES);
  }

}


int Shader::uniform_loc(const std::string& uniformName) const {
  int location = glGetUniformLocation(id, uniformName.c_str());
  if (location < 0) {
    WARNF("%s unable to find location for uniform \"%s\"", name.c_str(), uniformName.c_str());
  }
  return location;
}

void Shader::set_bool(const std::string &name, bool value) const {       
	glUniform1i(uniform_loc(name), (int)value); 
}

void Shader::set_int(const std::string &name, int value) const {
	glUniform1i(uniform_loc(name), value); 
}

void Shader::set_float(const std::string &name, float value) const { 
	glUniform1f(uniform_loc(name), value); 
}

void Shader::set_vec2(const std::string &name, vec2 v) const { 
	glUniform2f(uniform_loc(name), v.x, v.y); 
}

void Shader::set_vec3(const std::string &name, vec3 v) const { 
	glUniform3f(uniform_loc(name), v.x, v.y, v.z); 
}

void Shader::set_vec4(const std::string &name, vec4 v) const { 
	glUniform4f(uniform_loc(name), v.x, v.y, v.z, v.w); 
}

void Shader::set_mat3(const std::string &name, mat3 mat) const {
	glUniformMatrix3fv(uniform_loc(name), 1, GL_FALSE, value_ptr(mat));
}

void Shader::set_mat4(const std::string &name, mat4 mat) const {
	glUniformMatrix4fv(uniform_loc(name), 1, GL_FALSE, value_ptr(mat));
}

void Shader::set_tex2D(const std::string &name, uint texture_unit, uint texture_id) const {
	glUniform1i(uniform_loc(name), texture_unit);
  glActiveTexture(GL_TEXTURE0 + texture_unit);
  glBindTexture(GL_TEXTURE_2D, texture_id);
  glActiveTexture(GL_TEXTURE0);
}

void Shader::set_texCube(const std::string &name, uint texture_unit, uint texture_id) const {
	glUniform1i(uniform_loc(name), texture_unit);
  glActiveTexture(GL_TEXTURE0 + texture_unit);
  glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id);
  glActiveTexture(GL_TEXTURE0);
	GL_ERRORS();
}

//-------------------- Blit -------------------------

float n = 1.0f;
std::vector<float> quad_vertices = {
	-n, n, 0, 1, // tl
	-n, -n, 0, 0, // bl
	n, -n, 1, 0, // br
	-n, n, 0, 1, // tl
	n, -n, 1, 0, // br
	n, n, 1, 1 // tr
};

uint Blit::vao = 0;
uint Blit::vbo = 0;

Blit::Blit(const std::string& frag_path) {

	if (!Blit::vao || !Blit::vbo) {
		LOG("initializing Blit vao & vbo");
		glGenBuffers(1, &Blit::vbo);
		glGenVertexArrays(1, &Blit::vao);
		glBindVertexArray(Blit::vao);
		{
			glBindBuffer(GL_ARRAY_BUFFER, Blit::vbo);
			glBufferData(GL_ARRAY_BUFFER, 
					quad_vertices.size() * sizeof(float), quad_vertices.data(), GL_STATIC_DRAW);

			glVertexAttribPointer(
					0, // atrib index
					2, // num of data elems
					GL_FLOAT, // data type
					GL_FALSE, // normalized
					4 * sizeof(float), // stride size
					(void*)0); // offset in bytes since stride start
			glEnableVertexAttribArray(0);

			glVertexAttribPointer(
					1, // atrib index
					2, // num of data elems
					GL_FLOAT, // data type
					GL_FALSE, // normalized
					4 * sizeof(float), // stride size
					(void*)(2 * sizeof(float))); // offset in bytes since stride start
			glEnableVertexAttribArray(1);
		}
		glBindVertexArray(0);

	} 

	shader = Shader(frag_path);
}

void Blit::begin_pass() {
	glGetBooleanv(GL_DEPTH_TEST, &cached_depth_test);
	glDisable(GL_DEPTH_TEST);
	glUseProgram(shader.id);
	glBindVertexArray(Blit::vao);
	glBindBuffer(GL_ARRAY_BUFFER, Blit::vbo);
}

void Blit::end_pass() {
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glUseProgram(0);
	if (cached_depth_test == GL_TRUE) glEnable(GL_DEPTH_TEST);
}
