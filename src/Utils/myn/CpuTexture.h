//
// Created by miyehn on 11/13/2022.
//

#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>

namespace myn {

class CpuTexture {
public:
	CpuTexture(int width, int height);

	void storeTexel(int x, int y, const glm::vec4 &col);

	glm::vec4 loadTexel(int x, int y);

	void writeFile(const std::string &filename, bool gammaCorrect, bool openFile) const;

	int getWidth() const { return width; }

	int getHeight() const { return height; }

private:
	int width;
	int height;
	std::vector<glm::vec4> buffer;
};

}
