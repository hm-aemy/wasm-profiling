cmake_minimum_required(VERSION 3.5)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
project (smt32-wasm)
set(OPENCMDIR ${CMAKE_CURRENT_LIST_DIR}/../libopencm3)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Og")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Os")

set(STM32_COMP_OPTIONS "-DBENCHMARK=${BENCHMARK}")

if(DEFINED HEAP_TRACE )
add_compile_options(-fno-builtin-malloc -fno-builtin-realloc -fno-builtin-free 
    -Wl,--undefined=malloc,--wrap=malloc,--undefined=realloc,--wrap=realloc,--undefined=free,--wrap=free)
add_link_options(-Wl,--undefined=malloc,--wrap=malloc,--undefined=realloc,--wrap=realloc,--undefined=free,--wrap=free)
list(APPEND STM32_COMP_OPTIONS -DHEAP_TRACE=1)
endif()

if(DEFINED EMBENCH )
list(APPEND STM32_COMP_OPTIONS -DEMBENCH=1)
endif()

add_custom_command(OUTPUT ${CMAKE_CURRENT_LIST_DIR}/../target/thumbv7em-none-eabihf/release/libwasmi_staticlib.a
                    COMMAND cargo build --release
                    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/..)

add_executable(wasmi ${CMAKE_CURRENT_LIST_DIR}/src/main.c ${CMAKE_CURRENT_LIST_DIR}/src/benchmarks.c)
target_sources(wasmi PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../target/thumbv7em-none-eabihf/release/libwasmi_staticlib.a)
link_directories(${OPENCMDIR}/lib)

target_include_directories(wasmi PUBLIC ${OPENCMDIR}/include)
target_include_directories(wasmi PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)
target_link_options(wasmi PUBLIC
    --specs=nosys.specs -g -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -nostartfiles -mfpu=fpv4-sp-d16 -fno-common)
target_compile_options(wasmi PUBLIC --specs=nosys.specs -fno-common -g  -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 ${STM32_COMP_OPTIONS})



# find_library(libwasmi libwasmi_staticlib.a ${CMAKE_CURRENT_}/../target/thumbv7em-none-eabihf/release/)
target_link_libraries(wasmi PUBLIC m ${CMAKE_CURRENT_LIST_DIR}/../target/thumbv7em-none-eabihf/release/libwasmi_staticlib.a)