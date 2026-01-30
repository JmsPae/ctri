#include "common.h"

#include <cglm/affine-post.h>
#include <minitrace/minitrace.h>
#include <sokol/sokol_app.h>
#include <sokol/sokol_gfx.h>
#include <sokol/sokol_time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

g_static_meshes g_static_meshes_init(size_t capacity) {
    g_static_meshes meshes;

    meshes.bindings = malloc(sizeof(sg_bindings) * capacity);
    meshes.transforms = malloc(sizeof(mat4) * capacity);
    meshes.sizes = malloc(sizeof(size_t) * capacity);
    meshes.size = 0;
    meshes.capacity = capacity;

    return meshes;
}

size_t g_static_meshes_add(g_static_meshes *meshes, mat4 transform,
                           g_vertex *vertices, size_t num_vertices) {
    if (meshes->size >= meshes->capacity) {
        printf("Reached g_static_meshes capacity!");
        sapp_quit();
    }

    size_t idx = meshes->size;
    meshes->size++;

    meshes->bindings[idx] = (sg_bindings){
        .vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
            .data =
                (sg_range){
                    .ptr = vertices,
                    .size = sizeof(g_vertex) * num_vertices,
                },
        }),
    };

    glm_mat4_copy(transform, meshes->transforms[idx]);

    meshes->sizes[idx] = num_vertices;

    return idx;
}

void g_static_meshes_delete(g_static_meshes *meshes) {
    free(meshes->bindings);
    free(meshes->transforms);
    free(meshes->sizes);
}

bool g_actor_type_hostility(enum g_actor_type at) { return at >> 1 == 1; }

void g_actor_stack_init(g_actor_stack *stack) {
    stable_index_init(&stack->actor_index, MAX_G_ACTORS, 8);

    stack->alive_view =
        stable_index_add_view(&stack->actor_index, ACTOR_TYPE_ALIVE);

    stack->enemy_view = stable_index_add_view(
        &stack->actor_index, ACTOR_TYPE_ENEMY | ACTOR_TYPE_ALIVE);

    stack->ally_view = stable_index_add_view(
        &stack->actor_index, ACTOR_TYPE_ALLY | ACTOR_TYPE_ALIVE);
}

stable_index_handle g_actor_stack_create(g_actor_stack *stack,
                                         g_actor_stack_create_ctx *ctx) {
    stable_index_handle handle =
        stable_index_create_mask(&stack->actor_index, ctx->type);

    if (ctx != NULL) {
        glm_vec4_copy(ctx->color, stack->colors[handle.index]);
        stack->transforms[handle.index] = ctx->transform;
        stack->velocities[handle.index] = ctx->velocity;
        stack->drag[handle.index] = ctx->drag;
        stack->mass[handle.index] = ctx->mass;
    }

    return handle;
}

void g_actor_stack_remove(g_actor_stack *stack, stable_index_handle handle) {
    stable_index_remove(handle, &stack->actor_index);
}

// Check the validity of a stable index handle
bool g_actor_stack_valid(g_actor_stack *stack, stable_index_handle handle) {
    return handle.generation == stack->actor_index.generations[handle.index];
}

const vec2 g_unit_triangle[3] = {
    {0.5f, 0.0f},
    {-0.25f, 0.433f},
    {-0.25f, -0.433f},
};

static inline void calculate_normal(vec2 p1, vec2 p2, vec2 out) {
    vec2 d;
    glm_vec2_sub(p2, p1, d);
    glm_vec2_normalize(d);
    out[0] = d[1];
    out[1] = -d[0];
}

