#include "cxxopts/cxxopts.hpp"
#include "Scene/Scene.hpp"
#include "Scene/Camera.hpp"
#include "Scene/PathtracerController.h"
#include "Scene/RtxTriangle.h"
#include "Pathtracer/Pathtracer.hpp"
#include "Config.hpp"
#include "Render/Renderers/RayTracingRenderer.h"
#include "Render/Renderers/SimpleRenderer.h"
#include "Render/Mesh.h"
#include "Render/Texture.h"

#include "Render/Vulkan/SamplerCache.h"
#include "Render/Vulkan/VulkanUtils.h"
#include "Engine/DebugUI.h"

#include "Utils/myn/RenderDoc.h"

#include <SDL2/SDL.h>
#include <imgui.h>
#include <backends/imgui_impl_sdl.h>
#include <backends/imgui_impl_vulkan.h>

#include "Render/Materials/ComputeShaders.h"
#include "Render/Renderers/DeferredRenderer.h"

using namespace myn;

#ifdef _WIN32
#ifdef main
#undef main
#endif
#endif

//////////////////////////////////////////////////////////////////

Camera* Camera::Active;
Scene* Scene::Active;

namespace
{
	uint32_t width = 1280;
	uint32_t height = 720;

	std::string name;

	SDL_Window* window;
	SDL_GLContext context;

	myn::TimePoint previous_time;

	bool show_imgui_demo = false;

	struct // legacy
	{
		bool receiving = false;
		std::string text = "";
	} input;

	// other potentially temporary globals

	enum e_renderer {
		simple = 0,
		deferred = 1,
		pathtracer = 2
	} renderer_index = e_renderer::deferred;

	std::vector<Renderer*> renderers{};

} // fileprivate

static void init();

static bool process_input();

static void update(float elapsed);

static void draw();

static void cleanup();

//////////////////////////////////////////////////////////////////

static void init()
{
	vk::init_window("niar", width, height, &window);

	LOG("loading resources (vulkan)...");
	Texture2D::createDefaultTextures();
	initialize_pathtracer_config();

#if 0
	Scene* gltf = new Scene("Test stage");
	//gltf->load_tinygltf(Cfg.SceneSource, false);
	Scene::Active = gltf;

	// rtx
	auto tri = new RtxTriangle();
	Scene::Active->add_child(tri);

	// renderer setup
	auto rtRenderer = RayTracingRenderer::get();
	rtRenderer->outImage = tri->outImage;
	rtRenderer->debugSetup(nullptr);
	renderer = rtRenderer;
#else
	Scene* gltf = new Scene("Cfg.SceneSource");
	gltf->load_tinygltf(Cfg.SceneSource, false);
	gltf->add_child(new PathtracerController());
	Scene::Active = gltf;
#endif

	// find a camera and set it active
	Scene::Active->foreach_descendent_bfs([](SceneObject* obj) {
		auto cam = dynamic_cast<Camera*>(obj);
		if (cam) Camera::Active = cam;
	});

	ui::usePurpleStyle();
	ui::checkBox("show ImGui demo", &show_imgui_demo);

	// renderdoc
	if (Cfg.RenderDoc)
	{
		ui::button("capture frame", RenderDoc::captureNextFrame, "Rendering");
		ui::elem([](){ ImGui::SameLine(); }, "Rendering");
		ui::button("toggle renderdoc overlay", RenderDoc::toggleOverlay, "Rendering");
		ui::elem([](){ ImGui::Separator(); }, "Rendering");
	}

	// renderer selector
	{
		renderers.push_back(SimpleRenderer::get());
		renderers.push_back(DeferredRenderer::get());
		renderers.push_back(Pathtracer::get(width, height));

		auto rendererIndexRef = (int*)&renderer_index;
		ui::elem([rendererIndexRef]()
		{
			ImGui::Combo("renderer", rendererIndexRef, "Simple\0Deferred\0Pathtracer\0\0");
			ImGui::Separator();

			renderers[renderer_index]->draw_config_ui();

		}, "Rendering");
	}

	// scene hierarchy
	{
		static bool show_global_transform = false;
		ui::checkBox("show global transform", &show_global_transform, "Scene hierarchy (" + Scene::Active->name + ")");
		ui::elem([&]()
		{
			std::function<void(SceneObject* node)> make_tree = [&make_tree](SceneObject* node)
			{
				if (ImGui::TreeNode(node->name.c_str()))
				{
					if (ImGui::Button(node->enabled() ? "disable" : "enable")) {
						node->toggle_enabled();
					}
					if (node->show_transform) node->draw_transform_ui(show_global_transform);
					node->draw_config_ui();
					for (auto child : node->children) make_tree(child);
					ImGui::TreePop();
				}
			};
			for (auto child : Scene::Active->children) make_tree(child);
		}, "Scene hierarchy (" + Scene::Active->name + ")");
	}

}

