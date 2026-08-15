
add_library(server_network STATIC)

set(SERVER_NETWORK_LIB server_network)

target_sources(${SERVER_NETWORK_LIB} PRIVATE
        types.h
        events.h
        session_event.h
        tcp_server.h
        tcp_server.cpp
        session_fwd.h
        session.h
        session.cpp
)

target_include_directories(${SERVER_NETWORK_LIB} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(${SERVER_NETWORK_LIB} PUBLIC
        common
        server_core
)
