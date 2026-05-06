/**
 * @file  ConsoleCapture.hpp
 * @brief Defines test helpers for capturing standard console streams.
 */

#pragma once

#include <ios>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>

namespace Engine::Tests
{
/// @brief Selects which standard console stream buffers should be captured.
enum class ConsoleCaptureMode
{
    Cout,
    Cerr,
    CoutAndCerr
};

/// @brief Redirects selected standard console streams into a string for a scoped test block.
/// @details The original stream buffers are restored when capture finishes or the scope ends.
class ConsoleCaptureScope
{
  public:
    /// @brief Starts capturing the selected console streams.
    /// @param[out] output The string that receives captured output when capture finishes.
    /// @param[in] mode The console streams to capture.
    ConsoleCaptureScope(std::string &output, ConsoleCaptureMode mode) :
        m_Output(output), m_Mode(mode), m_CoutBuffer(std::cout.rdbuf()), m_CerrBuffer(std::cerr.rdbuf())
    {
        m_Output.clear();

        if (capturesCout())
            std::cout.rdbuf(m_Stream.rdbuf());

        if (capturesCerr())
            std::cerr.rdbuf(m_Stream.rdbuf());
    }

    /// @brief Restores captured streams and writes the captured text to the output string.
    ~ConsoleCaptureScope() { finish(); }

    ConsoleCaptureScope(ConsoleCaptureScope const &) = delete;
    ConsoleCaptureScope &operator=(ConsoleCaptureScope const &) = delete;

    /// @brief Checks whether the scope is still actively capturing streams.
    /// @return True when capture is active; otherwise false.
    bool isCapturing() const { return m_Capturing; }

    /// @brief Stops capture, restores original stream buffers, and stores captured text.
    void finish()
    {
        if (!m_Capturing)
            return;

        if (capturesCout())
            std::cout.rdbuf(m_CoutBuffer);

        if (capturesCerr())
            std::cerr.rdbuf(m_CerrBuffer);

        m_Output = m_Stream.str();
        m_Capturing = false;
    }

  private:
    /// @brief Checks whether stdout should be redirected.
    /// @return True when the current capture mode includes std::cout; otherwise false.
    bool capturesCout() const
    {
        return m_Mode == ConsoleCaptureMode::Cout || m_Mode == ConsoleCaptureMode::CoutAndCerr;
    }

    /// @brief Checks whether stderr should be redirected.
    /// @return True when the current capture mode includes std::cerr; otherwise false.
    bool capturesCerr() const
    {
        return m_Mode == ConsoleCaptureMode::Cerr || m_Mode == ConsoleCaptureMode::CoutAndCerr;
    }

    std::string &m_Output;
    ConsoleCaptureMode m_Mode;
    std::ostringstream m_Stream;
    std::streambuf *m_CoutBuffer = nullptr;
    std::streambuf *m_CerrBuffer = nullptr;
    bool m_Capturing = true;
};
} // namespace Engine::Tests

#define GAMEENGINE_CONCAT_IMPL(left, right) left##right
#define GAMEENGINE_CONCAT(left, right) GAMEENGINE_CONCAT_IMPL(left, right)

#define GAMEENGINE_CAPTURE_CONSOLE_IMPL(output, mode, variable)                                      \
    for (Engine::Tests::ConsoleCaptureScope variable(output, mode); variable.isCapturing();          \
         variable.finish())

#define CAPTURE_CONSOLE(output)                                                                     \
    GAMEENGINE_CAPTURE_CONSOLE_IMPL(output, Engine::Tests::ConsoleCaptureMode::CoutAndCerr,         \
                                    GAMEENGINE_CONCAT(gameengineConsoleCapture, __LINE__))

#define CAPTURE_CONSOLE_COUT(output)                                                                \
    GAMEENGINE_CAPTURE_CONSOLE_IMPL(output, Engine::Tests::ConsoleCaptureMode::Cout,                \
                                    GAMEENGINE_CONCAT(gameengineConsoleCapture, __LINE__))

#define CAPTURE_CONSOLE_CERR(output)                                                                \
    GAMEENGINE_CAPTURE_CONSOLE_IMPL(output, Engine::Tests::ConsoleCaptureMode::Cerr,                \
                                    GAMEENGINE_CONCAT(gameengineConsoleCapture, __LINE__))
