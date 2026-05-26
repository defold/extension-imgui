#include "imgui_impl_defold.h"

#include <dmsdk/dlib/array.h>
#include <dmsdk/dlib/hash.h>
#include <dmsdk/dlib/log.h>

#include <stdint.h>
#include <string.h>

namespace
{
    const uint32_t SHADER_STAGE_VERTEX   = 0x1;
    const uint32_t SHADER_STAGE_FRAGMENT = 0x2;

    const char* VERTEX_SHADER_GLSL_330 =
        "#version 330\n"
        "layout(location = 0) in vec2 Position;\n"
        "layout(location = 1) in vec2 UV;\n"
        "layout(location = 2) in vec4 Color;\n"
        "layout(std140) uniform imgui_uniforms { mat4 ProjMtx; };\n"
        "out vec2 Frag_UV;\n"
        "out vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "    Frag_UV = UV;\n"
        "    Frag_Color = Color;\n"
        "    gl_Position = ProjMtx * vec4(Position.xy, 0.0, 1.0);\n"
        "}\n";

    const char* FRAGMENT_SHADER_GLSL_330 =
        "#version 330\n"
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "uniform sampler2D Texture;\n"
        "out vec4 Out_Color;\n"
        "void main()\n"
        "{\n"
        "    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
        "}\n";

    const char* VERTEX_SHADER_GLES_300 =
        "#version 300 es\n"
        "precision highp float;\n"
        "layout(location = 0) in vec2 Position;\n"
        "layout(location = 1) in vec2 UV;\n"
        "layout(location = 2) in vec4 Color;\n"
        "layout(std140) uniform imgui_uniforms { mat4 ProjMtx; };\n"
        "out vec2 Frag_UV;\n"
        "out vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "    Frag_UV = UV;\n"
        "    Frag_Color = Color;\n"
        "    gl_Position = ProjMtx * vec4(Position.xy, 0.0, 1.0);\n"
        "}\n";

    const char* FRAGMENT_SHADER_GLES_300 =
        "#version 300 es\n"
        "precision mediump float;\n"
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "uniform sampler2D Texture;\n"
        "layout(location = 0) out vec4 Out_Color;\n"
        "void main()\n"
        "{\n"
        "    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
        "}\n";

    const char* VERTEX_SHADER_GLES_100 =
        "attribute vec2 Position;\n"
        "attribute vec2 UV;\n"
        "attribute vec4 Color;\n"
        "uniform mat4 ProjMtx;\n"
        "varying vec2 Frag_UV;\n"
        "varying vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "    Frag_UV = UV;\n"
        "    Frag_Color = Color;\n"
        "    gl_Position = ProjMtx * vec4(Position.xy, 0.0, 1.0);\n"
        "}\n";

    const char* FRAGMENT_SHADER_GLES_100 =
        "precision mediump float;\n"
        "uniform sampler2D Texture;\n"
        "varying vec2 Frag_UV;\n"
        "varying vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "    gl_FragColor = Frag_Color * texture2D(Texture, Frag_UV.st);\n"
        "}\n";

