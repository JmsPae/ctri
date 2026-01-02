// Vertex Shader
#version 330

layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec4 aColor;

uniform mat4 uModel;
uniform mat4 uVP;
uniform vec4 uColor;

out vec4 vColor;

void main() {
    vColor = aColor * uColor;
    gl_Position = uVP * uModel * vec4(aPosition, 0.0, 1.0);
}
