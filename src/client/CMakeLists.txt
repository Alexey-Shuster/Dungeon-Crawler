
set(CLIENT_SOURCES
        client.h
        client.cpp
        client_controller.h
        client_controller.cpp
        command_router.h
        command_router.cpp
        message_router.h
        message_router.cpp
        console_output.h
        client_commands.h
        keyboard_controller.h
        keyboard_controller.cpp
)

add_library(podzemeliya_client STATIC
        ${CLIENT_SOURCES}
)

target_include_directories(podzemeliya_client PUBLIC
        ${PROJECT_ROOT}/src/client
)

target_link_libraries(podzemeliya_client PUBLIC
        podzemeliya_common
        podzemeliya_domain
)

add_executable(dungeons_client
        main.cpp
)

target_link_libraries(dungeons_client PRIVATE
        podzemeliya_client
)

add_subdirectory(manager)
