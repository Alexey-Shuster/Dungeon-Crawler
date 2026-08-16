
# Gtest
find_package(GTest CONFIG REQUIRED)
include(GoogleTest)
include(CTest)

add_library(tests_base INTERFACE)

# нужно каждому тесту
target_link_libraries(tests_base INTERFACE
        GTest::gtest_main
)

# Отключение предупреждений nodiscard
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(tests_base INTERFACE -Wno-unused-result)
elseif (MSVC)
    target_compile_options(tests_base INTERFACE /wd4834)
endif ()

function(add_test_target TARGET_NAME INIT_SRC)
    get_filename_component(INIT_SRC_NAME "${INIT_SRC}" NAME)

    if (NOT "${INIT_SRC_NAME}" STREQUAL "tests_init_config_logger.cpp")
        message(FATAL_ERROR "add_test_target: Main source for '${TARGET_NAME}' must be exactly 'tests_init_config_logger.cpp'. Given: '${INIT_SRC}'")
    endif ()

    # ${ARGN} автоматически собирает все аргументы, переданные после TARGET_NAME
    add_executable(${TARGET_NAME} ${INIT_SRC} ${ARGN})

    target_link_libraries(${TARGET_NAME} PRIVATE tests_base)

    gtest_discover_tests(${TARGET_NAME})
endfunction()

add_test_target(test_client
        tests_init_config_logger.cpp
        client/tests_client.cpp
        client/tests_keyboard_controller.cpp
        client/tests_render_game_state.cpp
)

target_link_libraries(test_client PRIVATE
        client_network
        client_ui
        client_app
)

add_test_target(test_common
        tests_init_config_logger.cpp
        common/tests_config.cpp
        common/tests_frame_codec.cpp
        common/tests_hash.cpp
        common/tests_direction.cpp
        common/tests_serder.cpp
        common/tests_serder_dto.cpp
        common/tests_keyboard_reader.cpp
)

target_link_libraries(test_common PRIVATE
        common
)

if (BUILD_TESTS)
    target_compile_definitions(test_common PRIVATE BUILD_TESTS)
endif ()

add_test_target(test_server
        tests_init_config_logger.cpp
        server/tests_eventbus.cpp
        server/tests_session.cpp
        server/tests_session_registry.cpp
        server/tests_connection_manager.cpp
        server/tests_response_sender.cpp
        server/tests_message_dispatcher.cpp
        server/tests_lobby.cpp
        server/tests_lobby_registry.cpp
        server/tests_position.cpp
        server/tests_dungeon.cpp
        server/tests_entity_manager.cpp
        server/tests_game_manager.cpp
)

target_link_libraries(test_server PRIVATE
        server_app
)
