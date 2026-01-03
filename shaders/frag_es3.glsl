#version 300 es  
// Fragment Shader
precision mediump float;

in vec4 vColor;
out vec4 fragColor;

const float bayer[16] = float[16](
    0.0/16.0,  8.0/16.0,  2.0/16.0, 10.0/16.0,
   12.0/16.0,  4.0/16.0, 14.0/16.0,  6.0/16.0,
    3.0/16.0, 11.0/16.0,  1.0/16.0,  9.0/16.0,
   15.0/16.0,  7.0/16.0, 13.0/16.0,  5.0/16.0
);

void main() {
    ivec2 p = ivec2(gl_FragCoord.xy) & ivec2(3, 3);
    float threshold = bayer[p.y * 4 + p.x];

    if (vColor.a < threshold)
        discard;

    float gamma = 2.2;
    fragColor = vec4(pow(vColor.rgb, vec3(1.0/gamma)), 1.0);
}
