#include "imgui_impl_defold.h"

#include <dmsdk/dlib/array.h>
#include <dmsdk/dlib/hash.h>
#include <dmsdk/dlib/log.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

namespace
{
    const uint32_t SHADER_STAGE_VERTEX   = 0x1;
    const uint32_t SHADER_STAGE_FRAGMENT = 0x2;

    static const dmGraphics::ShaderDesc::MSLResourceMapping MSL_VERTEX_RESOURCES[] = {
        { 5242665690161703654ULL, 0, 0, 0 }, // imgui_uniforms
    };

    static const dmGraphics::ShaderDesc::MSLResourceMapping MSL_FRAGMENT_RESOURCES[] = {
        { 11509546322550963027ULL, 0, 1, 0 }, // Texture
        { 16057607012685438136ULL, 1, 1, 1 }, // Texture_separated
    };

    template <typename T>
    static auto SetHLSLRootParameterIndex(T& mapping, uint32_t index, int) -> decltype(mapping.m_RootParameterIndex = index, void())
    {
        mapping.m_RootParameterIndex = index;
    }

    template <typename T>
    static void SetHLSLRootParameterIndex(T&, uint32_t, ...)
    {
        // Defold 1.14.0 derives root parameter indices from resource order.
    }

    static dmGraphics::ShaderDesc::HLSLResourceMapping CreateHLSLResourceMapping(uint64_t name_hash, uint32_t binding, uint32_t set, uint32_t root_parameter_index)
    {
        dmGraphics::ShaderDesc::HLSLResourceMapping mapping = {};
        mapping.m_NameHash = name_hash;
        mapping.m_Binding = binding;
        mapping.m_Set = set;
        SetHLSLRootParameterIndex(mapping, root_parameter_index, 0);
        return mapping;
    }

    // Keep vertex then fragment resource order aligned with IMGUI_ROOT_SIGNATURE
    // for Defold 1.14.0; newer SDKs also use the explicit root parameter indices.
    // Binding/set refer to ShaderDesc reflection, rather than HLSL register numbers.
    static const dmGraphics::ShaderDesc::HLSLResourceMapping HLSL_VERTEX_RESOURCES[] = {
        CreateHLSLResourceMapping(5242665690161703654ULL, 0, 0, 0), // imgui_uniforms -> CBV(b0, space0)
    };

    static const dmGraphics::ShaderDesc::HLSLResourceMapping HLSL_FRAGMENT_RESOURCES[] = {
        CreateHLSLResourceMapping(11509546322550963027ULL, 0, 1, 1), // Texture -> SRV(t0, space1)
        CreateHLSLResourceMapping(16057607012685438136ULL, 1, 1, 2), // Texture_separated -> Sampler(s0, space1)
    };

    static const char* HLSL_ROOT_SIGNATURE_PATH = "/imgui/shaders/imgui_hlsl_51.rootsig";

    struct ShaderResourceSpec
    {
        const char*                         m_Path;
        dmGraphics::ShaderDesc::ShaderType  m_Type;
        dmGraphics::ShaderDesc::Language    m_Language;
        const dmGraphics::ShaderDesc::MSLResourceMapping* m_MslResources;
        uint32_t                            m_MslResourceCount;
    };

    static const ShaderResourceSpec STANDARD_SHADER_RESOURCES[] = {
        { "/imgui/shaders/imgui_glsl_330.vert.glsl", dmGraphics::ShaderDesc::SHADER_TYPE_VERTEX,   dmGraphics::ShaderDesc::LANGUAGE_GLSL_SM330, 0, 0 },
        { "/imgui/shaders/imgui_glsl_330.frag.glsl", dmGraphics::ShaderDesc::SHADER_TYPE_FRAGMENT, dmGraphics::ShaderDesc::LANGUAGE_GLSL_SM330, 0, 0 },
        { "/imgui/shaders/imgui_gles_300.vert.glsl", dmGraphics::ShaderDesc::SHADER_TYPE_VERTEX,   dmGraphics::ShaderDesc::LANGUAGE_GLES_SM300, 0, 0 },
        { "/imgui/shaders/imgui_gles_300.frag.glsl", dmGraphics::ShaderDesc::SHADER_TYPE_FRAGMENT, dmGraphics::ShaderDesc::LANGUAGE_GLES_SM300, 0, 0 },
        { "/imgui/shaders/imgui_gles_100.vert.glsl", dmGraphics::ShaderDesc::SHADER_TYPE_VERTEX,   dmGraphics::ShaderDesc::LANGUAGE_GLES_SM100, 0, 0 },
        { "/imgui/shaders/imgui_gles_100.frag.glsl", dmGraphics::ShaderDesc::SHADER_TYPE_FRAGMENT, dmGraphics::ShaderDesc::LANGUAGE_GLES_SM100, 0, 0 },
        { "/imgui/shaders/imgui.spv.vert",           dmGraphics::ShaderDesc::SHADER_TYPE_VERTEX,   dmGraphics::ShaderDesc::LANGUAGE_SPIRV, 0, 0 },
        { "/imgui/shaders/imgui.spv.frag",           dmGraphics::ShaderDesc::SHADER_TYPE_FRAGMENT, dmGraphics::ShaderDesc::LANGUAGE_SPIRV, 0, 0 },
    };

    static const ShaderResourceSpec METAL_SHADER_RESOURCES[] = {
        { "/imgui/shaders/imgui.msl.vert", dmGraphics::ShaderDesc::SHADER_TYPE_VERTEX,   dmGraphics::ShaderDesc::LANGUAGE_MSL_22, MSL_VERTEX_RESOURCES, 1 },
        { "/imgui/shaders/imgui.msl.frag", dmGraphics::ShaderDesc::SHADER_TYPE_FRAGMENT, dmGraphics::ShaderDesc::LANGUAGE_MSL_22, MSL_FRAGMENT_RESOURCES, 2 },
    };

    static const ShaderResourceSpec WEBGPU_SHADER_RESOURCES[] = {
        { "/imgui/shaders/imgui.wgsl.vert", dmGraphics::ShaderDesc::SHADER_TYPE_VERTEX,   dmGraphics::ShaderDesc::LANGUAGE_WGSL, 0, 0 },
        { "/imgui/shaders/imgui.wgsl.frag", dmGraphics::ShaderDesc::SHADER_TYPE_FRAGMENT, dmGraphics::ShaderDesc::LANGUAGE_WGSL, 0, 0 },
    };

