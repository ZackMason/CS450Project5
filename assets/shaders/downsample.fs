#version 460 core
out vec4 FragColor;

in vec2 voPos;
in vec2 voUV;

uniform sampler2D uTexture;
uniform vec2 uTextureSize;
uniform int uMipLevel;

vec3 PowVec3(vec3 v, float p)
{
    return vec3(pow(v.x, p), pow(v.y, p), pow(v.z, p));
}

const float invGamma = 1.0 / 2.2;
vec3 ToSRGB(vec3 v) { return PowVec3(v, invGamma); }

float RGBToLuminance(vec3 col)
{
    return dot(col, vec3(0.2126f, 0.7152f, 0.0722f));
}

float KarisAverage(vec3 col)
{
    // Formula is 1 / (1 + luma)
    float luma = RGBToLuminance(ToSRGB(col)) * 0.25f;
    return 1.0f / (1.0f + luma);
}

void main() {
    vec2 texel_size = 1.0 / uTextureSize;
    float x = texel_size.x;
    float y = texel_size.y;

    vec3 a = texture(uTexture, vec2(voUV.x - 2*x, voUV.y + 2*y)).rgb;
    vec3 b = texture(uTexture, vec2(voUV.x,       voUV.y + 2*y)).rgb;
    vec3 c = texture(uTexture, vec2(voUV.x + 2*x, voUV.y + 2*y)).rgb;

    vec3 d = texture(uTexture, vec2(voUV.x - 2*x, voUV.y)).rgb;
    vec3 e = texture(uTexture, vec2(voUV.x,       voUV.y)).rgb;
    vec3 f = texture(uTexture, vec2(voUV.x + 2*x, voUV.y)).rgb;

    vec3 g = texture(uTexture, vec2(voUV.x - 2*x, voUV.y - 2*y)).rgb;
    vec3 h = texture(uTexture, vec2(voUV.x,       voUV.y - 2*y)).rgb;
    vec3 i = texture(uTexture, vec2(voUV.x + 2*x, voUV.y - 2*y)).rgb;

    vec3 j = texture(uTexture, vec2(voUV.x - x, voUV.y + y)).rgb;
    vec3 k = texture(uTexture, vec2(voUV.x + x, voUV.y + y)).rgb;
    vec3 l = texture(uTexture, vec2(voUV.x - x, voUV.y - y)).rgb;
    vec3 m = texture(uTexture, vec2(voUV.x + x, voUV.y - y)).rgb;

    vec3 color = vec3(0.0);//e*0.125;
    vec3 groups[5];
    switch(uMipLevel) {
        case 0:
            groups[0] = (a+b+d+e) * (0.125/4.0);
            groups[1] = (b+c+e+f) * (0.125/4.0);
            groups[2] = (d+e+g+h) * (0.125/4.0);
            groups[3] = (e+f+h+i) * (0.125/4.0);
            groups[4] = (j+k+l+m) * (0.5/4.0);
            groups[0] *= KarisAverage(groups[0]);
            groups[1] *= KarisAverage(groups[1]);
            groups[2] *= KarisAverage(groups[2]);
            groups[3] *= KarisAverage(groups[3]);
            groups[4] *= KarisAverage(groups[4]);
            color = groups[0]+groups[1]+groups[2]+groups[3]+groups[4];
            break;
        default:
            color = e*0.125;
            color += (a+c+g+i)*0.03125;
            color += (b+d+f+h)*0.0625;
            color += (j+k+l+m)*0.125;
            break;
    }

    color = max(color, 0.0001);

    FragColor = vec4(color,1.);
}



