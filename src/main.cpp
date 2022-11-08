#define WIN32_MEAN_AND_LEAN
#define NOMINMAX

#include "core.hpp"

#include <glad/glad.h>

#include "imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "Engine/window.hpp"
#include "Engine/orbit_camera.hpp"

#include "Math/transform.hpp"

#include "Util/allocators.hpp"
#include "Util/console.hpp"
#include "Util/timer.hpp"
#include "Util/color.hpp"
#include "Util/mesh_loaders.hpp"

#include "Graphics/buffer.hpp"
#include "Graphics/shared_buffer.hpp"
#include "Graphics/shader.hpp"
#include "Graphics/vertex_array.hpp"
#include "Graphics/bloom_framebuffer.hpp"
#include "Graphics/framebuffer.hpp"
#include "Graphics/screen.hpp"

#define BLOOM_PASS 1

struct vertex_t {
    v3f p;
    v3f n;
    v2f t;
};


constexpr f32    WALL_HEIGHT        = 20.0f;
constexpr size_t WALL_COUNT         = 4;
constexpr size_t WALL_VERT_COUNT    = WALL_COUNT * 4;
constexpr size_t WALL_TRIS_COUNT    = WALL_COUNT * 6;

constexpr f32    VERTEX_SPACING = 1.0f;
constexpr size_t TERRAIN_ROW_COUNT = 1000;
constexpr size_t TERRAIN_COL_COUNT = TERRAIN_ROW_COUNT;
constexpr size_t TERRAIN_VERT_COUNT = TERRAIN_ROW_COUNT * TERRAIN_COL_COUNT;
constexpr size_t TERRAIN_TRIS_COUNT = TERRAIN_ROW_COUNT * TERRAIN_COL_COUNT * 6;

int run();

int main(int argc, char** argv) {
    try {
        return run();
    } catch (std::exception& e) {
        logger_t::exception(e.what());
    }
    return 0;
}

