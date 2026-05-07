/**
 * @file  TestTempDirectory.hpp
 * @brief Defines a test harness helper for isolated temporary directories.
 */

#pragma once

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <sstream>
#include <string>
#include <thread>

namespace Engine::Tests
{

/// @brief Creates and owns a unique temporary directory for a test scope.
/// @details The directory is created under the repository's tests/tmp directory and removed on
/// destruction.
class TestTempDirectory
{
public:
    /// @brief Creates a unique temporary directory using the supplied name prefix.
    /// @param[in] prefix The readable prefix to include in the generated directory name.
    explicit TestTempDirectory(std::string const &prefix) : m_Path(makeUniquePath(prefix))
    {
        std::filesystem::create_directories(m_Path);
    }

    /// @brief Removes the temporary directory and all contents created during the test.
    ~TestTempDirectory()
    {
        std::error_code error;
        std::filesystem::remove_all(m_Path, error);
    }

    TestTempDirectory(TestTempDirectory const &) = delete;
    TestTempDirectory &operator=(TestTempDirectory const &) = delete;

    /// @brief Gets the owned temporary directory path.
    /// @return The absolute path to the temporary directory.
    std::filesystem::path const &path() const
    {
        return m_Path;
    }

private:
    /// @brief Builds a unique path for a temporary test directory.
    /// @param[in] prefix The readable prefix to include in the generated directory name.
    /// @return The generated path under the repository's tests/tmp directory.
    static std::filesystem::path makeUniquePath(std::string const &prefix)
    {
        auto const now = std::chrono::steady_clock::now().time_since_epoch().count();
        auto const threadId = std::hash<std::thread::id>{}(std::this_thread::get_id());

        std::ostringstream directoryName;
        directoryName << prefix << "-" << now << "-" << threadId;

        return std::filesystem::path(GAMEENGINE_TEST_REPO_ROOT) / "tests" / "tmp" /
               directoryName.str();
    }

    std::filesystem::path m_Path;
};
} // namespace Engine::Tests
