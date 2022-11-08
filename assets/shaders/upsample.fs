#version 460 core
out vec4 FragColor;

in vec2 voPos;
in vec2 voUV;

uniform sampler2D uTexture;
uniform float uFilterRadius;

void main() {
    float x = uFilterRadius;
    float y = uFilterRadius;

    vec3 a = texture(uTexture, vec2(voUV.x - x, voUV.y + y)).rgb;
    vec3 b = texture(uTexture, vec2(voUV.x,     voUV.y + y)).rgb;
    vec3 c = texture(uTexture, vec2(voUV.x + x, voUV.y + y)).rgb;

    vec3 d = texture(uTexture, vec2(voUV.x - x, voUV.y)).rgb;
    vec3 e = texture(uTexture, vec2(voUV.x,     voUV.y)).rgb;
    vec3 f = texture(uTexture, vec2(voUV.x + x, voUV.y)).rgb;

    vec3 g = texture(uTexture, vec2(voUV.x - x, voUV.y - y)).rgb;
    vec3 h = texture(uTexture, vec2(voUV.x,     voUV.y - y)).rgb;
    vec3 i = texture(uTexture, vec2(voUV.x + x, voUV.y - y)).rgb;


    vec3 color = e*4.0;
    color += (b+d+f+h)*2.0;
    color += (a+c+g+i);
    color *= 1.0 / 16.0;

    FragColor = vec4(color,1.);
}



