#version 460 core

in vec3 normal;
in vec2 texcoord;
//in vec3 colour;
//in vec3 tangent;
//in vec3 bitangent;

layout (location = 0) out vec4 fragColour;

uniform sampler2D texture;

void main(){
	fragColour = texture2D(texture, texcoord).rgba;
	//fragColour = vec4(1, 0, 0, 1);
};