#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

void *(*__kittycat_malloc)(size_t);
void *(*__kittycat_realloc)(void *, size_t);
void (*__kittycat_free)(void *);
void *(*__kittycat_memcpy)(void *, const void *, size_t);

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