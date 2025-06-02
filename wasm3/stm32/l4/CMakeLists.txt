cmake_minimum_required(VERSION 3.1)
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
project(stm32-wasm-l4)
include(../CMakeLists.txt)
add_custom_command(OUTPUT ${OPENCMDIR}/lib/libopencm3_stm32l4.a
                    COMMAND make TARGETS='stm32/l4 CC=$ARMGCC/arm-none-eabi-gcc'
                    WORKING_DIRECTORY ${OPENCMDIR})

target_sources(wasm3int PRIVATE ./init.c ${OPENCMDIR}/lib/libopencm3_stm32l4.a)
target_link_libraries(wasm3int PUBLIC ${OPENCMDIR}/lib/libopencm3_stm32l4.a)
target_link_options(wasm3int PUBLIC -T ${CMAKE_CURRENT_LIST_DIR}/device.ld)
target_compile_options(wasm3int PUBLIC -DSTM32L4=1)