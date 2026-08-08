
# Platform-specific link dependencies

find_package(Threads REQUIRED)

add_library(platform_deps INTERFACE)

target_link_libraries(platform_deps INTERFACE Threads::Threads)

if (WIN32)
    target_compile_definitions(platform_deps INTERFACE
            WIN32_LEAN_AND_MEAN
            NOMINMAX
            # TODO: conan rebuild with os.version=10
            #            _WIN32_WINNT=0x0A00
    )

    target_link_libraries(platform_deps INTERFACE ws2_32)

    if (MINGW)
        target_link_libraries(platform_deps INTERFACE mswsock)
    endif ()

    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_definitions(platform_deps INTERFACE _WIN32_WINNT=0x0A00)
        target_link_libraries(platform_deps INTERFACE synchronization OneCore)
    endif ()

    if (MINGW AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_link_libraries(platform_deps INTERFACE pthread)
        target_link_libraries(platform_deps INTERFACE atomic)
    endif ()
endif ()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_link_libraries(platform_deps INTERFACE atomic)
endif ()