int run() {
    bool      animate_frag{true}, animate_vert{true};
    shader_t* shader {nullptr};
    shader_t* screen_shader {nullptr};
    shader_t* downsample_shader {nullptr};
    shader_t* upsample_shader {nullptr};
    stack_allocator_t alloc{1024*1024*64};

#pragma region CreateWindow
    window_t window;
    window.width = 640;
    window.height = 480;

    orbit_camera_t camera{45.0f, 1.0f, 1.0f, 0.1f, 2000.0f};
    window.set_event_callback([&](auto& event){
        event_handler_t h;

        h.dispatch<const mouse_scroll_event_t>(event, [&](const mouse_scroll_event_t& e){
            return camera.on_scroll_event(e);
        });
     
    });

    window.open_window();
    window.set_vsync(false);
    window.set_title("CS450 Project 5");
    window.make_imgui_context();
    
    v3f bg_color{.30f, .20f, 0.98f};

    glEnable(GL_DEPTH_TEST);
#pragma endregion CreateWindow

#pragma region CreateMesh
    size_t vertex_count = 0;
    size_t tri_count = 0;
    mapped_buffer_t<vertex_t, TERRAIN_VERT_COUNT + WALL_VERT_COUNT> vertex_buffer;
    mapped_buffer_t<u32, TERRAIN_TRIS_COUNT + WALL_TRIS_COUNT> tris_buffer;

    vertex_t* vertices = vertex_buffer.get_data();
    u32* tris = tris_buffer.get_data();

    const auto i_2d = [w=TERRAIN_ROW_COUNT](size_t x, size_t y) ->u32 {
        return y*w+x;
    };

    const v2f terrain_size = v2f{ TERRAIN_ROW_COUNT * VERTEX_SPACING, TERRAIN_COL_COUNT * VERTEX_SPACING };

    // generate quad
    for (size_t y = 0; y < TERRAIN_COL_COUNT; y++) {
        for (size_t x = 0; x < TERRAIN_ROW_COUNT; x++) {
            vertex_t vertex;
            vertex.p = (v3f{x, 0.0f, y} * VERTEX_SPACING) - v3f{terrain_size.x, 0.0f, terrain_size.y} * 0.5f;
            vertex.n = v3f{0.0f, 1.0f, 0.0f};
            vertex.t = v2f{x, y} / v2f{TERRAIN_ROW_COUNT-1, TERRAIN_COL_COUNT-1}; 

            vertices[vertex_count++] = vertex;

            u32 i = i_2d(x,y);
            if (x != TERRAIN_ROW_COUNT - 1 && y != TERRAIN_COL_COUNT - 1) {
                tris[tri_count++] = (i);
                tris[tri_count++] = (i + 1);
                tris[tri_count++] = (i + TERRAIN_ROW_COUNT + 1);
                
                tris[tri_count++] = (i);
                tris[tri_count++] = (i + TERRAIN_ROW_COUNT + 1);
                tris[tri_count++] = (i + TERRAIN_ROW_COUNT);
            }
        }
    }

    const auto make_wall = [&] (float dist, v3f n) {
        const v3f bt = v3f{0,1,0};
        const v3f t = cross(n, bt);

        const v3f p0 = -n*dist;
        const v3f p1 = p0 + t * dist;
        const v3f p2 = p0 - t * dist;
        const v3f p3 = p0 - t * dist + bt * dist * WALL_HEIGHT;
        const v3f p4 = p0 + t * dist + bt * dist * WALL_HEIGHT;

        const auto v_s = vertex_count;
        vertices[vertex_count++] = vertex_t{p1, n, v2f{0.0}};
        vertices[vertex_count++] = vertex_t{p2, n, v2f{0.0}};
        vertices[vertex_count++] = vertex_t{p3, n, v2f{0.0}};
        vertices[vertex_count++] = vertex_t{p4, n, v2f{0.0}};

        tris[tri_count++] = v_s;
        tris[tri_count++] = v_s + 1;
        tris[tri_count++] = v_s + 2;

        tris[tri_count++] = v_s;
        tris[tri_count++] = v_s + 1;
        tris[tri_count++] = v_s + 3;
    };

    make_wall(terrain_size.x * 0.5f, v3f{1,0,0});
    make_wall(terrain_size.x * 0.5f, v3f{-1,0,0});
    make_wall(terrain_size.y * 0.5f, v3f{0,0,1});
    make_wall(terrain_size.y * 0.5f, v3f{0,0,-1});
    

    vertex_buffer.sync();
    tris_buffer.sync();
    vertex_array_t vertex_array{vertex_buffer.id, tris_buffer.id, tri_count, sizeof(vertex_t)};
    vertex_array
        .set_attrib(0, 3, GL_FLOAT, offsetof(vertex_t, p))
        .set_attrib(1, 3, GL_FLOAT, offsetof(vertex_t, n))
        .set_attrib(2, 2, GL_FLOAT, offsetof(vertex_t, t));
    transform_t model;

    puts("Created Mesh\n");
#pragma endregion CreateMesh

#pragma region CreateShader
    shader_t::add_attribute_definition({
        "vec3 aPos",
        "vec3 aNorm",
        "vec2 aTex",
    }, "terrain", "./assets/");

    shader = alloc.alloc<shader_t>(1);
    screen_shader = alloc.alloc<shader_t>(1);
    downsample_shader = alloc.alloc<shader_t>(1);
    upsample_shader = alloc.alloc<shader_t>(1);
    
    new (shader) shader_t("terrain", {"shaders/terrain.vs", "shaders/terrain.fs"}, GAME_ASSETS_PATH);
    new (screen_shader) shader_t("screen", {"shaders/screen.vs", "shaders/tonemap.fs"}, GAME_ASSETS_PATH);
    new (downsample_shader) shader_t("down", {"shaders/screen.vs", "shaders/downsample.fs"}, GAME_ASSETS_PATH);
    new (upsample_shader) shader_t("up", {"shaders/screen.vs", "shaders/upsample.fs"}, GAME_ASSETS_PATH);

    shader->bind();
    puts("Created Shader\n");
#pragma endregion CreateShader

#ifdef BLOOM_PASS
    framebuffer_t msaa{static_cast<int>(window.width), static_cast<int>(window.height), true};
    framebuffer_t fbo{static_cast<int>(window.width), static_cast<int>(window.height)};

    bloom_framebuffer_t bloom_buffer{window.width, window.height, 5};
    screen_t screen;

    const auto bloom_pass = [&] {
        downsample_shader->bind();
        downsample_shader->set_int("uTexture", 0);
        downsample_shader->set_vec2("uTextureSize", window.size());

        fbo.mode = framebuffer_t::target_e::RENDER_TARGET;
        fbo.unbind();
        fbo.slot = 0;
        fbo.mode = framebuffer_t::target_e::TEXTURE;
        fbo.bind();

        bloom_buffer.resize(window.width, window.height);
        const auto& mip_chain = bloom_buffer.mip_chain;
        bloom_buffer.bind();
        int mipc = 0;
        for (const auto& mip: mip_chain) {
            downsample_shader->set_int("uMipLevel", mipc++);

            glViewport(0, 0, mip.size.x, mip.size.y);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mip.texture, 0);

            screen.draw();
            downsample_shader->set_vec2("uTextureSize", mip.fsize);
            glBindTexture(GL_TEXTURE_2D, mip.texture);
        }

        upsample_shader->bind();
        upsample_shader->set_int("uTexture", 0);
        upsample_shader->set_float("uFilterRadius", 0.0025f);

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glBlendEquation(GL_FUNC_ADD);

        for (int i = static_cast<int>(mip_chain.size()) - 1; i > 0; i--) {
            const auto& mip = mip_chain[i];
            const auto& next_mip = mip_chain[i-1];
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mip.texture);
            
            glViewport(0, 0, next_mip.size.x, next_mip.size.y);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                    GL_TEXTURE_2D, next_mip.texture, 0);
            
            screen.draw();
        }

        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // Restore if this was default
        glDisable(GL_BLEND);
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0,0, window.width, window.height);
        
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, mip_chain[0].texture);
        
        screen_shader->bind();
        
        fbo.bind();
        
        screen_shader->set_int("uTexture", 0);
        screen_shader->set_int("uBloomTexture", 1);
        
        
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        
        screen.draw();
    };

