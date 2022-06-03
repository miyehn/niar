#pragma once

#include <iostream>

// for showing last relative_path node, see: https://stackoverflow.com/questions/8487986/file-macro-shows-full-path
#ifdef WINOS
#define PATH_ELIM_SLASH '\\'
#endif
#ifdef MACOS
#define PATH_ELIM_SLASH '/'
#endif
#define __FILENAME__ (strrchr(__FILE__, PATH_ELIM_SLASH) ? strrchr(__FILE__, PATH_ELIM_SLASH) + 1 : __FILE__)

// colors
// for color formatting, see: https://stackoverflow.com/questions/2616906/how-do-i-output-coloured-text-to-a-linux-terminal
#define COLOR_RED printf("\033[31m");
#define COLOR_GREEN printf("\033[32m");
#define COLOR_YELLOW printf("\033[33m");
#define COLOR_BLUE printf("\033[34m");
#define COLOR_MAGENTA printf("\033[35m");
#define COLOR_CYAN printf("\033[36m");

#define COLOR_RESET printf("\033[0m");

// helpful component(s)
#define LOCATION printf("%s: %d", __FILENAME__, __LINE__)

// break (if using sdl)
#ifdef WINOS
#include <SDL2/SDL_assert.h>
#define DEBUG_BREAK __debugbreak();
#else
#define DEBUG_BREAK ;
#endif

// logging

#ifdef WINOS
#define NEWLINE { printf("\n"); fflush(stdout); }
#endif

#ifdef MACOS
#define NEWLINE printf("\n");
#endif

#define LOG(...) { \
	COLOR_GREEN \
	printf("["); LOCATION; printf("] "); \
	COLOR_RESET \
	printf(__VA_ARGS__); \
	NEWLINE \
}

#define LOGR(...) { \
	printf(__VA_ARGS__); \
	NEWLINE \
}

#define WARN(...) { \
	COLOR_YELLOW \
	printf("["); LOCATION; printf("] "); \
	COLOR_RESET \
	printf(__VA_ARGS__); \
	NEWLINE \
}

#define ERR(...) { \
	COLOR_RED \
	printf("["); LOCATION; printf("] "); \
	COLOR_RESET \
	printf(__VA_ARGS__); \
	NEWLINE \
	DEBUG_BREAK \
}

// pathtracer
#define TRACE(...) { \
	COLOR_MAGENTA \
	printf("[Pathtracer] "); \
	printf(__VA_ARGS__); \
	COLOR_RESET \
	NEWLINE \
}

// blue (assets)
#define ASSET(...) { \
	COLOR_BLUE \
	printf("[Asset] "); \
	printf(__VA_ARGS__); \
	COLOR_RESET \
	NEWLINE \
}

#define VKLOG(...) { \
	COLOR_CYAN \
	printf("[Vulkan validation] "); \
	printf(__VA_ARGS__); \
	COLOR_RESET \
	NEWLINE \
}
#define VKWARN(...) { \
	COLOR_YELLOW \
	printf("[Vulkan validation] "); \
	printf(__VA_ARGS__); \
	COLOR_RESET \
	NEWLINE \
}
#define VKERR(...) { \
	COLOR_RED \
	printf("[Vulkan validation] "); \
	printf(__VA_ARGS__); \
	COLOR_RESET \
	NEWLINE \
	DEBUG_BREAK \
}

// assertions

#if DEBUG

#define EXPECT_M(STATEMENT, EXPECTED, ...) { \
	if (STATEMENT != EXPECTED) { \
		COLOR_RED \
		printf("["); LOCATION; printf("]"); \
		printf("[Assertion failed] "); \
		printf(__VA_ARGS__); \
		COLOR_RESET \
		NEWLINE \
		DEBUG_BREAK \
	} \
}

#define EXPECT(STATEMENT, EXPECTED) EXPECT_M(STATEMENT, EXPECTED, "")

#define ASSERT_M(STATEMENT, ...) EXPECT_M(STATEMENT, true, __VA_ARGS__)

#define ASSERT(STATEMENT) EXPECT(STATEMENT, true)

#else

#define EXPECT_M(STATEMENT, EXPECTED, ...) STATEMENT;
#define EXPECT(STATEMENT, EXPECTED) STATEMENT;
#define ASSERT_M(STATEMENT) ;
#define ASSERT(STATEMENT) ;

#endif
