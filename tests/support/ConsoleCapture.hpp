#pragma once

#include <ios>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>

namespace Engine::Tests
{
enum class ConsoleCaptureMode
{
    Cout,
    Cerr,
    CoutAndCerr
};

class ConsoleCaptureScope
{
  public:
    ConsoleCaptureScope(std::string &output, ConsoleCaptureMode mode) :
        m_Output(output), m_Mode(mode), m_CoutBuffer(std::cout.rdbuf()), m_CerrBuffer(std::cerr.rdbuf())
    {
        m_Output.clear();

        if (capturesCout())
            std::cout.rdbuf(m_Stream.rdbuf());

        if (capturesCerr())
            std::cerr.rdbuf(m_Stream.rdbuf());
    }

    ~ConsoleCaptureScope() { finish(); }

    ConsoleCaptureScope(ConsoleCaptureScope const &) = delete;
    ConsoleCaptureScope &operator=(ConsoleCaptureScope const &) = delete;

    bool isCapturing() const { return m_Capturing; }

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
    bool capturesCout() const
    {
        return m_Mode == ConsoleCaptureMode::Cout || m_Mode == ConsoleCaptureMode::CoutAndCerr;
    }

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
