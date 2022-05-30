#pragma once

namespace myn
{
	struct Color
	{
		float r = 0.0f;
		float g = 0.0f;
		float b = 0.0f;
		float a = 0.0f;
	};
	static_assert(sizeof(Color) == sizeof(float) * 4);
}
