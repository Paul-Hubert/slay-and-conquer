#version 450

layout (location = 0) in vec2 v_uv;
layout (location = 1) in vec3 v_viewdir;
layout (location = 2) in flat int v_index;

layout (location = 0) out vec4 outColor;


const vec4 colors[4] = {
    vec4(0.),
    vec4(0.2, 0.2, 1.0, 0.5),
    vec4(0.2, 1.0, 0.2, 0.5),
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

layout (set = 1, binding = 0) uniform sampler2D normalMap;

layout (set = 1, binding = 1) uniform sampler2D dudvMap;

void main() {
    
    vec3 lightdir = normalize(vec3(0.5,0.5,0.5));
    vec3 viewdir = normalize(v_viewdir);
    
    //vec2 distortedTexCoords = texture(dudvMap, vec2(v_uv.x + time/100., v_uv.y)).rg*0.1;
    //distortedTexCoords = v_uv + vec2(distortedTexCoords.x, distortedTexCoords.y+time/100.) * 0.02;
    //vec2 totalDistortion = (texture(dudvMap, distortedTexCoords).rg * 2.0 - 1.0) * 0.02;
    
    // Calculate normal vector by summing two completely independant 
    vec2 normCoords  = vec2(v_uv.x, v_uv.y + time/400.);
    vec2 normCoords2 = vec2(v_uv.x + time/400, v_uv.y - time/800.);
    vec3 normalValue = texture(normalMap, normCoords + texture(dudvMap, normCoords).rg*0.01).rgb + texture(normalMap, normCoords2 + texture(dudvMap, normCoords2).rg*0.01).rgb;
    
    vec3 normal = normalize(vec3(normalValue.r * 2. - 1., (normalValue.b * 2. - 1.)*2., normalValue.g * 2. - 1.));
    
    float mul = obj[v_index].ambient + obj[v_index].diffuse * max(dot(normal, lightdir), 0.0)*0.2;
    
    // Calculate base color based on fresnel effect
    vec3 color = mix(vec3(138., 208., 218.)/255., vec3(136., 158., 210.)/255., dot(vec3(0, 1, 0), viewdir));
    
    // Calculate primary color
    outColor = vec4(mul * color, 1.0) + colors[obj[v_index].highlightcolor] * obj[v_index].ambient;
    
    
    // Specular highlights
    vec3 halfVector = normalize(lightdir + viewdir);
    outColor.rgb += vec3(1.0, 0.95, 0.85) * obj[v_index].specular * pow(clamp(dot(normal, halfVector), 0., 1.), 1200);
    
    // Wave whitecaps
    outColor.rgb += clamp((pow(normal.y, 20) - 0.75), 0.0, 1.0)*10.;
    
}
