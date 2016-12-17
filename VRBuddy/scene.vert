//Basic, does not take in any colour info to pass to frag shader, just Position and Normal.

#version 410

//Uniforms are the same for the entire rendering call.
uniform mat4 viewMatrix;	//World space to Camera Space.
uniform mat4 modelMatrix; //Model space to World space. Used for Vive teleporting.

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 color;

out vec3 fragColor;	//RGB
void main(){

	gl_Position = viewMatrix * modelMatrix * position;
	fragColor = color;
}