
add_library(client_ui OBJECT)

set(CLIENT_UI_LIB client_ui)

target_sources(${CLIENT_UI_LIB} PRIVATE
        client_commands.h
        console_output.h
        command_router.h
        command_router.cpp
        render_game_state.h
        render_game_state.cpp
        keyboard_controller.h
        keyboard_controller.cpp
        message_router.h
        message_router.cpp
)

target_include_directories(${CLIENT_UI_LIB} PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
)

target_link_libraries(${CLIENT_UI_LIB} PUBLIC
        common
)
