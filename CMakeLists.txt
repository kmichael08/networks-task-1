cmake_minimum_required(VERSION 3.5)
project(sieci1)

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -O2")
set(CLIENT err.h utils.h client.cc )
add_executable(client ${CLIENT})

set(SERVER err.h utils.h server.cc)
add_executable(server ${SERVER})

