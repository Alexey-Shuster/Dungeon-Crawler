set(DOMAIN_SOURCES
        lobby.h
        lobby.cpp
        map/direction.h
        map/position.h
        map/game_map.h
        map/game_map.cpp
        dungeon/dungeon.h
        dungeon/dungeon.cpp
        game/dungeon_registry.h
        game/dungeon_registry.cpp
        game_state_dto.h
        entity/entity_health.h
        entity/entity_types.h
        create_game_state.h
        render_game_state.h
        create_game_state.cpp
        render_game_state.cpp
)

add_library(podzemeliya_domain STATIC
        ${DOMAIN_SOURCES}
)

target_include_directories(podzemeliya_domain PUBLIC
        ${PROJECT_ROOT}/src/domain
)

target_link_libraries(podzemeliya_domain PUBLIC
        podzemeliya_common
)
