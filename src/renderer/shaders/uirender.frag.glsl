#version 450

layout (location = 0) in vec4 v_color;
layout (location = 1) in vec2 v_uv;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D color;

void main() {
    
    outColor = v_color * texture(color, v_uv);
    
}
