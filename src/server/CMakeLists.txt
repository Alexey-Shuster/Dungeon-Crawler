
set(SERVER_SOURCES
        connection_manager.h
        connection_manager.cpp
        message_router.h
        message_router.cpp
        response_sender.h
        response_sender.cpp
        session.h
        session.cpp
        session_registry.h
        session_registry.cpp
        tcp_server.h
        tcp_server.cpp
)

add_library(podzemeliya_server STATIC
        ${SERVER_SOURCES}
)

target_include_directories(podzemeliya_server PUBLIC
        ${PROJECT_ROOT}/src/server
        ${PROJECT_ROOT}/src/common
        ${PROJECT_ROOT}/src/app
        ${PROJECT_ROOT}/src/infra
)

target_link_libraries(podzemeliya_server PUBLIC
        podzemeliya_common
        podzemeliya_infra
        podzemeliya_app
)

add_executable(dungeons_server
        main.cpp
)

target_link_libraries(dungeons_server PRIVATE
        podzemeliya_server
)
