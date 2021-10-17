#include "Renderer.h"
#include "Asset/Mesh.h"
#include "Asset/Material.h"

void SimpleRenderer::render()
{
	auto cmdbuf = Vulkan::Instance->beginFrame();
	// render to RT, etc...
	Vulkan::Instance->beginSwapChainRenderPass(cmdbuf);
	for (auto drawable : drawables)
	{
		if (Mesh* m = dynamic_cast<Mesh*>(drawable))
		{
			m->material->set_parameters(m);
			m->material->use(cmdbuf);
			m->draw(cmdbuf);
		}
	}
	Vulkan::Instance->endSwapChainRenderPass(cmdbuf);
	Vulkan::Instance->endFrame();
}
