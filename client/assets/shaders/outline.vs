#version 330

layout(location = 0) in vec3 vertexPosition;
layout(location = 2) in vec3 vertexNormal;

uniform mat4 mvp;
uniform float outlineSize;

void main() {
    vec4 clipPos = mvp * vec4(vertexPosition, 1.0);
    vec4 clipNormalPos = mvp * vec4(vertexPosition + vertexNormal, 1.0);

    vec2 screenPos = clipPos.xy / clipPos.w;
    vec2 screenNormalPos = clipNormalPos.xy / clipNormalPos.w;
    vec2 direction = screenNormalPos - screenPos;

    float lengthSq = dot(direction, direction);
    if (lengthSq > 0.000001) {
        direction *= inversesqrt(lengthSq);
        clipPos.xy += direction * outlineSize * clipPos.w;
    }

    clipPos.z += 0.0002 * clipPos.w;
    gl_Position = clipPos;
}
