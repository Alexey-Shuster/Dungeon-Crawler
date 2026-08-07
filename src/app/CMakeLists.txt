
set(APP_SOURCES
        lobby_registry.h
        lobby_registry.cpp
        lobby_manager.h
        lobby_manager.cpp
        game_manager.h
        game_manager.cpp
)

add_library(podzemeliya_app STATIC
        ${APP_SOURCES}
)

target_include_directories(podzemeliya_app PUBLIC
        ${PROJECT_ROOT}/src/app
)

target_link_libraries(podzemeliya_app PUBLIC
        podzemeliya_common
        podzemeliya_domain
)
