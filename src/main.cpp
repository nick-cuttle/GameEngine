/**
 * @file  main.cpp
 * @brief Entry point for the GameEngine.
 *
 * Handles initialization and main loop execution.
 */

#include <Engine/Runtime/Context.hpp>
#include <cstdlib>

/// @brief  Main entry point for the application.
/// @return EXIT_SUCCESS on successful execution, otherwise EXIT_FAILURE.
int main()
{
    // // create engine and set global context pointer
    Engine::Context engine;

    engine.initialize();

    engine.logger.root()->warn("Test Warning Message!");
    engine.logger.root()->info("Test Info Message!");
    engine.logger.root()->debug("Test Debug Message!");
    engine.logger.root()->critical("Test Critical Message!");
    engine.logger.root()->error("Test Error Message!");
    engine.logger.root()->trace("Test Trace Message!");

    return EXIT_SUCCESS;
}