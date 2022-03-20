cmake_minimum_required(VERSION 3.0.0)
project(tf-steam-api-parser VERSION 0.1.0)
SET(CMAKE_CXX_STANDARD 20)

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /VERBOSE")
    #set(CMAKE_CXX_FLAGS_RELEASE "{CMAKE_CXX_FLAGS_RELEASE} /O2")
endif()

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()
add_executable(tf-steam-api-parser main.cpp main.h)
target_link_libraries(tf-steam-api-parser ${CONAN_LIBS})
add_custom_command(
        TARGET tf-steam-api-parser POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/stat_names.json
        ${CMAKE_CURRENT_BINARY_DIR}/bin/stat_names.json
)
add_custom_command(
        TARGET tf-steam-api-parser POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/templates
        ${CMAKE_CURRENT_BINARY_DIR}/bin/templates
)