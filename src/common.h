#ifndef COMMON_H
#define COMMON_H

#include "index.h"

#include <cglm/cglm.h>

#define max(a, b)                                                              \
    ({                                                                         \
        __typeof__(a) _a = (a);                                                \
        __typeof__(b) _b = (b);                                                \
        _a > _b ? _a : _b;                                                     \
    })

#define min(a, b)                                                              \
    ({                                                                         \
        __typeof__(a) _a = (a);                                                \
        __typeof__(b) _b = (b);                                                \
        _a < _b ? _a : _b;                                                     \
    })

// Triangle with a diameter of 1
extern const vec2 g_unit_triangle[3];

typedef struct {
    vec2 position;
    vec2 scale;
    // Rotation in radians
    float rotation;
    float z;
} g_transform;

typedef struct {
    vec2 linear;
    float angular;
} g_velocity;

typedef struct {
    float x, y;
    uint8_t r, g, b, a;
} g_vertex;

static inline float rand_float(float min, float max) {
    float scale = rand() / (float)RAND_MAX;
    return min + scale * (max - min);
}

static inline float g_vec2_length(vec2 v) {
    return sqrtf(glm_pow2(v[0]) + glm_pow2(v[1]));
}

// Initialize a g_transform
inline void g_transform_defaults(g_transform *transform) {
    transform->position[0] = 0.0f;
    transform->position[1] = 0.0f;
    transform->scale[0] = 1.0f;
    transform->scale[1] = 1.0f;
    transform->rotation = 0.0f;
    transform->z = 0.0f;
}

// Apply a g_transform to an existing mat4
static inline void g_transform_model(g_transform *transform, mat4 *m) {
    glm_translate(*m, (vec3){transform->position[0], transform->position[1],
                             transform->z});
    glm_rotate_z(*m, transform->rotation, *m);
    glm_scale(*m, (vec3){transform->scale[0], transform->scale[1], 0.0f});
}

// Apply a g_transform to an existing mat3 (ignoring z)
static inline void g_transform_model_2d(g_transform *transform, mat3 *m) {
    glm_translate2d(*m, transform->position);
    glm_rotate2d(*m, transform->rotation);
    glm_scale2d(*m, transform->scale);
}

// sg_bindings decl
struct sg_bindings;

typedef struct {
    struct sg_bindings *bindings;
    mat4 *transforms;
    size_t *sizes;

    size_t size;
    size_t capacity;
} g_static_meshes;

g_static_meshes g_static_meshes_init(size_t capacity);

// Returns the index to the static mesh
size_t g_static_meshes_add(g_static_meshes *meshes, mat4 transform,
                           g_vertex *vertices, size_t num_vertices);

void g_static_meshes_delete(g_static_meshes *meshes);

// Stable index mask
enum g_actor_type : stable_index_mask_t {
    ACTOR_TYPE_ALIVE = 1 << 0,
    ACTOR_TYPE_PLAYER = 1 << 1,
    ACTOR_TYPE_ALLY = 1 << 2,
    ACTOR_TYPE_ENEMY = 1 << 3,
};

#define MAX_G_ACTORS (size_t)512

typedef struct {
    stable_index actor_index;

    stable_index_view *alive_view;
    stable_index_view *enemy_view;
    stable_index_view *ally_view;

    vec4 colors[MAX_G_ACTORS];
    g_transform transforms[MAX_G_ACTORS];
    mat4 global_transforms[MAX_G_ACTORS];
    g_velocity velocities[MAX_G_ACTORS];
    float drags[MAX_G_ACTORS];

} g_actor_stack;

typedef struct {
    vec4 color;
    g_transform transform;
    g_velocity velocity;
    float drag;
    enum g_actor_type type;
} g_actor_stack_create_ctx;

void g_actor_stack_init(g_actor_stack *stack);

stable_index_handle g_actor_stack_create(g_actor_stack *stack,
                                         g_actor_stack_create_ctx *ctx);

void g_actor_stack_remove(g_actor_stack *stack, stable_index_handle handle);

void g_actor_stack_phys(float dt, g_actor_stack *stack);

void g_actor_stack_transform(g_actor_stack *stack, float fixed_overstep);

void g_actor_stack_delete(g_actor_stack *stack);

typedef struct {
    bool left;
    bool right;
    bool up;
    bool down;

    vec2 direction;
} g_input_map;

typedef struct {
    stable_index_handle actor_handle;
    g_input_map input_map;
} g_player;

typedef struct {
    vec2 position;
    float view_height;

    vec2 mouse_world_pos;
    vec2 mouse_pos;
} g_camera;

void g_enemy_update(float dt, g_actor_stack *stack,
                    stable_index_handle player_handle);

void g_ally_update(float dt, g_actor_stack *stack,
                   stable_index_handle player_handle);
void g_camera_update(g_player *player, g_actor_stack *stack, float dt,
                     g_camera *camera);

struct sapp_event;

void g_player_input_map(const struct sapp_event *event, g_input_map *imap);
void g_player_update(float dt, g_player *player, g_actor_stack *actors,
                     g_camera *camera);

void g_camera_mouse(const struct sapp_event *event, g_camera *camera);
void g_camera_view(g_camera *camera, mat4 view);
void g_camera_proj(const g_camera *camera, const float aspect, mat4 proj);

#endif
