#version 450

layout (location = 0) in vec3 v_normal;
layout (location = 1) in vec2 v_uv;
layout (location = 2) in vec3 v_viewdir;
layout (location = 3) in flat int v_index;

layout (location = 0) out vec4 outColor;

const vec4 colors[4] = {
    vec4(0.),
    vec4(0.2, 0.2, 1.0, 0.5),
    vec4(0.2, 0.5, 0.2, 0.1),
    vec4(1.0, 0.2, 0.2, 0.5)
};

struct perObject {
    mat4 model;
    int highlightcolor;
    float ambient;
    float diffuse;
    float specular;
};

layout (std140, set = 0, binding = 0) uniform Transform {
    float time;
    mat4 viewproj;
    vec4 viewpos;
    perObject obj[800];
};

layout (set = 1, binding = 0) uniform sampler2D color;

void main() {
    
    vec3 lightdir = normalize(vec3(0.5,0.5,0.5));
    vec3 normal = normalize(v_normal);
    
    vec3 halfVector = normalize(lightdir + normalize(v_viewdir));
    
    float mul = obj[v_index].ambient + obj[v_index].diffuse * (dot(normal, lightdir));
    
    outColor = mul * sqrt(texture(color, v_uv)) + colors[obj[v_index].highlightcolor] * obj[v_index].ambient;
    
    outColor.rgb += obj[v_index].specular * pow(dot(normal, halfVector), 600);
    
}
