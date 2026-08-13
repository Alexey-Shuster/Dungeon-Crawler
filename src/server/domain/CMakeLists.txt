
set(SERVER_DOMAIN_LIB server_domain)

add_library(${SERVER_DOMAIN_LIB} STATIC)

target_sources(${SERVER_DOMAIN_LIB} PRIVATE
        event_base.h
        events.h
        types.h
        dungeon/collision_check.h
        dungeon/command_queue.h
        dungeon/direction_gen.h
        dungeon/dungeon.h
        dungeon/dungeon.cpp
        dungeon/dungeon_registry.h
        dungeon/dungeon_registry.cpp
        dungeon/entity_manager.h
        dungeon/monster_control.h
        dungeon/position_gen.h
        dungeon/spawner.h
        dungeon/dungeon_state.h
        dungeon/create_game_state.h
        dungeon/create_game_state.cpp
        entity/entity.h
        entity/entity_health.h
        entity/entity_types.h
        lobby/lobby.h
        lobby/lobby.cpp
        lobby/lobby_registry.h
        lobby/lobby_registry.cpp
        lobby/lobby_manager.h
        lobby/lobby_manager.cpp
        map/position.h
        map/map_size.h
        map/game_map.h
        map/game_map.cpp
)

target_include_directories(${SERVER_DOMAIN_LIB} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(${SERVER_DOMAIN_LIB} PUBLIC
        common
        server_core
)
