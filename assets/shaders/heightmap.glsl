
float wiggly_shape(in vec3 pos, in vec2 uv) {
    float d = distance(uv, vec2(0.5));
    
    float edge = uState.radius;
    float falloff = 0.0125; 
    float s_d = 1.0-smoothstep(edge-falloff, edge+falloff, d);

    s_d = s_d * o_noise(pos.xz/100.0 + uTime, 8, 1.0) * cell(pos.xz/60.0 + vec2(o_noise(pos.xz/10.0, 3, 1.0)), 0.0);

    return s_d;
}

float lava_shape(in vec3 pos, in vec2 uv, in float time) {
    float d = distance(uv, vec2(0.5));
    
    float edge = uState.radius;
    float falloff = 0.0125; 
    float s_d = 1.0-smoothstep(edge-falloff, edge+falloff, d);

    vec2 w_p = pos.xz / 60.0;
    w_p += o_noise(w_p * 10.0, 3, 0.995) *0.1;
    
    vec2 v = voronoi(w_p, time);
    float rocks = v.y; 
    rocks *= voronoi_edge(w_p, time, 0.532);

    float rock_slope = 0.125;
    float rock_edge = 0.49;
    rocks = smoothstep(rock_edge - rock_slope, rock_edge + rock_slope, 1.0-rocks);
    rocks += so_noise(pos.xz/2.0 + vec2(so_noise(pos.xz/4.0, 3, 1.40)), 8, 0.50) * 0.84 * rocks;
    
    
    s_d = s_d * rocks;

    return s_d;
}

float cracked_lava_shape(in vec3 pos, in vec2 uv, in float time) {
    float d = distance(uv, vec2(0.5));
    
    float edge = uState.radius;
    float falloff = 0.0125; 
    float s_d = 1.0-smoothstep(edge-falloff, edge+falloff, d);
    
    vec2 ep = vec2(.8);
    vec2 xep = vec2(-ep.y, ep.x);
    vec2 cell_p = pos.xz/90.0;

    time *= 0.2;

    //cell_p += so_noise(cell_p / 10.0, 9 ,40.0)*0.3;        
    cell_p += o_noise(cell_p / 1.0,3 ,4.0)*0.01;        
    cell_p += fbm(cell_p / 10.0, 0.5);

    float rocks = voronoi(cell_p, time).y;
    
    rocks = 1.0 - voronoi_edge(cell_p, time, 0.28);
    rocks =  rocks * 2.50;
    
    
    float rock_slope = .9125;
    float rock_edge = 0.5;
    rocks = saturate(smoothstep(rock_edge - rock_slope, rock_edge + rock_slope, rocks));
    
    rocks += o_noise(cell_p *5.0 + vec2(o_noise(cell_p * 4.0, 3, 1.40)), 3, 0.50) * 0.84 * rocks;
    // rocks += o_noise(cell_p * 10.0, 13, 1.0) * 0.5 * rocks;
    // rocks += o_noise(cell_p * 59.0 + vec2(o_noise(cell_p * 10.0, 3, .40)) * 20., 8, 0.150) * 0.4 * rocks;
    rocks += fbm(cell_p / 3.0, 0.75) * rocks;

    s_d = s_d * rocks;

    return s_d;
}

float shape(in vec3 pos, in vec2 uv, in float time) {
    return cracked_lava_shape(pos, uv, time);
    return lava_shape(pos, uv, time);
}

vec3 shape_normal(in vec3 pos, in vec2 uv, in float time) {
    const float ep = 0.01;
    const vec2 eps = vec2(ep);
    const vec2 xeps = vec2(ep, -ep);

    const vec3 p1 = vec3(-eps.x, shape(pos - vec3(eps.x, 0.0, eps.y)* uState.height, uv - vec2(eps.x, eps.y), time), -eps.y);
    const vec3 p2 = vec3(eps.x, shape(pos + vec3(eps.x, 0.0, eps.y)* uState.height, uv + vec2(eps.x, eps.y), time), eps.y);

    const vec3 p3 = vec3(-xeps.x, shape(pos - vec3(xeps.x, 0.0, xeps.y)* uState.height, uv - vec2(xeps.x, xeps.y), time), -xeps.y);
    const vec3 p4 = vec3(xeps.x, shape(pos + vec3(xeps.x, 0.0, xeps.y)* uState.height, uv + vec2(xeps.x, xeps.y), time), xeps.y);


    return normalize(cross(p2-p1, p4-p3));
}

vec3 do_terrain(in vec3 pos, in vec2 uv, in float time) {
    pos.y += shape(pos, uv, time) * uState.height;
    return pos;
}