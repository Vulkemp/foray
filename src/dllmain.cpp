#include <sdl2/SDL.h>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include <vkbootstrap/VkBootstrap.h>

int test()
{
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Quit();
    uint32_t devicecount;
    vkb::InstanceBuilder builder;
    builder.build();
    spdlog::info("ayy");
    VkInstance ptr;
    vkCreateInstance(nullptr, nullptr, &ptr);
    return 0;
}