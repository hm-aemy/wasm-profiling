#![no_std]
#![feature(c_size_t)]

extern crate alloc;

use ::alloc::{ffi::CString, string::ToString};
use ::core::alloc::GlobalAlloc;
use ::core::ffi;
use ::core::panic::PanicInfo;
use ::core::ptr::null;
use ::core::slice;
use spin::mutex::Mutex;
use wasmi::{Engine, Error, Instance, Module, Store};

struct CAllocator(Mutex<()>);

extern "C" {
    pub fn malloc(size: ffi::c_size_t) -> *mut ffi::c_void;
    pub fn free(p: *mut ffi::c_void);
    pub fn realloc(p: *mut ffi::c_void, size: ffi::c_size_t) -> *mut ffi::c_void;
}

unsafe impl GlobalAlloc for CAllocator {
    unsafe fn alloc(&self, layout: ::core::alloc::Layout) -> *mut u8 {
        let _guard = self.0.lock();
        let size = layout.size();
        malloc(size) as *mut u8
    }

    unsafe fn dealloc(&self, ptr: *mut u8, _: ::core::alloc::Layout) {
        let _guard = self.0.lock();
        free(ptr as *mut ffi::c_void);
    }
    unsafe fn realloc(&self, ptr: *mut u8, _: ::core::alloc::Layout, new_size: usize) -> *mut u8 {
        let _guard = self.0.lock();
        realloc(ptr as *mut ffi::c_void, new_size) as *mut u8
    }
}

#[global_allocator]
static GLOBAL_ALLOCATOR: CAllocator = CAllocator(Mutex::new(()));

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

/// Interprets the Wasm bytecode using Wasmi.
///
/// # Safety
///
/// The caller is responsible to provide valid `input` and `len` parameters
/// that form the byte sequence of the WebAssembly (Wasm) input.
#[no_mangle]
pub unsafe extern "C" fn wasm_interp(
    input: *const ffi::c_uchar,
    len: ffi::c_size_t,
) -> *const ffi::c_char {
    let wasm = unsafe { slice::from_raw_parts(input, len) };
    match execute(wasm) {
        Ok(_result) => null(),
        Err(error) => CString::new(error.to_string()).unwrap().into_raw(),
    }
}

fn execute(wasm: &[u8]) -> Result<i32, Error> {
    let engine = Engine::default();
    let module: Module = Module::new(&engine, wasm)?;
    let mut store = Store::new(&engine, ());
    let instance = Instance::new(&mut store, &module, &[])?;
    instance
        .get_typed_func::<(), i32>(&store, "_run")?
        .call(&mut store, ())
}
