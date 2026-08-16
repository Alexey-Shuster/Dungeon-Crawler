
add_library(server_core INTERFACE)

set(SERVER_CORE_LIB server_core)

target_sources(${SERVER_CORE_LIB} PRIVATE
        event_base.h
        event_type.h
        events.h
        event_bus.h
        game_loop.h
)

target_include_directories(${SERVER_CORE_LIB} INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
)

target_link_libraries(${SERVER_CORE_LIB} INTERFACE
        common
)
