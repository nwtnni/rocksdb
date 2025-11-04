use core::ffi;

#[unsafe(no_mangle)]
extern "C" fn arctic_new() -> *mut ffi::c_void {
    let map = Box::new(arctic::concurrent::Map::<Vec<u8>, u64>::default());
    Box::into_raw(map).cast()
}

#[unsafe(no_mangle)]
extern "C" fn arctic_ref(map: *const ffi::c_void) -> *mut ffi::c_void {
    let pin = Box::new(unsafe {
        map.cast::<arctic::concurrent::Map<Vec<u8>, u64>>()
            .as_ref()
            .unwrap()
            .pin()
    });

    Box::into_raw(pin).cast()
}

#[unsafe(no_mangle)]
extern "C" fn arctic_insert(r#ref: *mut ffi::c_void, handle: *const ffi::c_void) {
    let r#ref = unsafe {
        r#ref
            .cast::<arctic::concurrent::MapRef<'static, Vec<u8>, u64>>()
            .as_mut()
            .unwrap()
    };

    r#ref.insert(
        unsafe { core::slice::from_raw_parts(handle.cast::<u8>(), 20) },
        handle as u64,
    );
}

#[unsafe(no_mangle)]
extern "C" fn arctic_ref_destroy(r#ref: *mut ffi::c_void) {
    unsafe {
        drop(Box::from_raw(r#ref.cast::<arctic::concurrent::MapRef<
            'static,
            Vec<u8>,
            u64,
        >>()))
    };
}

#[unsafe(no_mangle)]
extern "C" fn arctic_destroy(map: *mut ffi::c_void) {
    unsafe {
        drop(Box::from_raw(
            map.cast::<arctic::concurrent::Map<Vec<u8>, u64>>(),
        ))
    };
}
