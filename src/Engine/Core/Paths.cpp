/**
 * @file  Paths.cpp
 * @brief Implements the Paths class, which manages file paths for the engine.
 *
 *
 *
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

void Paths::Init(Config const &config)
{
    if (!config.BaseOverride.empty())
        m_Base = config.BaseOverride;
    else
        m_Base = GetExecutableDirectory();

    m_Logs = m_Base / "logs";
    m_Assets = m_Base / "assets";

    std::filesystem::create_directories(m_Logs);
}

std::filesystem::path const &Paths::Base() const { return m_Base; }
std::filesystem::path const &Paths::Logs() const { return m_Logs; }
std::filesystem::path const &Paths::Assets() const { return m_Assets; }

std::filesystem::path Paths::GetExecutableDirectory() const
{
#ifdef _WIN32
    char buffer[MAX_PATH];
    GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path();

#elif defined(__linux__)
    char buffer[PATH_MAX];

    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len <= 0)
        return std::filesystem::current_path(); // fallback safety

    buffer[len] = '\0';

    return std::filesystem::path(buffer).parent_path();

#else
    return std::filesystem::current_path();
#endif
}

} // namespace Engine