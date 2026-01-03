// A generational stable index w mask and masked index support.
#ifndef INDEX_H
#define INDEX_H

#include <stddef.h>

typedef unsigned int stable_index_t;
typedef unsigned short stable_index_mask_t;

typedef struct {
    stable_index_t index;
    stable_index_t generation;
} stable_index_handle;

// A 'view' of the stable index, based off of an external call.
typedef struct {
    // Mask used for the view.
    stable_index_mask_t mask;
    stable_index_t *indices;
    stable_index_t size;
    stable_index_t capacity;

    // Whether or not a newly created index handle should be assigned to the
    // this view.
} stable_index_view;

typedef struct {
    // Mask used for the view.
    stable_index_mask_t mask;
    size_t arr_idx;
} stable_index_view_key;

// A generational index
typedef struct {
    // Every 'available' index
    stable_index_t *available;

    // Index generations, starting from 0 and incremented at removal.
    stable_index_t *generations;
    // Index mask, can be used to create views.
    stable_index_mask_t *masks;

    size_t capacity;
    size_t availiable_size;

    // Share view_size and view_capacity with views
    stable_index_view_key *view_keys;
    // Share view_size and view_capacity with view_keys
    stable_index_view *views;

    size_t view_size;
    size_t view_capacity;
} stable_index;

// Check the remaining capacity
size_t stable_index_remaining_cap(stable_index *index);

// Populate a new stable index with default values
void stable_index_init(stable_index *index, size_t capacity,
                       size_t view_capacity);

// Add a new view to an index, derived from a given mask.
stable_index_view *stable_index_add_view(stable_index *index,
                                         stable_index_mask_t mask);

// Gets an existing view, otherwise returns null.
stable_index_view *stable_index_get_view(stable_index *index,
                                         stable_index_mask_t mask);

// Check the stable index element for any change to its mask, and adjust views
// accordingly.
void stable_index_view_update(stable_index_view *view, stable_index_t idx);

// Fetch an available index handle with a default mask.
stable_index_handle stable_index_create(stable_index *stack);

// Fetch an available index handle.
stable_index_handle stable_index_create_mask(stable_index *stack,
                                             stable_index_mask_t mask);

// Checks whether or not mask a contains b.
static inline bool stable_index_mask_contains(stable_index_mask_t a,
                                              stable_index_mask_t b) {
    return (a & b) == b;
}

// Update an index view with a newly masked element.
// Use stable_index_mask_contains to check if the element belongs to the
// view.
void stable_index_view_add(stable_index_view *view, const stable_index_t idx);

// Free an 'alive' index handle, incrementing the generation and moving the
// id into the 'available' list.
void stable_index_remove(stable_index_handle handle, stable_index *stack);

// Remove an element from an index.
// Use stable_index_mask_contains to check if the element belongs to the
// view.
void stable_index_view_remove(stable_index_view *view,
                              const stable_index_t idx);

// Free the index' allocations.
void stable_index_delete(stable_index *index);

#endif