static inline bool triangle_axis_overlap(vec2 t1[3], vec2 norm, vec2 t2[3],
                                         float *overlap_min,
                                         float *overlap_max) {
    float t2_min = FLT_MAX;
    float t2_max = -FLT_MAX;

    float t1_min = FLT_MAX;
    float t1_max = -FLT_MAX;

    for (int i = 0; i < 3; i++) {
        float t1_p = glm_vec2_dot(t1[i], norm);
        float t2_p = glm_vec2_dot(t2[i], norm);

        t1_min = glm_min(t1_min, t1_p);
        t1_max = glm_max(t1_max, t1_p);

        t2_min = glm_min(t2_min, t2_p);
        t2_max = glm_max(t2_max, t2_p);
    }

    if (t2_max >= t1_min && t1_max >= t2_min) {
        *overlap_min = max(t1_min, t2_min);
        *overlap_max = min(t1_max, t2_max);

        return true;
    }

    return false;
}

// Calculate the overlap of two seperate ranges
static inline float get_overlap(float min_a, float max_a, float min_b,
                                float max_b) {
    return glm_min(max_a, max_b) - glm_max(min_a, min_b);
}

typedef struct {
    bool intersection;
    // Whether or not the intersection normal originates from the first triangle
    bool first;
    float overlap;
    vec2 normal;
} g_phys_intersection_res;

// Test two triangles for physics interactions
void g_phys_intersection(vec2 p1, vec2 p2, vec2 t1[3], vec2 t2[3],
                         g_phys_intersection_res *result) {
    vec2 norm1[3];
    vec2 norm2[3];

    for (int i = 0; i < 3; i++) {
        int j = (i + 1) - 3 * (i == 2);

        calculate_normal(t1[i], t1[j], norm1[i]);
        calculate_normal(t2[i], t2[j], norm2[i]);
    }

    vec2 overlap_normal;

    g_phys_intersection_res res;
    res.intersection = true;
    res.overlap = FLT_MAX;
    res.first = true;

    for (int i = 0; i < 3; i++) {
        float current_overlap_min = FLT_MAX;
        float current_overlap_max = -FLT_MAX;

        bool overlap = triangle_axis_overlap(
            t1, norm1[i], t2, &current_overlap_min, &current_overlap_max);

        if (overlap &&
            current_overlap_max - current_overlap_min < res.overlap) {
            res.overlap = current_overlap_max - current_overlap_min;
            glm_vec2_copy(norm1[i], overlap_normal);

        } else if (!overlap) {
            res.intersection = false;
            *result = res;
            return;
        }
    }

    for (int i = 0; i < 3; i++) {
        float current_overlap_min = FLT_MAX;
        float current_overlap_max = -FLT_MAX;

        bool overlap = triangle_axis_overlap(
            t2, norm2[i], t1, &current_overlap_min, &current_overlap_max);

        if (overlap &&
            current_overlap_max - current_overlap_min < res.overlap) {
            res.overlap = current_overlap_max - current_overlap_min;
            glm_vec2_copy(norm2[i], overlap_normal);

            res.first = false;

        } else if (!overlap) {
            res.intersection = false;
            *result = res;
            return;
        }
    }

    vec2 dir;

    if (res.first) {
        glm_vec2_sub(p2, p1, dir);
    } else {
        glm_vec2_sub(p1, p2, dir);
    }

    if (glm_vec2_dot(overlap_normal, dir) < 0) {
        res.first = !res.first;
    }

    glm_vec2_copy(overlap_normal, res.normal);
    *result = res;
}

static inline void g_vec2_mirror(vec2 n, vec2 v) {
    const float dot = glm_vec2_dot(v, n);
    glm_vec2_mulsubs(n, (2.0f * dot) * (dot < 0.0f), v);
}

