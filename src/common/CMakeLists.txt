
add_subdirectory(network)
add_subdirectory(types)
add_subdirectory(utility)
add_subdirectory(wire)

add_library(common_deps INTERFACE)

target_link_libraries(common_deps INTERFACE
        platform_deps
        boost_deps
)

add_library(common STATIC)

target_include_directories(common PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
)

target_link_libraries(common PUBLIC
        common_deps
        common_network
        common_types
        common_utility
        common_wire
)
