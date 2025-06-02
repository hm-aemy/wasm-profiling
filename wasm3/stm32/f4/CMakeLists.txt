cmake_minimum_required(VERSION 3.1)
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
project(stm32-wasm-f4)
include(../CMakeLists.txt)
add_custom_command(OUTPUT ${OPENCMDIR}/lib/libopencm3_stm32f4.a
                    COMMAND make TARGETS='stm32/f4 CC=$ARMGCC/arm-none-eabi-gcc'
                    WORKING_DIRECTORY ${OPENCMDIR})

target_sources(wamr PRIVATE ./init.c)
target_link_libraries(wamr PUBLIC ${OPENCMDIR}/lib/libopencm3_stm32f4.a)
target_link_options(wamr PUBLIC -T ${CMAKE_CURRENT_LIST_DIR}/device.ld)
target_compile_options(wamr PUBLIC -DSTM32F4=1)