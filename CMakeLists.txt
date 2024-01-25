cmake_minimum_required(VERSION 2.8)
message(STATUS "Compiler ID: ${CMAKE_C_COMPILER_ID}")

project(Topodrone)

set(COMPLECT "Aquamaper" "P24" "Sony" "Xpd22-hdmi" "X-cam")
set(COMPLECT "" CACHE STRING "The complect name")

if(COMPLECT)
    message("Complect set to1: ${COMPLECT}")
else()
    message(FATAL_ERROR "Complect not specified. Use -DCOMPLECT=<value>")
endif()

list(FIND COMPLECT ${COMPLECT} CONFIG_INDEX)

if(CONFIG_INDEX EQUAL -1)
    message(FATAL_ERROR "Selected complect is not supported.")
else()
    message("Selected complect: ${COMPLECT}")
    file(READ "build_configurations/${COMPLECT}/complects_list.txt" FILE_CONTENT)

    string(REPLACE "\n" "" FILE_CONTENT ${FILE_CONTENT})
    string(REPLACE "\r" "" FILE_CONTENT ${FILE_CONTENT})
    string(REPLACE "\t" "" FILE_CONTENT ${FILE_CONTENT})
    string(REPLACE " " ";" FILE_CONTENT ${FILE_CONTENT})
    message("Including:")

    foreach(SUBSTRING ${FILE_CONTENT})
        message("__________${SUBSTRING}")
        include(build_configurations/${COMPLECT}/Config.cmake)
    endforeach()
endif()
