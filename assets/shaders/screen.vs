#version 460 core

layout ( location = 0 ) in vec2 aPos;
layout ( location = 1 ) in vec2 aUV;

out vec2 voPos;
out vec2 voUV;

void main()
{
    voPos = aPos.xy;
    voUV = aUV;
    
    gl_Position = vec4(aPos.xy, 0.0, 1.0);
}