
add_library(common_wire STATIC)

set(COMMON_WIRE_LIB common_wire)

target_sources(${COMMON_WIRE_LIB} PRIVATE
        message_utils.h
        serialization_base.h
        serialization.h
        serder_vector_base.h
        serder_vector.h
        serialization_game_state.h
        serialization_game_state.cpp
)

target_include_directories(${COMMON_WIRE_LIB} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# for serialization
find_package(libcbor REQUIRED)

target_link_libraries(${COMMON_WIRE_LIB} PUBLIC
        libcbor::libcbor
        common_network
        common_types
        common_utility
)

