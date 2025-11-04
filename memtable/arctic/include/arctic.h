#ifndef ARCTIC_H
#define ARCTIC_H

extern "C" void* arctic_new();

extern "C" void* arctic_ref(void* map);

extern "C" void* arctic_insert(void* ref, void* handle);

extern "C" void* arctic_destroy(void* map);

extern "C" void* arctic_ref_destroy(void* ref);

#endif
