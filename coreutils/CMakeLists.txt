cmake_minimum_required(VERSION 3.18)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_STANDARD 20)

project(coreutils LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(range-v3 GIT_REPOSITORY https://github.com/ericniebler/range-v3)
FetchContent_MakeAvailable(range-v3)
FetchContent_Declare(cxxopts GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git)
FetchContent_MakeAvailable(cxxopts)

add_executable(hexdump hexdump.cpp)
target_link_libraries(hexdump PRIVATE range-v3 cxxopts)