// Crappy naive collision detection and response (for now)
// ... What the fuck is this 'broad phase' you speak of???
void g_actor_stack_phys(float dt, g_actor_stack *stack) {
    static vec2 t1[3];
    static vec2 t2[3];
    static vec3 t = {0.0f, 0.0f, 1.0f};
    static vec3 to;
    static mat3 m1, m2;

    const stable_index_view *alive = stack->alive_view;
    for (int i = 0; i < alive->size - 1; i++) {
        for (int j = i + 1; j < alive->size; j++) {
            const stable_index_t idx1 = alive->indices[i];
            const stable_index_t idx2 =
                alive->indices[j % stack->actor_index.capacity];

            g_transform *tr1 = &stack->transforms[idx1];
            g_transform *tr2 = &stack->transforms[idx2];

            glm_mat3_identity(m1);
            glm_mat3_identity(m2);
            g_transform_model_2d(tr1, &m1);
            g_transform_model_2d(tr2, &m2);

            for (int i = 0; i < 3; i++) {
                t[0] = g_unit_triangle[i][0];
                t[1] = g_unit_triangle[i][1];

                glm_mat3_mulv(m1, t, to);
                t1[i][0] = to[0];
                t1[i][1] = to[1];

                glm_mat3_mulv(m2, t, to);
                t2[i][0] = to[0];
                t2[i][1] = to[1];
            }

            // Detection and response
            g_phys_intersection_res res;
            g_phys_intersection(tr1->position, tr2->position, t1, t2, &res);

            if (res.intersection) {
                const stable_index_t i1 = res.first ? idx1 : idx2;
                const stable_index_t i2 = res.first ? idx2 : idx1;

                g_velocity *v1 = &stack->velocities[i1];
                g_velocity *v2 = &stack->velocities[i2];

                float mass1 = stack->mass[i1];
                float mass2 = stack->mass[i2];

                float inv_mass1 = (mass1 > 0) ? 1.0f / mass1 : 0.0f;
                float inv_mass2 = (mass2 > 0) ? 1.0f / mass2 : 0.0f;

                float sum_inv_mass = inv_mass1 + inv_mass2;

                // If both objects are static, there's no physics to resolve
                // TODO: skip any collision checks between static object
                if (sum_inv_mass == 0)
                    continue;

                vec2 rv;
                glm_vec2_sub(v2->linear, v1->linear, rv);
                float vel_along_normal = glm_vec2_dot(rv, res.normal);

                // Only resolve if objects are moving towards each other
                if (vel_along_normal < 0) {
                    // Impulse/"bouncyness"
                    const float e = 0.5f;
                    float j = -(1.0f + e) * vel_along_normal;
                    j /= sum_inv_mass;

                    vec2 impulse;
                    glm_vec2_scale(res.normal, j, impulse);
                    glm_vec2_muladds(impulse, -inv_mass1, v1->linear);
                    glm_vec2_muladds(impulse, inv_mass2, v2->linear);
                }

                // Positional Correction
                const float percent =
                    0.6f; // Penetration percentage to correct (0.2 to 0.8)
                const float slop =
                    0.01f; // Penetration allowance (prevents jitter)

                float correction_mag =
                    glm_max(res.overlap - slop, 0.0f) / sum_inv_mass * percent;

                vec2 correction;
                glm_vec2_scale(res.normal, correction_mag, correction);

                glm_vec2_muladds(correction, -inv_mass1, tr1->position);
                glm_vec2_muladds(correction, inv_mass2, tr2->position);
            }
        }
    }

    for (size_t i = 0; i < alive->size; i++) {
        stable_index_t index = alive->indices[i];

        g_transform *transform = &stack->transforms[index];
        g_velocity *vel = &stack->velocities[index];

        glm_vec2_muladds(vel->linear, dt, transform->position);
        transform->rotation += vel->angular * dt;

        glm_vec2_mulsubs(vel->linear, dt, vel->linear);
    }
}

