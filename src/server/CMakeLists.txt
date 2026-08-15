
add_subdirectory(core)
add_subdirectory(app)
add_subdirectory(domain)
add_subdirectory(network)

add_executable(dungeons_server
        main.cpp
)

target_include_directories(dungeons_server PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(dungeons_server PRIVATE
        server_app
)
