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

add_executable(wamr ${CMAKE_CURRENT_LIST_DIR}/src/main.c ${CMAKE_CURRENT_LIST_DIR}/src/platform/platform_api_vmcore.c ${CMAKE_CURRENT_LIST_DIR}/src/benchmarks.c)
link_directories(${OPENCMDIR}/lib)

include_directories(${CMAKE_CURRENT_LIST_DIR}/../wasm-micro-runtime/core/iwasm/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../wasm-micro-runtime/core/shared/platform/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/src/platform ${CMAKE_CURRENT_LIST_DIR}/src)

if(DEFINED AOT)
list(APPEND STM32_COMP_OPTIONS -Dd_AOT=1)
set (WAMR_BUILD_AOT 1)
else()
set (WAMR_BUILD_AOT 0)
endif()

set (SHARED_PLATFORM_CONFIG ${CMAKE_CURRENT_LIST_DIR}/src/platform/shared_platform.cmake)
set (WAMR_BUILD_PLATFORM "platform")
set (WAMR_BUILD_TARGET "THUMBV7_VFP")
set (WAMR_BUILD_INTERP 1)
set (WAMR_BUILD_FAST_INTERP 0)
set (WAMR_BUILD_LIBC_BUILTIN 1)
set (WAMR_BUILD_LIBC_WASI 0)
set (WAMR_BUILD_LIB_PTHREAD 0)
set (WAMR_DISABLE_HW_BOUND_CHECK 0)
set (WAMR_DISABLE_STACK_HW_BOUND_CHECK 0)
set (WAMR_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../wasm-micro-runtime)
include(${CMAKE_CURRENT_LIST_DIR}/../wasm-micro-runtime/build-scripts/runtime_lib.cmake)
add_library(vmlib ${WAMR_RUNTIME_LIB_SOURCE})
target_compile_options(vmlib PUBLIC -g -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16)

list(APPEND STM32_COMP_OPTIONS -DBIND_LIBC=${WAMR_BUILD_LIBC_BUILTIN})

target_include_directories(wamr PUBLIC ${OPENCMDIR}/include)
target_include_directories(wamr PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)
target_link_options(wamr PUBLIC
    --specs=nosys.specs -g -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -nostartfiles -mfpu=fpv4-sp-d16 -fno-common)
target_compile_options(wamr PUBLIC --specs=nosys.specs -fno-common -g  -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 ${STM32_COMP_OPTIONS})
target_link_libraries(wamr PUBLIC vmlib m)