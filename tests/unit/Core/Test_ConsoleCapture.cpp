/**
 * @file  Test_ConsoleCapture.cpp
 * @brief Tests the ConsoleCaptureScope test helper for capturing standard console streams.
 */

#include <ConsoleCapture.hpp>

#include <catch2/catch_test_macros.hpp>

#include <iostream>
#include <string>

TEST_CASE("ConsoleCapture", "[unit][core][console]")
{
    SECTION("captures cout and cerr")
    {
        std::string output;

        CAPTURE_CONSOLE(output)
        {
            std::cout << "cout text";
            std::cerr << " and cerr text";
        }

        REQUIRE(output == "cout text and cerr text");
    }

    SECTION("can capture only cout")
    {
        std::string output;

        CAPTURE_CONSOLE_COUT(output)
        {
            std::cout << "cout text";
            std::cerr << "uncaptured cerr text";
        }

        REQUIRE(output == "cout text");
    }

    SECTION("can capture only cerr")
    {
        std::string output;

        CAPTURE_CONSOLE_CERR(output)
        {
            std::cout << "uncaptured cout text";
            std::cerr << "cerr text";
        }

        REQUIRE(output == "cerr text");
    }

    SECTION("restores streams after capture")
    {
        std::string firstOutput;
        std::string secondOutput;

        CAPTURE_CONSOLE(firstOutput)
        {
            std::cout << "first";
        }

        CAPTURE_CONSOLE(secondOutput)
        {
            std::cout << "second";
        }

        REQUIRE(firstOutput == "first");
        REQUIRE(secondOutput == "second");
    }

    SECTION("executes the capture body once")
    {
        std::string output;
        auto runCount = 0;

        CAPTURE_CONSOLE(output)
        {
            ++runCount;
            std::cout << "captured";
        }

        REQUIRE(runCount == 1);
        REQUIRE(output == "captured");
    }
}
