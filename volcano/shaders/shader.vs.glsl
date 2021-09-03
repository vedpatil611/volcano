#version 450
#pragma shader_stage(vertex)

layout (location = 0) in vec3 position;
layout (location = 1) in vec4 color;

//hidden (set = 0) <- descriptor set
layout (binding = 0) uniform UBOViewProj {
    mat4 proj;
    mat4 view;
} uboViewProj;

layout (binding = 1) uniform UBOModel {
    mat4 model;
} uboModel;

layout (location = 0) out vec4 v_color;

void main()
{
    gl_Position = uboViewProj.proj * uboViewProj.view * uboModel.model * vec4(position, 1.0f);
    v_color = color;
}
