
add_library(server_core STATIC)

set(SERVER_CORE_LIB server_core)

target_sources(${SERVER_CORE_LIB} PRIVATE
        # temporary solution for STATIC
        dummy.cpp
        event_base.h
        event_type.h
        events.h
        event_bus.h
        game_loop.h
)

target_include_directories(${SERVER_CORE_LIB} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(${SERVER_CORE_LIB} PUBLIC
        common
)
