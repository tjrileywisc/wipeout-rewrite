

add_executable(tests
    main.c
    test_name_gen.c
    test_network.c
    test_server.c
    test_xbr.c
    ${PROJECT_SOURCE_DIR}/src/xbr.c

# mocks and test utilities
    network_wrapper.c
    utils.c
)

target_include_directories(tests PRIVATE ${PROJECT_SOURCE_DIR}/src)

target_link_libraries(tests PRIVATE
    cmocka::cmocka
    network
    serverlib
)