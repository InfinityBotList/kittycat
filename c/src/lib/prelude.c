// Common stuff to be included in all files
#include "prelude.h"
#include "hashmap.h"

void *(*__kittycat_malloc)(size_t) = NULL;
void *(*__kittycat_realloc)(void *, size_t) = NULL;
void (*__kittycat_free)(void *) = NULL;
void *(*__kittycat_memcpy)(void *, const void *, size_t) = NULL;

void kittycat_set_allocator(
    // Malloc
    void *(*malloc)(size_t),
    // Realloc
    void *(*realloc)(void *, size_t),
    // Free
    void (*free)(void *),
    // Memcpy
    void *(*memcpy)(void *, const void *, size_t))
{
    __kittycat_malloc = malloc;
    __kittycat_realloc = realloc;
    __kittycat_free = free;
    __kittycat_memcpy = memcpy;

    // Ensure hashmap uses the same allocator by default unless explicitly set
    kittycat_hashmap_set_allocator(malloc, realloc, free);
}
