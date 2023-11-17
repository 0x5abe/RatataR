cmake_minimum_required(VERSION 3.0.0)
project(RatataR VERSION 1.0.0)

add_library(hook SHARED hook.cpp)
add_executable(RatataR WIN32 RatataR.cpp RatataR.rc)
target_link_libraries(hook Winmm)

set_target_properties(RatataR PROPERTIES OUTPUT_NAME "RatataR")