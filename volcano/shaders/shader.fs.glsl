#version 450
#pragma shader_stage(fragment)

layout(location = 0) in vec4 v_FragColour;

layout(location = 0) out vec4 outColour;

void main()
{
    outColour = v_FragColour;
}