    static const ShaderResourceSpec DIRECTX_SHADER_RESOURCES[] = {
        { "/imgui/shaders/imgui_hlsl_51.vert.dxbc", dmGraphics::ShaderDesc::SHADER_TYPE_VERTEX,   dmGraphics::ShaderDesc::LANGUAGE_HLSL_51, 0, 0 },
        { "/imgui/shaders/imgui_hlsl_51.frag.dxbc", dmGraphics::ShaderDesc::SHADER_TYPE_FRAGMENT, dmGraphics::ShaderDesc::LANGUAGE_HLSL_51, 0, 0 },
    };

    static const uint32_t MAX_SHADER_RESOURCE_COUNT = sizeof(STANDARD_SHADER_RESOURCES) / sizeof(STANDARD_SHADER_RESOURCES[0]);

    struct LoadedShaderResource
    {
        void*    m_Data;
        uint32_t m_Size;
    };

    struct TextureRef
    {
        uint32_t             m_Id;
        dmGraphics::HTexture m_Texture;
    };

    struct Renderer
    {
        dmGraphics::HContext                 m_Context;
        dmGraphics::HProgram                 m_Program;
        dmGraphics::HVertexStreamDeclaration m_StreamDeclaration;
        dmGraphics::HVertexDeclaration       m_VertexDeclaration;
        dmGraphics::HVertexBuffer            m_VertexBuffer;
        dmGraphics::HIndexBuffer             m_IndexBuffer;
        dmGraphics::HUniformLocation         m_TextureLocation;
        dmGraphics::HUniformLocation         m_ProjMtxLocation;
        dmResource::HFactory                 m_ResourceFactory;
        uint32_t                             m_NextTextureId;
        dmArray<TextureRef>                  m_Textures;

        // Offscreen UI target: imgui.render_to_texture() draws the current frame
        // into this texture so a render script can sample and composite it into
        // its own pipeline (which is what makes engine post processing apply to UI).
        dmGraphics::HRenderTarget            m_OffscreenTarget;
        dmGraphics::HTexture                 m_OffscreenTexture;
        uint32_t                             m_OffscreenWidth;
        uint32_t                             m_OffscreenHeight;
        // True while drawing into the offscreen target: its scissor origin is the
        // bottom-left corner, unlike the swapchain (see SetClipRect).
        bool                                 m_RenderingToTexture;
    };

    Renderer g_Renderer = {};

    // Defined further down together with the other offscreen target helpers;
    // forward declared here so shutdown can release the target.
    static void DestroyOffscreenTarget();

    static const ShaderResourceSpec* GetShaderResources(uint32_t* count, bool* separate_texture_sampler)
    {
        dmGraphics::AdapterFamily family = dmGraphics::GetInstalledAdapterFamily();
        if (family == dmGraphics::ADAPTER_FAMILY_DIRECTX)
        {
            *count = sizeof(DIRECTX_SHADER_RESOURCES) / sizeof(DIRECTX_SHADER_RESOURCES[0]);
            *separate_texture_sampler = true;
            return DIRECTX_SHADER_RESOURCES;
        }
        if (family == dmGraphics::ADAPTER_FAMILY_METAL)
        {
            *count = sizeof(METAL_SHADER_RESOURCES) / sizeof(METAL_SHADER_RESOURCES[0]);
            *separate_texture_sampler = true;
            return METAL_SHADER_RESOURCES;
        }
        if (family == dmGraphics::ADAPTER_FAMILY_WEBGPU)
        {
            *count = sizeof(WEBGPU_SHADER_RESOURCES) / sizeof(WEBGPU_SHADER_RESOURCES[0]);
            *separate_texture_sampler = true;
            return WEBGPU_SHADER_RESOURCES;
        }

        *count = sizeof(STANDARD_SHADER_RESOURCES) / sizeof(STANDARD_SHADER_RESOURCES[0]);
        *separate_texture_sampler = false;
        return STANDARD_SHADER_RESOURCES;
    }

    static void FreeShaderResources(LoadedShaderResource* resources, uint32_t resource_count)
    {
        for (uint32_t i = 0; i < resource_count; ++i)
        {
            free(resources[i].m_Data);
            resources[i].m_Data = 0;
            resources[i].m_Size = 0;
        }
    }

    static bool LoadShaderResources(const ShaderResourceSpec* specs, uint32_t resource_count, LoadedShaderResource* resources)
    {
        memset(resources, 0, sizeof(LoadedShaderResource) * resource_count);
        for (uint32_t i = 0; i < resource_count; ++i)
        {
            dmResource::Result result = dmResource::GetRaw(g_Renderer.m_ResourceFactory, specs[i].m_Path, &resources[i].m_Data, &resources[i].m_Size);
            if (result != dmResource::RESULT_OK)
            {
                dmLogError("Failed to load ImGui shader resource '%s' (%d)", specs[i].m_Path, result);
                FreeShaderResources(resources, resource_count);
                return false;
            }
        }
        return true;
    }

    static dmGraphics::ShaderDesc::Shader CreateShader(const ShaderResourceSpec& spec, const void* data, uint32_t data_size)
    {
        dmGraphics::ShaderDesc::Shader shader = {};
        shader.m_ShaderType     = spec.m_Type;
        shader.m_Language       = spec.m_Language;
        shader.m_Source.m_Data  = (uint8_t*) data;
        shader.m_Source.m_Count = data_size;
        shader.m_MslResourceMapping.m_Data  = (dmGraphics::ShaderDesc::MSLResourceMapping*) spec.m_MslResources;
        shader.m_MslResourceMapping.m_Count = spec.m_MslResourceCount;
        if (spec.m_Language == dmGraphics::ShaderDesc::LANGUAGE_HLSL_51)
        {
            bool vertex = spec.m_Type == dmGraphics::ShaderDesc::SHADER_TYPE_VERTEX;
            shader.m_HlslResourceMapping.m_Data = (dmGraphics::ShaderDesc::HLSLResourceMapping*) (vertex ? HLSL_VERTEX_RESOURCES : HLSL_FRAGMENT_RESOURCES);
            shader.m_HlslResourceMapping.m_Count = vertex ?
                sizeof(HLSL_VERTEX_RESOURCES) / sizeof(HLSL_VERTEX_RESOURCES[0]) :
                sizeof(HLSL_FRAGMENT_RESOURCES) / sizeof(HLSL_FRAGMENT_RESOURCES[0]);
        }
        return shader;
    }

