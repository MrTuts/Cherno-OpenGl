cmake_minimum_required(VERSION 3.16...4.3)
project(glm)

add_library(glm INTERFACE)


target_include_directories(glm INTERFACE include "${CMAKE_CURRENT_SOURCE_DIR}/")
