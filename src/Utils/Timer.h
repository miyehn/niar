#pragma once

#include <chrono>

namespace myn
{
	typedef std::chrono::time_point<std::chrono::high_resolution_clock> TimePoint;

#ifdef _WIN32
#define TIMER_BEGIN() auto __start_time = std::chrono::high_resolution_clock::now();

#define TIMER_END() \
	auto __end_time = std::chrono::high_resolution_clock::now(); \
	std::chrono::duration<double, std::milli> __time = __end_time - __start_time; \
	double execution_time = __time.count() * 1e-6;

#else // APPLE
	#include <sys/time.h>
#define TIMER_BEGIN() \
	struct timeval __start_time, __end_time; \
	gettimeofday(&__start_time, 0);

#define TIMER_END() \
	gettimeofday(&__end_time, 0); \
	long __sec = __end_time.tv_sec - __start_time.tv_sec; \
	long __msec = __end_time.tv_usec - __start_time.tv_usec; \
	double execution_time = __sec + __msec * 1e-6;

#endif

}// namespace myn
