#pragma once

#include <vector>
#include <mutex>

namespace myn
{
	//--------------- thread-safe queue -----------------------
	template <typename T>
	struct ThreadSafeQueue {

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

} // namespace myn

