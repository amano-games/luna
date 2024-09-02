#pragma sokol @vs vs

in vec4 pos;
in vec2 texcoord0;

out vec2 uv;

void main() {
    gl_Position = pos;
    // color = color0;
    uv = texcoord0;
}
#pragma sokol @end

#pragma sokol @fs fs
out vec4 frag_color;


in vec2 uv;
uniform texture2D tex;
uniform sampler smp;

void main() {
    frag_color = texture(sampler2D(tex, smp), vec2(uv.x,1.0 - uv.y));
}
#pragma sokol @end

@program simple vs fs
