
add_library(common_network INTERFACE)

set(COMMON_NETWORK_LIB common_network)

target_sources(${COMMON_NETWORK_LIB} PRIVATE
        byte_buffer.h
        raw_message.h
        frame_codec.h
        game_state_dto.h
)

target_include_directories(${COMMON_NETWORK_LIB} INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
)
