var<private> Out_Color : vec4f;

var<private> Frag_Color : vec4f;

@binding(0) @group(1) var Texture : texture_2d<f32>;

@binding(1) @group(1) var Texture_separated : sampler;

var<private> Frag_UV : vec2f;

fn main_1() {
  Out_Color = (Frag_Color * textureSample(Texture, Texture_separated, Frag_UV));
  return;
}

struct main_out {
  @location(0)
  Out_Color_1 : vec4f,
}

@fragment
fn main(@location(1) Frag_Color_param : vec4f, @location(0) Frag_UV_param : vec2f) -> main_out {
  Frag_Color = Frag_Color_param;
  Frag_UV = Frag_UV_param;
  main_1();
  return main_out(Out_Color);
}
