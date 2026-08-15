
add_subdirectory(network)
add_subdirectory(ui)
add_subdirectory(app)

add_executable(dungeons_client
        main.cpp
)

target_include_directories(dungeons_client PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
)

target_link_libraries(dungeons_client PRIVATE
        client_network
        client_ui
        client_app
)
