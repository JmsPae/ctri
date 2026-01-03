#ifndef WORLD_H
#define WORLD_H

#include "common.h"

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

void g_world_init(g_world *world);
void g_world_update(float dt, g_world *world);
void g_world_delete(g_world *world);

#endif
