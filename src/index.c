#include "index.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #define DEBUG_STABLE_INDEX

size_t stable_index_remaining_cap(stable_index *stack) {
    return stack->capacity - stack->alive_size;
}

void _stable_index_print(stable_index *stack) {
    printf("alive: ");
    for (size_t i = 0; i < stack->alive_size; i++) {
        stable_index_t idx = stack->alive[i];
        stable_index_t gen = stack->generations[idx];
        printf("{ i: %u, g: %u }, ", idx, gen);
    }
    printf("\navailable: ");

    for (size_t i = 0; i < stack->availiable_size; i++) {
        stable_index_t idx = stack->available[i];
        stable_index_t gen = stack->generations[idx];
        printf("{ i: %u, g: %u }, ", idx, gen);
    }

    printf("\n");
}

void stable_index_init(stable_index *stack, size_t capacity,
                       size_t view_capacity) {
    stack->capacity = capacity;
    stack->view_capacity = view_capacity;

    stack->alive = malloc(sizeof(stable_index_t) * capacity);
    stack->available = malloc(sizeof(stable_index_t) * capacity);
    stack->generations = malloc(sizeof(stable_index_t) * capacity);
    stack->masks = malloc(sizeof(stable_index_mask_t) * capacity);

    stack->views = malloc(sizeof(stable_index_view) * view_capacity);
    stack->view_keys = malloc(sizeof(stable_index_view) * view_capacity);

    // Reversed order for easy yoinks
    for (size_t i = 0; i < capacity; i++) {
        stack->available[i] = (capacity - 1) - i;
        stack->generations[i] = 0;
        stack->masks[i] = 0;
    }

    stack->view_size = 0;
    stack->alive_size = 0;
    stack->availiable_size = capacity;

#ifdef DEBUG_STABLE_INDEX
    printf("\ng_stable_index_init\n");
    _stable_index_print(stack);
#endif
}

stable_index_view *stable_index_add_view(stable_index *stack,
                                         stable_index_mask_t mask) {
    if (stack->view_size >= stack->view_capacity) {
        printf("Reached view capacity!\n");
        return nullptr;
    }

    stable_index_view *view = &stack->views[stack->view_size];

    stack->view_keys[stack->view_size] = (stable_index_view_key){
        .mask = mask,
        .arr_idx = stack->view_size,
    };

    view->mask = mask;
    view->indices = malloc(sizeof(stable_index_t) * stack->capacity);

    view->size = 0;
    view->capacity = stack->capacity;

    stack->view_size++;

    return view;
}

stable_index_handle stable_index_create(stable_index *stack) {
    return stable_index_create_mask(stack, STABLE_INDEX_MASK_ALIVE);
}

stable_index_handle stable_index_create_mask(stable_index *stack,
                                             stable_index_mask_t mask) {
    if (stack->availiable_size == 0) {
        printf("Reached maxiumum actor count!\n");
        return (stable_index_handle){.index = 0, .generation = 0};
    }

    // Index of the new index handle
    stable_index_t new_idx = stack->available[stack->availiable_size - 1];
    stable_index_t new_gen = stack->generations[new_idx];
    stack->masks[new_idx] = mask | STABLE_INDEX_MASK_ALIVE;

    stable_index_handle ret = {.index = new_idx, .generation = new_gen};

    // Target index for insertion into the 'alive' list.
    int32_t i = stack->alive_size - 1;

    while (i >= 0 && stack->alive[i] > new_idx) {
        stack->alive[i + 1] = stack->alive[i];
        i--;
    }

    stack->alive[i + 1] = new_idx;
    // stack->actors[new_actor_idx] = actor;
    stack->availiable_size--;
    stack->alive_size++;

#ifdef DEBUG_STABLE_INDEX
    printf("\ng_stable_index_create\n");
    _stable_index_print(stack);
#endif

    return ret;
}

void stable_index_view_add(stable_index_view *view, const stable_index_t idx) {
    // Target index for insertion into the 'alive' list.
    int32_t i = view->size - 1;

    while (i >= 0 && view->indices[i] > idx) {
        view->indices[i + 1] = view->indices[i];
        i--;
    }

    view->indices[i + 1] = idx;
    view->size++;
}

// Remove a living actor
void stable_index_remove(stable_index_handle handle, stable_index *stack) {
    int32_t i = 0;

    while (stack->alive[i] != handle.index && i < stack->alive_size) {
        i++;
    }

    if (i >= stack->alive_size || stack->alive[i] != handle.index) {
        printf("Couldn't find alive actor!\n");
        return;
    }

    stable_index_t found = stack->alive[i];

    for (int j = i; j < stack->alive_size - 1; j++) {
        stack->alive[j] = stack->alive[j + 1];
    }

    stack->alive_size--;

    i = stack->availiable_size - 1;

    // Shift elements that are smaller than value
    while (i >= 0 && stack->available[i] < found) {
        stack->available[i + 1] = stack->available[i];
        i--;
    }

    stack->available[i + 1] = found;
    stack->generations[found]++;
    stack->availiable_size++;

#ifdef DEBUG_STABLE_INDEX
    printf("\ng_stable_index_remove\n");
    _stable_index_print(stack);
#endif
}

void stable_index_view_remove(stable_index_view *view,
                              const stable_index_t idx) {
    int32_t i = 0;

    while (view->indices[i] != idx && i < view->size) {
        i++;
    }

    if (i >= view->size || view->indices[i] != idx) {
        printf("Couldn't find index in view!\n");
        return;
    }

    for (int j = i; j < view->size - 1; j++) {
        view->indices[j] = view->indices[j + 1];
    }

    view->size--;
}

// Get a living actor, otherwise NULL.
// g_actor *g_stable_index_get(g_stable_index *stack, g_actor_handle handle) {
//   if (stack->generations[handle.index] == handle.generation) {
//     return &stack->actors[handle.index];
//   }
//   return NULL;
// }

void stable_index_delete(stable_index *index) {
    for (int i = 0; i < index->view_size; i++) {
        free(index->views[i].indices);
    }

    free(index->alive);
    free(index->available);
    free(index->generations);
    free(index->masks);

    free(index->view_keys);
    free(index->views);
}
