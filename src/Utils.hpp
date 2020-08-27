#pragma once
#include "lib.h"

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
