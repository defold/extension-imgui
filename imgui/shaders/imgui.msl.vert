#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct imgui_uniforms
{
    float4x4 ProjMtx;
};

struct spvDescriptorSetBuffer0
{
    constant imgui_uniforms* m_23 [[id(0)]];
};

struct main0_out
{
    float2 Frag_UV [[user(locn0)]];
    float4 Frag_Color [[user(locn1)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float2 Position [[attribute(0)]];
    float2 UV [[attribute(1)]];
    float4 Color [[attribute(2)]];
};

vertex main0_out main0(main0_in in [[stage_in]], constant spvDescriptorSetBuffer0& spvDescriptorSet0 [[buffer(0)]])
{
    main0_out out = {};
    out.Frag_UV = in.UV;
    out.Frag_Color = in.Color;
    out.gl_Position = (*spvDescriptorSet0.m_23).ProjMtx * float4(in.Position, 0.0, 1.0);
    out.gl_Position.y = -(out.gl_Position.y);    // Invert Y-axis for Metal
    return out;
}

