
add_library(client_app OBJECT)

set(CLIENT_APP_LIB client_app)

target_sources(${CLIENT_APP_LIB} PRIVATE
        client_controller.h
        client_controller.cpp
)

target_include_directories(${CLIENT_APP_LIB} PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
)

target_link_libraries(${CLIENT_APP_LIB} PUBLIC
        client_network
        client_ui
)
