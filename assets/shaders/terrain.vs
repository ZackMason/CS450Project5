#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNorm;
layout(location = 2) in vec2 aTex;

out VS_OUT {
    vec3 pos;
    vec3 nrm;
    vec2 tex;
} vs_out;

struct TerrainState {
    float radius;
    float height;

    vec4 dirt_color;
    vec4 grass_color;
    vec4 s_water_color;
    vec4 d_water_color;
};

uniform TerrainState uState;

uniform float uTime;
uniform float uVertTime;
uniform mat4 uM;
uniform mat4 uVP;

#include <noise.glsl>
#include <heightmap.glsl>

void main() {
    vs_out.pos = vec3(uM * vec4(aPos, 1.0));
    vs_out.pos = do_terrain(vs_out.pos, aTex, uVertTime);

    vs_out.nrm = vec3(uM * vec4(aNorm, 0.0));
    vs_out.tex = aTex;

    gl_Position = uVP * vec4(vs_out.pos, 1.0);
}