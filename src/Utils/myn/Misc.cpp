#include "Misc.h"
#include <fstream>
#include "Log.h"

namespace myn
{
using namespace glm;

	std::string lower(const std::string& s) {
		std::string res = "";
		std::locale loc;
		for(int i=0; i<s.length(); i++) {
#ifdef WIN32
			res += std::tolower(s[i]);
#else
			res += std::tolower(s[i], loc);
#endif
		}
		return res;
	}

	std::vector<char> read_file(const std::string& filename) {
		// read binary file from the end
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		EXPECT_M(file.is_open(), true, "failed to open file %s", filename.c_str())
		// get file size from position
		size_t fileSize = (size_t) file.tellg();
		std::vector<char> buffer(fileSize);
		// seek to 0
		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();
		return buffer;
	}

	quat quat_from_dir(vec3 dir) {
		if (dot(dir, vec3(0, 0, -1)) > 1.0f - EPSILON) {
			return quat();
		}
		float angle = acos(dot(dir, vec3(0, 0, -1)));
		vec3 axis = normalize(cross(vec3(0, 0, -1), dir));

		float c = cos(angle / 2);
		float s = sin(angle / 2);

		return quat(c, s*axis.x, s*axis.y, s*axis.z);
	}

	std::string s3(vec3 v) {
		return "(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", "  + std::to_string(v.z) + ")";
	}


} // namespace myn
