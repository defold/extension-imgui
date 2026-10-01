#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct spvDescriptorSetBuffer0
{
    texture2d<float> m_30 [[id(0)]];
    sampler m_30Smplr [[id(1)]];
};

struct spvDescriptorSetBuffer1
{
    texture2d<float> Texture [[id(0)]];
    sampler Texture_separated [[id(1)]];
};

struct main0_out
{
    float4 Out_Color [[color(0)]];
};

struct main0_in
{
    float2 Frag_UV [[user(locn0)]];
    float4 Frag_Color [[user(locn1)]];
};

fragment main0_out main0(main0_in in [[stage_in]], constant spvDescriptorSetBuffer0& spvDescriptorSet0 [[buffer(0)]], constant spvDescriptorSetBuffer1& spvDescriptorSet1 [[buffer(1)]])
{
    main0_out out = {};
    out.Out_Color = in.Frag_Color * spvDescriptorSet1.Texture.sample(spvDescriptorSet1.Texture_separated, in.Frag_UV);
    return out;
}

