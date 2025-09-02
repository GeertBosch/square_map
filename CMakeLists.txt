cmake_minimum_required(VERSION 3.14)
project(square_map CXX)

# Generate compile commands for VSCode
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

# Add benchmark executable
add_executable(square_map_bm square_map_bm.cpp)
target_link_libraries(square_map_bm PRIVATE 
    benchmark::benchmark 
    Boost::container
    Boost::headers
)
target_include_directories(square_map_bm PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Add complexity test executable
add_executable(complexity_test complexity_test.cpp)
target_include_directories(complexity_test PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)
