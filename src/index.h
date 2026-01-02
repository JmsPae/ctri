// A simple generational stable index.
#ifndef INDEX_H
#define INDEX_H

#include <stddef.h>
#include <stdint.h>

typedef unsigned int stable_index_t;

typedef unsigned short stable_index_mask_t;

typedef struct {
    stable_index_t index;
    stable_index_t generation;
} stable_index_handle;

// A 'view' of the stable index, based off of an external call.
typedef struct {
    stable_index_t *indices;
    size_t indices_size;
    size_t indices_capacity;

    // Whether or not a newly created index handle should be assigned to the
    // this view.
    bool (*use_handle)(const stable_index_handle);
} stable_index_view;

// A generational index
typedef struct {
    // Every 'alive' index
    stable_index_t *alive;

    // Every 'available' index
    stable_index_t *available;

    // Index generations, starting from 0 and incremented at removal.
    stable_index_t *generations;
    stable_index_mask_t *masks;

    size_t capacity;
    size_t alive_size;
    size_t availiable_size;

    stable_index_view *views;

    size_t view_size;
    size_t view_capacity;
} stable_index;

// Check the remaining capacity
size_t stable_index_remaining_cap(stable_index *stack);

// Populate a new stable index with default values
void stable_index_init(stable_index *stack, size_t capacity,
                       size_t view_capacity);

// Populate a new stable index with default values
stable_index_view *
stable_index_add_view(stable_index *stack,
                      bool (*use_index)(const stable_index_handle));

// Fetch an available index handle.
stable_index_handle stable_index_create(stable_index *stack);

// Free an 'alive' index handle, incrementing the generation and moving the id
// into the 'available' list.
void stable_index_remove(stable_index_handle handle, stable_index *stack);

// Free the index' allocations.
void stable_index_delete(stable_index *index);

#endif
