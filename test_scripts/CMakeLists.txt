
# Gtest
find_package(GTest CONFIG REQUIRED)
include(GoogleTest)
include(CTest)

add_library(tests_base INTERFACE)

# Только то, что нужно каждому тесту
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

add_test_target(test_app
        tests_init_config_logger.cpp
        tests_lobby_registry.cpp
        tests_game_manager.cpp
)

target_link_libraries(test_app PRIVATE
        podzemeliya_app
)

add_test_target(test_client
        tests_init_config_logger.cpp
        tests_client.cpp
        tests_keyboard_controller.cpp
)

target_link_libraries(test_client PRIVATE
        podzemeliya_client
)

add_test_target(test_common
        tests_init_config_logger.cpp
        tests_config.cpp
        tests_frame_codec.cpp
        tests_hash.cpp
        tests_serialization.cpp
        tests_serialization_dto.cpp
        tests_keyboard_reader.cpp
)

target_link_libraries(test_common PRIVATE
        podzemeliya_common
)

if (BUILD_TESTS)
    target_compile_definitions(podzemeliya_common PRIVATE BUILD_TESTS)
    target_compile_definitions(test_common PRIVATE BUILD_TESTS)
endif ()

add_test_target(test_domain
        tests_init_config_logger.cpp
        tests_lobby.cpp
        tests_position.cpp
        tests_direction.cpp
        tests_dungeon.cpp
        tests_render_game_state.cpp
)

target_link_libraries(test_domain PRIVATE
        podzemeliya_domain
)

add_test_target(test_infra
        tests_init_config_logger.cpp
        tests_eventbus.cpp
)

target_link_libraries(test_infra PRIVATE
        podzemeliya_infra
)

add_test_target(test_server
        tests_init_config_logger.cpp
        tests_session.cpp
        tests_session_registry.cpp
        tests_connection_manager.cpp
        tests_response_sender.cpp
)

target_link_libraries(test_server PRIVATE
        podzemeliya_server
)
