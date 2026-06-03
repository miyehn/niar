#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#define INF std::numeric_limits<float>::infinity()
#define EPSILON 0.001f
#define PI 3.14159265359f
#define HALF_PI 1.57079632679f
#define ONE_OVER_PI 0.31830988618f
#define TWO_PI 6.28318530718f
#define ONE_OVER_TWO_PI 0.15915494309f

namespace myn
{
	std::string lower(const std::string& s);

	std::vector<char> read_file(const std::string& filename);

	inline time_t get_file_clock_now() {
		auto tp = std::chrono::system_clock::now();
		return std::chrono::system_clock::to_time_t(tp);
	}

	inline time_t get_file_last_write_time(const std::string& path) {
		auto file_time = std::filesystem::last_write_time(path);
		auto system_time = std::chrono::clock_cast<std::chrono::system_clock>(file_time);
		return std::chrono::system_clock::to_time_t(system_time);
	}

	template <typename T>
	T aligned_size(T alignment, T in_size) { return (in_size + (alignment - 1)) & ~(alignment - 1); }

	// TODO: make more robust
	glm::quat quat_from_dir(glm::vec3 dir);

	std::string s3(glm::vec3 v);

	glm::vec3 transform_point(const glm::mat4 &mat, const glm::vec3 &vec);

}// namespace myn