    static dmGraphics::ShaderDesc::ResourceBinding CreateBinding(const char* name, uint32_t binding, uint32_t set, dmGraphics::ShaderDesc::ShaderDataType type, uint32_t stage_flags)
    {
        dmGraphics::ShaderDesc::ResourceBinding res = {};
        res.m_Name                     = name;
        res.m_NameHash                 = dmHashString64(name);
        res.m_Binding                  = binding;
        res.m_Set                      = set;
        res.m_StageFlags               = stage_flags;
        res.m_Type.m_UseTypeIndex      = false;
        res.m_Type.m_Type.m_ShaderType = type;
        return res;
    }

    static dmGraphics::ShaderDesc::ResourceBinding CreateUniformBufferBinding(const char* name, uint32_t binding, uint32_t set, int32_t type_index, uint32_t block_size, uint32_t stage_flags)
    {
        dmGraphics::ShaderDesc::ResourceBinding res = {};
        res.m_Name                    = name;
        res.m_NameHash                = dmHashString64(name);
        res.m_Binding                 = binding;
        res.m_Set                     = set;
        res.m_StageFlags              = stage_flags;
        res.m_Type.m_UseTypeIndex     = true;
        res.m_Type.m_Type.m_TypeIndex = type_index;
        res.m_Bindinginfo.m_BlockSize = block_size;
        return res;
    }

    static dmGraphics::ShaderDesc::ResourceMember CreateMember(const char* name, dmGraphics::ShaderDesc::ShaderDataType type, uint32_t offset)
    {
        dmGraphics::ShaderDesc::ResourceMember member = {};
        member.m_Name                     = name;
        member.m_NameHash                 = dmHashString64(name);
        member.m_Offset                   = offset;
        member.m_ElementCount             = 1;
        member.m_Type.m_UseTypeIndex      = false;
        member.m_Type.m_Type.m_ShaderType = type;
        return member;
    }

    static dmGraphics::ShaderDesc CreateShaderDesc(const ShaderResourceSpec* specs,
                                                   uint32_t shader_resource_count,
                                                   bool separate_texture_sampler,
                                                   const LoadedShaderResource* shader_resources,
                                                   dmArray<dmGraphics::ShaderDesc::Shader>& shaders,
                                                   dmArray<dmGraphics::ShaderDesc::ResourceBinding>& inputs,
                                                   dmArray<dmGraphics::ShaderDesc::ResourceBinding>& textures,
                                                   dmArray<dmGraphics::ShaderDesc::ResourceBinding>& uniform_buffers,
                                                   dmArray<dmGraphics::ShaderDesc::ResourceTypeInfo>& types,
                                                   dmArray<dmGraphics::ShaderDesc::ResourceMember>& members)
    {
        shaders.OffsetCapacity(shader_resource_count);
        for (uint32_t i = 0; i < shader_resource_count; ++i)
        {
            shaders.Push(CreateShader(specs[i], shader_resources[i].m_Data, shader_resources[i].m_Size));
        }

        inputs.OffsetCapacity(3);
        inputs.Push(CreateBinding("Position", 0, 0, dmGraphics::ShaderDesc::SHADER_TYPE_VEC2, SHADER_STAGE_VERTEX));
        inputs.Push(CreateBinding("UV", 1, 0, dmGraphics::ShaderDesc::SHADER_TYPE_VEC2, SHADER_STAGE_VERTEX));
        inputs.Push(CreateBinding("Color", 2, 0, dmGraphics::ShaderDesc::SHADER_TYPE_VEC4, SHADER_STAGE_VERTEX));

        textures.OffsetCapacity(separate_texture_sampler ? 2 : 1);
        if (separate_texture_sampler)
        {
            textures.Push(CreateBinding("Texture", 0, 1, dmGraphics::ShaderDesc::SHADER_TYPE_TEXTURE2D, SHADER_STAGE_FRAGMENT));
            dmGraphics::ShaderDesc::ResourceBinding sampler = CreateBinding("Texture_separated", 1, 1, dmGraphics::ShaderDesc::SHADER_TYPE_SAMPLER, SHADER_STAGE_FRAGMENT);
            sampler.m_Bindinginfo.m_SamplerTextureIndex = 0;
            textures.Push(sampler);
        }
        else
        {
            textures.Push(CreateBinding("Texture", 1, 0, dmGraphics::ShaderDesc::SHADER_TYPE_SAMPLER2D, SHADER_STAGE_FRAGMENT));
        }

        members.OffsetCapacity(1);
        members.Push(CreateMember("ProjMtx", dmGraphics::ShaderDesc::SHADER_TYPE_MAT4, 0));

        types.OffsetCapacity(1);
        dmGraphics::ShaderDesc::ResourceTypeInfo type_info = {};
        type_info.m_Name            = "imgui_uniforms";
        type_info.m_NameHash        = dmHashString64(type_info.m_Name);
        type_info.m_Members.m_Data  = members.Begin();
        type_info.m_Members.m_Count = members.Size();
        types.Push(type_info);

        uniform_buffers.OffsetCapacity(1);
        uniform_buffers.Push(CreateUniformBufferBinding("imgui_uniforms", 0, 0, 0, 64, SHADER_STAGE_VERTEX));

        dmGraphics::ShaderDesc desc = {};
        desc.m_Shaders.m_Data                        = shaders.Begin();
        desc.m_Shaders.m_Count                       = shaders.Size();
        desc.m_Reflection.m_Inputs.m_Data            = inputs.Begin();
        desc.m_Reflection.m_Inputs.m_Count           = inputs.Size();
        desc.m_Reflection.m_Textures.m_Data          = textures.Begin();
        desc.m_Reflection.m_Textures.m_Count         = textures.Size();
        desc.m_Reflection.m_UniformBuffers.m_Data    = uniform_buffers.Begin();
        desc.m_Reflection.m_UniformBuffers.m_Count   = uniform_buffers.Size();
        desc.m_Reflection.m_Types.m_Data             = types.Begin();
        desc.m_Reflection.m_Types.m_Count            = types.Size();
        return desc;
    }

    static void SetState(dmGraphics::HContext context, dmGraphics::State state, bool enabled)
    {
        if (enabled)
            dmGraphics::EnableState(context, state);
        else
            dmGraphics::DisableState(context, state);
    }