    alignas(4) static const uint8_t VERTEX_SHADER_SPIRV[] = {
        0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, 0x0b, 0x00, 0x08, 0x00, 0x2b, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x06, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x47, 0x4c, 0x53, 0x4c, 0x2e, 0x73, 0x74, 0x64, 0x2e, 0x34, 0x35, 0x30,
        0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x0f, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e,
        0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00,
        0x11, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x03, 0x00, 0x03, 0x00,
        0x02, 0x00, 0x00, 0x00, 0xc2, 0x01, 0x00, 0x00, 0x05, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00,
        0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x04, 0x00, 0x09, 0x00, 0x00, 0x00,
        0x46, 0x72, 0x61, 0x67, 0x5f, 0x55, 0x56, 0x00, 0x05, 0x00, 0x03, 0x00, 0x0b, 0x00, 0x00, 0x00,
        0x55, 0x56, 0x00, 0x00, 0x05, 0x00, 0x05, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x46, 0x72, 0x61, 0x67,
        0x5f, 0x43, 0x6f, 0x6c, 0x6f, 0x72, 0x00, 0x00, 0x05, 0x00, 0x04, 0x00, 0x11, 0x00, 0x00, 0x00,
        0x43, 0x6f, 0x6c, 0x6f, 0x72, 0x00, 0x00, 0x00, 0x05, 0x00, 0x06, 0x00, 0x16, 0x00, 0x00, 0x00,
        0x67, 0x6c, 0x5f, 0x50, 0x65, 0x72, 0x56, 0x65, 0x72, 0x74, 0x65, 0x78, 0x00, 0x00, 0x00, 0x00,
        0x06, 0x00, 0x06, 0x00, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x67, 0x6c, 0x5f, 0x50,
        0x6f, 0x73, 0x69, 0x74, 0x69, 0x6f, 0x6e, 0x00, 0x06, 0x00, 0x07, 0x00, 0x16, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x67, 0x6c, 0x5f, 0x50, 0x6f, 0x69, 0x6e, 0x74, 0x53, 0x69, 0x7a, 0x65,
        0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x07, 0x00, 0x16, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
        0x67, 0x6c, 0x5f, 0x43, 0x6c, 0x69, 0x70, 0x44, 0x69, 0x73, 0x74, 0x61, 0x6e, 0x63, 0x65, 0x00,
        0x06, 0x00, 0x07, 0x00, 0x16, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x67, 0x6c, 0x5f, 0x43,
        0x75, 0x6c, 0x6c, 0x44, 0x69, 0x73, 0x74, 0x61, 0x6e, 0x63, 0x65, 0x00, 0x05, 0x00, 0x03, 0x00,
        0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x06, 0x00, 0x1c, 0x00, 0x00, 0x00,
        0x69, 0x6d, 0x67, 0x75, 0x69, 0x5f, 0x75, 0x6e, 0x69, 0x66, 0x6f, 0x72, 0x6d, 0x73, 0x00, 0x00,
        0x06, 0x00, 0x05, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x72, 0x6f, 0x6a,
        0x4d, 0x74, 0x78, 0x00, 0x05, 0x00, 0x03, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x05, 0x00, 0x05, 0x00, 0x22, 0x00, 0x00, 0x00, 0x50, 0x6f, 0x73, 0x69, 0x74, 0x69, 0x6f, 0x6e,
        0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x09, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x11, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00,
        0x02, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x0b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00, 0x16, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00,
        0x16, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
        0x48, 0x00, 0x05, 0x00, 0x16, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00, 0x47, 0x00, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
        0x48, 0x00, 0x04, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
        0x48, 0x00, 0x05, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x07, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x47, 0x00, 0x03, 0x00, 0x1c, 0x00, 0x00, 0x00,
        0x02, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x22, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x13, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00, 0x00, 0x21, 0x00, 0x03, 0x00,
        0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x16, 0x00, 0x03, 0x00, 0x06, 0x00, 0x00, 0x00,
        0x20, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
        0x02, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
        0x07, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00,
        0x03, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x07, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
        0x0d, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00,
        0x03, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x0d, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x15, 0x00, 0x04, 0x00, 0x13, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x13, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x04, 0x00, 0x15, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
        0x14, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x06, 0x00, 0x16, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00,
        0x06, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
        0x17, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00,
        0x17, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x15, 0x00, 0x04, 0x00,
        0x19, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00,
        0x19, 0x00, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x04, 0x00,
        0x1b, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x03, 0x00,
        0x1c, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x1d, 0x00, 0x00, 0x00,
        0x02, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x1d, 0x00, 0x00, 0x00,
        0x1e, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x1f, 0x00, 0x00, 0x00,
        0x02, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x0a, 0x00, 0x00, 0x00,
        0x22, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x06, 0x00, 0x00, 0x00,
        0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x06, 0x00, 0x00, 0x00,
        0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f, 0x36, 0x00, 0x05, 0x00, 0x02, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00,
        0x05, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00,
        0x0b, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00, 0x09, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00,
        0x3d, 0x00, 0x04, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00,
        0x3e, 0x00, 0x03, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x41, 0x00, 0x05, 0x00,
        0x1f, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x00,
        0x3d, 0x00, 0x04, 0x00, 0x1b, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
        0x3d, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00,
        0x51, 0x00, 0x05, 0x00, 0x06, 0x00, 0x00, 0x00, 0x26, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x51, 0x00, 0x05, 0x00, 0x06, 0x00, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00,
        0x23, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x50, 0x00, 0x07, 0x00, 0x0d, 0x00, 0x00, 0x00,
        0x28, 0x00, 0x00, 0x00, 0x26, 0x00, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00,
        0x25, 0x00, 0x00, 0x00, 0x91, 0x00, 0x05, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x29, 0x00, 0x00, 0x00,
        0x21, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x41, 0x00, 0x05, 0x00, 0x0e, 0x00, 0x00, 0x00,
        0x2a, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00,
        0x2a, 0x00, 0x00, 0x00, 0x29, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x01, 0x00, 0x38, 0x00, 0x01, 0x00
    };

