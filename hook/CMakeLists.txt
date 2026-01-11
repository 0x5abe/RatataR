add_library(hook SHARED
	"${CMAKE_CURRENT_SOURCE_DIR}/src/hook.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/config_handler.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/rpc-client/discord_manager.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/rpc-client/levels.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/rpc-client/rpc.cpp"
)

target_link_libraries(hook PRIVATE
	Winmm
	d3d9
	delayimp
	MinHook
	discord-rpc
)

target_include_directories(hook PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/rpc-client"
)

target_link_options(hook PRIVATE "/DELAYLOAD:discord-rpc.dll")

add_custom_command(
	TARGET hook
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
			${CMAKE_SOURCE_DIR}/external/discord-rpc/bin/discord-rpc.dll
			$<TARGET_FILE_DIR:hook>
)

set_target_properties(hook PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${RATATAR_OUTPUT_DIR}"
)