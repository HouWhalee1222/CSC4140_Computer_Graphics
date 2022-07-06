cmake_minimum_required(VERSION 2.8)

project(CSC_4140_Assignment2)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(OpenCV REQUIRED)
find_package(Eigen3 REQUIRED NO_MODULE)

include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${EIGEN3_INCLUDE_DIRS})

add_executable(asg2 main.cpp rasterizer.hpp rasterizer.cpp Triangle.hpp Triangle.cpp)

target_link_libraries(asg2 Eigen3::Eigen ${OpenCV_LIBS})
