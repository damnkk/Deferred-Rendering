#version 330 core

// 顶点着色器输入
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec2 vTexcoord;

// 传给片元着色器的变量
out vec2 texcoord;

void main() 
{
	gl_Position = vec4(vPosition, 1.0);
	texcoord = vTexcoord;
}
