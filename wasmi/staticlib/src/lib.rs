#![no_std]
#![feature(c_size_t)]

extern crate alloc;

use ::core::alloc::GlobalAlloc;
use ::core::ffi;
use ::core::panic::PanicInfo;
use ::core::ptr::null;
use alloc::ffi::CString;
use alloc::string::ToString;
use spin::mutex::Mutex;
use wasmi::*;

struct CAllocator(Mutex<()>);

extern "C" {
    pub fn malloc(size: ffi::c_size_t) -> *mut ffi::c_void;
    pub fn free(p: *mut ffi::c_void);
    pub fn realloc(p: *mut ffi::c_void, size: ffi::c_size_t) -> *mut ffi::c_void;
}

unsafe impl GlobalAlloc for CAllocator {
    unsafe fn alloc(&self, layout: ::core::alloc::Layout) -> *mut u8 {
        let guard = self.0.lock();
        let size = layout.size();
        let result = malloc(size) as *mut u8;
        drop(guard);
        result
    }

    unsafe fn dealloc(&self, ptr: *mut u8, _: ::core::alloc::Layout) {
        let guard = self.0.lock();
        free(ptr as *mut ffi::c_void);
        drop(guard)
    }
    unsafe fn realloc(&self, ptr: *mut u8, _: ::core::alloc::Layout, new_size: usize) -> *mut u8 {
        let guard = self.0.lock();
        let result = realloc(ptr as *mut ffi::c_void, new_size) as *mut u8;
        drop(guard);
        result
    }
}

#[global_allocator]
static GLOBAL_ALLOCATOR: CAllocator = CAllocator(Mutex::new(()));

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

#[no_mangle]
pub extern "C" fn wasm_interp(
    input: *const ffi::c_uchar,
    len: ffi::c_size_t,
) -> *const ffi::c_char {
    // First step is to create the Wasm execution engine with some config.
    // In this example we are using the default configuration.
    let result: Result<i32, wasmi::Error> = (|| {
        let engine = Engine::default();
        // Wasmi does not yet support parsing `.wat` so we have to convert
        // out `.wat` into `.wasm` before we compile and validate it.
        let module;
        unsafe {
            module = Module::new(&engine, ::core::slice::from_raw_parts(input, len))?;
        }

        // All Wasm objects operate within the context of a `Store`.
        // Each `Store` has a type parameter to store host-specific data,
        // which in this case we are using `42` for.
        type HostState = u32;
        let mut store = Store::new(&engine, 42);

        // In order to create Wasm module instances and link their imports
        // and exports we require a `Linker`.
        let linker = <Linker<HostState>>::new(&engine);
        // Instantiation of a Wasm module requires defining its imports and then
        // afterwards we can fetch exports by name, as well as asserting the
        // type signature of the function with `get_typed_func`.
        //
        // Also before using an instance created this way we need to start it.
        let instance = linker.instantiate(&mut store, &module)?.start(&mut store)?;
        let hello = instance.get_typed_func::<(), i32>(&store, "_run")?;

        // And finally we can call the wasm!
        hello.call(&mut store, ())
    })();
    if result.is_ok() {
        null()
    } else {
        result
            .map_err(|e| CString::new(e.to_string()).unwrap().into_raw())
            .map(|_| null::<ffi::c_char>())
            .unwrap_err()
    }
}
