cmake_minimum_required(VERSION 3.16)

project(cpp_visual LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV REQUIRED)

add_library(modules
    StatsRecorder.cpp
    Visualizer.cpp
)

target_include_directories(modules
    PUBLIC
        ${OpenCV_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(modules
    PUBLIC
        ${OpenCV_LIBS}
)

add_executable(test_stats_recorder test_main.cpp)
target_link_libraries(test_stats_recorder PRIVATE modules)

add_executable(test_visualizer test_visualizer.cpp)
target_link_libraries(test_visualizer PRIVATE modules)

add_executable(demo_visualizer demo_visualizer.cpp)
target_link_libraries(demo_visualizer PRIVATE modules)

enable_testing()
add_test(NAME stats_recorder COMMAND test_stats_recorder)
add_test(NAME visualizer COMMAND test_visualizer)
