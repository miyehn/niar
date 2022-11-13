//
// Created by miyehn on 11/10/2022.
//

#pragma once
#include <string>
#include <glm/glm.hpp>
#include <vector>
#include <functional>

namespace myn
{
	class ShaderSimulator {
	public:
		ShaderSimulator(int width, int height, const std::string &filename);

		virtual void runSim();

	protected:

		void dispatchShader(const std::function<glm::vec4(uint32_t, uint32_t)> &kernel);

		void storeTexel(int x, int y, const glm::vec4& col);

		void writeFile();
		void openFile();

		int width;
		int height;
		std::string filename;
		std::vector<glm::vec4> buffer;
	};

	namespace sky {
		class SkyAtmosphereSim : ShaderSimulator {
		public:
			SkyAtmosphereSim(int width, int height, const std::string &filename)
			: ShaderSimulator(width, height, filename) {}
			void runSim() override;
		};
	};
}

