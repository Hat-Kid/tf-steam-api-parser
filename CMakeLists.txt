cmake_minimum_required(VERSION 3.0.0)
project(tf-steam-api-parser VERSION 0.1.0)
SET(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()
add_executable(tf-steam-api-parser main.cpp main.h)
target_link_libraries(tf-steam-api-parser ${CONAN_LIBS})