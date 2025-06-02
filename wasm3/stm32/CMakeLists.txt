cmake_minimum_required(VERSION 3.1)
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
project (smt32-wasm)
set(OPENCMDIR ${CMAKE_CURRENT_LIST_DIR}/../libopencm3)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Og")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Os")

set(STM32_COMP_OPTIONS "-DBENCHMARK=${BENCHMARK}")

if(DEFINED HEAP_TRACE )
add_compile_options(-fno-builtin-malloc -fno-builtin-realloc -fno-builtin-free 
    -Wl,--undefined=calloc,--wrap=calloc,--undefined=malloc,--wrap=malloc,--undefined=realloc,--wrap=realloc,--undefined=free,--wrap=free)
add_link_options(-Wl,--undefined=calloc,--wrap=calloc,--undefined=malloc,--wrap=malloc,--undefined=realloc,--wrap=realloc,--undefined=free,--wrap=free)
list(APPEND STM32_COMP_OPTIONS -DHEAP_TRACE=1)
endif()

if(DEFINED EMBENCH )
list(APPEND STM32_COMP_OPTIONS -DEMBENCH=1)
endif()

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../wasm3/source/ ${CMAKE_CURRENT_BINARY_DIR}/m3)
target_compile_options(m3 PUBLIC -Dd_m3HasWASI=1)

add_executable(wasm3int ${CMAKE_CURRENT_LIST_DIR}/src/main.c ${CMAKE_CURRENT_LIST_DIR}/src/benchmarks.c)
link_directories(${OPENCMDIR}/lib)

include_directories(${CMAKE_CURRENT_LIST_DIR}/src/platform ${CMAKE_CURRENT_LIST_DIR}/src)
target_include_directories(wasm3int PRIVATE "$<TARGET_PROPERTY:m3,INTERFACE_INCLUDE_DIRECTORIES>")

list(APPEND STM32_COMP_OPTIONS -DBIND_LIBC=${WAMR_BUILD_LIBC_BUILTIN})

target_include_directories(wasm3int PUBLIC ${OPENCMDIR}/include)
target_include_directories(wasm3int PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)
target_link_options(wasm3int PUBLIC
    --specs=nosys.specs -g -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -nostartfiles -mfpu=fpv4-sp-d16 -fno-common)
target_compile_options(wasm3int PUBLIC --specs=nosys.specs -fno-common -g  -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 ${STM32_COMP_OPTIONS})
target_link_libraries(wasm3int PUBLIC m m3)