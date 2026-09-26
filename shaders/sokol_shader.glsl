@ctype vec2 v2
@ctype vec4 v4

// shared code for all shaders
@block uniforms
layout(binding=2) uniform s_params {
  // Matches enum sys_video_filter ordinals in sys-opts.h
  int filter_mode;
  float time;
};
layout(binding=3) uniform s_colors {
  vec4 app_colors[256];
  vec4 dbg_colors[256];
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

vec3 palette_color( ivec2 pos) {
  ivec2 tex_size = textureSize(sampler2D(tex, smp), 0);
  pos = clamp(pos, ivec2(0), tex_size - 1);
  int index = int(texelFetch(sampler2D(tex, smp), pos, 0).r * 255.0 + 0.5);
  return app_colors[index].rgb;
}

vec3 sample_palette(vec2 sample_uv) {
  vec2 tex_size = vec2(textureSize(sampler2D(tex, smp), 0));

  if(filter_mode == 1) { // SYS_VIDEO_FILTER_NEAREST == 1
    return palette_color(ivec2(floor(sample_uv * tex_size)));
  }

  // Blend colors, never palette indices.
  vec2 pixel = sample_uv * tex_size - 0.5;
  ivec2 base = ivec2(floor(pixel));
  vec2 weight = fract(pixel);
  vec3 top = mix(palette_color( base), palette_color(base + ivec2(1, 0)), weight.x);
  vec3 bottom = mix(palette_color( base + ivec2(0, 1)), palette_color(base + ivec2(1, 1)), weight.x);
  return mix(top, bottom, weight.y);
}

void main() {
  vec2 frag_coord = uv * win_size;
  vec2 rel = frag_coord - offset;

  if (
      rel.x < 0.0 ||
      rel.y < 0.0 ||
      rel.x >= size.x ||
      rel.y >= size.y ) {
    frag_color = vec4(app_colors[0].rgb, 1.0);
    return;
  }

  vec2 tex_uv = rel / size;
  tex_uv.y = 1.0 - tex_uv.y;

  // TODO: re-use palette_color
  ivec2 debug_size = textureSize(sampler2D(tex_debug, smp), 0);
  ivec2 debug_pos = clamp(ivec2(tex_uv * vec2(debug_size)), ivec2(0), debug_size - 1);
  int debug_index = int(texelFetch(sampler2D(tex_debug, smp), debug_pos, 0).r * 255.0 + 0.5);
  vec4 debug_color = dbg_colors[debug_index];

  // SYS_VIDEO_FILTER_SHARP == 3
  if(filter_mode == 3){
    tex_uv = uv_iq(tex_uv, ivec2(app_size));
  }
  vec3 app_color = sample_palette(tex_uv);
  vec4 col = vec4(mix(app_color, debug_color.rgb, debug_color.a), 1.0);
  frag_color = col;
  // frag_color = vec4(app_color, 1.0);
}
#pragma sokol @end

@program simple vs fs
