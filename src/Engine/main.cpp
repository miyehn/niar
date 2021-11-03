#include "cxxopts/cxxopts.hpp"
#include "Program.hpp"
#include "Scene/Scene.hpp"
#include "Scene/Camera.hpp"
#include "Pathtracer/Pathtracer.hpp"
#include "Input.hpp"
#include "Asset/GlMaterial.h"
#include "Render/Materials/DeferredBasepass.h"
#include "Render/Materials/DeferredLighting.h"
#include "Asset/Mesh.h"
#include "Render/Materials/Texture.h"

#include "Render/Vulkan/SamplerCache.h"
#include "Render/Vulkan/ShaderModule.h"
#include "Render/Vulkan/VulkanUtils.h"
#include "Engine/DebugUI.h"

#include "Utils/myn/RenderDoc.h"

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
	RenderDoc::load("niar");

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

	Program::Instance->run_vulkan();

	delete Program::Instance;
	return 0;
}

Program::Program(std::string name, int width, int height) {
	this->name = name;
	this->width = width;
	this->height = height;
}

void Program::load_resources_vulkan()
{
	LOG("loading resources (vulkan)...");
	// initialize_asset_config();
	initialize_pathtracer_config();

	Pathtracer::Instance = new Pathtracer(width, height, "Niar");

	Scene* scene;

	if (Cfg.UseCornellBoxScene)
	{
		scene = Pathtracer::load_cornellbox_scene(true);
		Camera::Active = new Camera(width, height);
	}

	/* Load from file or manually setup scene
	 * Renders with specified shader set: defaults to 0 (full deferred)
	 */
	else
	{
		scene = new Scene("Water Tower");
		scene->load(Cfg.SceneSource, false);
	}

	Scene::Active = scene;
	scenes.push_back(scene);
}

Program::~Program() {
}

void Program::run_vulkan()
{
	vk::init_window("niar", width, height, &window, &drawable_width, &drawable_height);

	load_resources_vulkan();

	Texture2D::createDefaultTextures();

	DeferredRenderer* deferredRenderer = DeferredRenderer::get();

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
	new MatDeferredLighting();

	Scene* scene = Scene::Active;
	scene->foreach_descendent_bfs([](Drawable* child){
		if (Mesh* m = dynamic_cast<Mesh*>(child))
		{
			m->material = Material::find("geometry");
		}
	});

	ui::usePurpleStyle();

	ui::button("capture frame", RenderDoc::captureNextFrame);

	ui::elem([](){
		ImGui::Separator();
	});

	ui::button("toggle renderdoc overlay", RenderDoc::toggleOverlay);

	static bool show_imgui_demo = false;
	ui::checkBox("show ImGui demo", &show_imgui_demo);

	static bool show_global_transform = false;
	ui::checkBox("show global transform", &show_global_transform, scene->name + " (scene)");
	ui::elem([&]()
	{
		std::function<void(Drawable* node)> make_tree = [&make_tree](Drawable* node)
		{
			if (ImGui::TreeNode(node->name.c_str()))
			{
				node->draw_transform_ui(show_global_transform);
				for (auto child : node->children) make_tree(child);
				ImGui::TreePop();
			}
		};
		for (auto child : scene->children) make_tree(child);
	}, scene->name + " (scene)");

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
			// pass on control to the rest of the app if event.type is not keyup
			bool imgui_processed_input = ImGui_ImplSDL2_ProcessEvent(&event);
			if (imgui_processed_input && event.type != SDL_KEYUP) continue;

			// console input
			if (event.type==SDL_KEYUP && !receiving_text && event.key.keysym.sym==SDLK_SLASH) {
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

		RenderDoc::potentiallyStartCapture();

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
			Pathtracer::Instance->draw_vulkan(cmdbuf);
		}
		else
		{
			deferredRenderer->camera = Camera::Active;
			deferredRenderer->drawable = Scene::Active;
			deferredRenderer->render(cmdbuf);
		}

		{
			SCOPED_DRAW_EVENT(cmdbuf, "ImGui UI")
			ImGui::Render();

			Vulkan::Instance->beginSwapChainRenderPass(cmdbuf);
			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdbuf);
			Vulkan::Instance->endSwapChainRenderPass(cmdbuf);
		}

		Vulkan::Instance->endFrame();

		RenderDoc::potentiallyEndCapture();
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

