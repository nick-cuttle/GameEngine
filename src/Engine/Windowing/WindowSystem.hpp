#pragma once

#include <memory>
#include <string_view>
#include <vector>

namespace Engine
{
struct WindowSize
{
    int width;
    int height;
};

struct WindowIdentifier
{
    unsigned int value;
};

enum class WindowEventType
{
    CloseRequest
};

struct WindowEvent
{
    WindowEventType type;
    WindowIdentifier windowIdentifier;
};

class WindowSystem
{
  public:
    WindowSystem();
    ~WindowSystem();

    WindowSystem(WindowSystem const &) = delete;
    WindowSystem &operator=(WindowSystem const &) = delete;

    void initialize();
    void shutdown();

    WindowIdentifier createPrimaryWindow(std::string_view title, WindowSize size);
    std::vector<WindowEvent> pollWindowEvents();

  private:
    struct Implementation;
    std::unique_ptr<Implementation> implementation;
};
} // namespace Engine
