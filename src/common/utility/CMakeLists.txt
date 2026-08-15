
add_library(common_utility STATIC)

set(COMMON_UTILITY_LIB common_utility)

target_sources(${COMMON_UTILITY_LIB} PRIVATE
        win_defs.h
        number_utils.h
        string_utils.h
        hash.h
        config.h
        config.cpp
        logger.h
        logger.cpp
        terminal_guard.h
        stdin_reader.h
        keyboard_reader.h
        keyboard_reader.cpp
)

target_include_directories(${COMMON_UTILITY_LIB} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(${COMMON_UTILITY_LIB} PUBLIC
        common_deps
)

# for KeyboardReader on Linux
if (NOT WIN32)
    find_package(X11 REQUIRED)
    target_link_libraries(${COMMON_UTILITY_LIB} PUBLIC
            X11::X11
    )
endif ()

# for config-testing
if (BUILD_TESTS)
    target_compile_definitions(${COMMON_UTILITY_LIB} PRIVATE BUILD_TESTS)
endif ()
