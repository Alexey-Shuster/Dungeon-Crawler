
set(COMMON_SOURCES
        config.h
        config.cpp
        events.h
        frame_codec.h
        hash.h
        logger.h
        logger.cpp
        serialization.h
        types.h
        message.h
        message_types.h
        message_utils.h
        message_dispatcher.h
        stdin_reader.h
        event_type.h
        direction.h
        string_utils.h
        serialization_vector.h
        serialization_game_state.h
        serialization_utils.h
        keyboard_reader.h
        keyboard_reader.cpp
        number_utils.h
        serialization_game_state.cpp
)

add_library(podzemeliya_common STATIC
        ${COMMON_SOURCES}
)

target_include_directories(podzemeliya_common PUBLIC
        ${PROJECT_ROOT}/src/common
)

# Cbor for serialization
find_package(libcbor REQUIRED)

target_link_libraries(podzemeliya_common PUBLIC
        platform_deps
        boost_deps
        libcbor::libcbor
)

# need for KeyboardReader on Linux
if (NOT WIN32)
    find_package(X11 REQUIRED)
    target_link_libraries(podzemeliya_common PUBLIC
            X11::X11
    )
endif ()
