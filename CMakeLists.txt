cmake_minimum_required(VERSION 3.10)

project(v8pp)

add_library(v8pp STATIC
        v8pp/class.cpp
        v8pp/context.cpp
        v8pp/convert.cpp
        v8pp/json.cpp
        v8pp/throw_ex.cpp)
        
target_include_directories(v8pp PUBLIC .)