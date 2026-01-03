#include "index.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #define DEBUG_STABLE_INDEX

size_t stable_index_remaining_cap(stable_index *stack) {
    return stack->availiable_size;
}

void _stable_index_view_print(stable_index *index, stable_index_view *view) {
    printf("\nview %i: ", view->mask);
    for (size_t i = 0; i < view->size; i++) {
        stable_index_t idx = view->indices[i];
        stable_index_t gen = index->generations[idx];
        printf("{ i: %u, g: %u }, ", idx, gen);
    }
}

void _stable_index_print(stable_index *stack) {
    for (int i = 0; i < stack->view_size; i++) {
        _stable_index_view_print(stack, &stack->views[i]);
    }

    printf("\navailable: ");

    for (size_t i = 0; i < stack->availiable_size; i++) {
        stable_index_t idx = stack->available[i];
        stable_index_t gen = stack->generations[idx];
        printf("{ i: %u, g: %u }, ", idx, gen);
    }

    printf("\n");
}

void stable_index_init(stable_index *index, size_t capacity,
                       size_t view_capacity) {
    index->capacity = capacity;
    index->view_capacity = view_capacity;

    index->available = malloc(sizeof(stable_index_t) * capacity);
    index->generations = malloc(sizeof(stable_index_t) * capacity);
    index->masks = malloc(sizeof(stable_index_mask_t) * capacity);

    index->views = malloc(sizeof(stable_index_view) * view_capacity);
    index->view_keys = malloc(sizeof(stable_index_view) * view_capacity);

    // Reversed order for easy yoinks
    for (size_t i = 0; i < capacity; i++) {
        index->available[i] = (capacity - 1) - i;
        index->generations[i] = 0;
        index->masks[i] = 0;
    }

    index->view_size = 0;
    index->availiable_size = capacity;

#ifdef DEBUG_STABLE_INDEX
    printf("\ng_stable_index_init\n");
    _stable_index_print(index);
#endif
}

stable_index_view *stable_index_add_view(stable_index *index,
                                         stable_index_mask_t mask) {
    if (index->view_size >= index->view_capacity) {
        printf("Reached view capacity!\n");
        return nullptr;
    }

    stable_index_view *view = &index->views[index->view_size];

    index->view_keys[index->view_size] = (stable_index_view_key){
        .mask = mask,
        .arr_idx = index->view_size,
    };

    view->mask = mask;
    view->indices = malloc(sizeof(stable_index_t) * index->capacity);

    view->size = 0;
    view->capacity = index->capacity;

    index->view_size++;

    return view;
}

stable_index_view *stable_index_get_view(stable_index *index,
                                         stable_index_mask_t mask) {
    for (int i = 0; i < index->view_size; i++) {
        if (index->view_keys[i].mask == mask) {
            return &index->views[index->view_keys[i].arr_idx];
        }
    }

    return NULL;
}

stable_index_handle stable_index_create(stable_index *stack) {
    return stable_index_create_mask(stack, 0);
}

stable_index_handle stable_index_create_mask(stable_index *index,
                                             stable_index_mask_t mask) {
    if (index->availiable_size == 0) {
        printf("Reached maxiumum actor count!\n");
        return (stable_index_handle){.index = 0, .generation = 0};
    }

    // Index of the new index handle
    stable_index_t new_idx = index->available[index->availiable_size - 1];
    stable_index_t new_gen = index->generations[new_idx];
    index->masks[new_idx] = mask;

    stable_index_handle ret = {.index = new_idx, .generation = new_gen};

    // stable_index_view_add(index->alive_view, new_idx);
    for (int j = 0; j < index->view_size; j++) {
        if (stable_index_mask_contains(index->masks[new_idx],
                                       index->views[j].mask)) {
            stable_index_view_add(&index->views[j], ret.index);
        }
    }

    // stack->actors[new_actor_idx] = actor;
    index->availiable_size--;

#ifdef DEBUG_STABLE_INDEX
    printf("\ng_stable_index_create_mask\n");
    _stable_index_print(index);
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
    int32_t i = stack->availiable_size - 1;

    // Shift elements that are smaller than value
    while (i >= 0 && stack->available[i] < handle.index) {
        stack->available[i + 1] = stack->available[i];
        i--;
    }

    stack->available[i + 1] = handle.index;
    stack->generations[handle.index]++;
    stack->availiable_size++;

    for (int j = 0; j < stack->view_size; j++) {
        if (stable_index_mask_contains(stack->views[j].mask, stack->masks[i])) {
            stable_index_view_remove(&stack->views[j], handle.index);
        }
    }

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
        printf("Couldn't find index in view %i!\n", view->mask);
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

    free(index->available);
    free(index->generations);
    free(index->masks);

    free(index->view_keys);
    free(index->views);
}
