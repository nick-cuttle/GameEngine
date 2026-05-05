/**
 * @file  Paths.hpp
 * @brief Defines the Paths class, which manages file paths for the engine.
 *
 *
 *
 */

#pragma once

#include <filesystem>

namespace Engine
{
class Paths
{
  public:
    /// @brief Configuration structure for initializing paths.
    struct Config
    {
        std::filesystem::path BaseOverride; // base path for which other paths derive from. If
                                            // empty, will use executable directory.
    };

    /// @brief      Initializes the paths.
    /// @param[in]  config Configuration for initializing paths.
    void Init(Config const &config = {});

    /// @brief Gets the Base Path
    /// @return The base path for the engine, which is the directory of the executable or an
    /// override if provided.
    std::filesystem::path const &Base() const;

    /// @brief    Returns the Logs directory path.
    /// @details  This is where log files will be stored. The directory is created if it does not
    ///           exist. Currently set to "<Base>/logs".
    /// @return   The path to the Logs directory.
    std::filesystem::path const &Logs() const;

    /// @brief    Returns the Assets directory path.
    /// @details  This is where game assets (like textures, models, etc.) will be stored. Currently
    ///           set to "<Base>/assets".
    /// @return   The path to the Assets directory.
    std::filesystem::path const &Assets() const;

  private:
    /// @brief Retrieves the directory of the currently running executable.
    /// @return Directory path of the executable.
    std::filesystem::path GetExecutableDirectory() const;

  private:
    std::filesystem::path m_Base;
    std::filesystem::path m_Logs;
    std::filesystem::path m_Assets;
};
} // namespace Engine