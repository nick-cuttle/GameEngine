#include "WindowSystem.hpp"
#include <SDL3/SDL.h>
#include <stdexcept>
#include <unordered_map>

namespace Engine
{
namespace
{
std::unordered_map<unsigned int, SDL_Window *> windows;
bool isVideoInitialized = false;
} // namespace
} // namespace Engine
