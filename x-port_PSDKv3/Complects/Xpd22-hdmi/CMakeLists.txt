cmake_minimum_required(VERSION 2.8)
project(X-Port_3 C)

set(PROJECT_NAME "apptest")

### Set toolchain flags
set(CMAKE_C_FLAGS "-D_REENTRANT -std=gnu99 -g -O3 -fsanitize=address")
set(CMAKE_EXE_LINKER_FLAGS "-pthread -lpthread")

set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")

set(PSDK_ARCH_SYS linux)

include_directories(hal/)
include_directories(osal/)
include_directories(utils/)

include_directories(../psdk_lib/include/)

include_directories(/usr/include/arm-linux-gnueabihf/libswscale/)


## Outputs
add_executable(apptest main.c)
target_sources(apptest PUBLIC pu.c)
target_sources(apptest PUBLIC common.h)
target_sources(apptest PUBLIC hal/hal_network.c hal/hal_uart.c)
target_sources(apptest PUBLIC osal/osal.c)
target_sources(apptest PUBLIC osal/osal_socket.c)
#target_sources(apptest PUBLIC monitor/sys_monitor.c)
target_sources(apptest PUBLIC utils/util_misc.c)

target_link_libraries(apptest /usr/lib/arm-linux-gnueabihf/libx264.so)
target_link_libraries(apptest /usr/lib/arm-linux-gnueabihf/libjpeg.so)

target_link_libraries(apptest ${CMAKE_SOURCE_DIR}/../psdk_lib/lib/arm-linux-gnueabihf-gcc/libpayloadsdk.a)
target_link_libraries(apptest m)
#target_link_libraries(apptest ${CMAKE_SOURCE_DIR}/../psdk_lib/lib/aarch64-linux-gnu-gcc/libpayloadsdk.a)
#target_link_libraries(apptest ${CMAKE_SOURCE_DIR}/../psdk_lib/lib/x86_64-linux-gnu-gcc/libpayloadsdk.a)



