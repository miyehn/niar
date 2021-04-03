#pragma once

#include <iostream>

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


// logging
#define LOG(...) { \
    COLOR_GREEN \
    printf("["); LOCATION; printf("] "); \
    COLOR_RESET \
    printf(__VA_ARGS__); \
    printf("\n"); \
}

#define LOGR(...) { \
    printf(__VA_ARGS__); \
    printf("\n"); \
}

#define WARN(...) { \
    COLOR_YELLOW \
    printf("["); LOCATION; printf("] "); \
    COLOR_RESET \
    printf(__VA_ARGS__); \
    printf("\n"); \
}

#define ERR(...) { \
    COLOR_RED \
    printf("["); LOCATION; printf("] "); \
    COLOR_RESET \
    printf(__VA_ARGS__); \
    printf("\n"); \
}

// pathtracer
#define TRACE(...) { \
    COLOR_MAGENTA \
    printf("[Pathtracer] "); \
    printf(__VA_ARGS__); \
    COLOR_RESET \
    printf("\n"); \
}

#define VKLOG(...) { \
    COLOR_CYAN \
    printf("[Vulkan validation] "); \
    printf(__VA_ARGS__); \
    COLOR_RESET \
    printf("\n"); \
}
#define VKWARN(...) { \
    COLOR_YELLOW \
    printf("[Vulkan validation] "); \
    printf(__VA_ARGS__); \
    COLOR_RESET \
    printf("\n"); \
}
#define VKERR(...) { \
    COLOR_RED \
    printf("[Vulkan validation] "); \
    printf(__VA_ARGS__); \
    COLOR_RESET \
    printf("\n"); \
}