#include "WindowSystem.hpp"
#include <SDL3/SDL.h>
#include <stdexcept>
#include <unordered_map>

namespace Engine
{

namespace
{
std::unordered_map<unsigned int, SDL_Window *> windows;
} // namespace

struct WindowSystem::Implementation
{
    bool isInitialized = false;
    std::unordered_map<unsigned int, SDL_Window *> windowByIdentifier;
};

WindowSystem::WindowSystem() : implementation(std::make_unique<Implementation>()) {}

WindowSystem::~WindowSystem() { shutdown(); }

void WindowSystem::initialize()
{
    if (implementation->isInitialized)
    {
        return;
    }

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        // TODO: Confirm whether or not this needs to use our custom logging system
        throw std::runtime_error(SDL_GetError());
    }

    implementation->isInitialized = true;
}

void WindowSystem::shutdown()
{
    if (!implementation->isInitialized)
    {
        return;
    }

    for (auto const &[windowIdentifier, window] : implementation->windowByIdentifier)
    {
        SDL_DestroyWindow(window);
    }

    implementation->windowByIdentifier.clear();

    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    implementation->isInitialized = false;
}

} // namespace Engine
