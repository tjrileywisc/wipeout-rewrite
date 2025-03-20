
add_executable(
    server
    main.c
    client.c
)

set(OS_LIBS "")
if(WIN32)
    set(OS_LIBS "Ws2_32.lib;${OS_LIBS}")
endif()

target_link_libraries(
    server PRIVATE
    ${OS_LIBS}
    network
)