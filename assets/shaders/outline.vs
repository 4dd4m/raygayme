#version 330

layout(location = 0) in vec3 vertexPosition;
layout(location = 2) in vec3 vertexNormal;

uniform mat4 mvp;
uniform float outlineSize;

void main() {
    vec3 pos = vertexPosition + vertexNormal * outlineSize;
    gl_Position = mvp * vec4(pos, 1.0);
}
