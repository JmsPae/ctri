#include "world.h"

#include <minitrace/minitrace.h>
#include <sokol/sokol_time.h>

void g_world_init(g_world *world) {
    world->start_time = stm_now();

    world->static_meshes = g_static_meshes_init(8);

    mat4 transform = GLM_MAT4_IDENTITY_INIT;
    glm_translated_z(transform, -8.0f);

    g_static_meshes_add(
        &world->static_meshes, transform,
        (g_vertex[]){
            {g_unit_triangle[0][0], g_unit_triangle[0][1], 128, 128, 128, 255},
            {g_unit_triangle[1][0], g_unit_triangle[1][1], 128, 128, 128, 255},
            {g_unit_triangle[2][0], g_unit_triangle[2][1], 128, 128, 128, 255},
        },
        3);

    g_actor_stack_init(&world->actors);
    glm_vec2((vec2){0.0f, 0.0f}, world->camera.position);
    world->camera.view_height = 15.0f;

    world->player.actor_handle = g_actor_stack_create(
        &world->actors, &(g_actor_stack_create_ctx){
                            .color = {0.0f, 0.0f, 0.0f, 1.0},
                            .type = ACTOR_TYPE_PLAYER | ACTOR_TYPE_ALIVE,
                            .transform.position = {2.0f, 0.0f},
                            .drag = 3.0f,
                        });

    stable_index_handle a = g_actor_stack_create(
        &world->actors, &(g_actor_stack_create_ctx){
                            .color = {1.0f, 0.0f, 0.0f, 1.0},
                            .type = ACTOR_TYPE_ENEMY | ACTOR_TYPE_ALIVE,
                            .transform.z = -1.0f,
                            .transform.position =
                                {
                                    rand_float(-10.0f, 10.0f),
                                    rand_float(-10.0f, 10.0f),
                                },
                            .drag = 1.0f,
                        });
    g_actor_stack_remove(&world->actors, a);

    g_actor_stack_create(&world->actors,
                         &(g_actor_stack_create_ctx){
                             .color = {1.0f, 0.0f, 0.0f, 1.0},
                             .type = ACTOR_TYPE_ENEMY | ACTOR_TYPE_ALIVE,
                             .transform.z = -1.0f,
                             .transform.position =
                                 {
                                     rand_float(-10.0f, 10.0f),
                                     rand_float(-10.0f, 10.0f),
                                 },
                             .drag = 1.0f,
                         });
}

void g_world_update(float dt, g_world *world) {
    MTR_BEGIN("frame", "world_update");
    world->elapsed_time = stm_since(world->start_time);

    world->physics_tick += dt;

    MTR_BEGIN("frame", "physics");
    while (world->physics_tick >= g_fixed_dt) {
        g_actor_stack_phys(g_fixed_dt, &world->actors);
        g_player_update(g_fixed_dt, &world->player, &world->actors,
                        &world->camera);
        g_enemy_update(g_fixed_dt, &world->actors, world->player.actor_handle);
        world->physics_tick -= g_fixed_dt;
    }
    MTR_END("frame", "physics");

    // g_actor_stack_update(dt, &world->actors);

    g_camera_update(&world->player, &world->actors, dt, &world->camera);

    g_actor_stack_transform(&world->actors, world->physics_tick);

    MTR_END("frame", "world_update");
}

void g_world_delete(g_world *world) {
    g_actor_stack_delete(&world->actors);
    g_static_meshes_delete(&world->static_meshes);
}
