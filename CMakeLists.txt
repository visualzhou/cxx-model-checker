cmake_minimum_required(VERSION 3.10)
project (checker)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# set(CMAKE_BUILD_TYPE RelWithDebInfo)

# Process Abseil's CMake build system
add_subdirectory(abseil-cpp)

add_executable(die_hard_checker die_hard_checker.cpp)
target_link_libraries(die_hard_checker absl::hash)

add_executable(mongo_raft_checker mongo_raft_checker.cpp)
target_link_libraries(mongo_raft_checker absl::hash)