    static void RestorePipelineState(dmGraphics::HContext context, const dmGraphics::PipelineState& state)
    {
        SetState(context, dmGraphics::STATE_BLEND, state.m_BlendEnabled);
        SetState(context, dmGraphics::STATE_CULL_FACE, state.m_CullFaceEnabled);
        SetState(context, dmGraphics::STATE_DEPTH_TEST, state.m_DepthTestEnabled);
        SetState(context, dmGraphics::STATE_STENCIL_TEST, state.m_StencilEnabled);
        SetState(context, dmGraphics::STATE_SCISSOR_TEST, state.m_ScissorTestEnabled);
        dmGraphics::SetBlendFuncSeparate(context,
            (dmGraphics::BlendFactor) state.m_BlendSrcFactor,
            (dmGraphics::BlendFactor) state.m_BlendDstFactor,
            (dmGraphics::BlendFactor) state.m_BlendSrcFactorAlpha,
            (dmGraphics::BlendFactor) state.m_BlendDstFactorAlpha);
        dmGraphics::SetBlendEquationSeparate(context,
            (dmGraphics::BlendEquation) state.m_BlendEquationColor,
            (dmGraphics::BlendEquation) state.m_BlendEquationAlpha);
        dmGraphics::SetColorMask(context,
            (state.m_WriteColorMask & 0x1) != 0,
            (state.m_WriteColorMask & 0x2) != 0,
            (state.m_WriteColorMask & 0x4) != 0,
            (state.m_WriteColorMask & 0x8) != 0);
        dmGraphics::SetDepthMask(context, state.m_WriteDepth);
        dmGraphics::SetDepthFunc(context, (dmGraphics::CompareFunc) state.m_DepthTestFunc);
        dmGraphics::SetCullFace(context, (dmGraphics::FaceType) state.m_CullFaceType);
    }

