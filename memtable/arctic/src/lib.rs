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
    if r#ref.is_null() {
        return;
    }

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
    if map.is_null() {
        return;
    }

    unsafe {
        drop(Box::from_raw(
            map.cast::<arctic::concurrent::Map<Vec<u8>, u64>>(),
        ))
    };
}

#[unsafe(no_mangle)]
extern "C" fn arctic_iter(r#ref: *mut ffi::c_void) -> *mut ffi::c_void {
    unsafe {
        r#ref
            .cast::<arctic::concurrent::MapRef<'static, Vec<u8>, u64>>()
            .as_mut()
            .map(|r#ref| Iter::new(r#ref))
            .map(Box::new)
            .map(Box::into_raw)
            .unwrap_or_default()
            .cast()
    }
}

#[unsafe(no_mangle)]
extern "C" fn arctic_iter_valid(iter: *mut ffi::c_void) -> bool {
    unsafe {
        iter.cast::<Iter>()
            .as_ref()
            .and_then(|iter| iter.next)
            .is_some()
    }
}

#[unsafe(no_mangle)]
extern "C" fn arctic_iter_key(iter: *const ffi::c_void) -> *const ffi::c_void {
    unsafe {
        iter.cast::<Iter>()
            .as_ref()
            .and_then(|iter| iter.next)
            .unwrap_or_default()
    }
}

#[unsafe(no_mangle)]
extern "C" fn arctic_iter_next(iter: *mut ffi::c_void) {
    unsafe {
        if let Some(iter) = iter.cast::<Iter>().as_mut() {
            iter.next = iter
                .iter
                .lend()
                .map(|(_, value)| value as *const ffi::c_void);
        }
    }
}

#[unsafe(no_mangle)]
extern "C" fn arctic_iter_destroy(iter: *mut ffi::c_void) {
    unsafe {
        if iter.is_null() {
            return;
        }

        drop(Box::from_raw(iter.cast::<Iter>()));
    }
}

struct Iter {
    iter: arctic::concurrent::PrefixIter<'static, 'static, Vec<u8>, u64, arctic::iter::Sorted>,
    _guard: arctic::concurrent::PrefixGuard<'static, 'static, Vec<u8>, u64>,
    next: Option<*const ffi::c_void>,
}

impl Iter {
    unsafe fn new(
        r#ref: &'static mut arctic::concurrent::MapRef<'static, Vec<u8>, u64>,
    ) -> Option<Self> {
        let guard = r#ref.prefix(&[])?;
        // HACK: work around self-referential lifetime

        let mut iter = guard.iter::<arctic::iter::Sorted>();
        let next = iter.lend().map(|(_, value)| value as *const ffi::c_void);

        let iter: arctic::concurrent::PrefixIter<
            'static,
            'static,
            Vec<u8>,
            u64,
            arctic::iter::Sorted,
        > = unsafe { core::mem::transmute(iter) };

        Some(Self {
            _guard: guard,
            iter,
            next,
        })
    }
}
