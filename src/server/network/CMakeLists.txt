
set(SERVER_NETWORK_LIB server_network)

add_library(${SERVER_NETWORK_LIB} STATIC)

target_sources(${SERVER_NETWORK_LIB} PRIVATE
        types.h
        events.h
        raw_message.h
        message_utils.h
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
