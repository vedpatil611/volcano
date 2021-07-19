#version 450
#pragma shader_stage(vertex)

vec3 positions[3] = vec3[](
    vec3( 0.0f, -0.5f, 0.0f),
    vec3( 0.5f,  0.5f, 0.0f),
    vec3(-0.5f,  0.5f, 0.0f)
);

vec4 colours[3] = vec4[](
    vec4(1.0f, 0.0f, 0.0f, 1.0f),
    vec4(0.0f, 1.0f, 0.0f, 1.0f),
    vec4(0.0f, 0.0f, 1.0f, 1.0f)
);

layout(location = 0) out vec4 v_FragColour;

void main()
{
    gl_Position = vec4(positions[gl_VertexIndex], 1.0f);
    v_FragColour = colours[gl_VertexIndex];
}
