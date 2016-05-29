cmake_minimum_required (VERSION 3.5)

if ("${CMAKE_CURRENT_LIST_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
	message (FATAL_ERROR "no in-source builds please")
endif ()

project (test_compiler CXX)
include (CheckCXXCompilerFlag)
set (CMAKE_CXX_STANDARD 14)

include (sanity/cmake/sanity.cmake)

project (protobuf_json CXX)

sanity_require(LIBRARY protobuf VERSION any)
sanity_require(LIBRARY boost VERSION any)

set(PROTO_FILES test.proto)
protobuf_configure_files (FILES ${PROTO_FILES} CPP CPP_HEADERS proto_hdrs CPP_SOURCES proto_srcs)
set_property(SOURCE ${PROTO_FILES} PROPERTY HEADER_FILE_ONLY ON)

set(SRCS
main.cpp
my_resolver.cpp my_resolver.hpp)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_executable(protobuf_json ${SRCS} ${PROTO_FILES} ${proto_hdrs} ${proto_srcs})
target_link_libraries(protobuf_json sanity::protobuf boost)

