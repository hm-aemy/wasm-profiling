#include <init.h>

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <wasm_export.h>
#include <wasm_c_api.h>
#include <lib_export.h>
#include "benchmarks-defs.h"
#include "wasmtime_ssp.h"

typedef uint16_t __wasi_errno_t;
typedef __wasi_errno_t wasi_errno_t;
typedef uint32_t __wasi_fd_t;
typedef __wasi_fd_t wasi_fd_t;
typedef __wasi_address_family_t wasi_address_family_t;
typedef __wasi_addr_t wasi_addr_t;
typedef __wasi_advice_t wasi_advice_t;
typedef __wasi_ciovec_t wasi_ciovec_t;
typedef __wasi_clockid_t wasi_clockid_t;
typedef __wasi_dircookie_t wasi_dircookie_t;
typedef __wasi_errno_t wasi_errno_t;
typedef __wasi_event_t wasi_event_t;
typedef __wasi_exitcode_t wasi_exitcode_t;
typedef __wasi_fdflags_t wasi_fdflags_t;
typedef __wasi_fdstat_t wasi_fdstat_t;
typedef __wasi_fd_t wasi_fd_t;
typedef __wasi_filedelta_t wasi_filedelta_t;
typedef __wasi_filesize_t wasi_filesize_t;
typedef __wasi_filestat_t wasi_filestat_t;
typedef __wasi_filetype_t wasi_filetype_t;
typedef __wasi_fstflags_t wasi_fstflags_t;
typedef __wasi_iovec_t wasi_iovec_t;
typedef __wasi_ip_port_t wasi_ip_port_t;
typedef __wasi_lookupflags_t wasi_lookupflags_t;
typedef __wasi_oflags_t wasi_oflags_t;
typedef __wasi_preopentype_t wasi_preopentype_t;
typedef __wasi_prestat_t wasi_prestat_t;
typedef __wasi_riflags_t wasi_riflags_t;
typedef __wasi_rights_t wasi_rights_t;
typedef __wasi_roflags_t wasi_roflags_t;
typedef __wasi_sdflags_t wasi_sdflags_t;
typedef __wasi_siflags_t wasi_siflags_t;
typedef __wasi_signal_t wasi_signal_t;
typedef __wasi_size_t wasi_size_t;
typedef __wasi_sock_type_t wasi_sock_type_t;
typedef __wasi_subscription_t wasi_subscription_t;
typedef __wasi_timestamp_t wasi_timestamp_t;
typedef __wasi_whence_t wasi_whence_t;

#define RIGHTS_TTY_BASE                                        \
	(__WASI_RIGHT_FD_READ | __WASI_RIGHT_FD_FDSTAT_SET_FLAGS | \
	 __WASI_RIGHT_FD_WRITE | __WASI_RIGHT_FD_FILESTAT_GET |    \
	 __WASI_RIGHT_POLL_FD_READWRITE)
#define RIGHTS_TTY_INHERITING 0

typedef struct iovec_app
{
	uint32_t buf_offset;
	uint32_t buf_len;
} iovec_app_t;
struct iovec
{
	void *iov_base;
	size_t iov_len;
};
uint16_t
wasmtime_ssp_fd_write(
	uint32_t fd, const wasi_ciovec_t *iov, size_t iovcnt, size_t *nwritten)
{

	ssize_t len = 0;
	/* redirect stdout/stderr output to BH_VPRINTF function */
	if (fd == 1 || fd == 2)
	{
		int i;
		const struct iovec *iov1 = (const struct iovec *)iov;

		for (i = 0; i < (int)iovcnt; i++, iov1++)
		{
			if (iov1->iov_len > 0 && iov1->iov_base)
			{
				int written = 0;
				while (written != -1 && written < iov1->iov_len)
				{
					written += write(fd, iov1->iov_base, iov1->iov_len);
				}
				len += written;
			}
		}
	}
	else
	{
		return -1;
	}
	*nwritten = (size_t)len;
	return 0;
}