#endif // BLOOM_PASS

    static timer32_t timer;
    const auto get_dt = [&] {
        return timer.get_dt(window.get_ticks());
    };

    using namespace color;
    struct terrain_state_t {
        f32 radius = 0.5f;
        f32 height = 2.0f;

        color4 dirt_color = "#c90e0eff"_c4;
        color4 grass_color = "#181111ff"_c4;
        color4 s_water_color;
        color4 d_water_color;
    } state;

    static float run_time = 0;
    const auto draw = [&] {

        glClearColor(bg_color.r, bg_color.g, bg_color.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        screen_shader->bind();
        screen_shader->set_float("uTime", run_time);

        shader->bind();
        shader->set_mat4("uM", model.to_matrix());
        shader->set_mat4("uVP", camera.view_projection());
        shader->set_vec3("uCamPos", camera.get_position());
        
        shader->set_float("uTime", run_time);
        shader->set_float("uFragTime", animate_frag ? run_time : 0.0f);
        shader->set_float("uVertTime", animate_vert ? run_time : 0.0f);

        // set terrain
        shader->set_float("uState.radius", state.radius);
        shader->set_float("uState.height", state.height);

        shader->set_vec4("uState.dirt_color", state.dirt_color);
        shader->set_vec4("uState.grass_color", state.grass_color);
        shader->set_vec4("uState.s_water_color", state.s_water_color);
        shader->set_vec4("uState.d_water_color", state.d_water_color);

        vertex_array.draw_elements(tri_count);
    };

    while(!window.should_close()) {
        fbo.resize(window.width, window.height);
        msaa.resize(window.width, window.height);
        /////////////////////////////

#pragma region Update
        const auto dt = get_dt();
        run_time += dt;
        camera.update(window, dt);
#pragma endregion Update

        /////////////////////////////

#pragma region Render
        msaa.bind();
        draw();
        msaa.unbind();
        fbo.blit(msaa);
        bloom_pass();
#pragma endregion Render

#pragma region UI
    window.imgui_new_frame();

        if (ImGui::Begin("Project 5")) {

            ImGui::Text(fmt::format("FPS: {}", 1.0f / dt).c_str());

            if (ImGui::Button("Reload Shader") || window.is_key_pressed('R')) {
                shader->~shader_t();
                screen_shader->~shader_t();
                new (shader) shader_t("terrain", {"shaders/terrain.vs", "shaders/terrain.fs"}, GAME_ASSETS_PATH);
                new (screen_shader) shader_t("tonemap", {"shaders/screen.vs", "shaders/tonemap.fs"}, GAME_ASSETS_PATH);

                shader->bind();
            }

            ImGui::DragFloat("radius", &state.radius, 0.01f);
            ImGui::DragFloat("height", &state.height);
            ImGui::ColorEdit4("lava", &state.dirt_color.x);
            ImGui::ColorEdit4("rock", &state.grass_color.x);

            static bool animate_both = true;

            if (ImGui::Checkbox("animate vert", &animate_vert)) {
                animate_both = animate_frag && animate_vert;
            }
            if (ImGui::Checkbox("animate frag", &animate_frag)) {
                animate_both = animate_frag && animate_vert;
            }
            if (ImGui::Checkbox("animate both", &animate_both)) {
                animate_vert = animate_frag = animate_both;
            }

            ImGui::End();
        }

    window.imgui_render();
#pragma endregion UI

        /////////////////////////////
        window.poll_events();
        window.swap_buffers();
    }

    return 0;
}