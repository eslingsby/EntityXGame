#version 460 core

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inColour;

out vec3 colour;

uniform GlobalMatrices {
	mat4 view;
	mat4 projection;
};

//uniform mat4 view;
//uniform mat4 projection;

void main(){
	gl_Position = projection * view * vec4(inPosition, 1);

	colour = inColour;
};