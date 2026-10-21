cmake_minimum_required(VERSION 3.14)
project(RockPaperScissors CXX)

# Force C++17 as required by these frameworks
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# 1. Fetch the latest FTXUI (v7.0.1)
FetchContent_Declare(
  ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
  GIT_TAG        v7.0.1  # 👈 Updated to the latest stable major release
)

# 2. Fetch the latest nlohmann/json (v3.12.0)
FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json
  GIT_TAG        v3.12.0  # 👈 Updated to the latest stable version
)

# Make both frameworks available to your project
FetchContent_MakeAvailable(ftxui json)

# 3. Define your executable program
add_executable(my_program ftxui.cpp)

# 4. Link BOTH libraries to your program
target_link_libraries(my_program PRIVATE 
    ftxui::screen 
    ftxui::dom 
    ftxui::component
    nlohmann_json::nlohmann_json
)
