/**
 * @file  main.cpp
 * @brief Entry point for the GameEngine.
 *
 * Handles initialization and main loop execution.
 */

#include <Engine/Runtime/Context.hpp>
#include <cstdlib>
#include <iostream>

/// @brief  Main entry point for the application.
/// @return EXIT_SUCCESS on successful execution, otherwise EXIT_FAILURE.
int main()
{
    Engine::Context engine;

    engine.Init();

    std::cout << "Hello, World!" << std::endl;
    return EXIT_SUCCESS;
}
