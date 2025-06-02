cmake_minimum_required(VERSION 3.1)
set(CMAKE_TOOLCHAIN_FILE ../TC-arm.cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
project(stm32-wasm-l4)
include(../CMakeLists.txt)
add_custom_command(OUTPUT ${OPENCMDIR}/lib/libopencm3_stm32l4.a
                    COMMAND make TARGETS='stm32/l4 CC=$ARMGCC'
                    WORKING_DIRECTORY ${OPENCMDIR})

target_sources(wamr PRIVATE ./init.c ${OPENCMDIR}/lib/libopencm3_stm32l4.a)
target_link_libraries(wamr PUBLIC ${OPENCMDIR}/lib/libopencm3_stm32l4.a)
target_link_options(wamr PUBLIC -T ${CMAKE_CURRENT_LIST_DIR}/device.ld)
target_compile_options(wamr PUBLIC -DSTM32L4=1)