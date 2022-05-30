#pragma once

#include <chrono>
#ifdef MACOS
#include <ctime>
#endif

namespace myn
{
	typedef std::chrono::time_point<std::chrono::high_resolution_clock> TimePoint;

#define TIMER_BEGIN auto __start_time = std::chrono::high_resolution_clock::now();

#define TIMER_END(out_var_name) \
	auto __end_time = std::chrono::high_resolution_clock::now(); \
	std::chrono::duration<double> __time = __end_time - __start_time; \
	double (out_var_name) = __time.count();
	//std::chrono::duration<double, std::milli> __time = __end_time - __start_time;

}// namespace myn
