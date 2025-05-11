

add_executable(tests
    main.c
    test_network.c
    test_server.c
)

set(MOCK_WRAP_FUNCS
    recvfrom
)

set(MOCK_LINKER_FLAGS "")
foreach(FUNC IN LISTS MOCK_WRAP_FUNCS)
    list(APPEND MOCK_LINKER_FLAGS "-Wl,--wrap=${FUNC}")
endforeach()

target_link_options(tests PRIVATE ${MOCK_LINKER_FLAGS})

target_link_libraries(tests PRIVATE
    cmocka::cmocka
    network
    serverlib
)