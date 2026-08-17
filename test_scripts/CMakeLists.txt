
# region GTEST_SETUP
find_package(GTest CONFIG REQUIRED)
include(GoogleTest)
include(CTest)
# endregion

# region TESTS_BASE_CONFIGURATION
add_library(tests_base INTERFACE)

target_link_libraries(tests_base INTERFACE
        GTest::gtest_main
)

# Отключение предупреждений nodiscard
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(tests_base INTERFACE -Wno-unused-result)
elseif (MSVC)
    target_compile_options(tests_base INTERFACE /wd4834)
endif ()
# endregion

# region TEST_TARGETS
add_executable(test_client
        tests_init_config_logger.cpp
        client/tests_client.cpp
        client/tests_keyboard_controller.cpp
        client/tests_render_game_state.cpp
)

target_link_libraries(test_client PRIVATE
        tests_base
        client_app
)
gtest_discover_tests(test_client)

add_executable(test_common
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
        tests_base
        common
)
gtest_discover_tests(test_common)

if (BUILD_TESTS)
    target_compile_definitions(test_common PRIVATE BUILD_TESTS)
endif ()

add_executable(test_server
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
        server/tests_collision_check.cpp
        server/tests_game_map.cpp
        server/tests_monster_control.cpp
)

target_link_libraries(test_server PRIVATE
        tests_base
        server_app
)
gtest_discover_tests(test_server)
# endregion
