
set(SERVER_APP_LIB server_app)

add_library(${SERVER_APP_LIB} STATIC)

target_sources(${SERVER_APP_LIB} PRIVATE
        events.h
        session_registry.h
        session_registry.cpp
        connection_manager.h
        connection_manager.cpp
        message_router.h
        message_router.cpp
        message_dispatcher.h
        response_sender.h
        response_sender.cpp
        game_manager.h
        game_manager.cpp
)

target_include_directories(${SERVER_APP_LIB} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(${SERVER_APP_LIB} PUBLIC
        server_core
        server_domain
        server_network
)
