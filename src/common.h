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
    // Rotation in radians
    float rotation;
    float z;
} g_transform;

typedef struct {
    vec2 linear;
    float angular;
} g_velocity;

// "Actor" is just a triangle boi that can move and do things.
// typedef struct {
//   g_transform transform;
//   vec4 color;
//   vec2 linear_vel;
//   float angular_vel;
// } g_actor;

typedef struct {
    float x, y;
    uint8_t r, g, b, a;
} g_vertex;

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

enum g_actor_type : uint8_t {
    ACTOR_TYPE_PLAYER,
    ACTOR_TYPE_ALLY,
    ACTOR_TYPE_ENEMY,
};

#define MAX_G_ACTORS (size_t)1024

typedef struct {
    stable_index actor_index;

    vec4 colors[MAX_G_ACTORS];
    g_transform transforms[MAX_G_ACTORS];
    mat4 global_transforms[MAX_G_ACTORS];
    g_velocity velocities[MAX_G_ACTORS];
    float drags[MAX_G_ACTORS];
    enum g_actor_type types[MAX_G_ACTORS];

} g_actor_stack;

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

static const float g_fixed_dt = 1.0f / 64;

typedef struct {
    g_actor_stack actors;
    g_static_meshes static_meshes;

    g_camera camera;
    g_player player;

    // Time at application start
    uint64_t start_time;
    // Time since application start
    uint64_t elapsed_time;

    float physics_tick;
} g_world;

struct sg_pipeline;
struct sg_bindings;
struct sg_pass_action;

inline float rand_float(float min, float max) {
    float scale = rand() / (float)RAND_MAX;
    return min + scale * (max - min);
}

// Initialize a g_transform
inline void g_transform_init(float x, float y, float rotation, float z,
                             g_transform *transform) {
    transform->position[0] = x;
    transform->position[1] = y;
    transform->rotation = rotation;
    transform->z = z;
}

// Apply a g_transform to an existing mat4
static inline void g_transform_model(g_transform *transform, mat4 *m) {
    glm_translate(*m, (vec3){transform->position[0], transform->position[1],
                             transform->z});
    glm_rotate_z(*m, transform->rotation, *m);
}

// Apply a g_transform to an existing mat3 (ignoring z)
static inline void g_transform_model_2d(g_transform *transform, mat3 *m) {
    glm_translate2d(*m, transform->position);
    glm_rotate2d(*m, transform->rotation);
}

bool g_actor_type_hostility(enum g_actor_type at);

// sokol_app.h
struct sapp_event;

void g_player_input_map(const struct sapp_event *event, g_input_map *imap);

void g_camera_mouse(const struct sapp_event *event, g_camera *camera);
void g_camera_view(g_camera *camera, mat4 view);
void g_camera_proj(const g_camera *camera, const float aspect, mat4 proj);

void g_world_init(g_world *world);
void g_world_update(float dt, g_world *world);
void g_world_delete(g_world *world);

#endif
