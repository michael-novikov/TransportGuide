cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_ENABLE_EXPORTS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls -O0")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")

set(this_project transport_guide)

set(utility ~/workspace/cpp-brown-belt/utility)
include_directories(${utility})

project(${this_project} CXX)

find_package(Protobuf REQUIRED)

include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

protobuf_generate_cpp(
    PROTO_SRCS
    PROTO_HDRS
    bus.proto
    stop.proto
    transport_catalog.proto
)

set(headers
  bus.h
  stop.h
  transport_manager.h
  transport_manager_command.h
  json.h
  json_api.h
  graph.h
  router.h
  svg.h
  projector_interface.h
  scanline_projection.h
  scanline_compressed_projection.h
  map_builder.h
  )

set(sources
  bus.cpp
  stop.cpp
  transport_manager.cpp
  json.cpp
  json_api.cpp
  svg.cpp
  scanline_projection.cpp
  scanline_compressed_projection.cpp
  map_builder.cpp
  main.cpp
  )

add_executable(${this_project} ${sources} ${headers} ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(${this_project} ${Protobuf_LIBRARIES})

