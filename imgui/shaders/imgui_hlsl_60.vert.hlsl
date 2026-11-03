#define IMGUI_ROOT_SIGNATURE "CBV(b0,space=0,visibility=SHADER_VISIBILITY_VERTEX),DescriptorTable(SRV(t0,space=1),visibility=SHADER_VISIBILITY_PIXEL),DescriptorTable(Sampler(s0,space=1),visibility=SHADER_VISIBILITY_PIXEL),RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT|DENY_HULL_SHADER_ROOT_ACCESS|DENY_DOMAIN_SHADER_ROOT_ACCESS|DENY_GEOMETRY_SHADER_ROOT_ACCESS)"

cbuffer imgui_uniforms : register(b0, space0)
{
    row_major float4x4 _23_ProjMtx : packoffset(c0);
};

static float4 gl_Position;
static float2 Frag_UV;
static float2 UV;
static float4 Frag_Color;
static float4 Color;
static float2 Position;

struct SPIRV_Cross_Input
{
    float2 Position : TEXCOORD0;
    float2 UV : TEXCOORD1;
    float4 Color : TEXCOORD2;
};

struct SPIRV_Cross_Output
{
    float2 Frag_UV : TEXCOORD0;
    float4 Frag_Color : TEXCOORD1;
    float4 gl_Position : SV_Position;
};

void vert_main()
{
    Frag_UV = UV;
    Frag_Color = Color;
    gl_Position = mul(float4(Position, 0.0f, 1.0f), _23_ProjMtx);
}

[RootSignature(IMGUI_ROOT_SIGNATURE)]
SPIRV_Cross_Output main(SPIRV_Cross_Input stage_input)
{
    UV = stage_input.UV;
    Color = stage_input.Color;
    Position = stage_input.Position;
    vert_main();
    SPIRV_Cross_Output stage_output;
    stage_output.gl_Position = gl_Position;
    stage_output.Frag_UV = Frag_UV;
    stage_output.Frag_Color = Frag_Color;
    return stage_output;
}
