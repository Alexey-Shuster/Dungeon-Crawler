
add_library(client_network OBJECT)

set(CLIENT_NETWORK_LIB client_network)

target_sources(${CLIENT_NETWORK_LIB} PRIVATE
        client_fwd.h
        client.h
        client.cpp
)

target_include_directories(${CLIENT_NETWORK_LIB} PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
)

target_link_libraries(${CLIENT_NETWORK_LIB} PUBLIC
        common
)
