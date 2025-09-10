cmake_minimum_required(VERSION 3.14)
project(square_map CXX)

# Generate compile commands for VSCode
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Set debug flags for better debugging experience
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Enable testing
enable_testing()

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(GTest REQUIRED)
find_package(benchmark REQUIRED)

# Set policy CMP0167 to NEW to suppress FindBoost warning
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()
find_package(Boost REQUIRED COMPONENTS container)

# Explicitly set Boost include directories if not set by find_package
if(NOT Boost_INCLUDE_DIRS)
    set(Boost_INCLUDE_DIRS "/usr/local/include")
endif()

message(STATUS "Found GTest: ${GTEST_INCLUDE_DIRS}")
message(STATUS "Found benchmark: ${benchmark_INCLUDE_DIRS}")
message(STATUS "Found Boost: ${Boost_INCLUDE_DIRS}")

# Now add the square_map test
add_executable(square_map_test square_map_test.cpp)
target_link_libraries(square_map_test PRIVATE GTest::GTest GTest::Main)
target_include_directories(square_map_test PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GTEST_INCLUDE_DIRS}
)

# Add the test
add_test(NAME square_map_test COMMAND square_map_test)

# Add merge_with_binary_search test
add_executable(merge_with_binary_search_test merge_with_binary_search_test.cpp)
target_link_libraries(merge_with_binary_search_test PRIVATE GTest::GTest GTest::Main)
target_include_directories(merge_with_binary_search_test PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GTEST_INCLUDE_DIRS}
)

# Add the test
add_test(NAME merge_with_binary_search_test COMMAND merge_with_binary_search_test)

# Add benchmark executable
add_executable(square_map_bm square_map_bm.cpp)
target_link_libraries(square_map_bm PRIVATE 
    benchmark::benchmark 
    Boost::container
)
target_include_directories(square_map_bm PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
)

# Add complexity test executable
add_executable(complexity_test complexity_test.cpp)
target_link_libraries(complexity_test PRIVATE 
    Boost::container
)
target_include_directories(complexity_test PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
)

# Add the complexity test
add_test(NAME complexity_test COMMAND complexity_test)
