
set(COMMON_LIB common)

add_library(common STATIC)

target_sources(${COMMON_LIB} PRIVATE
        win_defs.h
        config.h
        config.cpp
        logger.h
        logger.cpp
        byte_buffer.h
        frame_codec.h
        hash.h
        strong_id.h
        string_utils.h
        number_utils.h
        serialization.h
        message_types.h
        message_utils.h
        stdin_reader.h
        serialization_vector.h
        serialization_game_state.h
        serialization_utils.h
        keyboard_reader.h
        keyboard_reader.cpp
        game_state_dto.h
        serialization_game_state.cpp
        terminal_guard.h
)

target_include_directories(${COMMON_LIB} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Cbor for serialization
find_package(libcbor REQUIRED)

target_link_libraries(${COMMON_LIB} PUBLIC
        platform_deps
        boost_deps
        libcbor::libcbor
)

# X11 for KeyboardReader on Linux
if (NOT WIN32)
    find_package(X11 REQUIRED)
    target_link_libraries(${COMMON_LIB} PUBLIC
            X11::X11
    )
endif ()

if (BUILD_TESTS)
    target_compile_definitions(${COMMON_LIB} PRIVATE BUILD_TESTS)
endif ()
