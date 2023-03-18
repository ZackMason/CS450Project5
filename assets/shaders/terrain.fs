#version 460 core
#include <noise.glsl>

out vec4 FragColor;

in VS_OUT {
    vec3 pos;
    vec2 tex;
} fs_in;
uniform vec3 uCamPos;

struct TerrainState {
    float radius;
    float height;

    vec4 dirt_color;
    vec4 grass_color;
    vec4 s_water_color;
    vec4 d_water_color;
};

uniform float uFragTime;
uniform float uTime;
uniform TerrainState uState;

float grid(in vec2 v) {
    vec2 g = abs(fract(v - 0.5) - 0.5) / fwidth(v);
    return 1.0 - min(min(g.x, g.y), 1.0);
}
float grid(in vec3 v) {
    vec3 g = abs(fract(v - 0.5) - 0.5) / fwidth(v);
    return 1.0 - min(min(min(g.z,g.x), g.y), 1.0);
}

#include <heightmap.glsl>

// filament light model 
// https://google.github.io/filament/Filament.md.html
// LICENSE: https://github.com/google/filament/blob/main/LICENSE
vec3 filament_Schlick(const vec3 f0, float VoH) {
    float f = pow(1.0 - VoH, 5.0);
    return f + f0 * (1.0 - f);
}

vec3 filament_Schlick(const vec3 f0, float f90, float VoH) {
    return f0 + (f90 - f0) * pow(1.0 - VoH, 5.0);
}

float filament_Schlick(float f0, float f90, float VoH) {
    return f0 + (f90 - f0) * pow(1.0 - VoH, 5.0);
}

float filament_DGGX(float NoH, float roughness) {
    float a2 = roughness * roughness;
    float f = (NoH * a2 - NoH) * NoH + 1.0;
    return a2 / (3.14159 * f * f);
}

float filament_SmithGGX(float NoV, float NoL, float roughness) {
    float a2 = roughness * roughness;
    float GGXV = NoL * sqrt((NoV - a2 * NoV) * NoV + a2);
    float GGXL = NoV * sqrt((NoL - a2 * NoL) * NoL + a2);
    return min(0.5 / (GGXV + GGXL), 65504.0);
}

float filament_Burley(float roughness, float NoV, float NoL, float LoH) {
    // Burley 2012, "Physically-Based Shading at Disney"
    float f90 = 0.5 + 2.0 * roughness * LoH * LoH;
    float lightScatter = filament_Schlick(1.0, f90, NoL);
    float viewScatter  = filament_Schlick(1.0, f90, NoV);
    return lightScatter * viewScatter * (1.0 / 3.1415);
}

void main() {
    vec4 color = vec4(1,0,0,1);
    
    vec3 normal = vec3(0,1,0);
    if (uState.height > 0.0) {
        normal = shape_normal(fs_in.pos, fs_in.tex, uFragTime);
    } 

    vec3 v = normalize(uCamPos - fs_in.pos);

    if (dot(fs_in.tex, fs_in.tex) > 0.0 && distance(fs_in.tex, vec2(0.5)) < uState.radius) {
        float s = shape(fs_in.pos, fs_in.tex, uFragTime) / 1.0;

        s = clamp(s, 0.0, 1.0);

        // color.rgb = vec3(s);
        // break;

        vec4 lava_color = vec4(0.9059, 0.9333, 0.0667, 1.0) * 3.0;
        
        lava_color = 6.0 * mix(lava_color, uState.dirt_color, 
            saturate(o_noise(fs_in.pos.xz/14.0 + o_noise(fs_in.pos.xz/40.0 + vec2(uFragTime), 3, 1.0), 2, 1.0)));

        color.rgb = mix(lava_color, uState.grass_color * 4.0, s).rgb;
        

        vec3 light_dir = normalize(vec3(1));
        vec3 refl_dir = reflect(light_dir, normal);
        float kS = pow(max(dot(v, refl_dir), 0.0), 32.0);

        vec3 h = normalize(light_dir + v);
        float roughness = 0.8 * max(s, 0.85);
        roughness *= roughness;

        float ndf = filament_DGGX(saturate(dot(normal, h)), roughness);
        float g = filament_SmithGGX(
            saturate(dot(normal, v)) + 1e-5,
            saturate(dot(light_dir, h)),
            roughness
        );

        color.rgb *= max(0.40, dot(light_dir, normal));
        vec3 brdf = ndf * g * filament_Schlick(vec3(0.04), saturate(dot(light_dir, h)));
        
        vec3 kD = color.rgb * filament_Burley(roughness, saturate(dot(normal,v)), saturate(dot(normal, light_dir)), saturate(dot(light_dir, h)));

        //color += kS * 0.3;

        color.rgb = (kD + brdf) * 8.0 * saturate(dot(normal, light_dir));

        //color.rgb = s.xxx;

        // color.rgb = vec3(0.1294, 0.6549, 0.1294) * s;
    } else {
        color.rgb = vec3(grid(fs_in.pos / 40.0)) * 4.0;
    }

    color.rgb = pow(color.rgb, vec3(2.2));


    FragColor = color;
}