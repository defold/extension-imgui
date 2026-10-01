struct imgui_uniforms {
  /* @offset(0) */
  ProjMtx : mat4x4f,
}

var<private> Frag_UV : vec2f;

var<private> UV : vec2f;

var<private> Frag_Color : vec4f;

var<private> Color : vec4f;

var<private> gl_Position : vec4f;

@binding(0) @group(0) var<uniform> x_23 : imgui_uniforms;

var<private> Position : vec2f;

fn main_1() {
  Frag_UV = UV;
  Frag_Color = Color;
  gl_Position = (x_23.ProjMtx * vec4f(Position.x, Position.y, 0.0f, 1.0f));
  return;
}

struct main_out {
  @location(0)
  Frag_UV_1 : vec2f,
  @location(1)
  Frag_Color_1 : vec4f,
  @builtin(position)
  gl_Position_1 : vec4f,
}

@vertex
fn main(@location(1) UV_param : vec2f, @location(2) Color_param : vec4f, @location(0) Position_param : vec2f) -> main_out {
  UV = UV_param;
  Color = Color_param;
  Position = Position_param;
  main_1();
  return main_out(Frag_UV, Frag_Color, gl_Position);
}

// defold-webgpu-flipped-entry-point: _defold_webgpu_main_flipped
@vertex
fn _defold_webgpu_main_flipped(@location(1) UV_param : vec2f, @location(2) Color_param : vec4f, @location(0) Position_param : vec2f) -> main_out {
  UV = UV_param;
  Color = Color_param;
  Position = Position_param;
  main_1();
  gl_Position.y = -gl_Position.y;
  return main_out(Frag_UV, Frag_Color, gl_Position);
}
