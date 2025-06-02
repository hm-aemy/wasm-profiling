#ifndef PLATFORM_INTERNAL_H
#define PLATFORM_INTERNAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <inttypes.h>

typedef void* korp_mutex;
typedef void* korp_cond;
typedef void* korp_sem;
typedef void* korp_tid;
typedef int os_file_handle;
typedef int korp_rwlock;
typedef int os_dir_stream;
typedef int os_raw_file_handle;

#endif