static uint16_t
fd_write(wasm_exec_env_t exec_env, uint32_t fd,
		 const iovec_app_t *iovec_app, uint32_t iovs_len,
		 uint32_t *nwritten_app)
{
	wasm_module_inst_t module_inst = get_module_inst(exec_env);
	wasi_ciovec_t *ciovec, *ciovec_begin;
	uint64_t total_size;
	size_t nwritten;
	uint32_t i;
	uint16_t err;
	total_size = sizeof(iovec_app_t) * (uint64_t)iovs_len;
	if (!wasm_runtime_validate_native_addr(module_inst, nwritten_app, (uint32_t)sizeof(uint32_t)) ||
		total_size >= UINT32_MAX || !wasm_runtime_validate_native_addr(module_inst, (void *)iovec_app, (uint32_t)total_size))
		return (uint16_t)-1;

	total_size = sizeof(wasi_ciovec_t) * (uint64_t)iovs_len;
	if (total_size >= UINT32_MAX || !(ciovec_begin = wasm_runtime_malloc((uint32_t)total_size)))
		return (uint16_t)-1;

	ciovec = ciovec_begin;

	for (i = 0; i < iovs_len; i++, iovec_app++, ciovec++)
	{
		if (!validate_app_addr(iovec_app->buf_offset, iovec_app->buf_len))
		{
			err = (uint16_t)-1;
			goto fail;
		}
		ciovec->buf = (char *)addr_app_to_native(iovec_app->buf_offset);
		ciovec->buf_len = iovec_app->buf_len;
	}

	err = wasmtime_ssp_fd_write(fd, ciovec_begin, iovs_len, &nwritten);
	if (err)
		goto fail;

	*nwritten_app = (uint32_t)nwritten;

	/* success */
	err = 0;

fail:
	wasm_runtime_free(ciovec_begin);
	return err;
}

__wasi_errno_t
wasmtime_ssp_fd_seek(
	__wasi_fd_t fd, __wasi_filedelta_t offset, __wasi_whence_t whence,
	__wasi_filesize_t *newoffset)
{
	int nwhence;
	switch (whence)
	{
	case __WASI_WHENCE_CUR:
		nwhence = SEEK_CUR;
		break;
	case __WASI_WHENCE_END:
		nwhence = SEEK_END;
		break;
	case __WASI_WHENCE_SET:
		nwhence = SEEK_SET;
		break;
	default:
		return __WASI_EINVAL;
	}
	printf("seek %ld, %lld, %d\n", fd, offset, nwhence);

	off_t ret = lseek(fd, offset, nwhence);
	if (ret < 0)
		return ret;
	*newoffset = (__wasi_filesize_t)ret;
	return 0;
}
static wasi_errno_t
fd_seek(wasm_exec_env_t exec_env, wasi_fd_t fd, wasi_filedelta_t offset,
		wasi_whence_t whence, wasi_filesize_t *newoffset)
{
	wasm_module_inst_t module_inst = get_module_inst(exec_env);

	if (!wasm_runtime_validate_native_addr(module_inst, newoffset, sizeof(wasi_filesize_t)))
		return (wasi_errno_t)-1;

	return wasmtime_ssp_fd_seek(fd, offset, whence, newoffset);
}
static wasi_errno_t
fd_fdstat_get(wasm_exec_env_t exec_env, wasi_fd_t fd,
			  wasi_fdstat_t *fdstat_app)
{
	if (fd != 1 && fd != 2)
	{
		return -1;
	}
	fdstat_app->fs_filetype = __WASI_FILETYPE_CHARACTER_DEVICE;
	fdstat_app->fs_flags = 0;
	fdstat_app->fs_rights_base = RIGHTS_TTY_BASE;
	fdstat_app->fs_rights_inheriting = RIGHTS_TTY_INHERITING;
	return 0;
}
static wasi_errno_t
fd_close(wasm_exec_env_t exec_env, wasi_fd_t fd)
{
	printf("warning: unsupported close call\n");
	return -1;
}
static void
proc_exit(wasm_exec_env_t exec_env, wasi_exitcode_t rval)
{
	wasm_module_inst_t module_inst = get_module_inst(exec_env);
	wasm_runtime_set_exception(module_inst, "wasi proc exit");
}
static NativeSymbol native_symbols[] =
	{
		EXPORT_WASM_API_WITH_SIG(fd_write, "(i*i*)i"),
		EXPORT_WASM_API_WITH_SIG(fd_seek, "(iIi*)i"),
		EXPORT_WASM_API_WITH_SIG(fd_fdstat_get, "(i*)i"),
		EXPORT_WASM_API_WITH_SIG(fd_close, "(i)i"),
		EXPORT_WASM_API_WITH_SIG(proc_exit, "(i)"),
};
#if BIND_LIBC == 1
static float64_t exp2_wrapper(wasm_exec_env_t exec_env, float64_t f)
{
	return exp2(f);
}
static int32_t fwrite_wrapper(wasm_exec_env_t exec_env, const void *ptr, int32_t size, int32_t nmemb, FILE *stream)
{
	stream = stdout;
	size_t written = fwrite(ptr, size, nmemb, stream);
	return written;
}
static int32_t start_msecs = 0;
static int32_t stop_msecs = 0;
static void start_time_wrapper(wasm_exec_env_t exec_env) {
	start_msecs = (uint64_t)(clock() * 1000) / (CLOCKS_PER_SEC);
}
static void stop_time_wrapper(wasm_exec_env_t exec_env) {
	stop_msecs = (uint64_t)(clock() * 1000) / (CLOCKS_PER_SEC);
}
static int32_t get_time_wrapper(wasm_exec_env_t exec_env) {
	return stop_msecs - start_msecs;
}
static uint32_t get_milsecs_wrapper(wasm_exec_env_t exec_env) {
	return (uint64_t)(clock() * 1000) / CLOCKS_PER_SEC;
}
static NativeSymbol env_symbols[] =
	{
		REG_NATIVE_FUNC(exp2, "(F)F"),
		REG_NATIVE_FUNC(fwrite, "(*ii*)i"),
		REG_NATIVE_FUNC(get_milsecs, "()i"),
		REG_NATIVE_FUNC(start_time, "()"),
		REG_NATIVE_FUNC(stop_time, "()"),
		REG_NATIVE_FUNC(get_time, "()i"),
};
#endif
uint32_t register_wasi()
{
	int n_native_symbols = sizeof(native_symbols) / sizeof(NativeSymbol);
#if BIND_LIBC == 1
	if (!wasm_runtime_register_natives("env", env_symbols, sizeof(env_symbols) / sizeof(NativeSymbol)))
	{
		return 0;
	}
#endif
	return wasm_runtime_register_natives("wasi_snapshot_preview1",
										 native_symbols,
										 n_native_symbols);
}
extern uintptr_t _stack;
extern uintptr_t __bss_end__;
static void mark_stack()
{
	uintptr_t *sp;
	asm volatile("mov %0, sp"
				 : "=r"(sp));
	for (uintptr_t *ptr = sp - 0x100; ptr > &__bss_end__; ptr--)
	{
		*ptr = 0xDEADBEEF;
	}
}
static size_t count_stack()
{
	size_t result = 0;
	int consec_markers = 0;
	for (uintptr_t *ptr = &_stack - 1; ptr > &__bss_end__ && consec_markers < 8; ptr--)
	{
		if (*ptr == 0xDEADBEEF)
		{
			consec_markers += 1;
		}
		else
		{
			consec_markers = 0;
		}
		result += sizeof(uintptr_t);
	}
	return result - 8 * sizeof(uintptr_t);
}