    static void SetupRenderState(ImDrawData* draw_data, int fb_width, int fb_height)
    {
        dmGraphics::HContext context = g_Renderer.m_Context;

        dmGraphics::EnableState(context, dmGraphics::STATE_BLEND);
        dmGraphics::SetBlendEquationSeparate(context, dmGraphics::BLEND_EQUATION_ADD, dmGraphics::BLEND_EQUATION_ADD);
        dmGraphics::SetBlendFuncSeparate(context, dmGraphics::BLEND_FACTOR_SRC_ALPHA, dmGraphics::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, dmGraphics::BLEND_FACTOR_ONE, dmGraphics::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
        dmGraphics::DisableState(context, dmGraphics::STATE_CULL_FACE);
        dmGraphics::DisableState(context, dmGraphics::STATE_DEPTH_TEST);
        dmGraphics::DisableState(context, dmGraphics::STATE_STENCIL_TEST);
        dmGraphics::EnableState(context, dmGraphics::STATE_SCISSOR_TEST);
        dmGraphics::SetDepthMask(context, false);
        dmGraphics::SetColorMask(context, true, true, true, true);
        dmGraphics::SetViewport(context, 0, 0, fb_width, fb_height);

        const float L = draw_data->DisplayPos.x;
        const float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
        const float T = draw_data->DisplayPos.y;
        const float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
        dmVMath::Matrix4 projection(
            dmVMath::Vector4(2.0f / (R - L), 0.0f, 0.0f, 0.0f),
            dmVMath::Vector4(0.0f, 2.0f / (T - B), 0.0f, 0.0f),
            dmVMath::Vector4(0.0f, 0.0f, -1.0f, 0.0f),
            dmVMath::Vector4((R + L) / (L - R), (T + B) / (B - T), 0.0f, 1.0f));

        dmGraphics::EnableProgram(context, g_Renderer.m_Program);
        dmGraphics::SetConstantM4(context, &projection, 1, g_Renderer.m_ProjMtxLocation);
        dmGraphics::SetSampler(context, g_Renderer.m_TextureLocation, 0);
        dmGraphics::EnableVertexBuffer(context, g_Renderer.m_VertexBuffer, 0);
    }

    static void SetClipRect(dmGraphics::HContext context, const ImVec2& clip_min, const ImVec2& clip_max, int fb_width, int fb_height)
    {
        int32_t x = (int32_t) clip_min.x;
        int32_t y = (int32_t) clip_min.y;
        int32_t width = (int32_t) (clip_max.x - clip_min.x);
        int32_t height = (int32_t) (clip_max.y - clip_min.y);

        if (x < 0)
        {
            width += x;
            x = 0;
        }
        if (y < 0)
        {
            height += y;
            y = 0;
        }
        if (x + width > fb_width)
            width = fb_width - x;
        if (y + height > fb_height)
            height = fb_height - y;

        if (width < 0)
            width = 0;
        if (height < 0)
            height = 0;

        dmGraphics::AdapterFamily family = dmGraphics::GetInstalledAdapterFamily();
        // Defold's Metal backbuffer path flips scissor Y internally, so it needs
        // the same bottom-left coordinates as OpenGL here.
        //
        // Render targets need the same treatment: on the swapchain the engine flips
        // the image with a negative-height viewport while the scissor keeps a
        // top-left origin; a render target has no such flip, so its scissor origin
        // is bottom-left and the rect has to be flipped explicitly. Full-screen clip
        // rects are unaffected (a mirrored full-screen rect is itself), but smaller
        // ones such as popups or tooltips would be clipped at the mirrored position.
        if (family == dmGraphics::ADAPTER_FAMILY_OPENGL ||
            family == dmGraphics::ADAPTER_FAMILY_OPENGLES ||
            family == dmGraphics::ADAPTER_FAMILY_METAL ||
            g_Renderer.m_RenderingToTexture)
            y = fb_height - (y + height);

        dmGraphics::SetScissor(context, x, y, width, height);
    }

    static bool CreateDeviceObjects()
    {
        if (g_Renderer.m_Program != dmGraphics::INVALID_PROGRAM_HANDLE && g_Renderer.m_Program != 0)
        {
            return true;
        }

        uint32_t shader_resource_count = 0;
        bool separate_texture_sampler = false;
        const ShaderResourceSpec* specs = GetShaderResources(&shader_resource_count, &separate_texture_sampler);
        LoadedShaderResource shader_resources[MAX_SHADER_RESOURCE_COUNT];
        if (!LoadShaderResources(specs, shader_resource_count, shader_resources))
            return false;

        dmArray<dmGraphics::ShaderDesc::Shader> shaders;
        dmArray<dmGraphics::ShaderDesc::ResourceBinding> inputs;
        dmArray<dmGraphics::ShaderDesc::ResourceBinding> textures;
        dmArray<dmGraphics::ShaderDesc::ResourceBinding> uniform_buffers;
        dmArray<dmGraphics::ShaderDesc::ResourceTypeInfo> types;
        dmArray<dmGraphics::ShaderDesc::ResourceMember> members;
        dmGraphics::ShaderDesc shader_desc = CreateShaderDesc(specs, shader_resource_count, separate_texture_sampler, shader_resources, shaders, inputs, textures, uniform_buffers, types, members);

        LoadedShaderResource root_signature = {};
        if (dmGraphics::GetInstalledAdapterFamily() == dmGraphics::ADAPTER_FAMILY_DIRECTX)
        {
            dmResource::Result result = dmResource::GetRaw(g_Renderer.m_ResourceFactory, HLSL_ROOT_SIGNATURE_PATH, &root_signature.m_Data, &root_signature.m_Size);
            if (result != dmResource::RESULT_OK)
            {
                dmLogError("Failed to load ImGui root signature resource '%s' (%d)", HLSL_ROOT_SIGNATURE_PATH, result);
                FreeShaderResources(shader_resources, shader_resource_count);
                return false;
            }
            shader_desc.m_HlslRootSignature.m_Data = (uint8_t*) root_signature.m_Data;
            shader_desc.m_HlslRootSignature.m_Count = root_signature.m_Size;
        }

        char error_buffer[1024] = {};
        g_Renderer.m_Program = dmGraphics::NewProgram(g_Renderer.m_Context, &shader_desc, error_buffer, sizeof(error_buffer));
        free(root_signature.m_Data);
        FreeShaderResources(shader_resources, shader_resource_count);
        if (g_Renderer.m_Program == dmGraphics::INVALID_PROGRAM_HANDLE || g_Renderer.m_Program == 0)
        {
            dmLogError("Failed to create ImGui dmGraphics shader program: %s", error_buffer);
            return false;
        }

        g_Renderer.m_TextureLocation = dmGraphics::FindUniformLocation(g_Renderer.m_Program, "Texture");
        g_Renderer.m_ProjMtxLocation = dmGraphics::FindUniformLocation(g_Renderer.m_Program, "ProjMtx");
        if (g_Renderer.m_TextureLocation == dmGraphics::INVALID_UNIFORM_LOCATION || g_Renderer.m_ProjMtxLocation == dmGraphics::INVALID_UNIFORM_LOCATION)
        {
            dmLogError("Failed to resolve ImGui dmGraphics shader uniforms");
            dmGraphics::DeleteProgram(g_Renderer.m_Context, g_Renderer.m_Program);
            g_Renderer.m_Program = dmGraphics::INVALID_PROGRAM_HANDLE;
            return false;
        }

        g_Renderer.m_StreamDeclaration = dmGraphics::NewVertexStreamDeclaration(g_Renderer.m_Context);
        if (!g_Renderer.m_StreamDeclaration)
        {
            dmLogError("Failed to create ImGui dmGraphics vertex stream declaration");
            dmGraphics::DeleteProgram(g_Renderer.m_Context, g_Renderer.m_Program);
            g_Renderer.m_Program = dmGraphics::INVALID_PROGRAM_HANDLE;
            return false;
        }

        dmGraphics::AddVertexStream(g_Renderer.m_StreamDeclaration, "Position", 2, dmGraphics::TYPE_FLOAT, false);
        dmGraphics::AddVertexStream(g_Renderer.m_StreamDeclaration, "UV", 2, dmGraphics::TYPE_FLOAT, false);
        dmGraphics::AddVertexStream(g_Renderer.m_StreamDeclaration, "Color", 4, dmGraphics::TYPE_UNSIGNED_BYTE, true);
        g_Renderer.m_VertexDeclaration = dmGraphics::NewVertexDeclaration(g_Renderer.m_Context, g_Renderer.m_StreamDeclaration, sizeof(ImDrawVert));
        g_Renderer.m_VertexBuffer = dmGraphics::NewVertexBuffer(g_Renderer.m_Context, 0, 0, dmGraphics::BUFFER_USAGE_STREAM_DRAW);
        g_Renderer.m_IndexBuffer = dmGraphics::NewIndexBuffer(g_Renderer.m_Context, 0, 0, dmGraphics::BUFFER_USAGE_STREAM_DRAW);
        if (!g_Renderer.m_StreamDeclaration || !g_Renderer.m_VertexDeclaration || !g_Renderer.m_VertexBuffer || !g_Renderer.m_IndexBuffer)
        {
            dmLogError("Failed to create ImGui dmGraphics buffers or vertex declarations");
            if (g_Renderer.m_IndexBuffer)
                dmGraphics::DeleteIndexBuffer(g_Renderer.m_IndexBuffer);
            if (g_Renderer.m_VertexBuffer)
                dmGraphics::DeleteVertexBuffer(g_Renderer.m_VertexBuffer);
            if (g_Renderer.m_VertexDeclaration)
                dmGraphics::DeleteVertexDeclaration(g_Renderer.m_VertexDeclaration);
            if (g_Renderer.m_StreamDeclaration)
                dmGraphics::DeleteVertexStreamDeclaration(g_Renderer.m_StreamDeclaration);
            dmGraphics::DeleteProgram(g_Renderer.m_Context, g_Renderer.m_Program);
            g_Renderer.m_IndexBuffer = 0;
            g_Renderer.m_VertexBuffer = 0;
            g_Renderer.m_VertexDeclaration = 0;
            g_Renderer.m_StreamDeclaration = 0;
            g_Renderer.m_Program = dmGraphics::INVALID_PROGRAM_HANDLE;
            return false;
        }
        return true;
    }

    static void DestroyDeviceObjects()
    {
        if (g_Renderer.m_IndexBuffer)
            dmGraphics::DeleteIndexBuffer(g_Renderer.m_IndexBuffer);
        if (g_Renderer.m_VertexBuffer)
            dmGraphics::DeleteVertexBuffer(g_Renderer.m_VertexBuffer);
        if (g_Renderer.m_VertexDeclaration)
            dmGraphics::DeleteVertexDeclaration(g_Renderer.m_VertexDeclaration);
        if (g_Renderer.m_StreamDeclaration)
            dmGraphics::DeleteVertexStreamDeclaration(g_Renderer.m_StreamDeclaration);
        if (g_Renderer.m_Program && g_Renderer.m_Program != dmGraphics::INVALID_PROGRAM_HANDLE)
            dmGraphics::DeleteProgram(g_Renderer.m_Context, g_Renderer.m_Program);

        g_Renderer.m_IndexBuffer = 0;
        g_Renderer.m_VertexBuffer = 0;
        g_Renderer.m_VertexDeclaration = 0;
        g_Renderer.m_StreamDeclaration = 0;
        g_Renderer.m_Program = dmGraphics::INVALID_PROGRAM_HANDLE;
        g_Renderer.m_TextureLocation = dmGraphics::INVALID_UNIFORM_LOCATION;
        g_Renderer.m_ProjMtxLocation = dmGraphics::INVALID_UNIFORM_LOCATION;
    }



} // namespace

bool ImGui_ImplDefold_Init(dmResource::HFactory resource_factory)
{
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.BackendRendererUserData == 0 && "Already initialized a renderer backend!");

