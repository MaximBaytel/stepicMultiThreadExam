cmake_minimum_required(VERSION 2.8)
project(final)

add_definitions(-D__DAEMON__)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")

set(SRC_LIST ./src/main.cpp  ./src/httpheader.cpp ./src/httphandler.cpp)

add_executable(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} pthread ev)
