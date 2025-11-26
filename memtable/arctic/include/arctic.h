#include <cstdint>

#ifndef ARCTIC_H
#define ARCTIC_H

extern "C" void* arctic_new();

extern "C" void* arctic_ref(void* map);

extern "C" void* arctic_insert(void* ref, const char* kbuf, std::size_t klen,
                               void* handle);

extern "C" void* arctic_destroy(void* map);

extern "C" void* arctic_ref_destroy(void* ref);

extern "C" void* arctic_iter(void* ref);

extern "C" bool arctic_iter_valid(void* iter);

extern "C" void* arctic_iter_key(void* iter);

extern "C" void arctic_iter_next(void* iter);

extern "C" void arctic_iter_destroy(void* iter);

#endif
