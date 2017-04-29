cmake_minimum_required(VERSION 3.5)
project(sieci1)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CLIENT err.h client.c utils.h)
add_executable(client ${CLIENT})

set(SERVER err.h server.c utils.h)
add_executable(server ${SERVER})

