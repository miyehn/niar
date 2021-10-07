#include "cxxopts/cxxopts.hpp"
#include "Program.hpp"
#include "Scene/Scene.hpp"
#include "Scene/Camera.hpp"
#include "Pathtracer/Pathtracer.hpp"
#include "Input.hpp"

#include "Render/Vulkan/Vulkan.hpp"
#include "Render/gfx/gfx.h"
#include "Asset/Material.h"
#include "Asset/Mesh.h"

Program* Program::Instance;
#ifdef _WIN32
#ifdef main
#undef main
#endif
#endif
int main(int argc, const char * argv[])
{
	initialize_global_config();

	std::srand(time(nullptr));

	cxxopts::Options options("niar", "a toy renderer");
	options.allow_unrecognised_options();
	options.add_options()
		("w,width", "window width", cxxopts::value<int>())
		("h,height", "window height", cxxopts::value<int>())
		("o,output", "output path", cxxopts::value<std::string>());

	auto result = options.parse(argc, argv);
	//--------------------

	uint w = 800;
	uint h = 600;

	if (result.count("output")) {
		// render pathtracer scene to file
		w = 200; h = 150;
		if (result.count("width")) {
			w = result["width"].as<int>();
		}
		if (result.count("height")) {
			h = result["height"].as<int>();
		}
		std::string path = result["output"].as<std::string>();
		LOG("rendering pathtracer scene to file: %s", path.c_str());
		Program::pathtrace_to_file(w, h, path);
		return 0;
	}

	Program::Instance = new Program("niar", w, h);

	if (Cfg.TestVulkan) Program::Instance->run_vulkan();
	else Program::Instance->run_opengl();

	delete Program::Instance;
	return 0;
}

Program::Program(std::string name, int width, int height) {
	this->name = name;
	this->width = width;
	this->height = height;
}

void Program::init_opengl_window() {

	SDL_Init(SDL_INIT_VIDEO);
	
	// OpenGL settings
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	
	// create window
	this->window = SDL_CreateWindow(
		name.c_str(),
		100, 100, // SDL_WINDOWPOS_UNDEFINED, or SDL_WINDOWPOS_CENTERED
		width, height, // specify window size
		SDL_WINDOW_OPENGL
		);
	if (!window) {
		ERR("Error creating SDL window: %s", SDL_GetError());
		exit(1);
	}

	// store drawable sizes
	SDL_GL_GetDrawableSize(window, &drawable_width, &drawable_height);
	
	// create context
	this->context = SDL_GL_CreateContext(window);
	if (!context) {
		SDL_DestroyWindow(window);
		std::cerr << "Error creating OpenGL context: " << SDL_GetError() << std::endl;
		exit(1);
	}
	
	// glew setup
	// https://open.gl/context#Onemorething
	glewExperimental = GL_TRUE;
	glewInit();

}

void Program::load_resources_vulkan()
{
	LOG("loading resources (vulkan)...");
	// initialize_asset_config();

	Pathtracer::Instance = new Pathtracer(width, height, "Niar");
	Camera::Active = new Camera(width, height);

	Scene* scene;

	Camera::Active->move_speed = 16.0f;
	Camera::Active->position = vec3(0, -25, 15);
	Camera::Active->cutoffFar = 100.0f;

	scene = new Scene();
	scene->load(Cfg.SceneSource, false);

	// scene->initialize_graphics();

	Scene::Active = scene;
	scenes.push_back(scene);
}

Program::~Program() {
}

