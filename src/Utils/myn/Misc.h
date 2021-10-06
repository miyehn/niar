#pragma once
#include <string>
#include <vector>

namespace myn
{
	std::string lower(const std::string& s);
	std::vector<char> read_file(const std::string& filename);
}// namespace myn