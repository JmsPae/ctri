#define SOKOL_IMPL
#if !defined(SOKOL_GLCORE) && !defined(SOKOL_GLES3) &&                         \
    !defined(SOKOL_D3D11) && !defined(SOKOL_METAL) && !defined(SOKOL_WGPU) &&  \
    !defined(SOKOL_VULKAN)
#define SOKOL_GLCORE
#endif
#include <sokol/sokol_app.h>
#include <sokol/sokol_gfx.h>
#include <sokol/sokol_glue.h>
#include <sokol/sokol_log.h>
#include <sokol/sokol_time.h>

#include <minitrace/minitrace.h>

#include "common.h"
#include "shaders.h"

static struct {
    sg_pipeline pipeline;
    sg_bindings triangle_bind;
    sg_pass_action pass_action;

    g_world world;

    uint64_t time;
} state;

void create_main_pipeline() {
    sg_shader shader = sg_make_shader(&(sg_shader_desc){
        .vertex_func =
            (sg_shader_function){
                .source = (const char *)shader_vert,
                .entry = "main",
            },
        .fragment_func =
            (sg_shader_function){
                .source = (const char *)shader_frag,
                .entry = "main",
            },

        .uniform_blocks =
            {
                [0] =
                    {
                        .size = sizeof(mat4),
                        .stage = SG_SHADERSTAGE_VERTEX,

                        .glsl_uniforms =
                            {
                                [0] =
                                    {
                                        .type = SG_UNIFORMTYPE_MAT4,
                                        .glsl_name = "uModel",
                                    },
                            },
                    },
                [1] =
                    {
                        .size = sizeof(mat4),
                        .stage = SG_SHADERSTAGE_VERTEX,

                        .glsl_uniforms =
                            {
                                [0] =
                                    {
                                        .type = SG_UNIFORMTYPE_MAT4,
                                        .glsl_name = "uVP",
                                    },
                            },
                    },
                [2] =
                    {
                        .size = sizeof(vec4),
                        .stage = SG_SHADERSTAGE_VERTEX,

                        .glsl_uniforms =
                            {
                                [0] =
                                    {
                                        .type = SG_UNIFORMTYPE_FLOAT4,
                                        .glsl_name = "uColor",
                                    },
                            },
                    },
            },

        .attrs =
            {
                [0] = {.glsl_name = "aPosition"},
                [1] = {.glsl_name = "aColor"},
            },
    });

    state.pipeline = sg_make_pipeline(&(sg_pipeline_desc){
        .face_winding = SG_FACEWINDING_CCW,
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
        .shader = shader,
        .depth =
            {
                .write_enabled = true,
                .compare = SG_COMPAREFUNC_LESS_EQUAL,
            },
        .layout =
            {
                .attrs =
                    {
                        [0].format = SG_VERTEXFORMAT_FLOAT2,
                        [1].format = SG_VERTEXFORMAT_UBYTE4N,
                    },
            },
        .cull_mode = SG_CULLMODE_BACK,
    });
}

// Triangle with a diameter of ~1.
void create_triangle_binding() {
    // const float vertices[] = {0.5f,   0.0f,    0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    //                           -0.25f, 0.433f,  0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    //                           -0.25f, -0.433f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f};

    const g_vertex vertices[] = {
        {0.5f, 0.0f, 255, 255, 255, 255},
        {-0.25f, 0.433f, 255, 255, 255, 255},
        {-0.25f, -0.433f, 255, 255, 255, 255},
    };

    state.triangle_bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
        .data = SG_RANGE(vertices),
    });
}

void init(void) {
    MTR_META_PROCESS_NAME("ctest");
    MTR_META_THREAD_NAME("main thread");

    srand(time(NULL));
    sg_setup(&(sg_desc){
        .environment = sglue_environment(),
        .logger.func = slog_func,
    });

    stm_setup();
    state.time = 0;

    g_world_init(&state.world);

    create_triangle_binding();
    create_main_pipeline();

    state.pass_action = (sg_pass_action){
        .colors[0] =
            {
                .load_action = SG_LOADACTION_CLEAR,
                .clear_value = {1.0f, 1.0f, 1.0f, 1.0f},
            },
    };
}

void event(const sapp_event *event) {
    g_player_input_map(event, &state.world.player.input_map);
    g_camera_mouse(event, &state.world.camera);
}

void g_draw_actors() {
    sg_apply_bindings(&state.triangle_bind);

    stable_index_view *alive = state.world.actors.alive_view;
    for (size_t i = 0; i < alive->size; i++) {
        uint32_t actor_idx = alive->indices[i];

        mat4 *actor_global_transform =
            &state.world.actors.global_transforms[actor_idx];
        vec4 *actor_color = &state.world.actors.colors[actor_idx];

        sg_apply_uniforms(0, &SG_RANGE(*actor_global_transform));

        sg_apply_uniforms(2, &SG_RANGE(*actor_color));
        sg_draw(0, 3, 1);
    }
}

void g_draw_static() {
    sg_apply_uniforms(2, &SG_RANGE(GLM_VEC4_ONE));

    for (size_t i = 0; i < state.world.static_meshes.size; i++) {
        sg_apply_bindings(&state.world.static_meshes.bindings[i]);
        sg_apply_uniforms(0,
                          &SG_RANGE(state.world.static_meshes.transforms[i]));
        sg_draw(0, state.world.static_meshes.sizes[i], 1);
    }
}

void frame(void) {
    MTR_BEGIN("frame", "frame");

    float dt = stm_sec(stm_laptime(&state.time));

    g_world_update(dt, &state.world);

    mat4 view = GLM_MAT4_IDENTITY_INIT;
    g_camera_view(&state.world.camera, view);

    mat4 projection;
    float aspect = sapp_widthf() / sapp_heightf();
    g_camera_proj(&state.world.camera, aspect, projection);

    mat4 vp;
    glm_mat4_mul(projection, view, vp);

    sg_begin_pass(&(sg_pass){
        .action = state.pass_action,
        .swapchain = sglue_swapchain(),
    });

    sg_apply_pipeline(state.pipeline);
    sg_apply_uniforms(1, &SG_RANGE(vp));

    g_draw_static();
    g_draw_actors();

    sg_end_pass();
    sg_commit();
    MTR_END("frame", "frame");
}

void cleanup(void) {
    g_world_delete(&state.world);
    sg_destroy_pipeline(state.pipeline);
    sg_shutdown();

    mtr_flush();
    mtr_shutdown();
}

sapp_desc sokol_main(int argc, char *argv[]) {
    mtr_init("trace.json");
    (void)argc;
    (void)argv;
    return (sapp_desc){
        .init_cb = init,
        .event_cb = event,
        .frame_cb = frame,
        .cleanup_cb = cleanup,
        .width = 1280,
        .height = 720,
        .sample_count = 2,
#ifdef __EMSCRIPTEN__
        .gl_major_version = 3,
        .gl_minor_version = 0,
#else
        .gl_major_version = 3,
        .gl_minor_version = 3,
#endif
        .window_title = "Triangle",
        .icon.sokol_default = true,
        .logger.func = slog_func,
    };
}
