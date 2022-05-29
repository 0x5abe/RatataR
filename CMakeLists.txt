cmake_minimum_required(VERSION 3.0.0)
project(taR VERSION 1.0.0)

add_library(hook SHARED hook.cpp)
add_executable(taR WIN32 taR.cpp taR.rc)

set_target_properties(taR PROPERTIES OUTPUT_NAME "taR")