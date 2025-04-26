
file(GLOB PROTO_SRCS ${CMAKE_CURRENT_LIST_DIR}/*.c)

add_library(protos
    ${PROTO_SRCS}
)

target_link_libraries(protos PUBLIC PkgConfig::libprotobuf-c)

target_include_directories(protos PUBLIC ${CMAKE_CURRENT_LIST_DIR})