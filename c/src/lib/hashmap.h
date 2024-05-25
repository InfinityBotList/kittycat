// Copyright 2020 Joshua J Baker. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#ifndef KITTYCAT_HASHMAP_H
#define KITTYCAT_HASHMAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__cplusplus)
extern "C"
{
#endif // __cplusplus

    struct kittycat_hashmap;

    struct kittycat_hashmap *kittycat_hashmap_new(size_t elsize, size_t cap, uint64_t seed0,
                                                  uint64_t seed1,
                                                  uint64_t (*hash)(const void *item, uint64_t seed0, uint64_t seed1),
                                                  int (*compare)(const void *a, const void *b, void *udata),
                                                  void (*elfree)(void *item),
                                                  void *udata);

    struct kittycat_hashmap *kittycat_hashmap_new_with_allocator(void *(*malloc)(size_t),
                                                                 void *(*realloc)(void *, size_t), void (*free)(void *), size_t elsize,
                                                                 size_t cap, uint64_t seed0, uint64_t seed1,
                                                                 uint64_t (*hash)(const void *item, uint64_t seed0, uint64_t seed1),
                                                                 int (*compare)(const void *a, const void *b, void *udata),
                                                                 void (*elfree)(void *item),
                                                                 void *udata);

    void kittycat_hashmap_free(struct kittycat_hashmap *map);
    void kittycat_hashmap_clear(struct kittycat_hashmap *map, bool update_cap);
    size_t kittycat_hashmap_count(struct kittycat_hashmap *map);
    bool kittycat_hashmap_oom(struct kittycat_hashmap *map);
    const void *kittycat_hashmap_get(struct kittycat_hashmap *map, const void *item);
    const void *kittycat_hashmap_set(struct kittycat_hashmap *map, const void *item);
    const void *kittycat_hashmap_delete(struct kittycat_hashmap *map, const void *item);
    const void *kittycat_hashmap_probe(struct kittycat_hashmap *map, uint64_t position);
    bool kittycat_hashmap_scan(struct kittycat_hashmap *map, bool (*iter)(const void *item, void *udata), void *udata);
    bool kittycat_hashmap_iter(struct kittycat_hashmap *map, size_t *i, void **item);

    uint64_t kittycat_hashmap_sip(const void *data, size_t len, uint64_t seed0, uint64_t seed1);
    uint64_t kittycat_hashmap_murmur(const void *data, size_t len, uint64_t seed0, uint64_t seed1);
    uint64_t kittycat_hashmap_xxhash3(const void *data, size_t len, uint64_t seed0, uint64_t seed1);

    const void *kittycat_hashmap_get_with_hash(struct kittycat_hashmap *map, const void *key, uint64_t hash);
    const void *kittycat_hashmap_delete_with_hash(struct kittycat_hashmap *map, const void *key, uint64_t hash);
    const void *kittycat_hashmap_set_with_hash(struct kittycat_hashmap *map, const void *item, uint64_t hash);
    void kittycat_hashmap_set_grow_by_power(struct kittycat_hashmap *map, size_t power);
    void kittycat_hashmap_set_load_factor(struct kittycat_hashmap *map, double load_factor);

    // kittycat_hashmap_set_allocator allows for configuring a custom allocator for
    // all kittycat_hashmap library operations.
    //
    // Note: it is recommended to use kittycat_set_allocator instead
    void kittycat_hashmap_set_allocator(void *(*malloc)(size_t), void *(*realloc)(void *, size_t), void (*free)(void *));

#if defined(__cplusplus)
}
#endif // __cplusplus

#endif // KITTYCAT_HASHMAP_H