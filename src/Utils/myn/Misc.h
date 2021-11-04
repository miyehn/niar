#pragma once
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

	// TODO: make more robust
	glm::quat quat_from_dir(glm::vec3 dir);

	std::string s3(glm::vec3 v);

}// namespace myn