    alignas(4) static const uint8_t FRAGMENT_SHADER_SPIRV[] = {
        0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, 0x0b, 0x00, 0x08, 0x00, 0x18, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x06, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x47, 0x4c, 0x53, 0x4c, 0x2e, 0x73, 0x74, 0x64, 0x2e, 0x34, 0x35, 0x30,
        0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x0f, 0x00, 0x08, 0x00, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e,
        0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00,
        0x10, 0x00, 0x03, 0x00, 0x04, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x03, 0x00, 0x03, 0x00,
        0x02, 0x00, 0x00, 0x00, 0xc2, 0x01, 0x00, 0x00, 0x05, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00,
        0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x05, 0x00, 0x09, 0x00, 0x00, 0x00,
        0x4f, 0x75, 0x74, 0x5f, 0x43, 0x6f, 0x6c, 0x6f, 0x72, 0x00, 0x00, 0x00, 0x05, 0x00, 0x05, 0x00,
        0x0b, 0x00, 0x00, 0x00, 0x46, 0x72, 0x61, 0x67, 0x5f, 0x43, 0x6f, 0x6c, 0x6f, 0x72, 0x00, 0x00,
        0x05, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x00, 0x54, 0x65, 0x78, 0x74, 0x75, 0x72, 0x65, 0x00,
        0x05, 0x00, 0x04, 0x00, 0x14, 0x00, 0x00, 0x00, 0x46, 0x72, 0x61, 0x67, 0x5f, 0x55, 0x56, 0x00,
        0x47, 0x00, 0x04, 0x00, 0x09, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x47, 0x00, 0x04, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x47, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x47, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x47, 0x00, 0x04, 0x00, 0x14, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x13, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00, 0x00, 0x21, 0x00, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00,
        0x02, 0x00, 0x00, 0x00, 0x16, 0x00, 0x03, 0x00, 0x06, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
        0x17, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
        0x20, 0x00, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
        0x3b, 0x00, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
        0x20, 0x00, 0x04, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
        0x3b, 0x00, 0x04, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x19, 0x00, 0x09, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x03, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00,
        0x20, 0x00, 0x04, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00,
        0x3b, 0x00, 0x04, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x17, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
        0x20, 0x00, 0x04, 0x00, 0x13, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00,
        0x3b, 0x00, 0x04, 0x00, 0x13, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x36, 0x00, 0x05, 0x00, 0x02, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x03, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x05, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00,
        0x07, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00,
        0x0e, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00,
        0x12, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x57, 0x00, 0x05, 0x00,
        0x07, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00,
        0x85, 0x00, 0x05, 0x00, 0x07, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00,
        0x16, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00, 0x09, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00,
        0xfd, 0x00, 0x01, 0x00, 0x38, 0x00, 0x01, 0x00
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
        dmGraphics::HTexture                 m_FontTexture;
        uint32_t                             m_FontTextureId;
        uint32_t                             m_NextTextureId;
        dmArray<TextureRef>                  m_Textures;
    };

    Renderer g_Renderer = {};

