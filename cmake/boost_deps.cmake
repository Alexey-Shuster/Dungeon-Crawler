
# Boost-specific link dependencies

find_package(Boost CONFIG REQUIRED COMPONENTS
        system
        thread
        json
        log
)

add_library(boost_deps INTERFACE)

target_link_libraries(boost_deps INTERFACE
        Boost::system
        Boost::thread
        Boost::json
        Boost::log
)
