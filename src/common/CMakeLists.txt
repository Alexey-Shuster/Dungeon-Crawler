
add_library(common STATIC
        message_utils.h)

target_sources(common PRIVATE
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

target_include_directories(common PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Cbor for serialization
find_package(libcbor REQUIRED)

target_link_libraries(common PUBLIC
        platform_deps
        boost_deps
        libcbor::libcbor
)

# X11 for KeyboardReader on Linux
if (NOT WIN32)
    find_package(X11 REQUIRED)
    target_link_libraries(common PUBLIC
            X11::X11
    )
endif ()