    static dmGraphics::ShaderDesc::Shader CreateShader(dmGraphics::ShaderDesc::ShaderType type, dmGraphics::ShaderDesc::Language language, const void* data, uint32_t data_size)
    {
        dmGraphics::ShaderDesc::Shader shader = {};
        shader.m_ShaderType     = type;
        shader.m_Language       = language;
        shader.m_Source.m_Data  = (uint8_t*) data;
        shader.m_Source.m_Count = data_size;
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

    static dmGraphics::ShaderDesc CreateShaderDesc(dmArray<dmGraphics::ShaderDesc::Shader>& shaders,
                                                   dmArray<dmGraphics::ShaderDesc::ResourceBinding>& inputs,
                                                   dmArray<dmGraphics::ShaderDesc::ResourceBinding>& textures,
                                                   dmArray<dmGraphics::ShaderDesc::ResourceBinding>& uniform_buffers,
                                                   dmArray<dmGraphics::ShaderDesc::ResourceTypeInfo>& types,
                                                   dmArray<dmGraphics::ShaderDesc::ResourceMember>& members)
    {
        shaders.OffsetCapacity(8);
        shaders.Push(CreateShader(dmGraphics::ShaderDesc::SHADER_TYPE_VERTEX, dmGraphics::ShaderDesc::LANGUAGE_GLSL_SM330, VERTEX_SHADER_GLSL_330, (uint32_t) strlen(VERTEX_SHADER_GLSL_330)));
        shaders.Push(CreateShader(dmGraphics::ShaderDesc::SHADER_TYPE_FRAGMENT, dmGraphics::ShaderDesc::LANGUAGE_GLSL_SM330, FRAGMENT_SHADER_GLSL_330, (uint32_t) strlen(FRAGMENT_SHADER_GLSL_330)));
        shaders.Push(CreateShader(dmGraphics::ShaderDesc::SHADER_TYPE_VERTEX, dmGraphics::ShaderDesc::LANGUAGE_GLES_SM300, VERTEX_SHADER_GLES_300, (uint32_t) strlen(VERTEX_SHADER_GLES_300)));
        shaders.Push(CreateShader(dmGraphics::ShaderDesc::SHADER_TYPE_FRAGMENT, dmGraphics::ShaderDesc::LANGUAGE_GLES_SM300, FRAGMENT_SHADER_GLES_300, (uint32_t) strlen(FRAGMENT_SHADER_GLES_300)));
        shaders.Push(CreateShader(dmGraphics::ShaderDesc::SHADER_TYPE_VERTEX, dmGraphics::ShaderDesc::LANGUAGE_GLES_SM100, VERTEX_SHADER_GLES_100, (uint32_t) strlen(VERTEX_SHADER_GLES_100)));
        shaders.Push(CreateShader(dmGraphics::ShaderDesc::SHADER_TYPE_FRAGMENT, dmGraphics::ShaderDesc::LANGUAGE_GLES_SM100, FRAGMENT_SHADER_GLES_100, (uint32_t) strlen(FRAGMENT_SHADER_GLES_100)));
        shaders.Push(CreateShader(dmGraphics::ShaderDesc::SHADER_TYPE_VERTEX, dmGraphics::ShaderDesc::LANGUAGE_SPIRV, VERTEX_SHADER_SPIRV, sizeof(VERTEX_SHADER_SPIRV)));
        shaders.Push(CreateShader(dmGraphics::ShaderDesc::SHADER_TYPE_FRAGMENT, dmGraphics::ShaderDesc::LANGUAGE_SPIRV, FRAGMENT_SHADER_SPIRV, sizeof(FRAGMENT_SHADER_SPIRV)));

        inputs.OffsetCapacity(3);
        inputs.Push(CreateBinding("Position", 0, 0, dmGraphics::ShaderDesc::SHADER_TYPE_VEC2, SHADER_STAGE_VERTEX));
        inputs.Push(CreateBinding("UV", 1, 0, dmGraphics::ShaderDesc::SHADER_TYPE_VEC2, SHADER_STAGE_VERTEX));
        inputs.Push(CreateBinding("Color", 2, 0, dmGraphics::ShaderDesc::SHADER_TYPE_VEC4, SHADER_STAGE_VERTEX));

        textures.OffsetCapacity(1);
        textures.Push(CreateBinding("Texture", 1, 0, dmGraphics::ShaderDesc::SHADER_TYPE_SAMPLER2D, SHADER_STAGE_FRAGMENT));

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
        float projection[16] = {
            2.0f / (R - L),       0.0f,                  0.0f, 0.0f,
            0.0f,                 2.0f / (T - B),        0.0f, 0.0f,
            0.0f,                 0.0f,                 -1.0f, 0.0f,
            (R + L) / (L - R),    (T + B) / (B - T),     0.0f, 1.0f
        };

        dmGraphics::EnableProgram(context, g_Renderer.m_Program);
        dmGraphics::SetConstantM4(context, (const dmVMath::Vector4*) projection, 1, g_Renderer.m_ProjMtxLocation);
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
        if (family == dmGraphics::ADAPTER_FAMILY_OPENGL || family == dmGraphics::ADAPTER_FAMILY_OPENGLES)
            y = fb_height - (y + height);

        dmGraphics::SetScissor(context, x, y, width, height);
    }

    static bool CreateDeviceObjects()
    {
        if (g_Renderer.m_Program != dmGraphics::INVALID_PROGRAM_HANDLE && g_Renderer.m_Program != 0)
        {
            return true;
        }

        dmArray<dmGraphics::ShaderDesc::Shader> shaders;
        dmArray<dmGraphics::ShaderDesc::ResourceBinding> inputs;
        dmArray<dmGraphics::ShaderDesc::ResourceBinding> textures;
        dmArray<dmGraphics::ShaderDesc::ResourceBinding> uniform_buffers;
        dmArray<dmGraphics::ShaderDesc::ResourceTypeInfo> types;
        dmArray<dmGraphics::ShaderDesc::ResourceMember> members;
        dmGraphics::ShaderDesc shader_desc = CreateShaderDesc(shaders, inputs, textures, uniform_buffers, types, members);

        char error_buffer[1024] = {};
        g_Renderer.m_Program = dmGraphics::NewProgram(g_Renderer.m_Context, &shader_desc, error_buffer, sizeof(error_buffer));
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
        if (g_Renderer.m_FontTexture)
        {
            ImGui_ImplDefold_DestroyTexture(g_Renderer.m_FontTexture);
            ImGui_ImplDefold_UnregisterTexture(g_Renderer.m_FontTextureId);
            ImGui::GetIO().Fonts->SetTexID(0);
            g_Renderer.m_FontTexture = 0;
            g_Renderer.m_FontTextureId = 0;
        }

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

    static bool CreateFontsTexture()
    {
        ImGuiIO& io = ImGui::GetIO();
        unsigned char* pixels = 0;
        int width = 0;
        int height = 0;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        g_Renderer.m_FontTexture = ImGui_ImplDefold_CreateTexture(width, height, pixels);
        if (!g_Renderer.m_FontTexture)
            return false;

        g_Renderer.m_FontTextureId = ImGui_ImplDefold_RegisterTexture(g_Renderer.m_FontTexture);
        io.Fonts->SetTexID((ImTextureID)(intptr_t) g_Renderer.m_FontTextureId);
        return true;
    }
}

bool ImGui_ImplDefold_Init()
{
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.BackendRendererUserData == 0 && "Already initialized a renderer backend!");

    g_Renderer.m_Context = dmGraphics::GetInstalledContext();
    g_Renderer.m_Program = dmGraphics::INVALID_PROGRAM_HANDLE;
    g_Renderer.m_NextTextureId = 1;
    g_Renderer.m_StreamDeclaration = 0;
    g_Renderer.m_VertexDeclaration = 0;
    g_Renderer.m_VertexBuffer = 0;
    g_Renderer.m_IndexBuffer = 0;
    g_Renderer.m_TextureLocation = dmGraphics::INVALID_UNIFORM_LOCATION;
    g_Renderer.m_ProjMtxLocation = dmGraphics::INVALID_UNIFORM_LOCATION;
    g_Renderer.m_FontTexture = 0;
    g_Renderer.m_FontTextureId = 0;

    io.BackendRendererUserData = &g_Renderer;
    io.BackendRendererName = "imgui_impl_defold";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    return g_Renderer.m_Context != 0;
}

void ImGui_ImplDefold_Shutdown()
{
    DestroyDeviceObjects();
    g_Renderer.m_Textures.SetSize(0);
    g_Renderer.m_Textures.SetCapacity(0);

    ImGuiIO& io = ImGui::GetIO();
    io.BackendFlags &= ~ImGuiBackendFlags_RendererHasVtxOffset;
    io.BackendRendererName = 0;
    io.BackendRendererUserData = 0;
}

void ImGui_ImplDefold_NewFrame()
{
    if (CreateDeviceObjects() && !g_Renderer.m_FontTexture)
        CreateFontsTexture();
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

void ImGui_ImplDefold_RenderDrawData(ImDrawData* draw_data)
{
    int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0 || !CreateDeviceObjects())
        return;

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

    dmGraphics::DisableVertexBuffer(context, g_Renderer.m_VertexBuffer);
    dmGraphics::DisableVertexDeclaration(context, g_Renderer.m_VertexDeclaration);
    dmGraphics::DisableProgram(context);
    RestorePipelineState(context, previous_state);
    dmGraphics::SetViewport(context, previous_viewport_x, previous_viewport_y, (int32_t) previous_viewport_width, (int32_t) previous_viewport_height);
}
