
add_library(network
    msg.c
    network.c
)

set(OS_LIBS "")
if(WIN32)
    set(OS_LIBS "Ws2_32.lib;${OS_LIBS}")
endif()

target_link_libraries(network PUBLIC ${OS_LIBS})

target_include_directories(network PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})