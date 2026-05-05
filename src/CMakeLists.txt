# -------------------------
# Engine library
# -------------------------

set(ENGINE_SOURCES
    Engine/Core/Paths.cpp
    Engine/Runtime/Context.cpp
)

# Make a library so external projects can link to it if needed.
# The main executable will also link to this.
add_library(Engine STATIC ${ENGINE_SOURCES})

target_include_directories(Engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Set compiler warnings for the Engine library.
target_compile_options(Engine PRIVATE
    -Wall
    -Werror
)

# create executable
add_executable(GameEngine main.cpp)

# link main executable to Engine library
target_link_libraries(GameEngine PRIVATE Engine)