void g_actor_stack_transform(g_actor_stack *stack, float fixed_overstep) {
    const stable_index_view *alive = stack->alive_view;
    for (size_t i = 0; i < alive->size; i++) {
        stable_index_t index = alive->indices[i];

        g_velocity *velocity = &stack->velocities[index];
        g_transform *transform = &stack->transforms[index];

        g_transform lerped_transform = (g_transform){
            .rotation = transform->rotation,
            .z = transform->z,
        };

        glm_vec2_copy(transform->position, lerped_transform.position);
        glm_vec2_copy(transform->scale, lerped_transform.scale);
        glm_vec2_muladds(velocity->linear, fixed_overstep,
                         lerped_transform.position);

        lerped_transform.rotation += velocity->angular * fixed_overstep;

        mat4 *global_transform = &stack->global_transforms[index];

        mat4 t = GLM_MAT4_IDENTITY_INIT;
        g_transform_model(&lerped_transform, &t);
        glm_mat4_copy(t, *global_transform);
    }
}

void g_actor_stack_delete(g_actor_stack *stack) {
    stable_index_delete(&stack->actor_index);
}

void g_player_input_map(const sapp_event *event, g_input_map *imap) {
    bool update_dir = false;
    if (event->type == SAPP_EVENTTYPE_UNFOCUSED) {
        imap->left = false;
        imap->right = false;
        imap->up = false;
        imap->down = false;

        update_dir = true;
    } else if (event->type == SAPP_EVENTTYPE_KEY_DOWN ||
               event->type == SAPP_EVENTTYPE_KEY_UP) {
        bool val = event->type == SAPP_EVENTTYPE_KEY_DOWN;

        if (event->key_code == SAPP_KEYCODE_A) {
            imap->left = val;
        } else if (event->key_code == SAPP_KEYCODE_D) {
            imap->right = val;
        } else if (event->key_code == SAPP_KEYCODE_W) {
            imap->up = val;
        } else if (event->key_code == SAPP_KEYCODE_S) {
            imap->down = val;
        }
        update_dir = true;
    }

    if (update_dir) {
        imap->direction[0] = 1.0f * imap->right - 1.0f * imap->left;
        imap->direction[1] = 1.0f * imap->up - 1.0f * imap->down;

        if (!glm_vec2_eq_eps(imap->direction, 0.0f)) {
            float len = glm_vec2_norm(imap->direction);
            glm_vec2_divs(imap->direction, len, imap->direction);
        }
    }
}

static inline float wrapMax(float x, float max) {
    /* integer math: `(max + x % max) % max` */
    return fmodf(max + fmodf(x, max), max);
}
/* wrap x -> [min,max) */
static inline float wrapMinMax(float x, float min, float max) {
    return min + wrapMax(x - min, max - min);
}

void g_player_update(float dt, g_player *player, g_actor_stack *stack,
                     g_camera *camera) {
    if (!g_actor_stack_valid(stack, player->actor_handle)) {
        printf("No player actor!");
        return;
    }

    size_t index = player->actor_handle.index;
    g_transform *transform = &stack->transforms[index];
    g_velocity *vel = &stack->velocities[index];

    vec2 target_vel;
    glm_vec2_scale(player->input_map.direction, 10.0f, target_vel);
    glm_vec2_lerp(vel->linear, target_vel, dt * 10.0f, vel->linear);

    vec2 dir_to_cam;
    glm_vec2_add(camera->mouse_world_pos, camera->position, dir_to_cam);
    glm_vec2_sub(dir_to_cam, transform->position, dir_to_cam);
    glm_vec2_normalize(dir_to_cam);

    vel->angular =
        wrapMinMax(atan2(dir_to_cam[1], dir_to_cam[0]) - transform->rotation,
                   -M_PI, M_PI) *
        20.0f;

    // transform->rotation = atan2(dir_to_cam[1], dir_to_cam[0]);
}

