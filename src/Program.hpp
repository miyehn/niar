#pragma once
#include "lib.h"
#include <chrono>

struct Drawable;
struct Scene;

struct Program {

	static Program* Instance;
	
	static void pathtrace_to_file(size_t w, size_t h, const std::string& path);
	
	Program(
			std::string name, 
			int width, 
			int height);
	~Program();
	void run();
		
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
	
	// implementation of these decides game behavior.
	void load_resources();

	void setup();
	void update(float elapsed);
	void draw();

	void release_resources();

};

