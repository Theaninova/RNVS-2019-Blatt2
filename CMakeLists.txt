cmake_minimum_required(VERSION 3.15)
project(Blatt2)

set(CMAKE_CXX_STANDARD 99)

add_executable(Blatt2
        client.c
        CMakeLists.txt
        quotes.txt
        server.c)
