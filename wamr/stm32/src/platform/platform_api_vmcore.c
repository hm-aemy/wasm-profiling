#include "platform_api_vmcore.h"
#include <stdio.h>
#include <stdarg.h>
/****************************************************
 *                     Section 1                    *
 *        Interfaces required by the runtime        *
 ****************************************************/

/**
 * Initialize the platform internal resources if needed,
 * this function is called by wasm_runtime_init() and
 * wasm_runtime_full_init()
 *
 * @return 0 if success
 */
int bh_platform_init(void)
{
    return 0;
}

/**
 * Destroy the platform internal resources if needed,
 * this function is called by wasm_runtime_destroy()
 */
void bh_platform_destroy(void)
{
    return;
}

/**
 ******** memory allocator APIs **********
 */

void *
os_malloc(unsigned size)
{
    return malloc(size);
}

void *
os_realloc(void *ptr, unsigned size)
{
    return realloc(ptr, size);
}

void os_free(void *ptr)
{
    return free(ptr);
}

void *
os_mmap(void *hint, size_t size, int prot, int flags, os_file_handle file)
{
    if ((uint64)size >= UINT32_MAX)
        return NULL;
    return malloc((uint32)size);
}

void os_munmap(void *addr, size_t size)
{
#if defined(CONFIG_ARCH_USE_TEXT_HEAP)
    if (up_textheap_heapmember(addr))
    {
        up_textheap_free(addr);
        return;
    }
#endif
    return free(addr);
}

int os_mprotect(void *addr, size_t size, int prot)
{
    return 0;
}

void os_dcache_flush(void)
{
}

/**
 * Note: the above APIs can simply return NULL if wasm runtime
 *       isn't initialized with Alloc_With_System_Allocator.
 *       Refer to wasm_runtime_full_init().
 */

int os_printf(const char *format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    int result = vfprintf(stdout, format, argptr);
    va_end(argptr);
    return result;
}

int os_vprintf(const char *format, va_list ap)
{
    return vfprintf(stdout, format, ap);
}

/**
 * Get microseconds after boot.
 */
uint64
os_time_get_boot_microsecond(void)
{
    return 0;
}

/**
 * Get current thread id.
 * Implementation optional: Used by runtime for logging only.
 */
korp_tid
os_self_thread(void)
{
    return NULL;
}

/**
 * Get current thread's stack boundary address, used for runtime
 * to check the native stack overflow. Return NULL if it is not
 * easy to implement, but may have potential issue.
 */
uint8 *
os_thread_get_stack_boundary(void)
{
    return NULL;
}

/**
 ************** mutext APIs ***********
 *  vmcore:  Not required until pthread is supported by runtime
 *  app-mgr: Must be implemented
 */

int os_mutex_init(korp_mutex *mutex)
{
    return 0;
}

int os_mutex_destroy(korp_mutex *mutex)
{
    return 0;
}

int os_mutex_lock(korp_mutex *mutex)
{
    return 0;
}

int os_mutex_unlock(korp_mutex *mutex)
{
    return 0;
}

/**
 * Dump memory information of the current process
 * It may have variant implementations in different platforms
 *
 * @param out the output buffer. It is for sure the return content
 *            is a c-string which ends up with '\0'
 * @param size the size of the output buffer
 *
 * @return 0 if success, -1 otherwise
 */
int os_dumps_proc_mem_info(char *out, unsigned int size)
{
    return -1;
}

/**
 * This function creates a condition variable
 *
 * @param cond [OUTPUT] pointer to condition variable
 *
 * @return 0 if success
 */
int os_cond_init(korp_cond *cond)
{
    return -1;
}

/**
 * This function destroys condition variable
 *
 * @param cond pointer to condition variable
 *
 * @return 0 if success
 */
int os_cond_destroy(korp_cond *cond)
{
    return -1;
}

uint64 os_time_get_boot_us(void)
{
    return 0;
}

unsigned os_getpagesize()
{
    return 1 << 12;
}

os_file_handle
os_get_invalid_handle()
{
    return -1;
}

void *
os_mremap(void *old_addr, size_t old_size, size_t new_size)
{
    printf("old_addr: %p, old_size: %d\n", old_addr, old_size);
    if ((!old_addr) || (!old_size))
    {
        return os_mmap(0, new_size, 0, 0, 0);
    }
    else
    {
        return os_realloc(old_addr, new_size);
    }
}

void
os_thread_jit_write_protect_np(bool enabled)
{}

void
os_icache_flush(void *start, size_t len)
{}