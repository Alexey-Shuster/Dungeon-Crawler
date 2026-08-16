
add_library(common_wire OBJECT)

set(COMMON_WIRE_LIB common_wire)

target_sources(${COMMON_WIRE_LIB} PRIVATE
        message_utils.h
        serder_base.h
        serder_base.cpp
        serder.h
        serder.cpp
        serder_vector_base.h
        serder_vector.h
        serder_game_state.h
        serder_game_state.cpp
)

target_include_directories(${COMMON_WIRE_LIB} PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
)

# for serialization
find_package(libcbor REQUIRED)

target_link_libraries(${COMMON_WIRE_LIB} PUBLIC
        libcbor::libcbor
        common_network
        common_types
        common_utility
)

