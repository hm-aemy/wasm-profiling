set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_FIND_ROOT_PATH $ENV{ARM_ROOT})
set(CMAKE_C_COMPILER $ENV{ARMGCC})
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)