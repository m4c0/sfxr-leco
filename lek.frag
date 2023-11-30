#version 450

layout(set = 0, binding = 0) uniform sampler2D smp;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 frag_colour;

void main() {
  frag_colour = texture(smp, uv);
}
