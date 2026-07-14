set(MINHOOK_SOURCES
    src/buffer.c
    src/hook.c
    src/trampoline.c
)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    list(APPEND MINHOOK_SOURCES src/hde/hde64.c)
else()
    list(APPEND MINHOOK_SOURCES src/hde/hde32.c)
endif()

add_library(MinHook STATIC ${MINHOOK_SOURCES})

target_include_directories(MinHook
    PUBLIC include
    PRIVATE src src/hde
)
