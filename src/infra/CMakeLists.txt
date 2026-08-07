
set(INFRA_SOURCES
        eventbus.h
        # temporary solution for STATIC
        dummy.cpp
        game_loop.h
)

add_library(podzemeliya_infra STATIC
        ${INFRA_SOURCES}
)

target_include_directories(podzemeliya_infra PUBLIC
        ${PROJECT_ROOT}/src/infra
)

target_link_libraries(podzemeliya_infra PUBLIC
        podzemeliya_common
)
