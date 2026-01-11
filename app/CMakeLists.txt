add_executable(RatataR WIN32
	"${CMAKE_CURRENT_SOURCE_DIR}/src/RatataR.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/resources/RatataR.rc"
)

target_link_libraries(RatataR
	Winmm
	kernel32
	user32
)

set_target_properties(RatataR PROPERTIES
	OUTPUT_NAME "RatataR"
    RUNTIME_OUTPUT_DIRECTORY "${RATATAR_OUTPUT_DIR}"
)