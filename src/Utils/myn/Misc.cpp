#include "Misc.h"
#include <fstream>
#include "Log.h"

namespace myn
{
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

} // namespace myn
