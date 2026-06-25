#pragma once
#include <string>
#include <unordered_map>
#include "Render/BindlessResources.h"
#include "Render/Vulkan/Vulkan.hpp"
#include "Render/Vulkan/ImageCreator.h"

struct ImageFormat {
	int numChannels;
	int channelDepth;
	int SRGB;
};

/*

Let textures go bindless:

use a global texture registry that keeps account of all textures that can possibly be sampled: binding 0 for 2D textures, other bindings for 3D textures, cubemaps, etc.
such textures would include all material textures and possibly also rendertargets

struct BindlessHandle: stores resource type, index, version/generation

struct BindlessTable
- responsible for actually updating the bindless resources on the gpu
- maintain a record that keeps track of which array indices are taken, which ones are free.
- having a fixed max texture count is fine for now (1024?)
- every time a texture is added, add it to an available descriptor array index.
- every time a texture is removed, waitDeviceIdle(), and mark its descriptor array index as free.
- API would basically include adding texture(s) and removing texture(s).

Texture class should still maintain its name -> Texture* mapping (through texturePool) for now while bindless is in development. Hopefully this mapping will eventually get removed
Texture class (and subclasses) would also still expose access to VkImageView, VmaAllocatedImage etc.
- Texture class (and/or its subclass) should be responsible for bindless registration/unregistration. Register combined image + caller-given sampler pair. Ignore glTF samplers for now
- but let registration be optional and default to false. Only register bindless if it's told to, through constructor argument.

SceneAsset, EnvironmentMapAsset, or any other asset that contains textures
- when loaded, would add the texture(s) to BindlessTable
- when unloaded, would remove the texture(s) from BindlessTable, which internally does waitDeviceIdle and marks the slot(s) as free
- they transitively "own" bindless registrations

On program exit, because all resources are unloaded (and unregisters their textures), BindlessTable should eventually become empty.

if trying to get a nonexistent bindless texture, return one of the global defaults instead (probably just _black for now)

In the shader would need a few macros like
#define SAMPLE_BINDLESS_TEX2D(index, uv) ...
#define SAMPLE_BINDLESS_TEX3D(index, uv) ...
#define SAMPLE_BINDLESS_CUBE(index, uv) ...
...

Places that store texture references by name can continue to do so for now (GltfMaterialInfo, etc.)

Buffers can probably go bindless in a similar fashion too, but for later.

*/

/*
 * Pooled textures: textures loaded from file / gltf asset;
 */
class Texture
{
public:
	template<typename T>
	static T* get(const std::string &path) {
		auto it = texturePool.find(path);
		if (it != texturePool.end()) return dynamic_cast<T*>((*it).second);

		WARN("retrieving texture '%s' before it is added to the pool. Returning black dummy..", path.c_str())
		return dynamic_cast<T*>(texturePool.find("_black")->second);
	}
	virtual ~Texture();

	VmaAllocatedImage resource;

protected:
	Texture() = default;
	static std::unordered_map<std::string, Texture *> texturePool;
};

class Texture2D : public Texture
{
public:

	VkImageView imageView;
	VkFormat imageFormat;

	// load from file (POOLED)
	explicit Texture2D(
		const std::string &name,
		const std::string &path,
		ImageFormat textureFormat={4,8,1},
		const BindlessTexture2DInfo& bindlessInfo = {});

	// create with code but provide pixel data (ie. gltf) (POOLED)
	explicit Texture2D(
		const std::string &name,
		uint8_t* data,
		uint32_t width,
		uint32_t height,
		ImageFormat format,
		bool generateMips = true,
		const BindlessTexture2DInfo& bindlessInfo = {}
		);

	// allocate programmatically (NOT POOLED)
	explicit Texture2D(
		ImageCreator &imageCreator,
		const BindlessTexture2DInfo& bindlessInfo = {});

	uint32_t getWidth() const { return width; }
	uint32_t getHeight() const { return height; }
	BindlessTexture2DHandle getBindlessHandle() const { return bindlessHandle; }

	~Texture2D() override;

	static void createDefaultTextures(); // (POOLED)
	static void cleanupDefaultTextures();

private:
	uint32_t width;
	uint32_t height;

	BindlessTexture2DHandle bindlessHandle;

	void registerBindless(const BindlessTexture2DInfo& bindlessInfo);
	void unregisterBindless();

#if TMP_BINDLESS_DEBUG
	static void runBindlessLifetimeSelfTest();
#endif
};
