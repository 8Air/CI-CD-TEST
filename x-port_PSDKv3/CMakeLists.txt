cmake_minimum_required(VERSION 2.8)
project(X-Port_3 C)

set(PROJECT_NAME "apptest")

### Set toolchain flags
set(CMAKE_C_FLAGS "-D_REENTRANT -std=gnu99 -g -O3")
set(CMAKE_EXE_LINKER_FLAGS "-pthread -lpthread")

set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")

set(PSDK_ARCH_SYS linux)
include_directories(hal/)
include_directories(osal/)
include_directories(utils/)
include_directories(Complects/${COMPLECT}/)
include_directories(.) #change it!!!

include_directories(${PROJECT_NAME} PUBLIC psdk_lib_v3/include)
get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
    message(STATUS "dir='${dir}'")
endforeach()

## Outputs
add_executable(${PROJECT_NAME} Complects/${COMPLECT}/main.c)
target_sources(${PROJECT_NAME} PUBLIC pu.c)
target_sources(${PROJECT_NAME} PUBLIC hal/hal_network.c hal/hal_uart.c)
target_sources(${PROJECT_NAME} PUBLIC osal/osal.c)
target_sources(${PROJECT_NAME} PUBLIC utils/util_misc.c)

##FIXME

if (UNIX)
    if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64")
        target_link_libraries(${PROJECT_NAME} /usr/lib/x86_64-linux-gnu/libx264.so)
        target_link_libraries(${PROJECT_NAME} /usr/lib/x86_64-linux-gnu/libjpeg.so)
        target_link_libraries(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/psdk_lib_v3/lib/x86_64-linux-gnu-gcc/libpayloadsdk.a)
        target_link_libraries(${PROJECT_NAME} m) # added -lm library
    elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "armv7l")
        target_link_libraries(${PROJECT_NAME} /usr/lib/arm-linux-gnueabihf/libx264.so)
        target_link_libraries(${PROJECT_NAME} /usr/lib/arm-linux-gnueabihf/libjpeg.so)
        target_link_libraries(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/psdk_lib_v3/lib/arm-linux-gnueabihf-gcc/libpayloadsdk.a)
        target_link_libraries(${PROJECT_NAME} m) # added -lm library
    else()
        message("Unknown OS architecture: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()
endif()

