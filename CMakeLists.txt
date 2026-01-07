cmake_minimum_required(VERSION 3.16)
project(RatataR VERSION 1.0.0)

add_library(hook SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hook.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rich-presence/rpc-client/src/discord_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rich-presence/rpc-client/src/levels.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rich-presence/rpc-client/src/rpc.cpp
)

target_link_libraries(hook
    Winmm
    d3d9
    ${CMAKE_CURRENT_SOURCE_DIR}/external/MinHook/lib/libMinHook.x86.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/rich-presence/discord-rpc/lib/discord-rpc.lib
)

add_custom_command(
    TARGET hook
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/rich-presence/discord-rpc/bin/discord-rpc.dll
            $<TARGET_FILE_DIR:hook>
)

target_include_directories(hook PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/    
    ${CMAKE_CURRENT_SOURCE_DIR}/rich-presence/rpc-client/include
    ${CMAKE_CURRENT_SOURCE_DIR}/rich-presence/discord-rpc/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/MinHook/include
)

add_executable(RatataR WIN32
    ${CMAKE_CURRENT_SOURCE_DIR}/src/RatataR.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/RatataR.rc
)

target_link_libraries(RatataR
    Winmm
    kernel32
    user32
)

set_target_properties(RatataR PROPERTIES OUTPUT_NAME "RatataR")