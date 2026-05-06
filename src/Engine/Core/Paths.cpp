/**
 * @file  Paths.cpp
 * @brief Implements the Paths class, which manages file paths for the engine.
 */

#include "Paths.hpp"

#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__)
#include <limits.h>
#include <unistd.h>
#endif

namespace Engine
{

void Paths::init(Config const &config)
{
    if (!config.baseOverride.empty())
        m_Base = config.baseOverride;
    else
        m_Base = getExecutableDirectory();

    m_Logs = m_Base / "logs";
    m_Assets = m_Base / "assets";

    std::filesystem::create_directories(m_Logs);
}

std::filesystem::path const &Paths::base() const { return m_Base; }
std::filesystem::path const &Paths::logs() const { return m_Logs; }
std::filesystem::path const &Paths::assets() const { return m_Assets; }

std::filesystem::path Paths::getExecutableDirectory() const
{
#ifdef _WIN32
    char buffer[MAX_PATH];
    GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path();

#elif defined(__linux__)
    char buffer[PATH_MAX];

    ssize_t const len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len <= 0)
        return std::filesystem::current_path(); // fallback safety

    buffer[len] = '\0';

    return std::filesystem::path(buffer).parent_path();

#else
    return std::filesystem::current_path();
#endif
}

} // namespace Engine
