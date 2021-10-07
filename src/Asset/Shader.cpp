#include "Shader.h"

uint Shader::quad_vert = 0;

std::unordered_map<std::string, Shader*> Shader::shader_pool;

Shader* Shader::get(const std::string& name) {
	auto res = shader_pool.find(name);
	if (res != shader_pool.end()) {
		return res->second;
	} else {
		ERR("no shader named %s", name.c_str());
		return nullptr;
	}
}

void Shader::add(const std::string& name, Shader* shader) {
	if (!shader) {
		ERR("trying to store null as shader %s. Skipping..", name.c_str());
		return;
	}
	shader_pool[name] = shader;
	LOG("%s", name.c_str());
}

void Shader::cleanup() {
	// TODO
}

Shader::Shader(const std::string& frag_path) {
	id = glCreateProgram();
	uint vertexShader, fragmentShader;

	int  success;
	char infoLog[512];
	if (Shader::quad_vert) {
		vertexShader = Shader::quad_vert;
	} else {
		// load vertex shader file
		std::ifstream vIfs(ROOT_DIR"/shaders/quad.vert");
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
		Shader::quad_vert = vertexShader;
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


int Shader::uniform_loc(const std::string& uniformName, bool optional) const {
	int location = glGetUniformLocation(id, uniformName.c_str());
	if (location < 0 && !optional) {
		WARN("%s unable to find location for uniform \"%s\"", name.c_str(), uniformName.c_str());
	}
	return location;
}

void Shader::set_bool(const std::string &name, bool value, bool optional) const {       
	int loc = uniform_loc(name, optional);
	if (loc>=0) glUniform1i(loc, (int)value); 
}

void Shader::set_int(const std::string &name, int value, bool optional) const {
	int loc = uniform_loc(name, optional);
	if (loc>=0) glUniform1i(uniform_loc(name), value); 
}

void Shader::set_float(const std::string &name, float value, bool optional) const { 
	int loc = uniform_loc(name, optional);
	if (loc>=0) glUniform1f(uniform_loc(name), value); 
}

void Shader::set_vec2(const std::string &name, vec2 v, bool optional) const { 
	int loc = uniform_loc(name, optional);
	if (loc>=0) glUniform2f(uniform_loc(name), v.x, v.y); 
}

void Shader::set_vec3(const std::string &name, vec3 v, bool optional) const { 
	int loc = uniform_loc(name, optional);
	if (loc>=0) glUniform3f(uniform_loc(name), v.x, v.y, v.z); 
}

void Shader::set_vec4(const std::string &name, vec4 v, bool optional) const { 
	int loc = uniform_loc(name, optional);
	if (loc>=0) glUniform4f(uniform_loc(name), v.x, v.y, v.z, v.w); 
}

void Shader::set_mat3(const std::string &name, mat3 mat, bool optional) const {
	int loc = uniform_loc(name, optional);
	if (loc>=0) glUniformMatrix3fv(uniform_loc(name), 1, GL_FALSE, value_ptr(mat));
}

void Shader::set_mat4(const std::string &name, mat4 mat, bool optional) const {
	int loc = uniform_loc(name, optional);
	if (loc>=0) glUniformMatrix4fv(uniform_loc(name), 1, GL_FALSE, value_ptr(mat));
}

void Shader::set_tex2D(const std::string &name, uint texture_unit, uint texture_id, bool optional) const {
	int loc = uniform_loc(name, optional);
	if (loc>=0) {
		glUniform1i(uniform_loc(name), texture_unit);
		glActiveTexture(GL_TEXTURE0 + texture_unit);
		glBindTexture(GL_TEXTURE_2D, texture_id);
		glActiveTexture(GL_TEXTURE0);
	}
}

void Shader::set_texCube(const std::string &name, uint texture_unit, uint texture_id, bool optional) const {
	int loc = uniform_loc(name, optional);
	if (loc>=0) {
		glUniform1i(uniform_loc(name), texture_unit);
		glActiveTexture(GL_TEXTURE0 + texture_unit);
		glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id);
		glActiveTexture(GL_TEXTURE0);
	}
}
