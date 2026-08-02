#include "alloc.h"
#include "kc_string.h"
#include "perms.h"
#include "hashmap.h"

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
    // Ensure hashmap uses the same allocator by default unless explicitly set
    kittycat_hashmap_set_allocator(malloc, realloc, free);
    kittycat_kc_string_set_allocator(malloc, realloc, free, memcpy);
    kittycat_perms_set_allocator(malloc, realloc, free);
}
