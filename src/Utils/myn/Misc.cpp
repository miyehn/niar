#include "Misc.h"
#include <locale>

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
} // namespace myn