void g_ally_update(float dt, g_actor_stack *stack,
                   stable_index_handle player_handle) {
    if (!g_actor_stack_valid(stack, player_handle)) {
        printf("No player actor!");
        return;
    }

    float r = 1.0f;
    float c = r * M_PI * 2;
    int n = (int)(c / 2.0f);
    int ci = 0;

    g_transform *player_transform = &stack->transforms[player_handle.index];

    stable_index_view *allies = stack->ally_view;
    for (int i = 0; i < allies->size; i++) {
        stable_index_t idx = allies->indices[i];

        g_transform *transform = &stack->transforms[idx];

        if (i >= n) {
            r += 1.0f;
            c = r * M_PI * 2;
            n = (int)(c / 2.0f);
            ci = 0;
        }

        vec2 target_pos = {r * cos(ci * (M_PI / n)), r * sin(ci * (M_PI / n))};
        ci++;

        g_velocity *vel = &stack->velocities[idx];

        vec2 dir;
        glm_vec2_sub(target_pos, transform->position, dir);
        float len = glm_min(g_vec2_length(dir) * 10.0f, 10.0f);

        if (len > 0.0001f) {
            glm_vec2_muladds(dir, dt, vel->linear);
        }
    }
}

void g_enemy_update(float dt, g_actor_stack *stack,
                    stable_index_handle player_handle) {
    if (!g_actor_stack_valid(stack, player_handle)) {
        printf("No player actor!");
        return;
    }

    g_transform *player_transform = &stack->transforms[player_handle.index];

    stable_index_view *enemies = stack->enemy_view;
    for (int i = 0; i < enemies->size; i++) {
        size_t idx = enemies->indices[i];

        g_velocity *vel = &stack->velocities[idx];
        g_transform *transform = &stack->transforms[idx];

        vel->angular =
            wrapMinMax(
                atan2f(player_transform->position[1] - transform->position[1],
                       player_transform->position[0] - transform->position[0]) -
                    transform->rotation,
                -M_PI, M_PI) *
            5.0f;

        vel->linear[0] +=
            (cosf(transform->rotation) * 10.0f - vel->linear[0]) * dt * 5.0f;
        vel->linear[1] +=
            (sinf(transform->rotation) * 10.0f - vel->linear[1]) * dt * 5.0f;
    }
}

// --------------------------------- g_camera ---------------------------------

void g_camera_mouse(const sapp_event *event, g_camera *camera) {
    if (event->type == SAPP_EVENTTYPE_MOUSE_MOVE) {
        vec2 mouse_pos;
        glm_vec2((vec2){event->mouse_x, event->mouse_y}, mouse_pos);
        glm_vec2_copy(mouse_pos, camera->mouse_pos);

        vec2 norm_mouse_pos;
        glm_vec2_div(mouse_pos, (vec2){sapp_widthf(), sapp_heightf()},
                     norm_mouse_pos);

        float aspect = sapp_widthf() / sapp_heightf();

        float w = camera->view_height * aspect;
        float h = camera->view_height;

        float hw = w * 0.5f;
        float hh = h * 0.5f;

        float mx = -hw + w * norm_mouse_pos[0];
        float my = hh - h * norm_mouse_pos[1];

        // TODO: Move world position logic into update().
        glm_vec2((vec2){mx, my}, camera->mouse_world_pos);
    }
}

void g_camera_update(g_player *player, g_actor_stack *stack, float dt,
                     g_camera *camera) {

    if (!g_actor_stack_valid(stack, player->actor_handle)) {
        printf("No player actor!");
        sapp_quit();
    }

    size_t index = player->actor_handle.index;
    g_transform *player_transform = &stack->transforms[index];

    glm_vec2_lerp(camera->position, player_transform->position, dt * 2.0f,
                  camera->position);
}

void g_camera_view(g_camera *camera, mat4 view) {
    glm_translate(view, (vec3){camera->position[0], camera->position[1], 0.0f});
    glm_mat4_inv(view, view);
}

void g_camera_proj(const g_camera *camera, const float aspect, mat4 proj) {
    const float hh = camera->view_height * 0.5f;
    const float hw = (camera->view_height * aspect) * 0.5f;

    glm_ortho(-hw, hw, -hh, hh, -10.0f, 10.0, proj);
}
