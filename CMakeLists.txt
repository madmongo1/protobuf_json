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

protobuf_configure_files (FILES test.proto CPP CPP_HEADERS proto_hdrs CPP_SOURCES proto_srcs)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_executable(protobuf_json main.cpp ${proto_hdrs} ${proto_srcs})
target_link_libraries(protobuf_json sanity::protobuf)

