//Renders as a blank blue color.

#version 410 core

in vec3 fragColor;
out vec3 outColor;	//RGB

void main(){
	outColor = fragColor;
}