#pragma once
#include "lib.h"
#include <unordered_map>

//--------------- thread-safe queue -----------------------
template <typename T>
struct TaskQueue {
	
	size_t size() {
		std::lock_guard<std::mutex> lock(queue_mutex);
		return queue.size();
	}

	bool dequeue(T& out_task) {
		std::lock_guard<std::mutex> lock(queue_mutex);
		if (queue.size() == 0) return false;
		out_task = queue.back();
		queue.pop_back();
		return true;
	}

	void enqueue(T task) {
		std::lock_guard<std::mutex> lock(queue_mutex);
		queue.insert(queue.begin(), task);
	}

	void clear() {
		std::lock_guard<std::mutex> lock(queue_mutex);
		queue.clear();
	}

private:
	std::vector<T> queue;
	std::mutex queue_mutex;
};
//---------------------------------------------------------

struct AABB {
	AABB() {
		min = vec3(INF);
		max = vec3(-INF);
	}
	void add_point(vec3 p);
	void merge(const AABB& other);
	static AABB merge(const AABB& A, const AABB& B);
	std::vector<vec3> corners();
	std::string str() { return "min: " + s3(min) + ", max: " + s3(max); }

	vec3 min, max;
};

/* use Texture.Get to get a texture. Returns one if it's already there, load and add to pool if not
 * has private constructor
 * is passed around by ptr
 * stores related info (width, height, format, etc.) as private member, has public getters
 * keeps a pool of textures, has a static cleanup function that releases it (gets called on program exit)
 */
struct Texture {
	
	static Texture* Checker;
	static Texture* get(const std::string& path);
	static void cleanup();

	uint id() { return id_value; }
	int width() { return width_value; }
	int height() { return height_value; }
	int num_channels() { return num_channels_value; }

private:
	uint id_value = 0;
	int width_value = 0;
	int height_value = 0;
	int num_channels_value = 0;

	static std::unordered_map<std::string, Texture*> texture_pool;
	
};

unsigned char* load_image(const std::string& path, int& width, int& height, int& nChannels);
