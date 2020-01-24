#version 450

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_uv;

layout (location = 0) out vec2 v_uv;
layout (location = 1) out vec3 v_viewdir;
layout (location = 2) out flat int v_index;

out gl_PerVertex {
    vec4 gl_Position;
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

void main() {
    vec4 position = obj[gl_InstanceIndex].model * vec4(a_position, 1.0);
    gl_Position = viewproj * position;
    v_uv = position.xz/30.;
    v_viewdir = viewpos.xyz - position.xyz;
    v_index = gl_InstanceIndex;
}
