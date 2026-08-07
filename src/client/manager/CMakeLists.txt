
add_executable(dungeons_client_manager
        main.cpp
        client_manager.h
        client_manager.cpp
)

target_link_libraries(dungeons_client_manager PRIVATE
        podzemeliya_client
)
