#ifndef KITTYCAT_ALLOC_H
#define KITTYCAT_ALLOC_H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#if defined(__cplusplus)
extern "C"
{
#endif // __cplusplus

    // kittycat_set_allocator allows for configuring a custom allocator for
    // all kittycat library operations.
    //
    // This *must* be called before any other kittycat library functions are called
    // otherwise it is undefined behavior.
    void kittycat_set_allocator(
        // Malloc
        void *(*malloc)(size_t),
        // Realloc
        void *(*realloc)(void *, size_t),
        // Free
        void (*free)(void *),
        // Memcpy
        void *(*memcpy)(void *, const void *, size_t));

#if defined(__cplusplus)
}
#endif // __cplusplus

#endif // KITTYCAT_ALLOC_H