int post_main()
{
	for (volatile uint8_t i = 1; i; i++)
		;
	trace_buffer("trace ready!\n", 13);
	int err = run_active_bench(NULL);
	if (!err)
	{
		size_t stack_usage = count_stack();
		printf("Max stack use: %d\n", stack_usage);
	}
	printf("END OF TEST\n");
	trace_buffer("TRACE_DONE\n", 11);
	return err;
}

int main()
{
	init();
	mark_stack();
	return post_main();
}

#ifdef HEAP_TRACE
enum __attribute__((__packed__)) AllocType
{
	Malloc,
	Realloc,
	Free
};
typedef struct __attribute__((__packed__)) TraceData
{
	enum AllocType tag;
	union
	{
		struct
		{
			uintptr_t old;
			uintptr_t new;
			size_t size;
		} realloc;
		struct
		{
			uintptr_t new;
			size_t size;
		} malloc;
		struct
		{
			uintptr_t old;
		} free;

	} as;
} TraceData;

void *__real_malloc(size_t);
void *__wrap_malloc(size_t __size)
{
	void *ptr = __real_malloc(__size);
	TraceData trace = {
		.tag = Malloc,
		.as = {
			.malloc = {
				.new = ptr,
				.size = __size,
			}}};
	trace_buffer((uint8_t *)&trace, sizeof(trace));
	return ptr;
}

void *__real_realloc(void *, size_t);
void *__wrap_realloc(void *ptr, size_t n)
{
	void *ptr_new = __real_realloc(ptr, n);
	TraceData trace = {
		.tag = Realloc,
		.as = {
			.realloc = {
				.new = ptr_new,
				.old = ptr,
				.size = n,
			}}};
	trace_buffer((uint8_t *)&trace, sizeof(trace));
	return ptr_new;
}

void __real_free(void *);
void __wrap_free(void *ptr)
{
	TraceData trace = {
		.tag = Free,
		.as = {
			.free = {
				.old = ptr,
			}}};
	trace_buffer((uint8_t *)&trace, sizeof(trace));
	return __real_free(ptr);
}

#endif