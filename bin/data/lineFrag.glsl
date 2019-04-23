#version 460 core

in vec3 colour;

layout (location = 0) out vec4 fragColour;

void main(){
	fragColour = vec4(colour, 1);
};