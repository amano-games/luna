#pragma sokol @vs vs

in vec4 pos;
in vec2 texcoord0;
out vec2 uv;

void main() {
    gl_Position = pos;
    uv = texcoord0;

}
#pragma sokol @end

#pragma sokol @fs fs
out vec4 frag_color;
in vec2 uv;

layout(binding=0) uniform texture2D tex;
layout(binding=1) uniform texture2D tex_debug;
layout(binding=0) uniform sampler smp;

void main() {
  vec2 pos = vec2(uv.x, 1.0 - uv.y);
  vec4 app_sample = texture(sampler2D(tex, smp), pos);
  vec4 debug_sample = texture(sampler2D(tex_debug, smp), pos);
  vec4 debug_color = vec4(1, 0, 0, 1.0);
  vec4 app_white_color = vec4(0.64, 0.64, 0.64, 1.0);
  vec4 app_black_color= vec4(0.05, 0.04, 0.06, 1.0);
  // vec4 app_color = mix(app_black_color, app_white_color, app_sample.r);
  vec4 app_color = app_sample;

  vec4 col = mix(app_color, debug_color, debug_sample.r > 0 ? 0.5: 0);
  frag_color = col;
}
#pragma sokol @end

@program simple vs fs