bool Program::one_loop() {

	SDL_Event event;
	bool quit = false;

	// currently everything handles everything (except quit)
	while (SDL_PollEvent(&event)==1 && !quit) {

		// termination
		if (event.type == SDL_QUIT) { quit=true; break; }
		else if (event.type==SDL_KEYUP && 
				event.key.keysym.sym==SDLK_ESCAPE) { quit=true; break; }

		// console input
		else if (event.type==SDL_KEYUP && !receiving_text && event.key.keysym.sym==SDLK_SLASH) {
			input_str = "";
			receiving_text = true;
			std::cout << "> " << std::flush;
		}
		else if (event.type == SDL_TEXTINPUT && receiving_text) {
			input_str += event.text.text;
			std::cout << event.text.text << std::flush;
		}
		else if (event.type == SDL_KEYUP && receiving_text && event.key.keysym.sym==SDLK_BACKSPACE) {
			input_str = input_str.substr(0, input_str.length()-1);
			std::cout << "\r> " << input_str << std::flush;
		}
		else if (event.type == SDL_KEYUP && receiving_text && event.key.keysym.sym==SDLK_RETURN) {
			receiving_text = false;
			std::cout << std::endl;
			process_input();
		}

		else if (!receiving_text && !Cfg.TestVulkan) {
			// toggle between rasterizer & pathtracer
			if (event.type==SDL_KEYUP && event.key.keysym.sym==SDLK_TAB) {
				if (Pathtracer::Instance->enabled) Pathtracer::Instance->disable();
				else Pathtracer::Instance->enable();
			}
			// update singletons
			if (Pathtracer::Instance->enabled)
				Pathtracer::Instance->handle_event(event);
			// let all scene(s) handle the input
			for (uint i=0; i<scenes.size(); i++) {
				scenes[i]->handle_event(event);
			}
		}
	}
	if (quit) return false;
	
	TimePoint current_time = std::chrono::high_resolution_clock::now();
	float elapsed = std::chrono::duration<float>(current_time - previous_time).count();
	elapsed = std::min(0.1f, elapsed);
	previous_time = current_time;

	if (Cfg.TestVulkan)
	{
#if 0
		if (Scene::Active && Scene::Active->enabled)
		{
			// TODO: this currently crashes because Mesh::Draw() needs a material, but mesh materials are init with the rest of opengl stuff.
			// TODO: push back all gpu init to a specific time.
			Scene::Active->material_set = Cfg.MaterialSet->get();
			Scene::Active->draw_content();
		}
#else
		//Vulkan::Instance->drawFrame();
#endif
	}
	else
	{
		update(elapsed);
		draw();
	}

	return true;
}

void Program::run_opengl() {
	
	init_opengl_window();

	load_resources();

	this->previous_time = std::chrono::high_resolution_clock::now();
	while (one_loop()) {
		SDL_GL_SwapWindow(window);
	}

	release_resources();

	#ifndef WIN32 // for whatever reason, on Windows including any of these lines makes SDL unable to close the window and quit properly
	// tear down
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();
	#endif
}

void Program::run_vulkan()
{
	gfx::init_window("niar", width, height, &window, &drawable_width, &drawable_height);

	MatVulkan* matVulkan = new MatVulkan();
	matVulkan->pipeline = new gfx::Pipeline();

	Vulkan::Instance->initSampleInstance(matVulkan->pipeline);

	load_resources_vulkan();
	while(true)
	{
		SDL_Event event;
		bool quit = false;
		while (SDL_PollEvent(&event)==1 && !quit)
		{
			// termination
			if (event.type == SDL_QUIT) { quit=true; break; }
			else if (event.type==SDL_KEYUP &&
					 event.key.keysym.sym==SDLK_ESCAPE) { quit=true; break; }

		}
		if (quit) break;

		auto cmdbuf = Vulkan::Instance->beginFrame();
		// render to RT, etc...
		Vulkan::Instance->beginSwapChainRenderPass(cmdbuf, matVulkan->pipeline->getRenderPass());
		//Vulkan::Instance->testDraw(cmdbuf, matVulkan->pipeline);
		Scene* scene = Scene::Active;
		for (int i = 0; i < scene->children.size(); i++)
		{
			if (Mesh* m = dynamic_cast<Mesh*>(scene->children[i]))
			{
				m->draw(cmdbuf, matVulkan->pipeline);
			}
		}
		Vulkan::Instance->endSwapChainRenderPass(cmdbuf);
		Vulkan::Instance->endFrame();
	}
	Vulkan::Instance->waitDeviceIdle();
	gfx::ShaderModule::cleanup();

	release_resources();

	delete matVulkan;
	delete Vulkan::Instance;
	SDL_DestroyWindow(window);
	SDL_Quit();
}

void Program::update(float elapsed) {
	
	// camera
	if (Camera::Active) Camera::Active->update_control(elapsed);
	// pathtracer
	if (Pathtracer::Instance && Pathtracer::Instance->enabled)
		Pathtracer::Instance->update(elapsed);
	// scenes (only the active one updates)
	if (Scene::Active && Scene::Active->enabled) {
		Scene::Active->update(elapsed);
	}
	
}

void Program::draw() {

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glClearColor(0.1f, 0.1f, 0.1f, 1);
	glClearDepth(1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// scenes
	if (Scene::Active && Scene::Active->enabled) {
		Scene::Active->draw();
	}
	// pathtracer
	if (Pathtracer::Instance && Pathtracer::Instance->enabled) 
		Pathtracer::Instance->draw();
		
}