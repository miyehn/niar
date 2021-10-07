#pragma once
#include "Utils/lib.h"
#include "Utils/myn/Timer.h"
#include "vulkan/vulkan/vulkan.h"
#include <chrono>

struct Drawable;
struct Scene;
struct Vulkan;

struct Program {

	static Program* Instance;
	
	static void pathtrace_to_file(size_t w, size_t h, const std::string& path);
		
	// generic program properties
	std::string name;
	size_t width;
	size_t height;
	int drawable_width;
	int drawable_height;
	
	SDL_Window* window;
	SDL_GLContext context;

	TimePoint previous_time;

	// input
	bool receiving_text = false;
	std::string input_str;
	void process_input();

	std::vector<Scene*> scenes;

	//-------- program lifecycle --------
	
	Program(
			std::string name, 
			int width, 
			int height);
	~Program();

	void init_opengl_window();

	bool one_loop();

	void run_opengl();
	void run_vulkan();
	
	// implementation of these decides game behavior.
	void load_resources();
	void load_resources_vulkan();

	void update(float elapsed);
	void draw();

	void release_resources();

};
