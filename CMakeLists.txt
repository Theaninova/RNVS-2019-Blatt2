cmake_minimum_required(VERSION 3.13.2)
project(Blatt2)

set(CMAKE_CXX_STANDARD 99)

add_executable(client
        client.c)
add_executable(server
        server.c
        quotes.txt)
