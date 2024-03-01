cmake_minimum_required(VERSION 3.18)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_STANDARD 20)

project(main LANGUAGES CXX)

add_executable(main main.cpp)

include(FetchContent)
FetchContent_Declare(range-v3 GIT_REPOSITORY https://github.com/ericniebler/range-v3)
FetchContent_MakeAvailable(range-v3)
target_link_libraries(main PRIVATE range-v3)
FetchContent_Declare(cxxopts GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git)
FetchContent_MakeAvailable(cxxopts)
target_link_libraries(main PRIVATE cxxopts)
