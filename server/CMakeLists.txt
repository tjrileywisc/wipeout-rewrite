
cmake_minimum_required(VERSION 3.28 FATAL_ERROR)

project(server)

add_executable(
    server
    main.c
    network.c
)

if(WIN32)
    target_link_libraries(
        server PRIVATE
        Ws2_32.lib
    )
endif()