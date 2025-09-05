@ctype vec2 v2

// shared code for all shaders
@block uniforms
layout(binding=2) uniform s_params {
  int pixel_perfect;
  float time;
};
layout(binding=3) uniform s_colors {
  vec3 color_black;
  vec3 color_white;
  vec3 color_debug;
};
layout(binding=4) uniform s_buffer_params {
  vec2 offset;
  vec2 size;
  vec2 app_size;
  vec2 win_size;
  vec2 scale;
};
@end

#pragma sokol @vs vs

@include_block uniforms

in vec4 pos;
in vec2 texcoord0;
out vec2 uv;

void main() {
    gl_Position = pos;
    uv = texcoord0;

}
#pragma sokol @end

#pragma sokol @fs fs

@include_block uniforms

out vec4 frag_color;
in vec2 uv;

layout(binding=0) uniform texture2D tex;
layout(binding=0) uniform sampler smp;
layout(binding=1) uniform texture2D tex_debug;

// https://jorenjoestar.github.io/post/pixel_art_filtering/
vec2 uv_iq(vec2 uv, ivec2 texture_size) {
    vec2 pixel = uv * vec2(texture_size);

    vec2 seam = floor(pixel + 0.5);
    vec2 dudv = fwidth(pixel);
    pixel = seam + clamp((pixel - seam) / dudv, -0.5, 0.5);

    return pixel / vec2(texture_size);
}

void main() {
  vec2 frag_coord = uv * win_size;
  vec2 rel = frag_coord - offset;

  if (
      rel.x < 0.0 ||
      rel.y < 0.0 ||
      rel.x >= size.x ||
      rel.y >= size.y ) {
    frag_color = vec4(color_black, 1.0);
    return;
  }

  vec2 tex_uv = rel / size;
  tex_uv.y = 1.0 - tex_uv.y;
  if(pixel_perfect != 1){
    tex_uv = uv_iq(tex_uv, ivec2(app_size));
  }
  vec4 app_sample = texture(sampler2D(tex, smp), tex_uv);
  vec4 debug_sample = texture(sampler2D(tex_debug, smp), tex_uv);
  // vec3 app_color = (app_sample.r > 0.0) ? color_white : color_black;
  vec3 app_color = app_sample.rgb;
  vec4 col = mix(vec4(app_color, 1.0), vec4(color_debug, 1.0), debug_sample.r > 0.0 ? 0.5 : 0.0);
  frag_color = col;
}
#pragma sokol @end

@program simple vs fs

