cmake_minimum_required(VERSION 3.1)
project(IpcMessaging)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++")

set(SOURCE_FILES main.cpp)

add_executable(IpcMessaging_Demo
        ${SOURCE_FILES}
        typedefs.h
        messagebus.h
        deserializer.h
        stream_processor.h)