    g_Renderer.m_Context = dmGraphics::GetInstalledContext();
    g_Renderer.m_ResourceFactory = resource_factory;
    g_Renderer.m_Program = dmGraphics::INVALID_PROGRAM_HANDLE;
    g_Renderer.m_NextTextureId = 1;
    g_Renderer.m_StreamDeclaration = 0;
    g_Renderer.m_VertexDeclaration = 0;
    g_Renderer.m_VertexBuffer = 0;
    g_Renderer.m_IndexBuffer = 0;
    g_Renderer.m_TextureLocation = dmGraphics::INVALID_UNIFORM_LOCATION;
    g_Renderer.m_ProjMtxLocation = dmGraphics::INVALID_UNIFORM_LOCATION;

    // 1.92 dynamic font atlas: the backend services create/update/destroy requests from
    // ImGuiPlatformIO::Textures[], baking glyphs on demand at runtime - arbitrary character
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;

    io.BackendRendererUserData = &g_Renderer;
    io.BackendRendererName = "imgui_impl_defold";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    return g_Renderer.m_Context != 0 && g_Renderer.m_ResourceFactory != 0;
}


void ImGui_ImplDefold_Shutdown()
{
    DestroyOffscreenTarget();
    DestroyDeviceObjects();

    // Destroy GPU resources of dynamic textures still referenced (mirrors the official GL backend)
    for (ImTextureData* tex : ImGui::GetPlatformIO().Textures)
    {
        if (tex->RefCount == 1)
        {
            uint32_t id = (uint32_t)(intptr_t) tex->TexID;
            dmGraphics::HTexture texture = ImGui_ImplDefold_GetTexture(id);
            if (texture)
            {
                ImGui_ImplDefold_DestroyTexture(texture);
                ImGui_ImplDefold_UnregisterTexture(id);
            }
            tex->SetTexID(0);
        }
    }
    g_Renderer.m_Textures.SetSize(0);
    g_Renderer.m_Textures.SetCapacity(0);

    ImGuiIO& io = ImGui::GetIO();
    io.BackendFlags &= ~(ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasTextures);
    io.BackendRendererName = 0;
    io.BackendRendererUserData = 0;
}

void ImGui_ImplDefold_NewFrame()
{
    CreateDeviceObjects();
}

dmGraphics::HTexture ImGui_ImplDefold_CreateTexture(int width, int height, const void* rgba_pixels)
{
    dmGraphics::HContext context = dmGraphics::GetInstalledContext();
    dmGraphics::TextureCreationParams creation_params;
    creation_params.m_Type = dmGraphics::TEXTURE_TYPE_2D;
    creation_params.m_Width = (uint16_t) width;
    creation_params.m_Height = (uint16_t) height;
    creation_params.m_OriginalWidth = (uint16_t) width;
    creation_params.m_OriginalHeight = (uint16_t) height;
    creation_params.m_MipMapCount = 1;
    creation_params.m_UsageHintBits = dmGraphics::TEXTURE_USAGE_FLAG_SAMPLE;

    dmGraphics::HTexture texture = dmGraphics::NewTexture(context, creation_params);
    if (!texture)
        return 0;

    dmGraphics::TextureParams params;
    params.m_Data = rgba_pixels;
    params.m_DataSize = width * height * 4;
    params.m_Format = dmGraphics::TEXTURE_FORMAT_RGBA;
    params.m_MinFilter = dmGraphics::TEXTURE_FILTER_LINEAR;
    params.m_MagFilter = dmGraphics::TEXTURE_FILTER_LINEAR;
    params.m_UWrap = dmGraphics::TEXTURE_WRAP_CLAMP_TO_EDGE;
    params.m_VWrap = dmGraphics::TEXTURE_WRAP_CLAMP_TO_EDGE;
    params.m_Width = (uint16_t) width;
    params.m_Height = (uint16_t) height;
    params.m_Depth = 1;
    params.m_LayerCount = 1;
    params.m_MipMap = 0;
    params.m_SubUpdate = false;
    dmGraphics::SetTexture(context, texture, params);
    dmGraphics::SetTextureParams(context, texture, dmGraphics::TEXTURE_FILTER_LINEAR, dmGraphics::TEXTURE_FILTER_LINEAR, dmGraphics::TEXTURE_WRAP_CLAMP_TO_EDGE, dmGraphics::TEXTURE_WRAP_CLAMP_TO_EDGE, 0.0f);
    return texture;
}

void ImGui_ImplDefold_DestroyTexture(dmGraphics::HTexture texture)
{
    if (texture)
        dmGraphics::DeleteTexture(dmGraphics::GetInstalledContext(), texture);
}

uint32_t ImGui_ImplDefold_RegisterTexture(dmGraphics::HTexture texture)
{
    TextureRef ref;
    ref.m_Id = g_Renderer.m_NextTextureId++;
    ref.m_Texture = texture;
    if (g_Renderer.m_Textures.Full())
        g_Renderer.m_Textures.OffsetCapacity(8);
    g_Renderer.m_Textures.Push(ref);
    return ref.m_Id;
}

void ImGui_ImplDefold_UnregisterTexture(uint32_t texture_id)
{
    for (uint32_t i = 0; i < g_Renderer.m_Textures.Size(); ++i)
    {
        if (g_Renderer.m_Textures[i].m_Id == texture_id)
        {
            g_Renderer.m_Textures.EraseSwap(i);
            return;
        }
    }
}

