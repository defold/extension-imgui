#define IMGUI_ROOT_SIGNATURE "CBV(b0,space=0,visibility=SHADER_VISIBILITY_VERTEX),DescriptorTable(SRV(t0,space=1),visibility=SHADER_VISIBILITY_PIXEL),DescriptorTable(Sampler(s0,space=1),visibility=SHADER_VISIBILITY_PIXEL),RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT|DENY_HULL_SHADER_ROOT_ACCESS|DENY_DOMAIN_SHADER_ROOT_ACCESS|DENY_GEOMETRY_SHADER_ROOT_ACCESS)"

Texture2D<float4> Texture : register(t0, space1);
SamplerState _Texture_sampler : register(s0, space1);

static float4 Out_Color;
static float4 Frag_Color;
static float2 Frag_UV;

struct SPIRV_Cross_Input
{
    float2 Frag_UV : TEXCOORD0;
    float4 Frag_Color : TEXCOORD1;
};

struct SPIRV_Cross_Output
{
    float4 Out_Color : SV_Target0;
};

void frag_main()
{
    Out_Color = Frag_Color * Texture.Sample(_Texture_sampler, Frag_UV);
}

[RootSignature(IMGUI_ROOT_SIGNATURE)]
SPIRV_Cross_Output main(SPIRV_Cross_Input stage_input)
{
    Frag_Color = stage_input.Frag_Color;
    Frag_UV = stage_input.Frag_UV;
    frag_main();
    SPIRV_Cross_Output stage_output;
    stage_output.Out_Color = Out_Color;
    return stage_output;
}