static bool process_input()
{
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
		//if (imgui_processed_input) continue;
		if (imgui_processed_input && event.type != SDL_KEYUP) continue;
		else
		{
			// let all scene(s) handle the input
			Scene::Active->handle_event(event);
		}
	}
	return quit;
}

static void update(float elapsed)
{
	// camera
	if (Camera::Active &&
		!ImGui::GetIO().WantCaptureMouse &&
		!ImGui::GetIO().WantCaptureKeyboard)
	{
		Camera::Active->update_control(elapsed);
	}
	// scenes (only the active one updates)
	if (Scene::Active && Scene::Active->enabled()) {
		Scene::Active->update(elapsed);
	}
}

static void draw()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL2_NewFrame(window);
	ImGui::NewFrame();

	if (show_imgui_demo) ImGui::ShowDemoWindow();

	ui::drawUI();

	// renderer selection and configuration

	static Renderer* prev_renderer = nullptr;
	Renderer* renderer = renderers[renderer_index];
	renderer->camera = Camera::Active;
	renderer->drawable = Scene::Active;

	if (renderer != prev_renderer) {
		if (renderer) renderer->on_selected();
		if (prev_renderer) prev_renderer->on_unselected();
		prev_renderer = renderer;
	}

	// draw with current renderer

	auto cmdbuf = Vulkan::Instance->beginFrame();
	{
		renderer->render(cmdbuf);
		{
			SCOPED_DRAW_EVENT(cmdbuf, "ImGui UI")
			ImGui::Render();

			Vulkan::Instance->beginSwapChainRenderPass(cmdbuf);
			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdbuf);
			Vulkan::Instance->endSwapChainRenderPass(cmdbuf);
		}
	}
	Vulkan::Instance->endFrame();
}

static void cleanup()
{
	Vulkan::Instance->waitDeviceIdle();

	delete Scene::Active;
	delete Vulkan::Instance;

	SDL_DestroyWindow(window);
	SDL_Quit();
	Asset::clear_references();
}

int main(int argc, const char * argv[])
{
	std::srand(time(nullptr));

	new ConfigFile("config.ini", [](const ConfigFile &cfg){
		Cfg.SceneSource = std::string(ROOT_DIR"/") + cfg.lookup<const char*>("SceneSource");
		Cfg.PathtracerConfigSource = std::string(ROOT_DIR"/") + cfg.lookup<const char*>("PathtracerConfigSource");
#ifdef MACOS
		Cfg.RenderDoc = 0;
		Cfg.RTX = 0;
#else
		Cfg.RenderDoc = config_src.lookup("Debug.RenderDoc");
		Cfg.RTX = config_src.lookup("Debug.RTX");
#endif
		Cfg.CollapseSceneTree = cfg.lookup<int>("Debug.CollapseSceneTree");
	});

	//initialize_global_config();

	cxxopts::Options options("niar", "a toy renderer");
	options.allow_unrecognised_options();
	options.add_options()
		("w,width", "window width", cxxopts::value<int>())
		("h,height", "window height", cxxopts::value<int>())
		("o,output", "output path", cxxopts::value<std::string>());

	auto result = options.parse(argc, argv);

	//------------- pathtrace to file --------------

	if (result.count("output")) // pathtrace to file
	{
		// render pathtracer scene to file
		int w = 200;
		int h = 150;
		if (result.count("width")) {
			w = result["width"].as<int>();
		}
		if (result.count("height")) {
			h = result["height"].as<int>();
		}
		std::string path = result["output"].as<std::string>();
		LOG("rendering pathtracer scene to file: %s", path.c_str());
		Pathtracer::pathtrace_to_file(w, h, path);
		return 0;
	}

	//------------- else: load and run the scene --------------

	if (Cfg.RenderDoc) RenderDoc::load("niar");

	init();

	while(true)
	{
		TimePoint current_time = std::chrono::high_resolution_clock::now();
		float elapsed = std::chrono::duration<float>(current_time - previous_time).count();
		elapsed = std::min(0.1f, elapsed);
		previous_time = current_time;

		bool should_quit = process_input();
		if (should_quit) break;

		RenderDoc::potentiallyStartCapture();

		update(elapsed);

		draw();

		RenderDoc::potentiallyEndCapture();
	}

	cleanup();

	return 0;
}