dmGraphics::HTexture ImGui_ImplDefold_GetTexture(uint32_t texture_id)
{
    for (uint32_t i = 0; i < g_Renderer.m_Textures.Size(); ++i)
    {
        if (g_Renderer.m_Textures[i].m_Id == texture_id)
            return g_Renderer.m_Textures[i].m_Texture;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// 1.92 dynamic textures (dynamic font atlas): drive GPU texture creation / sub-region updates /
// destruction according to ImGuiPlatformIO::Textures[] requests. Glyphs are baked on demand at
// runtime; arbitrary character input (e.g. chat) is not limited by pre-baked glyph ranges.
// ---------------------------------------------------------------------------
static void ImGui_ImplDefold_DestroyDynamicTexture(ImTextureData* tex)
{
    uint32_t id = (uint32_t)(intptr_t) tex->TexID;
    dmGraphics::HTexture texture = ImGui_ImplDefold_GetTexture(id);
    if (texture)
    {
        ImGui_ImplDefold_DestroyTexture(texture);
        ImGui_ImplDefold_UnregisterTexture(id);
    }
    tex->SetTexID(0);
    tex->SetStatus(ImTextureStatus_Destroyed);
}

static void ImGui_ImplDefold_UpdateTexture(ImTextureData* tex)
{
    IM_ASSERT(tex->Format == ImTextureFormat_RGBA32 && "dmGraphics backend only supports RGBA32 atlas format");
    if (tex->Status == ImTextureStatus_WantCreate)
    {
        IM_ASSERT(tex->TexID == ImTextureID_Invalid);
        dmGraphics::HTexture texture = ImGui_ImplDefold_CreateTexture(tex->Width, tex->Height, tex->GetPixels());
        if (!texture)
            return;
        uint32_t id = ImGui_ImplDefold_RegisterTexture(texture);
        tex->SetTexID((ImTextureID)(intptr_t) id);
        tex->SetStatus(ImTextureStatus_OK);
    }
    else if (tex->Status == ImTextureStatus_WantUpdates)
    {
        // Atlas dirty-rect update: re-upload the whole texture. dmGraphics sub-region uploads offer no
        // source row-pitch parameter (the Vulkan staging path reads tightly-packed rows at sub-region
        // width), while the atlas buffer has full-atlas-width pitch - when the rect is narrower than
        // the atlas, rows shift horizontally and freshly baked glyphs render garbled (verified);
        // the official GL backend likewise re-uploads the whole texture.
        uint32_t id = (uint32_t)(intptr_t) tex->TexID;
        dmGraphics::HTexture texture = ImGui_ImplDefold_GetTexture(id);
        if (!texture)
            return;
        dmGraphics::HContext context = dmGraphics::GetInstalledContext();
        dmGraphics::TextureParams params;
        params.m_Data = tex->GetPixels();
        params.m_DataSize = tex->Width * tex->Height * tex->BytesPerPixel;
        params.m_Format = dmGraphics::TEXTURE_FORMAT_RGBA;
        params.m_MinFilter = dmGraphics::TEXTURE_FILTER_LINEAR;
        params.m_MagFilter = dmGraphics::TEXTURE_FILTER_LINEAR;
        params.m_UWrap = dmGraphics::TEXTURE_WRAP_CLAMP_TO_EDGE;
        params.m_VWrap = dmGraphics::TEXTURE_WRAP_CLAMP_TO_EDGE;
        params.m_X = 0;
        params.m_Y = 0;
        params.m_Width = (uint16_t) tex->Width;
        params.m_Height = (uint16_t) tex->Height;
        params.m_Depth = 1;
        params.m_LayerCount = 1;
        params.m_MipMap = 0;
        params.m_SubUpdate = true;
        dmGraphics::SetTexture(context, texture, params);
        tex->SetStatus(ImTextureStatus_OK);
    }
    else if (tex->Status == ImTextureStatus_WantDestroy && tex->UnusedFrames > 0)
    {
        ImGui_ImplDefold_DestroyDynamicTexture(tex);
    }
}

static void ImGui_ImplDefold_UpdateTextures(ImDrawData* draw_data)
{
    // Drain texture requests. Most frames the list is empty or all OK (nothing to do).
    // (Taken from ImDrawData rather than PlatformIO directly so callers may override/disable updates.)
    if (draw_data->Textures != nullptr)
        for (ImTextureData* tex : *draw_data->Textures)
            if (tex->Status != ImTextureStatus_OK)
                ImGui_ImplDefold_UpdateTexture(tex);
}


namespace
{
    static void DestroyOffscreenTarget()
    {
        if (g_Renderer.m_OffscreenTarget)
        {
            // The color attachment texture is owned by the render target and is
            // destroyed with it; do not call DeleteTexture on it separately.
            dmGraphics::DeleteRenderTarget(g_Renderer.m_Context, g_Renderer.m_OffscreenTarget);
        }
        g_Renderer.m_OffscreenTarget  = 0;
        g_Renderer.m_OffscreenTexture = 0;
        g_Renderer.m_OffscreenWidth   = 0;
        g_Renderer.m_OffscreenHeight  = 0;
    }

    static bool EnsureOffscreenTarget(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0)
        {
            return false;
        }

        dmGraphics::HContext context = g_Renderer.m_Context;
        if (g_Renderer.m_OffscreenTarget && g_Renderer.m_OffscreenWidth == width && g_Renderer.m_OffscreenHeight == height)
        {
            // Handle looks valid but the backend dropped the texture (e.g. on a context
            // rebuild): recreate it once instead of sampling a dead handle.
            if (g_Renderer.m_OffscreenTexture && dmGraphics::GetRenderTargetTexture(context, g_Renderer.m_OffscreenTarget, dmGraphics::BUFFER_TYPE_COLOR0_BIT))
            {
                return true;
            }
            DestroyOffscreenTarget();
        }
        else
        {
            DestroyOffscreenTarget();
        }

        dmGraphics::RenderTargetCreationParams params = {};

        // Creation params only describe the allocation (size / format / usage);
        // sampling state belongs to m_ColorBufferParams below.
        dmGraphics::TextureCreationParams& color_create = params.m_ColorBufferCreationParams[0];
        color_create.m_Width  = (uint16_t) width;
        color_create.m_Height = (uint16_t) height;

        dmGraphics::TextureParams& color_params = params.m_ColorBufferParams[0];
        color_params.m_Width     = (uint16_t) width;
        color_params.m_Height    = (uint16_t) height;
        color_params.m_Depth     = 1;
        color_params.m_Format    = dmGraphics::TEXTURE_FORMAT_RGBA;
        color_params.m_MinFilter = dmGraphics::TEXTURE_FILTER_LINEAR;
        color_params.m_MagFilter = dmGraphics::TEXTURE_FILTER_LINEAR;
        color_params.m_UWrap     = dmGraphics::TEXTURE_WRAP_CLAMP_TO_EDGE;
        color_params.m_VWrap     = dmGraphics::TEXTURE_WRAP_CLAMP_TO_EDGE;

        // The whole UI is redrawn every frame, so the pass can start from a clear.
        params.m_ColorBufferLoadOps[0]  = dmGraphics::ATTACHMENT_OP_CLEAR;
        // STORE must be set explicitly: the store op of a zero-initialized struct is
        // DONT_CARE, and a Vulkan render pass then discards the color attachment when
        // the pass ends, leaving undefined texel values behind (sampled as noise or
        // solid black). The engine's own render.render_target also sets STORE.
        params.m_ColorBufferStoreOps[0] = dmGraphics::ATTACHMENT_OP_STORE;
        params.m_ColorBufferClearValue[0][0] = 0.0f;
        params.m_ColorBufferClearValue[0][1] = 0.0f;
        params.m_ColorBufferClearValue[0][2] = 0.0f;
        params.m_ColorBufferClearValue[0][3] = 0.0f;
        params.m_SampleCount            = 1;

        g_Renderer.m_OffscreenTarget = dmGraphics::NewRenderTarget(context, dmGraphics::BUFFER_TYPE_COLOR0_BIT, params);
        if (!g_Renderer.m_OffscreenTarget)
        {
            dmLogError("Failed to create ImGui offscreen render target (%u x %u)", width, height);
            return false;
        }

        g_Renderer.m_OffscreenTexture = dmGraphics::GetRenderTargetTexture(context, g_Renderer.m_OffscreenTarget, dmGraphics::BUFFER_TYPE_COLOR0_BIT);
        if (!g_Renderer.m_OffscreenTexture)
        {
            dmLogError("ImGui offscreen render target has no color texture (%u x %u)", width, height);
            DestroyOffscreenTarget();
            return false;
        }

        g_Renderer.m_OffscreenWidth  = width;
        g_Renderer.m_OffscreenHeight = height;
        return true;
    }
} // namespace

dmGraphics::HTexture ImGui_ImplDefold_RenderToTexture(uint32_t* out_width, uint32_t* out_height)
{
    ImDrawData* draw_data = ImGui::GetDrawData();
    if (!draw_data || out_width == 0 || out_height == 0)
    {
        return 0;
    }

    int fb_width  = (int) (draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int) (draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0 || draw_data->CmdListsCount <= 0)
    {
        return 0;
    }

    if (!EnsureOffscreenTarget((uint32_t) fb_width, (uint32_t) fb_height))
    {
        return 0;
    }

    dmGraphics::HContext context = g_Renderer.m_Context;
    dmGraphics::SetRenderTarget(context, g_Renderer.m_OffscreenTarget, 0);
    // Transparent clear: pixels the UI does not cover keep alpha = 0 so the
    // compositing pass in the render script lets the scene show through.
    dmGraphics::Clear(context, dmGraphics::BUFFER_TYPE_COLOR0_BIT, 0, 0, 0, 0, 1.0f, 0);
    g_Renderer.m_RenderingToTexture = true;
    ImGui_ImplDefold_RenderDrawData(draw_data);
    g_Renderer.m_RenderingToTexture = false;
    dmGraphics::SetRenderTarget(context, 0, 0);

    *out_width  = g_Renderer.m_OffscreenWidth;
    *out_height = g_Renderer.m_OffscreenHeight;
    return g_Renderer.m_OffscreenTexture;
}

void ImGui_ImplDefold_RenderDrawData(ImDrawData* draw_data)
{
    int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0 || !CreateDeviceObjects())
        return;

    // Drain this frame's texture create / update / destroy requests (must complete before drawing)
    ImGui_ImplDefold_UpdateTextures(draw_data);

    dmGraphics::HContext context = g_Renderer.m_Context;
    dmGraphics::PipelineState previous_state = dmGraphics::GetPipelineState(context);
    int32_t previous_viewport_x = 0;
    int32_t previous_viewport_y = 0;
    uint32_t previous_viewport_width = 0;
    uint32_t previous_viewport_height = 0;
    dmGraphics::GetViewport(context, &previous_viewport_x, &previous_viewport_y, &previous_viewport_width, &previous_viewport_height);

    SetupRenderState(draw_data, fb_width, fb_height);

    ImVec2 clip_off = draw_data->DisplayPos;
    ImVec2 clip_scale = draw_data->FramebufferScale;
    dmGraphics::Type index_type = sizeof(ImDrawIdx) == 2 ? dmGraphics::TYPE_UNSIGNED_SHORT : dmGraphics::TYPE_UNSIGNED_INT;

    for (int n = 0; n < draw_data->CmdListsCount; ++n)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        uint32_t vtx_buffer_size = cmd_list->VtxBuffer.Size * sizeof(ImDrawVert);
        uint32_t idx_buffer_size = cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx);
        dmGraphics::SetVertexBufferData(g_Renderer.m_VertexBuffer, vtx_buffer_size, cmd_list->VtxBuffer.Data, dmGraphics::BUFFER_USAGE_STREAM_DRAW);
        dmGraphics::SetIndexBufferData(g_Renderer.m_IndexBuffer, idx_buffer_size, cmd_list->IdxBuffer.Data, dmGraphics::BUFFER_USAGE_STREAM_DRAW);
        dmGraphics::EnableVertexBuffer(context, g_Renderer.m_VertexBuffer, 0);

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; ++cmd_i)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
            {
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    SetupRenderState(draw_data, fb_width, fb_height);
                else
                    pcmd->UserCallback(cmd_list, pcmd);
                continue;
            }

            ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x, (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
            ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x, (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);
            if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                continue;

            dmGraphics::HTexture texture = ImGui_ImplDefold_GetTexture((uint32_t)(intptr_t) pcmd->GetTexID());
            if (!texture)
                continue;

            SetClipRect(context, clip_min, clip_max, fb_width, fb_height);
            dmGraphics::EnableVertexDeclaration(context, g_Renderer.m_VertexDeclaration, 0, pcmd->VtxOffset * sizeof(ImDrawVert), g_Renderer.m_Program);
            dmGraphics::EnableTexture(context, 0, 0, texture);
            dmGraphics::DrawElements(context, dmGraphics::PRIMITIVE_TRIANGLES, pcmd->IdxOffset * sizeof(ImDrawIdx), pcmd->ElemCount, index_type, g_Renderer.m_IndexBuffer, 1);
            dmGraphics::DisableTexture(context, 0, texture);
        }
    }

    dmGraphics::DisableVertexDeclaration(context, g_Renderer.m_VertexDeclaration);
    dmGraphics::DisableVertexBuffer(context, g_Renderer.m_VertexBuffer);
    dmGraphics::DisableProgram(context);
    RestorePipelineState(context, previous_state);
    dmGraphics::SetViewport(context, previous_viewport_x, previous_viewport_y, (int32_t) previous_viewport_width, (int32_t) previous_viewport_height);
}
