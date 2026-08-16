
add_library(common_types INTERFACE)

set(COMMON_TYPES_LIB common_types)

target_sources(${COMMON_TYPES_LIB} PRIVATE
        direction.h
        message_types.h
        message_utils.h
        strong_id.h
)

target_include_directories(${COMMON_TYPES_LIB} INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
)
