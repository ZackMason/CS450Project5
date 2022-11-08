#version 460 core

#include <noise.glsl>

out vec4 FragColor;

in vec2 voPos;
in vec2 voUV;

uniform sampler2D uTexture;
uniform sampler2D uBloomTexture;

uniform float uTime;
uniform int uTonemap = 0;
uniform float uExposure = 2.0;
uniform float uBloomStrength = 0.04;

vec3 ACESFilm(vec3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

void main()
{
    vec2 uv = voUV;
    vec3 bloom_color = texture(uBloomTexture, voUV).rgb;

    vec3 color = texture(uTexture, uv).rgb;

    color = mix(color, bloom_color, uBloomStrength);
    int tone_map = uTonemap;
    switch (tone_map) {
        case 0:
            color = ACESFilm(color);
            break;
        case 1:
            color = color / (color + vec3(1.0));
            break;
        case 2:
            color = vec3(1.0) - exp(-color * uExposure);
    }
    
    //color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(vec3(color),1.);
}



