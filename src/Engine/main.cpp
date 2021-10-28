#include "cxxopts/cxxopts.hpp"
#include "Program.hpp"
#include "Scene/Scene.hpp"
#include "Scene/Camera.hpp"
#include "Pathtracer/Pathtracer.hpp"
#include "Input.hpp"
#include "Asset/GlMaterial.h"
#include "Render/Materials/DeferredBasepass.h"
#include "Render/Materials/DeferredPointLighting.h"
#include "Asset/Mesh.h"
#include "Render/Materials/Texture.h"

#include "Render/Vulkan/SamplerCache.h"
#include "Render/Vulkan/ShaderModule.h"
#include "Render/Vulkan/VulkanUtils.h"
#include "Engine/DebugUI.h"

#include <imgui.h>
#include <backends/imgui_impl_sdl.h>
#include <backends/imgui_impl_vulkan.h>

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

	uint w = 1280;
	uint h = 720;

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
	initialize_pathtracer_config();

	Pathtracer::Instance = new Pathtracer(width, height, "Niar");
	Camera::Active = new Camera(width, height);

	Scene* scene;

	if (Cfg.UseCornellBoxScene)
	{
		scene = Pathtracer::load_cornellbox_scene(true);
	}

	/* Load from file or manually setup scene
	 * Renders with specified shader set: defaults to 0 (full deferred)
	 */
	else
	{
		Camera::Active->move_speed = 16.0f;
		Camera::Active->position = vec3(0, -10, 2);
		Camera::Active->cutoffFar = 100.0f;

		scene = new Scene();
		scene->load(Cfg.SceneSource, false);
	}

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

void renderdoc_capture()
{
	LOG("capture!")
}

void Program::run_vulkan()
{
	vk::init_window("niar", width, height, &window, &drawable_width, &drawable_height);

	load_resources_vulkan();

	Texture2D::createDefaultTextures();

	DeferredRenderer* deferredRenderer = DeferredRenderer::get();

	// TODO: format mysteries?
	new Texture2D(std::string(ROOT_DIR"/") + "media/water_tower/Base_color.png", {4, 8, 1});
	new Texture2D(std::string(ROOT_DIR"/") + "media/water_tower/normal.png", {4, 8, 0});
	new Texture2D(std::string(ROOT_DIR"/") + "media/water_tower/metallic.png", {1, 8, 0});
	new Texture2D(std::string(ROOT_DIR"/") + "media/water_tower/roughness.png", {1, 8, 0});

	new MatDeferredBasepass(
		std::string(ROOT_DIR"/") + "media/water_tower/Base_color.png",
		std::string(ROOT_DIR"/") + "media/water_tower/normal.png",
		std::string(ROOT_DIR"/") + "media/water_tower/metallic.png",
		std::string(ROOT_DIR"/") + "media/water_tower/roughness.png",
		"");
	new MatDeferredPointLighting();

	Scene* scene = Scene::Active;
	for (int i = 0; i < scene->children.size(); i++)
	{
		if (Mesh* m = dynamic_cast<Mesh*>(scene->children[i]))
		{
			m->material = Material::find("geometry");
		}
	}

	ui::button("capture frame", renderdoc_capture);

	static bool show_imgui_demo = false;
	ui::checkBox("show ImGui demo", &show_imgui_demo);

	while(true)
	{
		// update
		TimePoint current_time = std::chrono::high_resolution_clock::now();
		float elapsed = std::chrono::duration<float>(current_time - previous_time).count();
		elapsed = std::min(0.1f, elapsed);
		previous_time = current_time;

		SDL_Event event;
		bool quit = false;
		while (SDL_PollEvent(&event)==1 && !quit)
		{
			// termination
			if (event.type == SDL_QUIT) { quit=true; break; }
			else if (event.type==SDL_KEYUP &&
					 event.key.keysym.sym==SDLK_ESCAPE) { quit=true; break; }

			// imgui
			if (ImGui_ImplSDL2_ProcessEvent(&event)) continue;

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

			else if (!receiving_text) {
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
		if (quit) break;

		if (Camera::Active &&
			!ImGui::GetIO().WantCaptureMouse &&
			!ImGui::GetIO().WantCaptureKeyboard)
		{
			Camera::Active->update_control(elapsed);
		}

		update(elapsed);

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL2_NewFrame(window);
		ImGui::NewFrame();

		if (show_imgui_demo) ImGui::ShowDemoWindow();

		ui::drawUI();

		// draw
		auto cmdbuf = Vulkan::Instance->beginFrame();

		if (Pathtracer::Instance && Pathtracer::Instance->enabled)
		{
			Pathtracer::Instance->draw_vulkan();
		}
		else
		{
			deferredRenderer->camera = Camera::Active;
			deferredRenderer->drawables = Scene::Active->children;
			deferredRenderer->render(cmdbuf);
		}

		ImGui::Render();

		Vulkan::Instance->beginSwapChainRenderPass(cmdbuf);
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdbuf);
		Vulkan::Instance->endSwapChainRenderPass(cmdbuf);

		Vulkan::Instance->endFrame();
	}

	Vulkan::Instance->waitDeviceIdle();
	DeferredRenderer::cleanup();
	ShaderModule::cleanup();
	Texture::cleanup();
	SamplerCache::cleanup();
	Material::cleanup();

	DescriptorSet::releasePool();
	DescriptorSetLayoutCache::cleanup();
	release_resources();

	delete Vulkan::Instance;
	SDL_DestroyWindow(window);
	SDL_Quit();
}

void Program::update(float elapsed) {
	
	// camera
	// if (Camera::Active) Camera::Active->update_control(elapsed);
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
