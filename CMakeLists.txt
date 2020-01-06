cmake_minimum_required(VERSION 3.12)
project (checker)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Process Abseil's CMake build system
add_subdirectory(abseil-cpp)

add_executable(checker checker.cpp)
target_link_libraries(checker absl::hash)


