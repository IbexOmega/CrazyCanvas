#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 out_ParticleImage;

void main()
{
	out_ParticleImage = vec4(vec3(1.0f), 1.0f);
}