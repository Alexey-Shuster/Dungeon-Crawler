
add_library(server_app STATIC)

set(SERVER_APP_LIB server_app)

target_sources(${SERVER_APP_LIB} PRIVATE
        events.h
        session_registry_fwd.h
        session_registry.h
        session_registry.cpp
        connection_manager.h
        connection_manager.cpp
        msg_router.h
        msg_router.cpp
        msg_dispatcher_dtl.h
        msg_dispatcher.h
        msg_dispatcher.cpp
        response_sender.h
        response_sender.cpp
        game_manager.h
        game_manager.cpp
)

target_include_directories(${SERVER_APP_LIB} PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(${SERVER_APP_LIB} PUBLIC
        common
        server_core
        server_domain
        server_network
)
