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
class TestTempDirectory
{
  public:
    explicit TestTempDirectory(std::string const &prefix) : m_Path(makeUniquePath(prefix))
    {
        std::filesystem::create_directories(m_Path);
    }

    ~TestTempDirectory()
    {
        std::error_code error;
        std::filesystem::remove_all(m_Path, error);
    }

    TestTempDirectory(TestTempDirectory const &) = delete;
    TestTempDirectory &operator=(TestTempDirectory const &) = delete;

    std::filesystem::path const &path() const { return m_Path; }

  private:
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
