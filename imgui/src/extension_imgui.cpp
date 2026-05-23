/** Dear ImGUI
 * Functions and constants for interacting with Dear ImGUI
 * @document
 * @namespace imgui
 */

#define LIB_NAME "ImGui"

// Need this preamble for Linux gl headers to work
#include "imgui/imgui.h"
#include "imgui/imconfig.h"

// set in imconfig.h
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
#include <GL/gl3w.h>
#endif

// imgui renderer backend and possible platform extras
#if defined(DM_PLATFORM_ANDROID)
#include "imgui/imgui_impl_android.h"
#endif
#include "imgui/imgui_impl_opengl3.h"

#include <dmsdk/sdk.h>

#include "extension_imgui.h"

#if !defined(DM_HEADLESS)

#define MODULE_NAME "imgui"

#include <dmsdk/dlib/crypt.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include "stb/stb_image.h"

#define MAX_HISTOGRAM_VALUES    1000 * 1024
#define MAX_IMAGE_NAME          256

#define TEXTBUFFER_SIZE         sizeof(char) * 1000 * 1024

typedef struct ImgObject
{
    int                w;
    int                h;
    int                comp;
    GLuint             tid;
    char               name[MAX_IMAGE_NAME];
    unsigned char *    data;
} ImgObject;

enum ExtImGuiGlyphRanges {
    ExtImGuiGlyphRanges_Default,
    ExtImGuiGlyphRanges_Greek,
    ExtImGuiGlyphRanges_Korean,
    ExtImGuiGlyphRanges_Japanese,
    ExtImGuiGlyphRanges_ChineseFull,
    ExtImGuiGlyphRanges_ChineseSimplifiedCommon,
    ExtImGuiGlyphRanges_Cyrillic,
    ExtImGuiGlyphRanges_Thai,
    ExtImGuiGlyphRanges_Vietnamese
};

static bool g_imgui_NewFrame        = false;
static char* g_imgui_TextBuffer     = 0;
static dmArray<ImFont*> g_imgui_Fonts;
static dmArray<ImgObject> g_imgui_Images;
static bool g_VerifyGraphicsCalls   = false;
static bool g_RenderingEnabled      = true;

#define DEFOLD_IMGUI_MAX_DRAW_CALLBACKS 16

struct DefoldImGuiDrawCallbackEntry
{
    DefoldImGuiDrawCallback m_Callback;
    void*                   m_UserData;
};

static DefoldImGuiDrawCallbackEntry g_DrawCallbacks[DEFOLD_IMGUI_MAX_DRAW_CALLBACKS];

extern "C" DM_DLLEXPORT bool DefoldImGui_RegisterDrawCallback(DefoldImGuiDrawCallback callback, void* user_data)
{
    if (!callback)
    {
        return false;
    }

    // Idempotent registration.
    for (uint32_t i = 0; i < DEFOLD_IMGUI_MAX_DRAW_CALLBACKS; ++i)
    {
        if (g_DrawCallbacks[i].m_Callback == callback &&
            g_DrawCallbacks[i].m_UserData == user_data)
        {
            return true;
        }
    }

    for (uint32_t i = 0; i < DEFOLD_IMGUI_MAX_DRAW_CALLBACKS; ++i)
    {
        if (!g_DrawCallbacks[i].m_Callback)
        {
            g_DrawCallbacks[i].m_Callback = callback;
            g_DrawCallbacks[i].m_UserData = user_data;
            return true;
        }
    }

    return false;
}

extern "C" DM_DLLEXPORT void DefoldImGui_UnregisterDrawCallback(DefoldImGuiDrawCallback callback, void* user_data)
{
    for (uint32_t i = 0; i < DEFOLD_IMGUI_MAX_DRAW_CALLBACKS; ++i)
    {
        if (g_DrawCallbacks[i].m_Callback == callback &&
            g_DrawCallbacks[i].m_UserData == user_data)
        {
            g_DrawCallbacks[i].m_Callback = 0;
            g_DrawCallbacks[i].m_UserData = 0;
        }
    }
}

static void imgui_InvokeDrawCallbacks()
{
    // Iterate by index so callbacks may unregister themselves safely.
    for (uint32_t i = 0; i < DEFOLD_IMGUI_MAX_DRAW_CALLBACKS; ++i)
    {
        DefoldImGuiDrawCallback callback = g_DrawCallbacks[i].m_Callback;
        void* user_data = g_DrawCallbacks[i].m_UserData;
        if (callback)
        {
            callback(user_data);
        }
    }
}

static void imgui_ClearDrawCallbacks()
{
    for (uint32_t i = 0; i < DEFOLD_IMGUI_MAX_DRAW_CALLBACKS; ++i)
    {
        g_DrawCallbacks[i].m_Callback = 0;
        g_DrawCallbacks[i].m_UserData = 0;
    }
}


static void imgui_ClearGLError()
{
    if (!g_VerifyGraphicsCalls) return;
    GLint err = glGetError();
    while (err != 0)
    {
        err = glGetError();
    }
}


// ----------------------------
// ----- IMAGES ---------------
// ----------------------------

// extern unsigned char *stbi_load(char const *filename, int *x, int *y, int *comp, int req_comp);
//

/** ImageB64Decode
 * @name image_b64_decode
 * @string data
 * @number length
 * @treturn string decoded data
 */
static int imgui_ImageB64Decode(lua_State *L)
{
    DM_LUA_STACK_CHECK(L, 1);
    const char *data = luaL_checkstring(L,1);
    uint32_t datalen = luaL_checknumber(L,2);
    uint32_t dstlen  = 4*(datalen/3 + 1);

    char *datastr = (char *)malloc(dstlen);
    bool result;
    result = dmCrypt::Base64Decode((const uint8_t*)data, datalen, (uint8_t*)datastr, &dstlen);

    lua_pushlstring(L, datastr, dstlen);
    free(datastr);
    return 1;
}


static int imgui_ImageInternalLoad(const char *filename, ImgObject *image)
{
    if (image->data == 0)
    {
        dmLogError("Error loading image: %s", filename);
        return 0;
    }

    dmLogInfo("imgui_ImageInternalLoad before %d", image->tid);

    glGenTextures(1, &image->tid);
    dmLogInfo("imgui_ImageInternalLoad after %d", image->tid);
    glBindTexture(GL_TEXTURE_2D, image->tid);

    strcpy(image->name, filename);
    if (g_imgui_Images.Full())
    {
        g_imgui_Images.OffsetCapacity(2);
    }
    g_imgui_Images.Push(*image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->w, image->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->data);

    return 1;
}


// Image handling needs to be smarter, but this will do for the time being.

/** ImageLoadData
 * @name image_load_data
 * @string filename
 * @string data
 * @number data_length
 * @treturn number texture_id
 * @treturn number width
 * @treturn number height
 */
static int imgui_ImageLoadData(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 3);
    const char * filename = luaL_checkstring(L, 1);
    ImgObject image;

    // If its already in the vector, return the id
    for (int i=0; i<g_imgui_Images.Size(); i++)
    {
        if (strcmp(g_imgui_Images[i].name, filename) == 0)
        {
            image = g_imgui_Images[i];
            lua_pushinteger(L, image.tid);
            lua_pushinteger(L, image.w);
            lua_pushinteger(L, image.h);
            return 3;
        }
    }

    unsigned char *strdata = (unsigned char *)luaL_checkstring(L, 2);
    int lendata = luaL_checkinteger(L, 3);
    image.data = stbi_load_from_memory( strdata, lendata, &image.w, &image.h, NULL, STBI_rgb_alpha);
    if (image.data == 0)
    {
        dmLogError("Error loading image: %s", filename);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        return 3;
    }

    if (!imgui_ImageInternalLoad(filename, &image))
    {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        return 3;
    }

    stbi_image_free(image.data);
    image.data = 0;
    lua_pushinteger(L, image.tid);
    lua_pushinteger(L, image.w);
    lua_pushinteger(L, image.h);
    return 3;
}

// Image handling needs to be smarter, but this will do for the time being.
/** ImageLoad
 * @name image_load
 * @string filename
 * @treturn number texture_id
 * @treturn number width
 * @treturn number height
 */
static int imgui_ImageLoad(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 3);
    const char * filename = luaL_checkstring(L, 1);
    dmLogInfo("imgui_ImageLoad %s", filename);
    ImgObject image;

    // If its already in the vector, return the id
    for (int i = 0; i < g_imgui_Images.Size(); i++)
    {
        if(strcmp(g_imgui_Images[i].name, filename) == 0)
        {
            image = g_imgui_Images[i];
            lua_pushinteger(L, image.tid);
            lua_pushinteger(L, image.w);
            lua_pushinteger(L, image.h);
            return 3;
        }
    }

    image.data = stbi_load(filename, &image.w, &image.h, NULL, STBI_rgb_alpha);
    if (image.data == 0)
    {
        dmLogError("Error loading image: %s", filename);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        return 3;
    }

    if (!imgui_ImageInternalLoad(filename, &image))
    {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        return 3;
    }

    stbi_image_free(image.data);
    image.data = 0;
    lua_pushinteger(L, image.tid);
    lua_pushinteger(L, image.w);
    lua_pushinteger(L, image.h);
    return 3;
}

/** ImageGet
 * @name image_get
 * @number texture_id Texture id
 * @treturn number texture_id Texture id
 * @treturn number width Image width
 * @treturn height Image height
 */
static int imgui_ImageGet( lua_State *L )
{
    DM_LUA_STACK_CHECK(L, 3);
    GLuint tid = (GLuint)luaL_checkinteger(L, 1);

    for (int i = 0; i < g_imgui_Images.Size(); i++)
    {
        if (g_imgui_Images[i].tid == tid)
        {
            ImgObject image = g_imgui_Images[i];
            lua_pushinteger(L, image.tid);
            lua_pushinteger(L, image.w);
            lua_pushinteger(L, image.h);
            return 3;
        }
    }

    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
    return 3;
}

/** ImageAdd
 * @name image_add
 * @number texture_id Texture id
 * @number width Image width
 * @number height Image height
 * @number [uv0x]
 * @number [uv0y]
 * @number [uv1x]
 * @number [uv2x]
 */
static int imgui_ImageAdd( lua_State *L )
{
    DM_LUA_STACK_CHECK(L, 0);
    GLuint tid = (GLuint)luaL_checkinteger(L, 1);
    int w = luaL_checknumber(L, 2);
    int h = luaL_checknumber(L, 3);

    ImVec2 uv0 = ImVec2(0, 0);
    if (lua_isnumber(L, 4) && lua_isnumber(L, 5)) {
        uv0.x = luaL_checknumber(L, 4);
        uv0.y = luaL_checknumber(L, 5);
    }
    ImVec2 uv1 = ImVec2(1, 1);
    if (lua_isnumber(L, 6) && lua_isnumber(L, 7)) {
        uv1.x = luaL_checknumber(L, 6);
        uv1.y = luaL_checknumber(L, 7);
    }
    ImGui::Image((void*)(intptr_t)tid, ImVec2(w, h), uv0, uv1);
    return 0;
}

static int imgui_ImageFree( lua_State *L )
{
    DM_LUA_STACK_CHECK(L, 0);
    GLuint tid = (GLuint)luaL_checkinteger(L, 1);
    for (int i = 0; i < g_imgui_Images.Size(); i++)
    {
        if (g_imgui_Images[i].tid == tid)
        {
            glDeleteTextures(1, &tid);
            g_imgui_Images.EraseSwap(i);
            break;
        }
    }
    return 0;
}

// ----------------------------
// ----- PRIMITIVES -----------
// ----------------------------

/** DrawListAddLine
 * @name add_line
 * @number x1
 * @number y1
 * @number x2
 * @number y2
 * @number r
 * @number g
 * @number b
 * @number a
 * @number [thickness]
 */
static int imgui_DrawListAddLine(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    float x1 = luaL_checknumber(L, 1);
    float y1 = luaL_checknumber(L, 2);
    float x2 = luaL_checknumber(L, 3);
    float y2 = luaL_checknumber(L, 4);

    float r = (float)luaL_checknumber(L, 5);
    float g = (float)luaL_checknumber(L, 6);
    float b = (float)luaL_checknumber(L, 7);
    float a = (float)luaL_checknumber(L, 8);

    float thickness = 1.0f;
    if (lua_isnumber(L, 9))
    {
        thickness = luaL_checknumber(L, 9);
    }

    ImGui::GetWindowDrawList()->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), IM_COL32(r, g, b, a), thickness);
    return 0;
}

/** DrawListAddRect
 * @name add_rect
 * @number x1
 * @number y1
 * @number x2
 * @number y2
 * @number r
 * @number g
 * @number b
 * @number a
 * @number [thickness]
 */
static int imgui_DrawListAddRect(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    float x1 = luaL_checknumber(L, 1);
    float y1 = luaL_checknumber(L, 2);
    float x2 = luaL_checknumber(L, 3);
    float y2 = luaL_checknumber(L, 4);

    float r = (float)luaL_checknumber(L, 5);
    float g = (float)luaL_checknumber(L, 6);
    float b = (float)luaL_checknumber(L, 7);
    float a = (float)luaL_checknumber(L, 8);

    float thickness = 1.0f;
    if (lua_isnumber(L, 9))
    {
        thickness = luaL_checknumber(L, 9);
    }
    float rounding = 0.0f;
    int flags = 0;

    ImGui::GetWindowDrawList()->AddRect(ImVec2(x1, y1), ImVec2(x2, y2), IM_COL32(r, g, b, a), rounding, flags, thickness);
    return 0;
}

// ----------------------------
// ----- FRAMES ---------------
// ----------------------------

static void imgui_NewFrame()
{
    if (g_imgui_NewFrame == false)
    {
        ImGui_ImplOpenGL3_NewFrame();
        imgui_ClearGLError();
        ImGui::NewFrame();
        g_imgui_NewFrame = true;
    }
}

/** SetDisplaySize
 * @name set_display_size
 * @number w
 * @number h
 */
static int imgui_SetDisplaySize(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    float w = luaL_checknumber(L, 1);
    float h = luaL_checknumber(L, 2);

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(w, h);
    return 0;
}


// ----------------------------
// ----- INPUT ----------------
// ----------------------------
/** SetMouseInput
 * @name set_mouse_input
 * @number x
 * @number y
 * @number down1
 * @number down2
 * @number down3
 * @number wheel
 */
static int imgui_SetMouseInput(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    ImGuiIO& io = ImGui::GetIO();

    const ImVec2 mouse_pos_backup = io.MousePos;
    io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);

    if (io.WantSetMousePos)
    {
        return luaL_error(L, "WantSetMousePos not supported yet.");
    }
    else
    {
        io.MousePos = ImVec2(luaL_checknumber(L, 1), luaL_checknumber(L, 2));
    }

    io.MouseDown[0] = luaL_checknumber(L, 3);
    io.MouseDown[1] = luaL_checknumber(L, 4);
    io.MouseDown[2] = luaL_checknumber(L, 5);
    io.MouseWheel += luaL_checknumber(L, 6);

    return 0;
}
/** SetMousePos
 * @name set_mouse_pos
 * @number x
 * @number y
 */
static int imgui_SetMousePos(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    ImGuiIO& io = ImGui::GetIO();

    const ImVec2 mouse_pos_backup = io.MousePos;
    io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);

    if (io.WantSetMousePos)
    {
        return luaL_error(L, "WantSetMousePos not supported yet.");
    }
    else
    {
        io.MousePos = ImVec2(luaL_checknumber(L, 1), luaL_checknumber(L, 2));
    }

    return 0;
}
/** SetMouseButton
 * @name set_mouse_button
 * @number index
 * @number down
 */
static int imgui_SetMouseButton(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    ImGuiIO& io = ImGui::GetIO();
    int index = luaL_checknumber(L, 1);
    io.MouseDown[index] = luaL_checknumber(L, 2);
    return 0;
}
/** SetMouseWheel
 * @name set_mouse_wheel
 * @number wheel
 */
static int imgui_SetMouseWheel(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    ImGuiIO& io = ImGui::GetIO();
    io.MouseWheel += luaL_checknumber(L, 1);
    return 0;
}
/** SetKeyDown
 * @name set_key_down
 * @number key
 * @bool down
 */
static int imgui_SetKeyDown(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    ImGuiIO& io = ImGui::GetIO();
    uint32_t key = luaL_checknumber(L, 1);
    io.AddKeyEvent((ImGuiKey)key, lua_toboolean(L, 2));
    return 0;
}


// ----------------------------
// ----- KEY MODIFIERS --------
// ----------------------------
/** SetKeyModifierCtrl
 * @name set_key_modifier_ctrl
 * @bool down
 */
static int imgui_SetKeyModifierCtrl(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    ImGuiIO& io = ImGui::GetIO();
    io.KeyCtrl = lua_toboolean(L, 1);
    return 0;
}
/** SetKeyModifierShift
 * @name set_key_modifier_shift
 * @bool down
 */
static int imgui_SetKeyModifierShift(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    ImGuiIO& io = ImGui::GetIO();
    io.KeyShift = lua_toboolean(L, 1);
    return 0;
}
/** SetKeyModifierAlt
 * @name set_key_modifier_alt
 * @bool down
 */
static int imgui_SetKeyModifierAlt(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    ImGuiIO& io = ImGui::GetIO();
    io.KeyAlt = lua_toboolean(L, 1);
    return 0;
}
/** SetKeyModifierSuper
 * @name set_key_modifier_super
 * @bool down
 */
static int imgui_SetKeyModifierSuper(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    ImGuiIO& io = ImGui::GetIO();
    io.KeySuper = lua_toboolean(L, 1);
    return 0;
}


// ----------------------------
// ----- TEXT INPUT -----------
// ----------------------------
/** AddInputCharacter
 * @name add_input_character
 * @string character
 */
static int imgui_AddInputCharacter(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    ImGuiIO& io = ImGui::GetIO();
    const char* s = luaL_checkstring(L, 1);
    unsigned int c = s[0];
    io.AddInputCharacter(c);
    return 0;
}
/** AddInputCharacters
 * @name add_input_characters
 * @string characters
 */
static int imgui_AddInputCharacters(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    ImGuiIO& io = ImGui::GetIO();
    const char* s = luaL_checkstring(L, 1);
    io.AddInputCharactersUTF8(s);
    return 0;
}


// ----------------------------
// ----- TREE -----------------
// ----------------------------
/** TreeNode
 * @name tree_node
 * @string text
 * @number flags
 */
static int imgui_TreeNode(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    imgui_NewFrame();
    const char* text = luaL_checkstring(L, 1);
    uint32_t flags = 0;
    if (lua_isnumber(L, 2))
    {
        flags = luaL_checkint(L, 2);
    }
    bool result = ImGui::TreeNodeEx(text, flags);
    lua_pushboolean(L, result);
    return 1;
}

/** TreePop
 * @name tree_pop
 */
static int imgui_TreePop(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();
    ImGui::TreePop();
    return 0;
}

/** CollapsingHeader
 * @name collapsing_header
 * @string label
 * @bool visible Optional; if omitted or nil, no close button is shown.
 * @number flags Optional.
 * @treturn bool open
 */
static int imgui_CollapsingHeader(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    imgui_NewFrame();
    const char* label = luaL_checkstring(L, 1);
    bool* p_visible = 0;
    bool visible = true;
    uint32_t flags = 0;
    int argc = lua_gettop(L);
    if (argc >= 2)
    {
        if (lua_isboolean(L, 2))
        {
            visible = lua_toboolean(L, 2);
            p_visible = &visible;
        }
        else if (!lua_isnil(L, 2))
        {
            return luaL_error(L, "collapsing_header: arg2 must be boolean or nil");
        }
        if (argc >= 3)
        {
            flags = luaL_checkint(L, 3);
        }
    }
    bool result = ImGui::CollapsingHeader(label, p_visible, flags);
    lua_pushboolean(L, result);
    return 1;
}


// ----------------------------
// ----- Push/Pop ID ----------
// ----------------------------
/** PushId
 * @name push_id
 * @string text
 */
static int imgui_PushId(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    const char* text = luaL_checkstring(L, 1);
    ImGui::PushID(text);

    return 0;
}

/** PopId
 * @name pop_id
 */
static int imgui_PopId(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    ImGui::PopID();
    return 0;
}


// ----------------------------
// ----- WINDOW ---------------
// ----------------------------
/** Begin
 * @name begin_window
 * @string title
 * @bool is_open
 * @number flags
 * @treturn bool result
 * @treturn bool is_open
 */
static int imgui_Begin(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 2);
    imgui_NewFrame();
    const char* title = luaL_checkstring(L, 1);
    bool is_open = true;
    bool* is_open_ptr = NULL;
    if (lua_isboolean(L, 2) && lua_toboolean(L, 2))
    {
        is_open_ptr = &is_open;
    }
    uint32_t flags = 0;
    if (lua_isnumber(L, 3))
    {
        flags = luaL_checkint(L, 3);
    }
    bool result = ImGui::Begin(title, is_open_ptr, flags);
    lua_pushboolean(L, result);
    lua_pushboolean(L, is_open);
    return 2;
}
/** End
 * @name end_window
 */
static int imgui_End(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();
    ImGui::End();
    return 0;
}
/** SetNextWindowSize
 * @name set_next_window_size
 * @number width
 * @number height
 * @number cond
 */
static int imgui_SetNextWindowSize(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();
    float width = luaL_checknumber(L, 1);
    float height = luaL_checknumber(L, 2);
    uint32_t cond = ImGuiCond_None;
    if (lua_isnumber(L, 3))
    {
        cond = luaL_checkint(L, 3);
    }
    ImGui::SetNextWindowSize(ImVec2(width, height), cond);
    return 0;
}
/** SetNextWindowPos
 * @name set_next_window_pos
 * @number x
 * @number y
 * @number cond
 * @number pivot_x
 * @number pivot_y
 */
static int imgui_SetNextWindowPos(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();
    float x = luaL_checknumber(L, 1);
    float y = luaL_checknumber(L, 2);
    uint32_t cond = ImGuiCond_None;
    if (lua_isnumber(L, 3))
    {
        cond = luaL_checkint(L, 3);
    }
    ImVec2 pivot = ImVec2(0, 0);
    if (lua_isnumber(L, 4) && lua_isnumber(L, 5))
    {
        pivot.x = luaL_checknumber(L, 4);
        pivot.y = luaL_checknumber(L, 5);
    }
    ImGui::SetNextWindowPos(ImVec2(x, y), cond, pivot);
    return 0;
}
/** SetNextWindowCollapsed
* @name set_next_window_collapsed
* @bool collapsed
* @number cond
*/
static int imgui_SetNextWindowCollapsed(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();
    bool collapsed = lua_toboolean(L, 1);
    uint32_t cond = ImGuiCond_None;
    if (lua_isnumber(L, 2))
    {
        cond = luaL_checkint(L, 2);
    }
    ImGui::SetNextWindowCollapsed(collapsed, cond);
    return 0;
}
/** GetWindowSize
 * @name get_window_size
 * @treturn number width
 * @treturn number height
 */
static int imgui_GetWindowSize(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 2);
    imgui_NewFrame();
    ImVec2 size = ImGui::GetWindowSize();
    lua_pushnumber(L, size.x);
    lua_pushnumber(L, size.y);
    return 2;
}
/** GetWindowPos
 * @name get_window_pos
 * @treturn number x
 * @treturn number y
 */
static int imgui_GetWindowPos(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 2);
    imgui_NewFrame();
    ImVec2 pos = ImGui::GetWindowPos();
    lua_pushnumber(L, pos.x);
    lua_pushnumber(L, pos.y);
    return 2;
}
/** IsWindowFocused
 * @name is_window_focused
 * @number flags
 * @treturn boolean focused
 */
static int imgui_IsWindowFocused(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    imgui_NewFrame();
    uint32_t flags = 0;
    if (lua_isnumber(L, 1))
    {
        flags = luaL_checkint(L, 1);
    }
    bool focused = ImGui::IsWindowFocused(flags);
    lua_pushboolean(L, focused);
    return 1;
}

/** IsWindowHovered
 * @name is_window_hovered
 * @treturn bool result
 */
static int imgui_IsWindowHovered(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    imgui_NewFrame();
    bool hovered = ImGui::IsWindowHovered();
    lua_pushboolean(L, hovered);
    return 1;
}

/** IsWindowAppearing
 * @name is_window_appearing
 * @treturn bool result
 */
static int imgui_IsWindowAppearing(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    imgui_NewFrame();
    bool appearing = ImGui::IsWindowAppearing();
    lua_pushboolean(L, appearing);
    return 1;
}

/** GetContentRegionAvail
 * @name get_content_region_avail
 * @treturn number x
 * @treturn number y
 */
static int imgui_GetContentRegionAvail(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 2);
    imgui_NewFrame();
    ImVec2 region = ImGui::GetContentRegionAvail();
    lua_pushnumber(L, region.x);
    lua_pushnumber(L, region.y);
    return 2;
}

/** GetWindowContentRegionMax
 * @name get_window_content_region_max
 * @treturn number x
 * @treturn number y
 */
static int imgui_GetWindowContentRegionMax(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 2);
    imgui_NewFrame();
    ImVec2 region = ImGui::GetWindowContentRegionMax();
    lua_pushnumber(L, region.x);
    lua_pushnumber(L, region.y);
    return 2;
}

// ----------------------------
// ----- CHILD WINDOW ---------
// ----------------------------
/** BeginChild
 * @name begin_child
 * @string title
 * @number width
 * @number border
 * @number flags
 * @treturn bool result
 */
static int imgui_BeginChild(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    imgui_NewFrame();

    const char* title = luaL_checkstring(L, 1);
    float width = luaL_checknumber(L, 2);
    float height = luaL_checknumber(L, 3);
    bool border = false;
    if (lua_isnumber(L, 4)) border = luaL_checkinteger(L, 4) == 1;
    int flags = 0;
    if (lua_isnumber(L, 5)) flags = luaL_checknumber(L, 5);

    bool result = ImGui::BeginChild(title, ImVec2(width, height), border, flags);
    lua_pushboolean(L, result);

    return 1;
}
/** EndChild
 * @name end_child
 */
static int imgui_EndChild(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();
    ImGui::EndChild();
    return 0;
}


// ----------------------------
// ----- POPUP ---------
// ----------------------------

/** BeginPopupContextItem
 * @name begin_popup_context_item
 * If id is omitted, uses the last item ID (same as ImGui helper behavior).
 * Lua - call with no arguments to use the last item; passing nil is not accepted.
 * @string id Optional; if omitted, uses the last item ID.
 * @treturn boolean result
 */
static int imgui_BeginPopupContextItem(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    imgui_NewFrame();
    //if stack 1 then
    int stack = lua_gettop(L);
    if(stack == 1){
        const char* id = luaL_checkstring(L, 1);
        bool result = ImGui::BeginPopupContextItem(id);
        lua_pushboolean(L, result);
    }else{
        bool result = ImGui::BeginPopupContextItem();
        lua_pushboolean(L, result);
    }
    return 1;
}

/** BeginPopup
 * @name begin_popup
 * @string id
 * @number flags
 * @treturn boolean result
 */
static int imgui_BeginPopup(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    imgui_NewFrame();
    const char* id = luaL_checkstring(L, 1);
    int flags = 0;
    if (lua_isnumber(L, 2)) flags = luaL_checknumber(L, 2);
    bool result = ImGui::BeginPopup(id, flags);
    lua_pushboolean(L, result);
    return 1;
}

/** BeginPopupModal
 * @name begin_popup_modal
 * @string id
 * @number flags
 * @treturn boolean result
 */
static int imgui_BeginPopupModal(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    imgui_NewFrame();
    const char* name = luaL_checkstring(L, 1);
    int flags = 0;
    if (lua_isnumber(L, 2)) flags = luaL_checknumber(L, 2);
    bool result = ImGui::BeginPopupModal(name, 0, flags);
    lua_pushboolean(L, result);
    return 1;
}

/** OpenPopup
 * @name open_popup
 * @string id
 * @number flags
 * @treturn boolean result
*/
static int imgui_OpenPopup(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();
    const char* id = luaL_checkstring(L, 1);
    int flags = 0;
    if (lua_isnumber(L, 2)) flags = luaL_checknumber(L, 2);
    ImGui::OpenPopup(id, flags);
    return 0;
}

/** CloseCurrentPopup
 * @name close_current_popup
 */
static int imgui_CloseCurrentPopup(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();
    ImGui::CloseCurrentPopup();
    return 0;
}

/** EndPopup
 * @name end_popup
 */
static int imgui_EndPopup(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();
    ImGui::EndPopup();
    return 0;
}

// ----------------------------
// ----- DRAG AND DROP --------
// ----------------------------
/** BeginDragDropSource
 * @name begin_drag_drop_source
 * @number flags
 * @treturn boolean result
 */
static int imgui_BeginDragDropSource(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    imgui_NewFrame();
    uint32_t flags = 0;
    if (lua_isnumber(L, 1))
    {
        flags = luaL_checkint(L, 1);
    }
    bool result = ImGui::BeginDragDropSource(flags);
    lua_pushboolean(L, result);
    return 1;
}
/** EndDragDropSource
 * @name end_drag_drop_source
 */
static int imgui_EndDragDropSource(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();
    ImGui::EndDragDropSource();
    return 0;
}
/** BeginDragDropTarget
 * @name begin_drag_drop_target
 * @treturn boolean result
 */
static int imgui_BeginDragDropTarget(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    imgui_NewFrame();
    bool result = ImGui::BeginDragDropTarget();
    lua_pushboolean(L, result);
    return 1;
}
/** EndDragDropTarget
 * @name end_drag_drop_target
 */
static int imgui_EndDragDropTarget(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();
    ImGui::EndDragDropTarget();
    return 0;
}

/** SetDragDropPayload
 * @name set_drag_drop_payload
 * @string type
 * @string payload
 * @treturn boolean result
 */
static int imgui_SetDragDropPayload(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    imgui_NewFrame();
    const char* type = luaL_checkstring(L, 1);
    const char* payload = luaL_checkstring(L, 2);

    bool result = ImGui::SetDragDropPayload(type, payload, strlen(payload));
    lua_pushboolean(L, result);
    return 1;
}
/** AcceptDragDropPayload
 * @name accept_drag_drop_payload
 * @string type
 * @number flags
 * @treturn string payload
 */
static int imgui_AcceptDragDropPayload(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    imgui_NewFrame();
    const char* type = luaL_checkstring(L, 1);
    uint32_t flags = 0;
    if (lua_isnumber(L, 2))
    {
        flags = luaL_checkint(L, 2);
    }
    const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(type, flags);
    if (payload) {
        lua_pushlstring(L, (char*)payload->Data, payload->DataSize);
    }
    else {
        lua_pushnil(L);
    }
    return 1;
}

// ----------------------------
// ----- COMBO ---------
// ----------------------------
/** BeginCombo
 * @name begin_combo
 * @string label
 * @string preview
 * @treturn boolean result
 */
static int imgui_BeginCombo(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    imgui_NewFrame();
    const char* label = luaL_checkstring(L, 1);
    const char* preview = luaL_checkstring(L, 2);
    bool result = ImGui::BeginCombo(label, preview);
    lua_pushboolean(L, result);
    return 1;
}
/** EndCombo
 * @name end_combo
 */
static int imgui_EndCombo(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();
    ImGui::EndCombo();
    return 0;
}
/** Combo
 * @name combo
 * @string label
 * @number current
 * @table items
 * @treturn boolean result
 * @treturn number current
 */
static int imgui_Combo(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 2);
    imgui_NewFrame();
    const char* label = luaL_checkstring(L, 1);

    int current = luaL_checknumber(L, 2) - 1;
    if(!lua_istable(L, 3))
    {
        luaL_error(L, "You must provide a table");
    }

    int next = current;

    // get the current value and use it as the combo box preview
    lua_pushnumber(L, current + 1);
    lua_gettable(L, 3);
    const char* preview = luaL_checkstring(L, -1);
    bool result = ImGui::BeginCombo(label, preview);
    lua_pop(L, 1); // pop the current value

    if (result)
    {
        // iterate the full list of values and create Selectables for each
        const size_t len = lua_objlen(L, 3);
        for(int i=0; i<len; i++)
        {
            lua_pushnumber(L, i + 1);
            lua_gettable(L, 3);
            const char* item = luaL_checkstring(L, -1);
            bool selected = (i == current);
            bool clicked = ImGui::Selectable(item, &selected, 0);
            lua_pop(L, 1); // pop the item
            if (clicked)
            {
                next = i;
            }
        }
        ImGui::EndCombo();
    }
    lua_pushboolean(L, result);
    lua_pushnumber(L, next + 1);

    return 2;
}


// ----------------------------
// ----- TABLES ---------
// ----------------------------
/** BeginTable
 * @name begin_table
 * @string id
 * @number column
 * @number [flags]
 * @number [x]
 * @number [y]
 * @treturn boolean result
 */
static int imgui_BeginTable(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    imgui_NewFrame();
    const char* id = luaL_checkstring(L, 1);
    int column = luaL_checkinteger(L, 2);
    uint32_t flags = 0;
    if (lua_isnumber(L, 3))
    {
        flags = luaL_checkint(L, 3);
    }
    ImVec2 outer_size = ImVec2(0.0f, 0.0f);
    if (lua_isnumber(L, 4) && lua_isnumber(L, 5))
    {
        outer_size.x = luaL_checknumber(L, 4);
        outer_size.y = luaL_checknumber(L, 5);
    }
    bool result = ImGui::BeginTable(id, column, flags, outer_size);
    lua_pushboolean(L, result);
    return 1;
}
/** EndTable
 * @name end_table
 */
static int imgui_EndTable(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();
    ImGui::EndTable();
    return 0;
}
/** TableHeadersRow
 * @name table_headers_row
 */
static int imgui_TableHeadersRow(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();
    ImGui::TableHeadersRow();
    return 0;
}
/** TableSetupColumn
 * @name table_setup_column
 * @string label
 * @number [flags]
 * @number [weight]
 */
static int imgui_TableSetupColumn(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();
    const char* label = luaL_checkstring(L, 1);
    uint32_t flags = 0;
    if (lua_isnumber(L, 2))
    {
        flags = luaL_checkint(L, 2);
    }
    float weight = 0.0;
    if (lua_isnumber(L, 3))
    {
        weight = luaL_checknumber(L, 3);
    }
    ImGui::TableSetupColumn(label, flags, weight);
    return 0;
}
/** TableSetColumnIndex
 * @name table_set_column_index
 * @number column_index
 */
static int imgui_TableSetColumnIndex(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();
    int column = luaL_checkinteger(L, 1);
    ImGui::TableSetColumnIndex(column);
    return 0;
}
/** TableNextColumn
 * @name table_next_column
 */
static int imgui_TableNextColumn(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();
    ImGui::TableNextColumn();
    return 0;
}
/** TableNextRow
 * @name table_next_row
 */
static int imgui_TableNextRow(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();
    ImGui::TableNextRow();
    return 0;
}


/** TableSetupScrollFreeze
 * @name table_setup_scroll_freeze
 * @number freeze_cols
 * @number freeze_rows
 */
static int imgui_TableSetupScrollFreeze(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();
    int freeze_cols = luaL_checkinteger(L, 1);
    int freeze_rows = luaL_checkinteger(L, 2);
    ImGui::TableSetupScrollFreeze(freeze_cols, freeze_rows);
    return 0;
}


// ----------------------------
// ----- TOOLTIP ---------
// ----------------------------
/** BeginTooltip
 * @name begin_tooltip
 */
static int imgui_BeginTooltip(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();
    ImGui::BeginTooltip();
    return 0;
}
/** EndTooltip
 * @name end_tooltip
 */
static int imgui_EndTooltip(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();
    ImGui::EndTooltip();
    return 0;
}


// ----------------------------
// ----- TAB BAR ---------
// ----------------------------
/** BeginTabBar
 * @name begin_tab_bar
 */
static int imgui_BeginTabBar(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    imgui_NewFrame();
    const char* id = luaL_checkstring(L, 1);
    bool result = ImGui::BeginTabBar(id);
    lua_pushboolean(L, result);
    return 1;
}
/** EndTabBar
 * @name end_tab_bar
 */
static int imgui_EndTabBar(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();
    ImGui::EndTabBar();
    return 0;
}
/** BeginTabItem
 * @name begin_tab_item
 */
static int imgui_BeginTabItem(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 2);
    imgui_NewFrame();
    const char* label = luaL_checkstring(L, 1);

    bool* p_open = 0;
    bool open = 1;
    if (lua_isboolean(L, 2))
    {
        open = lua_toboolean(L, 2);
        p_open = &open;
    }

    uint32_t flags = 0;
    if (lua_isnumber(L, 3))
    {
        flags = luaL_checkint(L, 3);
    }
    bool result = ImGui::BeginTabItem(label, p_open, flags);
    lua_pushboolean(L, result);
    lua_pushboolean(L, open);
    return 2;
}
/** EndTabItem
 * @name end_tab_item
 */
static int imgui_EndTabItem(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();
    ImGui::EndTabItem();
    return 0;
}


// ----------------------------
// ----- WIDGETS ---------
// ----------------------------

/** Text
 * @name text
 * @string text
 * @number [wrapped]
 */
static int imgui_Text(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();
    const char* text = luaL_checkstring(L, 1);
    int wrapped =  0;
    if(lua_isnumber(L, 2)) wrapped = luaL_checknumber(L, 2);

    if(wrapped == 1)
    {
        ImGui::TextWrapped("%s", text);
    }
    else
    {
        ImGui::Text("%s", text);
    }
    return 0;
}

/** TextGetSize
 * @name text_get_size
 * @string text
 * @number font_size
 * @number [fontid]
 * @treturn number width
 * @treturn number height
 */
static int imgui_TextGetSize(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 2);
    int argc = lua_gettop(L);
    const char* text = luaL_checkstring(L, 1);
    float font_size = luaL_checknumber(L, 2);
    int fontid = 0;
    if(argc > 2)
    {
        fontid = luaL_checkinteger(L, 3);
    }
    ImFont *font = g_imgui_Fonts[fontid];
    ImVec2 sz = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, text);

    lua_pushnumber(L, sz.x);
    lua_pushnumber(L, sz.y);
    return 2;
}

/** TextColored
 * @name text_colored
 * @string text
 * @number r
 * @number g
 * @number b
 * @number a
 */
static int imgui_TextColored(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();
    const char* text = luaL_checkstring(L, 1);
    float r = (float)luaL_checknumber(L, 2);
    float g = (float)luaL_checknumber(L, 3);
    float b = (float)luaL_checknumber(L, 4);
    float a = (float)luaL_checknumber(L, 5);
    ImVec4    color(r, g, b, a);
    ImGui::TextColored(color, "%s", text);
    return 0;
}

/** InputText
 * @name input_text
 * @string label
 * @string text
 * @number [flags]
 * @treturn boolean changed
 * @treturn string text
 */
static int imgui_InputText(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 2);
    imgui_NewFrame();
    const char* label = luaL_checkstring(L, 1);
    const char* text = luaL_checkstring(L, 2);
    dmStrlCpy(g_imgui_TextBuffer, text, TEXTBUFFER_SIZE);
    uint32_t flags = 0;
    if (lua_isnumber(L, 3))
    {
        flags = luaL_checkint(L, 3);
    }
    bool changed = ImGui::InputText(label, g_imgui_TextBuffer, TEXTBUFFER_SIZE, flags);
    lua_pushboolean(L, changed);
    if (changed)
    {
        lua_pushstring(L, g_imgui_TextBuffer);
    }
    else
    {
        lua_pushnil(L);
    }
    return 2;
}

/** InputInt
 * @name input_int
 * @string label
 * @number value
 * @number [flags]
 * @treturn boolean changed
 * @treturn number value
 */
static int imgui_InputInt(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 2);
    imgui_NewFrame();
    const char* label = luaL_checkstring(L, 1);
    int value = luaL_checkinteger(L, 2);
    bool changed = ImGui::InputInt(label, &value);
    lua_pushboolean(L, changed);
    if (changed)
    {
        lua_pushinteger(L, value);
    }
    else
    {
        lua_pushnil(L);
    }
    return 2;
}

/** InputFloat
 * @name input_float
 * @string label
 * @number value
 * @number step
 * @number step_fast
 * @number precision_count
 * @number [flags]
 * @treturn boolean changed
 * @treturn number value
 */
static int imgui_InputFloat(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 2);
    imgui_NewFrame();
    const char* label = luaL_checkstring(L, 1);
    float value = luaL_checknumber(L, 2);
    float step = 0.01f;
    float step_fast = 1.0f;
    char float_precision[20] = { "%.6f" };

    // check if the third argument is a number
    if (lua_isnumber(L, 3))
    {
        step = luaL_checknumber(L, 3);
        // Only accept 4th if we have 3rd param
        if (lua_isnumber(L, 4))
        {
            step_fast = luaL_checknumber(L, 4);

            // Only accept 5th if we have 4th param
            if (lua_isnumber(L, 5))
            {
                int precision_count = lua_tointeger(L, 5);
                dmSnPrintf(float_precision, sizeof(float_precision), "%%.%df", precision_count );
            }
        }
    }
    bool changed = ImGui::InputFloat(label, &value, step, step_fast, float_precision);
    lua_pushboolean(L, changed);
    if (changed)
    {
        lua_pushnumber(L, value);
    }
    else
    {
        lua_pushnil(L);
    }
    return 2;
}

/** InputDouble
 * @name input_double
 * @string label
 * @number value
 * @number step
 * @number step_fast
 * @number precision_count
 * @number [flags]
 * @treturn boolean changed
 * @treturn number value
 */
static int imgui_InputDouble(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 2);
    imgui_NewFrame();
    const char* label = luaL_checkstring(L, 1);
    double value = luaL_checknumber(L, 2);
    double step = 0.01f;
    double step_fast = 1.0f;
    char dbl_precision[20] = { "%.6f" };

    // check if the third argument is a number
    if (lua_isnumber(L, 3))
    {
        step = luaL_checknumber(L, 3);
        // Only accept 4th if we have 3rd param
        if (lua_isnumber(L, 4))
        {
            step_fast = luaL_checknumber(L, 4);

            // Only accept 5th if we have 4th param
            if (lua_isnumber(L, 5))
            {
                int precision_count = lua_tointeger(L, 5);
                dmSnPrintf(dbl_precision, sizeof(dbl_precision), "%%.%df", precision_count );
            }
        }
    }
    bool changed = ImGui::InputDouble(label, &value, step, step_fast, dbl_precision);
    lua_pushboolean(L, changed);
    if (changed)
    {
        lua_pushnumber(L, value);
    }
    else
    {
        lua_pushnil(L);
    }
    return 2;
}

/** InputInt4
 * @name input_int4
 * @string label
 * @number v1
 * @number v2
 * @number v3
 * @number v4
 * @treturn number v1
 * @treturn number v2
 * @treturn number v3
 * @treturn number v4
 */
static int imgui_InputInt4(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 5);
    imgui_NewFrame();
    const char* label = luaL_checkstring(L, 1);
    int v[4];
    v[0]  = luaL_checkinteger(L, 2);
    v[1]  = luaL_checkinteger(L, 3);
    v[2]  = luaL_checkinteger(L, 4);
    v[3]  = luaL_checkinteger(L, 5);

    bool changed = ImGui::InputInt4(label, v);
    lua_pushboolean(L, changed);
    if (changed)
    {
        lua_pushnumber(L, v[0]);
        lua_pushnumber(L, v[1]);
        lua_pushnumber(L, v[2]);
        lua_pushnumber(L, v[3]);
    }
    else
    {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
    }
    return 5;
}

/** InputInt2
 * @name input_int2
 * @string label
 * @number v1
 * @number v2
 * @treturn number v1
 * @treturn number v2
 */
static int imgui_InputInt2(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 3);
    imgui_NewFrame();
    const char* label = luaL_checkstring(L, 1);
    int v[2];
    v[0]  = luaL_checkinteger(L, 2);
    v[1]  = luaL_checkinteger(L, 3);

    bool changed = ImGui::InputInt2(label, v);
    lua_pushboolean(L, changed);
    if (changed)
    {
        lua_pushnumber(L, v[0]);
        lua_pushnumber(L, v[1]);
    }
    else
    {
        lua_pushnil(L);
        lua_pushnil(L);
    }
    return 3;
}


/** InputFloat3
 * @name input_float3
 * @string label
 * @number v1
 * @number v2
 * @number v3
 * @treturn number v1
 * @treturn number v2
 * @treturn number v3
 */
static int imgui_InputFloat3(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 4);
    imgui_NewFrame();
    const char* label = luaL_checkstring(L, 1);
    float v[3];
    v[0]  = luaL_checknumber(L, 2);
    v[1]  = luaL_checknumber(L, 3);
    v[2]  = luaL_checknumber(L, 4);

    bool changed = ImGui::InputFloat3(label, v);
    lua_pushboolean(L, changed);
    if (changed)
    {
        lua_pushnumber(L, v[0]);
        lua_pushnumber(L, v[1]);
        lua_pushnumber(L, v[2]);
    }
    else
    {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
    }
    return 4;
}

/** InputFloat4
 * @name input_float4
 * @string label
 * @number v1
 * @number v2
 * @number v3
 * @number v4
 * @treturn number v1
 * @treturn number v2
 * @treturn number v3
 * @treturn number v4
 */
static int imgui_InputFloat4(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 5);
    imgui_NewFrame();
    const char* label = luaL_checkstring(L, 1);
    float v[4];
    v[0]  = luaL_checknumber(L, 2);
    v[1]  = luaL_checknumber(L, 3);
    v[2]  = luaL_checknumber(L, 4);
    v[3]  = luaL_checknumber(L, 5);

    bool changed = ImGui::InputFloat4(label, v);
    lua_pushboolean(L, changed);
    if (changed)
    {
        lua_pushnumber(L, v[0]);
        lua_pushnumber(L, v[1]);
        lua_pushnumber(L, v[2]);
        lua_pushnumber(L, v[3]);
    }
    else
    {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
    }
    return 5;
}

/** DragFloat
 * @name drag_float
 * @string label
 * @number value
 * @number speed
 * @number min
 * @number max
 * @number precision
 * @treturn number value
 */
static int imgui_DragFloat(lua_State* L)
{
    imgui_NewFrame();
    const char* label = luaL_checkstring(L, 1);
    float value = luaL_checknumber(L, 2);
    float speed = luaL_checknumber(L, 3);
    float min = luaL_checknumber(L, 4);
    float max = luaL_checknumber(L, 5);
    char float_precision[20] = { "%.6f" };

    if (lua_isnumber(L, 6))
    {
        int precision_count = lua_tointeger(L, 6);
        dmSnPrintf(float_precision, sizeof(float_precision), "%%.%df", precision_count );
    }

    bool changed = ImGui::DragFloat(label, &value, speed, min, max, float_precision);
    lua_pushboolean(L, changed);
    if (changed)
    {
        lua_pushnumber(L, value);
    }
    else
    {
        lua_pushnil(L);
    }
    return 2;
}

/** SliderFloat
 * @name slider_float
 * @string label
 * @number value
 * @number min
 * @number max
 * @number precision
 * @treturn number value
 */
static int imgui_SliderFloat(lua_State* L)
{
    imgui_NewFrame();
    const char* label = luaL_checkstring(L, 1);
    float value = luaL_checknumber(L, 2);
    float min = luaL_checknumber(L, 3);
    float max = luaL_checknumber(L, 4);
    char float_precision[20] = { "%.6f" };

    if (lua_isnumber(L, 5))
    {
        int precision_count = lua_tointeger(L, 5);
        dmSnPrintf(float_precision, sizeof(float_precision), "%%.%df", precision_count );
    }

    bool changed = ImGui::SliderFloat(label, &value, min, max, float_precision);
    lua_pushboolean(L, changed);
    if (changed)
    {
        lua_pushnumber(L, value);
    }
    else
    {
        lua_pushnil(L);
    }
    return 2;
}

/** SliderFloat3
 * @name slider_float3
 * @string label
 * @number value1
 * @number value2
 * @number value3
 * @number min
 * @number max
 * @number precision
 * @treturn number value1
 * @treturn number value2
 * @treturn number value3
 */
static int imgui_SliderFloat3(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 4);
    imgui_NewFrame();
    const char* label = luaL_checkstring(L, 1);
    float v[3];
    v[0] = luaL_checknumber(L, 2);
    v[1] = luaL_checknumber(L, 3);
    v[2] = luaL_checknumber(L, 4);
    float min = luaL_checknumber(L, 5);
    float max = luaL_checknumber(L, 6);
    char float_precision[20] = { "%.6f" };

    if (lua_isnumber(L, 7))
    {
        int precision_count = lua_tointeger(L, 7);
        dmSnPrintf(float_precision, sizeof(float_precision), "%%.%df", precision_count);
    }

    bool changed = ImGui::SliderFloat3(label, v, min, max, float_precision);
    lua_pushboolean(L, changed);
    if (changed)
    {
        lua_pushnumber(L, v[0]);
        lua_pushnumber(L, v[1]);
        lua_pushnumber(L, v[2]);
    }
    else
    {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
    }
    return 4;
}

/** SliderFloat4
 * @name slider_float4
 * @string label
 * @number value1
 * @number value2
 * @number value3
 * @number value4
 * @number min
 * @number max
 * @number precision
 * @treturn number value1
 * @treturn number value2
 * @treturn number value3
 * @treturn number value4
 */
static int imgui_SliderFloat4(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 5);
    imgui_NewFrame();
    const char* label = luaL_checkstring(L, 1);
    float v[4];
    v[0] = luaL_checknumber(L, 2);
    v[1] = luaL_checknumber(L, 3);
    v[2] = luaL_checknumber(L, 4);
    v[3] = luaL_checknumber(L, 5);
    float min = luaL_checknumber(L, 6);
    float max = luaL_checknumber(L, 7);
    char float_precision[20] = { "%.6f" };

    if (lua_isnumber(L, 8))
    {
        int precision_count = lua_tointeger(L, 8);
        dmSnPrintf(float_precision, sizeof(float_precision), "%%.%df", precision_count);
    }

    bool changed = ImGui::SliderFloat4(label, v, min, max, float_precision);
    lua_pushboolean(L, changed);
    if (changed)
    {
        lua_pushnumber(L, v[0]);
        lua_pushnumber(L, v[1]);
        lua_pushnumber(L, v[2]);
        lua_pushnumber(L, v[3]);
    }
    else
    {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
    }
    return 5;
}


/** ColorEdit4
 * @name color_edit4
 * @string label
 * @vec4 color
 * @number flags
 */
static int imgui_ColorEdit4(lua_State* L)
{
    imgui_NewFrame();
    const char* label = luaL_checkstring(L, 1);
    dmVMath::Vector4* color = dmScript::CheckVector4(L, 2);
    uint32_t flags = 0x0;
    if (lua_isnumber(L, 3))
    {
        flags = luaL_checknumber(L, 3);
    }

    ImVec4 c(color->getX(), color->getY(), color->getZ(), color->getW());
    ImGui::ColorEdit4(label, (float*)&c, flags);
    color->setX(c.x);
    color->setY(c.y);
    color->setZ(c.z);
    color->setW(c.w);
    return 0;
}




/** Selectable
 * @name selectable
 * @string text
 * @boolean selected
 * @number [flags]
 * @treturn boolean selected
 */
static int imgui_Selectable(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 2);
    imgui_NewFrame();
    const char* text = luaL_checkstring(L, 1);
    bool selected = lua_toboolean(L, 2);
    uint32_t flags = 0;
    if (lua_isnumber(L, 3))
    {
        flags = luaL_checkint(L, 3);
    }
    bool pushed = ImGui::Selectable(text, &selected, flags);
    lua_pushboolean(L, selected);
    lua_pushboolean(L, pushed);
    return 2;
}

/** Button
 * @name button
 * @string text
 * @number [width]
 * @number [height]
 * @treturn boolean pushed
 */
static int imgui_Button(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    int argc = lua_gettop(L);
    imgui_NewFrame();
    const char* text = luaL_checkstring(L, 1);
    bool pushed = false;
    if(argc > 1)
    {
        int width = luaL_checkinteger(L, 2);
        int height = luaL_checkinteger(L, 3);
        pushed = ImGui::Button(text, ImVec2(width, height));
    }
    else
    {
        pushed = ImGui::Button(text);
    }
    lua_pushboolean(L, pushed);
    return 1;
}

/** SmallButton
 * @name small_button
 * @string text
 * @treturn boolean pushed
 */
static int imgui_SmallButton(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    imgui_NewFrame();
    const char* text = luaL_checkstring(L, 1);
    bool pushed = ImGui::SmallButton(text);
    lua_pushboolean(L, pushed);
    return 1;
}

/** ButtonImage
 * @name button_image
 * @number texture_id
 * @number [width]
 * @number [height]
 * @treturn boolean pushed
 */
static int imgui_ButtonImage(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    int argc = lua_gettop(L);
    imgui_NewFrame();
    GLuint tid = (GLuint)luaL_checknumber(L, 1);
    bool pushed = false;
    if(argc > 1)
    {
        int width = luaL_checkinteger(L, 2);
        int height = luaL_checkinteger(L, 3);
        pushed = ImGui::ImageButton((void*)(intptr_t)tid, ImVec2(width, height));
    }
    else
    {
        pushed = ImGui::ImageButton((void*)(intptr_t)tid, ImVec2(0,0));
    }
    lua_pushboolean(L, pushed);
    return 1;
}

/** ButtonArrow
 * @name button_arrow
 * @string label
 * @number direction
 * @treturn bool pushed
 */
static int imgui_ButtonArrow(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    imgui_NewFrame();
    const char* label = luaL_checkstring(L, 1);
    uint32_t direction = luaL_checkint(L, 2);
    bool pushed = ImGui::ArrowButton(label, direction);
    lua_pushboolean(L, pushed);
    return 1;
}

/** Checkbox
 * @name checkbox
 * @string text
 * @boolean checked
 * @treturn boolean changed
 * @treturn boolean checked
 */
static int imgui_Checkbox(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 2);
    imgui_NewFrame();
    const char* text = luaL_checkstring(L, 1);
    bool checked = lua_toboolean(L, 2);
    bool changed = ImGui::Checkbox(text, &checked);
    lua_pushboolean(L, changed);
    lua_pushboolean(L, checked);
    return 2;
}

/** RadioButton
 * @name radio_button
 * @string text
 * @boolean checked
 * @treturn boolean changed
 * @treturn boolean clicked
 */
static int imgui_RadioButton(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 2);
    imgui_NewFrame();
    const char* text = luaL_checkstring(L, 1);
    bool checked = lua_toboolean(L, 2);
    bool changed = ImGui::RadioButton(text, checked);
    lua_pushboolean(L, changed);
    lua_pushboolean(L, checked);
    return 2;
}

/** BeginMenuBar
 * @name begin_menu_bar
 * @treturn boolean result
 */
static int imgui_BeginMenuBar(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    imgui_NewFrame();

    bool result = ImGui::BeginMenuBar();
    lua_pushboolean(L, result);

    return 1;
}

/** EndMenuBar
 * @name end_menu_bar
 */
static int imgui_EndMenuBar(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();

    ImGui::EndMenuBar();

    return 0;
}

/** BeginMainMenuBar
 * @name begin_main_menu_bar
 * @treturn boolean result
 */
static int imgui_BeginMainMenuBar(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    imgui_NewFrame();

    bool result = ImGui::BeginMainMenuBar();
    lua_pushboolean(L, result);

    return 1;
}

/** EndMainMenuBar
 * @name end_main_menu_bar
 */
static int imgui_EndMainMenuBar(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();

    ImGui::EndMainMenuBar();

    return 0;
}

/** BeginMenu
 * @name begin_menu
 * @string label
 * @boolean [enabled]
 * @treturn boolean result
 */
static int imgui_BeginMenu(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    imgui_NewFrame();

    const char* label = luaL_checkstring(L, 1);
    bool enabled = true;

    if (lua_isboolean(L, 2)) {
        enabled = lua_toboolean(L, 2);
    }

    bool result = ImGui::BeginMenu(label, enabled);
    lua_pushboolean(L, result);

    return 1;
}

/** EndMenu
 * @name end_menu
 */
static int imgui_EndMenu(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();

    ImGui::EndMenu();

    return 0;
}

/** MenuItem
 * @name menu_item
 * @string label
 * @string shortcut
 * @boolean selected
 * @boolean [enabled]
 * @treturn boolean result
 * @treturn boolean selected
 */
static int imgui_MenuItem(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 2);
    imgui_NewFrame();

    const char* label = luaL_checkstring(L, 1);
    const char* shortcut = lua_tostring(L, 2);
    bool selected = lua_toboolean(L, 3);
    bool enabled = true;

    if (lua_isboolean(L, 4)) {
        enabled = lua_toboolean(L, 4);
    }

    bool result = ImGui::MenuItem(label, shortcut, &selected, enabled);
    lua_pushboolean(L, result);
    lua_pushboolean(L, selected);

    return 2;
}

// ----------------------------
// ----- LAYOUT ---------
// ----------------------------
/** SameLine
 * @name same_line
 * @number [offset]
 */
static int imgui_SameLine(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();

    float offset = 0.0f;
    if (lua_isnumber(L, 1))
    {
        offset = lua_tonumber(L, 1);
    }
    ImGui::SameLine(offset);
    return 0;
}
/** NewLine
 * @name new_line
 */
static int imgui_NewLine(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();
    ImGui::NewLine();
    return 0;
}

/** Bullet
 * @name bullet
 */
static int imgui_Bullet(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();
    ImGui::Bullet();
    return 0;
}

/** Indent
 * @name indent
 */
static int imgui_Indent(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();
    ImGui::Indent();
    return 0;
}

/** Unindent
 * @name unindent
 */
static int imgui_Unindent(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();
    ImGui::Unindent();
    return 0;
}

/** Spacing
 * @name spacing
 */
static int imgui_Spacing(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();
    ImGui::Spacing();
    return 0;
}

/** Separator
 * @name separator
 */
static int imgui_Separator(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();
    ImGui::Separator();
    return 0;
}

/** GetCursorScreenPos
 * @name get_cursor_screen_pos
 * @treturn number x Cursor screen x position
 * @treturn number y Cursor screen y position
 */
static int imgui_GetCursorScreenPos(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 2);

    ImVec2 p = ImGui::GetCursorScreenPos();
    lua_pushnumber(L, p.x);
    lua_pushnumber(L, p.y);
    return 2;
}

/** GetCursorPos
 * @name get_cursor_pos
 * @treturn number x Cursor x position
 * @treturn number y Cursor y position
 */
static int imgui_GetCursorPos(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 2);

    ImVec2 p = ImGui::GetCursorPos();
    lua_pushnumber(L, p.x);
    lua_pushnumber(L, p.y);
    return 2;
}

// ----------------------------
// ----- IMGUI PLOT -----------
// ----------------------------

static float     values_lines[MAX_HISTOGRAM_VALUES];

/** PlotLines
 * @name plot_lines
 * @string label
 * @integer offset
 * @integer width
 * @integer height
 * @number[] values
 */
static int imgui_PlotLines(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    const char *lbl = luaL_checkstring(L, 1);
    int valoff = luaL_checkinteger(L, 2);
    int width = luaL_checkinteger(L, 3);
    int height = luaL_checkinteger(L, 4);
    luaL_checktype(L, 5, LUA_TTABLE);

    // Table is at idx 5
    lua_pushnil(L);
    int valct = 0;
    // Build a number array matching the buffer. They are all assumed to be type float (for the time being)
    while(( lua_next( L, 5 ) != 0) && (valct < MAX_HISTOGRAM_VALUES)) {
        values_lines[valct++] = (float)lua_tonumber( L, -1 );
        lua_pop( L, 1 );
    }

    imgui_NewFrame();
    ImVec2    gsize(width, height);
    ImGui::PlotLines(lbl, values_lines, valct, valoff, NULL, FLT_MAX, FLT_MAX, gsize);
    return 0;
}

// Keep a label mapped histograms. This minimised alloc and realloc of value mem
static float     values_hist[MAX_HISTOGRAM_VALUES];

/** PlotHistogram
 * @name plot_histogram
 * @string label
 * @integer offset
 * @integer width
 * @integer height
 * @table values
 */
static int imgui_PlotHistogram(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    const char *lbl = luaL_checkstring(L, 1);
    int valoff = luaL_checkinteger(L, 2);
    int width = luaL_checkinteger(L, 3);
    int height = luaL_checkinteger(L, 4);
    luaL_checktype(L, 5, LUA_TTABLE);

    // Table is at idx 5
    lua_pushnil(L);
    int valct = 0;
    // Build a number array matching the buffer. They are all assumed to be type float (for the time being)
    while(( lua_next( L, 5 ) != 0) && (valct < MAX_HISTOGRAM_VALUES)) {
        values_hist[valct++] = (float)lua_tonumber( L, -1 );
        lua_pop( L, 1 );
    }

    imgui_NewFrame();
    ImVec2    gsize(width, height);
    ImGui::PlotHistogram(lbl, values_hist, valct, valoff, NULL, FLT_MAX, FLT_MAX, gsize);
    return 0;
}

// ----------------------------
// ----- IMGUI DEMO -----------
// ----------------------------
/** Demo
 * @name demo
 */
static int imgui_Demo(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();
    bool show_demo_window = true;
    ImGui::ShowDemoWindow(&show_demo_window);
    return 0;
}



// ----------------------------
// ----- INPUT ----------------
// ----------------------------
/** IsMouseDoubleClicked
 * @name is_mouse_double_clicked
 * @number button
 * @treturn boolean double_clicked
 */
static int imgui_IsMouseDoubleClicked(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    imgui_NewFrame();
    uint32_t button = luaL_checknumber(L, 1);
    bool clicked = ImGui::IsMouseDoubleClicked(button);
    lua_pushboolean(L, clicked);
    return 1;
}

/** IsMouseClicked
 * @name is_mouse_clicked
 * @number button
 * @treturn boolean clicked
 */
static int imgui_IsMouseClicked(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    imgui_NewFrame();
    uint32_t button = luaL_checknumber(L, 1);
    bool clicked = ImGui::IsMouseClicked(button);
    lua_pushboolean(L, clicked);
    return 1;
}

/** IsItemActive
 * @name is_item_active
 * @treturn boolean active
 */
static int imgui_IsItemActive(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    imgui_NewFrame();
    bool active = ImGui::IsItemActive();
    lua_pushboolean(L, active);
    return 1;
}

/** IsItemFocused
 * @name is_item_focused
 * @treturn boolean focused
 */
static int imgui_IsItemFocused(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    imgui_NewFrame();
    bool focused = ImGui::IsItemFocused();
    lua_pushboolean(L, focused);
    return 1;
}

/** IsItemClicked
 * @name is_item_clicked
 * @treturn boolean clicked
 */
static int imgui_IsItemClicked(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    imgui_NewFrame();
    uint32_t button = luaL_checknumber(L, 1);
    bool clicked = ImGui::IsItemClicked(button);
    lua_pushboolean(L, clicked);
    return 1;
}

/** IsItemDoubleClicked
 * @name is_item_double_clicked
 * @treturn boolean double_clicked
 */
static int imgui_IsItemDoubleClicked(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    imgui_NewFrame();
    uint32_t button = luaL_checknumber(L, 1);
    bool clicked = ImGui::IsItemClicked(button);
    bool double_clicked = ImGui::IsMouseDoubleClicked(button);
    lua_pushboolean(L, clicked && double_clicked);
    return 1;
}

/** IsItemHovered
 * @name is_item_hovered
 * @treturn boolean hovered
 */
static int imgui_IsItemHovered(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    imgui_NewFrame();
    bool hovered = ImGui::IsItemHovered();
    lua_pushboolean(L, hovered);
    return 1;
}

/** GetItemRectMax
 * @name get_item_rect_max
 * @treturn number x
 * @treturn number y
 */
static int imgui_GetItemRectMax(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 2);
    imgui_NewFrame();
    ImVec2 rect = ImGui::GetItemRectMax();
    lua_pushnumber(L, rect.x);
    lua_pushnumber(L, rect.y);
    return 2;
}

/** SetKeyboardFocusHere
 * @name set_keyboard_focus_here
 * @number offset
 */
static int imgui_SetKeyboardFocusHere(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();
    int offset = luaL_checknumber(L, 1);
    ImGui::SetKeyboardFocusHere(offset);
    return 0;
}

// ----------------------------
// ----- STYLE ----------------
// ----------------------------


/** GetStyle
 * @name get_style
 * @treturn table style
 */
static int imgui_GetStyle(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    ImGuiStyle& style = ImGui::GetStyle();

    lua_newtable(L);

    lua_pushliteral(L, "Alpha");        // float
    lua_pushnumber(L, style.Alpha);
    lua_rawset(L, -3);

    lua_pushliteral(L, "DisabledAlpha");        // float
    lua_pushnumber(L, style.DisabledAlpha);
    lua_rawset(L, -3);

    lua_pushliteral(L, "WindowPadding");        // ImVec2
    dmScript::PushVector3(L, dmVMath::Vector3(style.WindowPadding.x, style.WindowPadding.y, 0));
    lua_rawset(L, -3);

    lua_pushliteral(L, "WindowRounding");       // float
    lua_pushnumber(L, style.WindowRounding);
    lua_rawset(L, -3);

    lua_pushliteral(L, "WindowBorderSize");     // float
    lua_pushnumber(L, style.WindowBorderSize);
    lua_rawset(L, -3);

    lua_pushliteral(L, "WindowMinSize");        // ImVec2
    dmScript::PushVector3(L, dmVMath::Vector3(style.WindowMinSize.x, style.WindowMinSize.y, 0));
    lua_rawset(L, -3);

    lua_pushliteral(L, "WindowTitleAlign");     // ImVec2
    dmScript::PushVector3(L, dmVMath::Vector3(style.WindowTitleAlign.x, style.WindowTitleAlign.y, 0));
    lua_rawset(L, -3);

    lua_pushliteral(L, "WindowMenuButtonPosition");     // ImGuiDir
    lua_pushnumber(L, style.WindowMenuButtonPosition);
    lua_rawset(L, -3);

    lua_pushliteral(L, "ChildRounding");        // float
    lua_pushnumber(L, style.ChildRounding);
    lua_rawset(L, -3);

    lua_pushliteral(L, "ChildBorderSize");      // float
    lua_pushnumber(L, style.ChildBorderSize);
    lua_rawset(L, -3);

    lua_pushliteral(L, "PopupRounding");        // float
    lua_pushnumber(L, style.PopupRounding);
    lua_rawset(L, -3);

    lua_pushliteral(L, "PopupBorderSize");      // float
    lua_pushnumber(L, style.PopupBorderSize);
    lua_rawset(L, -3);

    lua_pushliteral(L, "FramePadding");     // ImVec2
    dmScript::PushVector3(L, dmVMath::Vector3(style.FramePadding.x, style.FramePadding.y, 0));
    lua_rawset(L, -3);

    lua_pushliteral(L, "FrameRounding");        // float
    lua_pushnumber(L, style.FrameRounding);
    lua_rawset(L, -3);

    lua_pushliteral(L, "FrameBorderSize");      // float
    lua_pushnumber(L, style.FrameBorderSize);
    lua_rawset(L, -3);

    lua_pushliteral(L, "ItemSpacing");      // ImVec2
    dmScript::PushVector3(L, dmVMath::Vector3(style.ItemSpacing.x, style.ItemSpacing.y, 0));
    lua_rawset(L, -3);

    lua_pushliteral(L, "ItemInnerSpacing");     // ImVec2
    dmScript::PushVector3(L, dmVMath::Vector3(style.ItemInnerSpacing.x, style.ItemInnerSpacing.y, 0));
    lua_rawset(L, -3);

    lua_pushliteral(L, "CellPadding");      // ImVec2
    dmScript::PushVector3(L, dmVMath::Vector3(style.CellPadding.x, style.CellPadding.y, 0));
    lua_rawset(L, -3);

    lua_pushliteral(L, "TouchExtraPadding");        // ImVec2
    dmScript::PushVector3(L, dmVMath::Vector3(style.TouchExtraPadding.x, style.TouchExtraPadding.y, 0));
    lua_rawset(L, -3);

    lua_pushliteral(L, "IndentSpacing");        // float
    lua_pushnumber(L, style.IndentSpacing);
    lua_rawset(L, -3);

    lua_pushliteral(L, "ColumnsMinSpacing");        // float
    lua_pushnumber(L, style.ColumnsMinSpacing);
    lua_rawset(L, -3);

    lua_pushliteral(L, "ScrollbarSize");        // float
    lua_pushnumber(L, style.ScrollbarSize);
    lua_rawset(L, -3);

    lua_pushliteral(L, "ScrollbarRounding");        // float
    lua_pushnumber(L, style.ScrollbarRounding);
    lua_rawset(L, -3);

    lua_pushliteral(L, "GrabMinSize");      // float
    lua_pushnumber(L, style.GrabMinSize);
    lua_rawset(L, -3);

    lua_pushliteral(L, "GrabRounding");     // float
    lua_pushnumber(L, style.GrabRounding);
    lua_rawset(L, -3);

    lua_pushliteral(L, "LogSliderDeadzone");        // float
    lua_pushnumber(L, style.LogSliderDeadzone);
    lua_rawset(L, -3);

    lua_pushliteral(L, "TabRounding");      // float
    lua_pushnumber(L, style.TabRounding);
    lua_rawset(L, -3);

    lua_pushliteral(L, "TabBorderSize");        // float
    lua_pushnumber(L, style.TabBorderSize);
    lua_rawset(L, -3);

    lua_pushliteral(L, "TabMinWidthForCloseButton");        // float
    lua_pushnumber(L, style.TabMinWidthForCloseButton);
    lua_rawset(L, -3);

    lua_pushliteral(L, "ColorButtonPosition");      // ImGuiDir
    lua_pushnumber(L, style.ColorButtonPosition);
    lua_rawset(L, -3);

    lua_pushliteral(L, "ButtonTextAlign");      // ImVec2
    dmScript::PushVector3(L, dmVMath::Vector3(style.ButtonTextAlign.x, style.ButtonTextAlign.y, 0));
    lua_rawset(L, -3);

    lua_pushliteral(L, "SelectableTextAlign");      // ImVec2
    dmScript::PushVector3(L, dmVMath::Vector3(style.SelectableTextAlign.x, style.SelectableTextAlign.y, 0));
    lua_rawset(L, -3);
    // float       SeparatorTextBorderSize;    // Thickkness of border in SeparatorText()
    // ImVec2      SeparatorTextAlign;         // Alignment of text within the separator. Defaults to (0.0f, 0.5f) (left aligned, center).
    // ImVec2      SeparatorTextPadding;       // Horizontal offset of text from each edge of the separator + spacing on other axis. Generally small values. .y is recommended to be == FramePadding.y.

    lua_pushliteral(L, "DisplayWindowPadding");     // ImVec2
    dmScript::PushVector3(L, dmVMath::Vector3(style.DisplayWindowPadding.x, style.DisplayWindowPadding.y, 0));
    lua_rawset(L, -3);

    lua_pushliteral(L, "DisplaySafeAreaPadding");       // ImVec2
    dmScript::PushVector3(L, dmVMath::Vector3(style.DisplaySafeAreaPadding.x, style.DisplaySafeAreaPadding.y, 0));
    lua_rawset(L, -3);

    lua_pushliteral(L, "MouseCursorScale");     // float
    lua_pushnumber(L, style.MouseCursorScale);
    lua_rawset(L, -3);

    lua_pushliteral(L, "AntiAliasedLines");     // bool
    lua_pushboolean(L, style.AntiAliasedLines);
    lua_rawset(L, -3);

    lua_pushliteral(L, "AntiAliasedLinesUseTex");       // bool
    lua_pushboolean(L, style.AntiAliasedLinesUseTex);
    lua_rawset(L, -3);

    lua_pushliteral(L, "AntiAliasedFill");      // bool
    lua_pushboolean(L, style.AntiAliasedFill);
    lua_rawset(L, -3);

    lua_pushliteral(L, "CurveTessellationTol");     // float
    lua_pushnumber(L, style.CurveTessellationTol);
    lua_rawset(L, -3);

    lua_pushliteral(L, "HoverStationaryDelay");        // float
    lua_pushnumber(L, style.HoverStationaryDelay);
    lua_rawset(L, -3);

    lua_pushliteral(L, "HoverDelayShort");        // float
    lua_pushnumber(L, style.HoverDelayShort);
    lua_rawset(L, -3);

    lua_pushliteral(L, "HoverDelayNormal");        // float
    lua_pushnumber(L, style.HoverDelayNormal);
    lua_rawset(L, -3);

    return 1;
}

/** SetStyle
 * @name set_style
 * @table style
 */
static int imgui_SetStyle(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    ImGuiStyle& style = ImGui::GetStyle();
    if (lua_isnil(L, 1)) {
        return 0;
    }
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_pushvalue(L, 1);
    lua_pushnil(L);
    while (lua_next(L, -2)) {
        const char* attr = lua_tostring(L, -2);
        if (strcmp(attr, "Alpha") == 0)
        {
            style.Alpha = luaL_checknumber(L, -1);
        }
        else if (strcmp(attr, "DisabledAlpha") == 0)
        {
            style.DisabledAlpha = luaL_checknumber(L, -1);
        }
        else if (strcmp(attr, "WindowPadding") == 0)
        {
            dmVMath::Vector3* v3 = dmScript::CheckVector3(L, -1);
            style.WindowPadding.x = v3->getX();
            style.WindowPadding.y = v3->getY();
        }
        else if (strcmp(attr, "WindowRounding") == 0)
        {
            style.WindowRounding = luaL_checknumber(L, -1);
        }
        else if (strcmp(attr, "WindowBorderSize") == 0)
        {
            style.WindowBorderSize = luaL_checknumber(L, -1);
        }
        else if (strcmp(attr, "WindowMinSize") == 0)
        {
            dmVMath::Vector3* v3 = dmScript::CheckVector3(L, -1);
            style.WindowMinSize.x = v3->getX();
            style.WindowMinSize.y = v3->getY();
        }
        else if (strcmp(attr, "WindowTitleAlign") == 0)
        {
            dmVMath::Vector3* v3 = dmScript::CheckVector3(L, -1);
            style.WindowTitleAlign.x = v3->getX();
            style.WindowTitleAlign.y = v3->getY();
        }
        else if (strcmp(attr, "WindowMenuButtonPosition") == 0)
        {
            style.WindowMenuButtonPosition = luaL_checkinteger(L, -1);
        }
        else if (strcmp(attr, "ChildRounding") == 0)
        {
            style.ChildRounding = luaL_checknumber(L, -1);
        }
        else if (strcmp(attr, "ChildBorderSize") == 0)
        {
            style.ChildBorderSize = luaL_checknumber(L, -1);
        }
        else if (strcmp(attr, "PopupRounding") == 0)
        {
            style.PopupRounding = luaL_checknumber(L, -1);
        }
        else if (strcmp(attr, "PopupBorderSize") == 0)
        {
            style.PopupBorderSize = luaL_checknumber(L, -1);
        }
        else if (strcmp(attr, "FramePadding") == 0)
        {
            dmVMath::Vector3* v3 = dmScript::CheckVector3(L, -1);
            style.FramePadding.x = v3->getX();
            style.FramePadding.y = v3->getY();
        }
        else if (strcmp(attr, "FrameRounding") == 0)
        {
            style.FrameRounding = luaL_checknumber(L, -1);
        }
        else if (strcmp(attr, "FrameBorderSize") == 0)
        {
            style.FrameBorderSize = luaL_checknumber(L, -1);
        }
        else if (strcmp(attr, "ItemSpacing") == 0)
        {
            dmVMath::Vector3* v3 = dmScript::CheckVector3(L, -1);
            style.ItemSpacing.x = v3->getX();
            style.ItemSpacing.y = v3->getY();
        }
        else if (strcmp(attr, "ItemInnerSpacing") == 0)
        {
            dmVMath::Vector3* v3 = dmScript::CheckVector3(L, -1);
            style.ItemInnerSpacing.x = v3->getX();
            style.ItemInnerSpacing.y = v3->getY();
        }
        else if (strcmp(attr, "CellPadding") == 0)
        {
            dmVMath::Vector3* v3 = dmScript::CheckVector3(L, -1);
            style.CellPadding.x = v3->getX();
            style.CellPadding.y = v3->getY();
        }
        else if (strcmp(attr, "TouchExtraPadding") == 0)
        {
            dmVMath::Vector3* v3 = dmScript::CheckVector3(L, -1);
            style.TouchExtraPadding.x = v3->getX();
            style.TouchExtraPadding.y = v3->getY();
        }
        else if (strcmp(attr, "IndentSpacing") == 0)
        {
            style.IndentSpacing = luaL_checknumber(L, -1);
        }
        else if (strcmp(attr, "ColumnsMinSpacing") == 0)
        {
            style.ColumnsMinSpacing = luaL_checknumber(L, -1);
        }
        else if (strcmp(attr, "ScrollbarSize") == 0)
        {
            style.ScrollbarSize = luaL_checknumber(L, -1);
        }
        else if (strcmp(attr, "ScrollbarRounding") == 0)
        {
            style.ScrollbarRounding = luaL_checknumber(L, -1);
        }
        else if (strcmp(attr, "GrabMinSize") == 0)
        {
            style.GrabMinSize = luaL_checknumber(L, -1);
        }
        else if (strcmp(attr, "GrabRounding") == 0)
        {
            style.GrabRounding = luaL_checknumber(L, -1);
        }
        else if (strcmp(attr, "LogSliderDeadzone") == 0)
        {
            style.LogSliderDeadzone = luaL_checknumber(L, -1);
        }
        else if (strcmp(attr, "TabRounding") == 0)
        {
            style.TabRounding = luaL_checknumber(L, -1);
        }
        else if (strcmp(attr, "TabBorderSize") == 0)
        {
            style.TabBorderSize = luaL_checknumber(L, -1);
        }
        else if (strcmp(attr, "TabMinWidthForCloseButton") == 0)
        {
            style.TabMinWidthForCloseButton = luaL_checknumber(L, -1);
        }
        else if (strcmp(attr, "ColorButtonPosition") == 0)
        {
            style.ColorButtonPosition = luaL_checkinteger(L, -1);
        }
        else if (strcmp(attr, "ButtonTextAlign") == 0)
        {
            dmVMath::Vector3* v3 = dmScript::CheckVector3(L, -1);
            style.ButtonTextAlign.x = v3->getX();
            style.ButtonTextAlign.y = v3->getY();
        }
        else if (strcmp(attr, "SelectableTextAlign") == 0)
        {
            dmVMath::Vector3* v3 = dmScript::CheckVector3(L, -1);
            style.SelectableTextAlign.x = v3->getX();
            style.SelectableTextAlign.y = v3->getY();
        }
        else if (strcmp(attr, "DisplayWindowPadding") == 0)
        {
            dmVMath::Vector3* v3 = dmScript::CheckVector3(L, -1);
            style.DisplayWindowPadding.x = v3->getX();
            style.DisplayWindowPadding.y = v3->getY();
        }
        else if (strcmp(attr, "DisplaySafeAreaPadding") == 0)
        {
            dmVMath::Vector3* v3 = dmScript::CheckVector3(L, -1);
            style.DisplaySafeAreaPadding.x = v3->getX();
            style.DisplaySafeAreaPadding.y = v3->getY();
        }
        else if (strcmp(attr, "MouseCursorScale") == 0)
        {
            style.MouseCursorScale = luaL_checknumber(L, -1);
        }
        else if (strcmp(attr, "AntiAliasedLines") == 0)
        {
            style.AntiAliasedLines = lua_toboolean(L, -1);
        }
        else if (strcmp(attr, "AntiAliasedLinesUseTex") == 0)
        {
            style.AntiAliasedLinesUseTex = lua_toboolean(L, -1);
        }
        else if (strcmp(attr, "AntiAliasedFill") == 0)
        {
            style.AntiAliasedFill = lua_toboolean(L, -1);
        }
        else if (strcmp(attr, "CurveTessellationTol") == 0)
        {
            style.CurveTessellationTol = luaL_checknumber(L, -1);
        }
        else if (strcmp(attr, "HoverStationaryDelay") == 0)
        {
            style.HoverStationaryDelay = luaL_checknumber(L, -1);
        }
        else if (strcmp(attr, "HoverDelayShort") == 0)
        {
            style.HoverDelayShort = luaL_checknumber(L, -1);
        }
        else if (strcmp(attr, "HoverDelayNormal") == 0)
        {
            style.HoverDelayNormal = luaL_checknumber(L, -1);
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    return 0;
}

/** SetStyleColor
 * @name set_style_color
 * @number color_index
 * @number r
 * @number g
 * @number b
 * @number a
 */
static int imgui_SetStyleColor(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    ImGuiStyle& style = ImGui::GetStyle();
    double r = luaL_checknumber(L, 2);
    double g = luaL_checknumber(L, 3);
    double b = luaL_checknumber(L, 4);
    double a = luaL_checknumber(L, 5);
    style.Colors[luaL_checkinteger(L, 1)] = ImVec4(r, g, b, a);
    return 0;
}
/** PushStyleColor
 * @name push_style_color
 * @number color_index
 * @number r
 * @number g
 * @number b
 * @number a
 */
static int imgui_PushStyleColor(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    double r = luaL_checknumber(L, 2);
    double g = luaL_checknumber(L, 3);
    double b = luaL_checknumber(L, 4);
    double a = luaL_checknumber(L, 5);
    ImGuiCol col = luaL_checkinteger(L, 1);
    ImGui::PushStyleColor(col, ImVec4(r, g, b, a));
    return 0;
}
/** PopStyleColor
 * @name pop_style_color
 * @number count
 */
static int imgui_PopStyleColor(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    int count = 1;
    if (lua_isnumber(L, 1))
    {
        count = luaL_checkint(L, 1);
    }
    ImGui::PopStyleColor(count);
    return 0;
}

/** PushStyleVar
 * @name push_style_var
 * @tparam number style_var enum for the style to be pushed
 * @param value number or vmath.vector3 specifying the value of the style var
 */
static int imgui_PushStyleVar(lua_State *L)
{
    DM_LUA_STACK_CHECK(L, 0);
    uint32_t style_var = luaL_checkint(L, 1);
    switch(style_var)
    {
        case ImGuiStyleVar_Alpha:
        case ImGuiStyleVar_DisabledAlpha:
        case ImGuiStyleVar_WindowRounding:
        case ImGuiStyleVar_WindowBorderSize:
        case ImGuiStyleVar_ChildRounding:
        case ImGuiStyleVar_ChildBorderSize:
        case ImGuiStyleVar_PopupRounding:
        case ImGuiStyleVar_PopupBorderSize:
        case ImGuiStyleVar_FrameRounding:
        case ImGuiStyleVar_FrameBorderSize:
        case ImGuiStyleVar_IndentSpacing:
        case ImGuiStyleVar_ScrollbarSize:
        case ImGuiStyleVar_ScrollbarRounding:
        case ImGuiStyleVar_GrabMinSize:
        case ImGuiStyleVar_GrabRounding:
        case ImGuiStyleVar_TabRounding:
        case ImGuiStyleVar_TabBorderSize:
        case ImGuiStyleVar_TabBarBorderSize:
        case ImGuiStyleVar_TableAngledHeadersAngle:
        case ImGuiStyleVar_SeparatorTextBorderSize:
        {
            float val_float = luaL_checknumber(L, 2);
            ImGui::PushStyleVar(style_var, val_float);
            break;
        }
        case ImGuiStyleVar_WindowPadding:
        case ImGuiStyleVar_WindowMinSize:
        case ImGuiStyleVar_WindowTitleAlign:
        case ImGuiStyleVar_FramePadding:
        case ImGuiStyleVar_ItemSpacing:
        case ImGuiStyleVar_ItemInnerSpacing:
        case ImGuiStyleVar_CellPadding:
        case ImGuiStyleVar_TableAngledHeadersTextAlign:
        case ImGuiStyleVar_ButtonTextAlign:
        case ImGuiStyleVar_SelectableTextAlign:
        case ImGuiStyleVar_SeparatorTextAlign:
        case ImGuiStyleVar_SeparatorTextPadding:
        {
            dmVMath::Vector3* val_vec3 = dmScript::CheckVector3(L, 2);
            ImVec2 val_vec2(val_vec3->getX(), val_vec3->getY());
            ImGui::PushStyleVar(style_var, val_vec2);
            break;
        }
    }
    return 0;
}

/** PopStyleVar
 * @name pop_style_var
 * @number count number of style vars to pop; optional, defaults to 1
 */
static int imgui_PopStyleVar(lua_State *L)
{
    DM_LUA_STACK_CHECK(L, 0);
    int count = 1;
    if (lua_isnumber(L, 1))
    {
        count = luaL_checkint(L, 1);
    }
    ImGui::PopStyleVar(count);
    return 0;
}

/** SetWindowFontScale
 * @name set_window_font_scale
 * @number scale
 */
static int imgui_SetWindowFontScale(lua_State *L)
{
    DM_LUA_STACK_CHECK(L, 0);
    float scale = luaL_checknumber(L, 1);
    ImGui::SetWindowFontScale(scale);
    return 0;
}

/** SetGlobalFontScale
 * @name set_global_font_scale
 * @number scale
 */
static int imgui_SetGlobalFontScale(lua_State *L)
{
    DM_LUA_STACK_CHECK(L, 0);
    float scale = luaL_checknumber(L, 1);
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = scale;
    return 0;
}

/** ScaleAllSizes
 * @name scale_all_sizes
 * @number scale
 */
static int imgui_ScaleAllSizes(lua_State *L)
{
    DM_LUA_STACK_CHECK(L, 0);
    float scale = luaL_checknumber(L, 1);
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(scale);
    return 0;
}


/** SetCursorPos
 * @name set_cursor_pos
 * @number x
 * @number y
 */
static int imgui_SetCursorPos(lua_State *L)
{
    DM_LUA_STACK_CHECK(L, 0);
    int posx = luaL_checkinteger(L, 1);
    int posy = luaL_checkinteger(L, 2);
    ImVec2     pos(posx, posy);
    ImGui::SetCursorPos(pos);
    return 0;
}

// ----------------------------
// ----- ITEM WIDTH -----------
// ----------------------------

/** PushItemWidth
 * @name push_item_width
 * @number width
 */
static int imgui_PushItemWidth(lua_State *L)
{
    DM_LUA_STACK_CHECK(L, 0);
    float width = luaL_checknumber(L, 1);
    ImGui::PushItemWidth(width);
    return 0;
}

/** PopItemWidth
 * @name pop_item_width
 */
static int imgui_PopItemWidth(lua_State *L)
{
    DM_LUA_STACK_CHECK(L, 0);
    ImGui::PopItemWidth();
    return 0;
}

/** SetNextItemWidth
 * @name set_next_item_width
 * @number width
 */
static int imgui_SetNextItemWidth(lua_State *L)
{
    DM_LUA_STACK_CHECK(L, 0);
    float width = luaL_checknumber(L, 1);
    ImGui::SetNextItemWidth(width);
    return 0;
}

/** CalcItemWidth
 * @name calc_item_width
 * @treturn number width
 */
static int imgui_CalcItemWidth(lua_State *L)
{
    DM_LUA_STACK_CHECK(L, 1);
    float width = ImGui::CalcItemWidth();
    lua_pushnumber(L, width);
    return 1;
}

// ----------------------------
// ----- NAVIGATION -----------------
// ----------------------------

/** SetItemDefaultFocus
 * @name set_item_default_focus
 */
static int imgui_SetItemDefaultFocus(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();
    ImGui::SetItemDefaultFocus();
    return 0;
}

/** SetScrollHereY
 * @name set_scroll_here_y
 * @number center_y_ratio
 */
static int imgui_SetScrollHereY(lua_State *L)
{
    DM_LUA_STACK_CHECK(L, 0);
    imgui_NewFrame();
    float center_y_ratio = luaL_checknumber(L, 1);
    ImGui::SetScrollHereY(center_y_ratio);
    return 0;
}


// ----------------------------
// ----- FONT -----------------
// ----------------------------

static int imgui_StoreFont(ImFont* font)
{
    if (g_imgui_Fonts.Full())
    {
        g_imgui_Fonts.OffsetCapacity(2);
    }
    g_imgui_Fonts.Push(font);
    return g_imgui_Fonts.Size() - 1;
}

static ImFont* imgui_GetFont(int index)
{
    if (index >= 0 && index < g_imgui_Fonts.Size())
    {
        return g_imgui_Fonts[index];
    }
    return 0;
}

ImWchar* LuaToGlyphRanges(lua_State * L, int index) {
    const ImWchar* glyph_ranges = NULL;
    if (!lua_isnil(L, index)) {
        ImGuiIO& io = ImGui::GetIO();
        ExtImGuiGlyphRanges range = (ExtImGuiGlyphRanges)lua_tointeger(L, index);
        switch (range) {
            case ExtImGuiGlyphRanges_Greek:
                glyph_ranges = io.Fonts->GetGlyphRangesGreek();
                break;
            case ExtImGuiGlyphRanges_Korean:
                glyph_ranges = io.Fonts->GetGlyphRangesKorean();
                break;
            case ExtImGuiGlyphRanges_Japanese:
                glyph_ranges = io.Fonts->GetGlyphRangesJapanese();
                break;
            case ExtImGuiGlyphRanges_ChineseFull:
                glyph_ranges = io.Fonts->GetGlyphRangesChineseFull();
                break;
            case ExtImGuiGlyphRanges_ChineseSimplifiedCommon:
                glyph_ranges = io.Fonts->GetGlyphRangesChineseSimplifiedCommon();
                break;
            case ExtImGuiGlyphRanges_Cyrillic:
                glyph_ranges = io.Fonts->GetGlyphRangesCyrillic();
                break;
            case ExtImGuiGlyphRanges_Thai:
                glyph_ranges = io.Fonts->GetGlyphRangesThai();
                break;
            case ExtImGuiGlyphRanges_Vietnamese:
                glyph_ranges = io.Fonts->GetGlyphRangesVietnamese();
                break;
            default:
            case ExtImGuiGlyphRanges_Default:
                glyph_ranges = io.Fonts->GetGlyphRangesDefault();
                break;
        }
    }
    return (ImWchar*)glyph_ranges;
}

/** FontAddTTFFile
 * @name font_add_ttf_file
 * @string filename
 * @number font_size
 * @string glyph_ranges
 * @treturn number font_index
 */
static int imgui_FontAddTTFFile(lua_State * L)
{
    DM_LUA_STACK_CHECK(L, 1);
    const char * ttf_filename = luaL_checkstring(L, 1);
    float font_size = luaL_checknumber(L, 2);
    const ImFontConfig* font_cfg = NULL;
    const ImWchar* glyph_ranges = LuaToGlyphRanges(L, 3);

    ImGuiIO& io = ImGui::GetIO();
    ImFont* font = io.Fonts->AddFontFromFileTTF(ttf_filename, font_size, font_cfg, glyph_ranges);
    // Put font in map.
    if(font != NULL)
    {
        int index = imgui_StoreFont(font);
        lua_pushinteger(L, index);
    }
    else
    {
        lua_pushnil(L);
    }
    return 1;
}

/** FontAddTTFData
 * @name font_add_ttf_data
 * @string ttf_data
 * @number font_size
 * @number font_pixels
 * @string glyph_ranges
 * @treturn number font_index
 */
static int imgui_FontAddTTFData(lua_State * L)
{
    DM_LUA_STACK_CHECK(L, 1);
    const char * ttf_data = luaL_checkstring(L, 1);
    int ttf_data_size = luaL_checknumber(L, 2);
    float font_size = luaL_checknumber(L, 3);
    int font_pixels = luaL_checknumber(L, 4);
    const ImFontConfig* font_cfg = NULL;
    const ImWchar* glyph_ranges = LuaToGlyphRanges(L, 5);

    char *ttf_data_cpy = (char *)calloc(ttf_data_size, sizeof(char));
    if (!ttf_data_cpy)
    {
        dmLogWarning("Failed to allocate memory for ttf data of size %d", ttf_data_size);
        lua_pushnil(L);
        return 1;
    }
    if (!memcpy(ttf_data_cpy, ttf_data, ttf_data_size))
    {
        dmLogWarning("Failed to copy ttf data");
        lua_pushnil(L);
        return 1;
    }

    ImGuiIO& io = ImGui::GetIO();
    ImFont* font = io.Fonts->AddFontFromMemoryTTF((void *)ttf_data_cpy, ttf_data_size, font_pixels, font_cfg, glyph_ranges);
    // Put font in map.
    if(font != NULL)
    {
        int index = imgui_StoreFont(font);
        lua_pushinteger(L, index);
    }
    else
    {
        lua_pushnil(L);
    }
    return 1;
}

/** FontPush
 * @name font_push
 * @number font_index
 */
static int imgui_FontPush(lua_State *L)
{
    DM_LUA_STACK_CHECK(L, 0);
    int fontid = luaL_checkinteger(L, 1);
    ImFont* font = imgui_GetFont(fontid);
    if (font)
    {
        ImGui::PushFont(font);
    }
    return 0;
}

/** FontPop
 * @name font_pop
 */
static int imgui_FontPop(lua_State *L)
{
    DM_LUA_STACK_CHECK(L, 0);
    ImGui::PopFont();
    return 0;
}

/** FontScale
 * @name font_scale
 * @number font_id
 * @number font_scale
 * @treturn number old_scale
 */
static int imgui_FontScale(lua_State *L)
{
    DM_LUA_STACK_CHECK(L, 1);
    float oldscale = 1.0f;
    int fontid = luaL_checkinteger(L, 1);
    float fontscale = luaL_checknumber(L, 2);

    ImFont* font = imgui_GetFont(fontid);
    if(font)
    {
        oldscale = font->Scale;
        font->Scale = fontscale;
    }
    lua_pushnumber(L, oldscale);
    return 1;
}


/** GetFrameHeight
 * @name get_frame_height
 */
static int imgui_GetFrameHeight(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    imgui_NewFrame();
    lua_pushnumber(L, ImGui::GetFrameHeight());
    return 1;
}

/** GetFontSize
 * @name get_font_size
 * @treturn number font_size
 */
static int imgui_GetFontSize(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    lua_pushnumber(L, ImGui::GetFontSize());
    return 1;
}


// ----------------------------
// ----- DRAW -----------------
// ----------------------------
static dmExtension::Result imgui_Draw(dmExtension::Params* params)
{
    imgui_NewFrame();

    imgui_InvokeDrawCallbacks();

    ImGui::Render();

    if (g_RenderingEnabled)
    {
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
    
    imgui_ClearGLError();

    g_imgui_NewFrame = false;
    return dmExtension::RESULT_OK;
}

/** DrawLine
 * @name draw_line
 * @number x1
 * @number y1
 * @number x2
 * @number y2
 * @number color
 */
static int imgui_DrawLine(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    int x1 = luaL_checkinteger(L, 1);
    int y1 = luaL_checkinteger(L, 2);
    int x2 = luaL_checkinteger(L, 3);
    int y2 = luaL_checkinteger(L, 4);
    unsigned int col = (unsigned int)luaL_checkinteger(L, 5);
    ImDrawList *dl = ImGui::GetWindowDrawList();
    ImVec2 p1(x1, y1);
    ImVec2 p2(x2, y2);
    dl->AddLine(p1, p2, col);
    return 0;
}

/** DrawRect
 * @name draw_rect
 * @number x
 * @number y
 * @number width
 * @number height
 * @number color
 */
static int imgui_DrawRect(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);
    int w = luaL_checkinteger(L, 3);
    int h = luaL_checkinteger(L, 4);
    unsigned int col = (unsigned int)luaL_checkinteger(L, 5);
    ImDrawList *dl = ImGui::GetWindowDrawList();
    ImVec2 minv(x, y);
    ImVec2 maxv(x + w, y + h);
    dl->AddRect(minv, maxv, col);
    return 0;
}

/** DrawRectFilled
 * @name draw_rect_filled
 * @number x
 * @number y
 * @number width
 * @number height
 * @number color
 */
static int imgui_DrawRectFilled(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);
    int w = luaL_checkinteger(L, 3);
    int h = luaL_checkinteger(L, 4);
    unsigned int col = (unsigned int)luaL_checkinteger(L, 5);
    ImDrawList *dl = ImGui::GetWindowDrawList();
    ImVec2 minv(x, y);
    ImVec2 maxv(x + w, y + h);
    dl->AddRectFilled(minv, maxv, col);
    return 0;
}

/** DrawProgressBar
 * @name draw_progress_bar
 * @number progress
 * @number xsize
 * @number ysize
 */
static int imgui_DrawProgressBar(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    float progress = luaL_checknumber(L, 1);
    float xsize = luaL_checknumber(L, 2);
    float ysize = luaL_checknumber(L, 3);

    ImVec2 size_param(xsize, ysize);
    ImGui::ProgressBar(progress, size_param);
    return 0;
}

/** SetRenderingEnabled
 * @name set_rendering_enabled
 * @boolean enabled
 */
static int imgui_SetRenderingEnabled(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    bool enabled = lua_toboolean(L, 1);
    g_RenderingEnabled = enabled;
    return 0;
}

// ----------------------------
// ----- INPUT CAPTURE -----------------
// ----------------------------

/** WantCaptureMouse
 * @name want_capture_mouse
 */
static int imgui_WantCaptureMouse(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    ImGuiIO& io = ImGui::GetIO();
    lua_pushboolean(L, io.WantCaptureMouse);
    return 1;
}

/** WantCaptureKeyboard
 * @name want_capture_keyboard
 */
static int imgui_WantCaptureKeyboard(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    ImGuiIO& io = ImGui::GetIO();
    lua_pushboolean(L, io.WantCaptureKeyboard);
    return 1;
}

/** WantCaptureText
 * @name want_capture_text
 */
static int imgui_WantCaptureText(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    ImGuiIO& io = ImGui::GetIO();
    lua_pushboolean(L, io.WantTextInput);
    return 1;
}

// ----------------------------
// ----- CONFIG -----------------
// ----------------------------
/** SetDefaults
 * @name set_defaults
 */
static int imgui_SetDefaults(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    ImGuiIO& io = ImGui::GetIO();
    ImFont* def = io.Fonts->AddFontDefault();
    imgui_StoreFont(def);
    return 0;
}

/** SetIniFilename
 * @name set_ini_filename
 * @string [filename]
 */
static int imgui_SetIniFilename(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    const char *filename = 0;
    if (lua_isstring(L, 1)) {
        filename = luaL_checkstring(L, 1);
    }

    ImGui::GetIO().IniFilename = filename;

    return 0;
}

// ----------------------------
// ----- IMGUI INIT/SHUTDOWN --
// ----------------------------

static void imgui_Init(float width, float height)
{
    #if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
    int r = gl3wInit();
    if (r != GL3W_OK) {
        dmLogError("Failed to initialize OpenGL: %d", r);
    }
    #endif

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(width, height);

    // init keymap list
    // We will be sending the correct ImGuiKey_ enums from Lua
    for (int i = 0; i < 512; i++)
    {
        io.KeyMap[i] = 0;
    }

    ImGui_ImplOpenGL3_Init();
    imgui_ClearGLError();
}

static void imgui_Shutdown()
{
    dmLogInfo("imgui_Shutdown");

    ImGui_ImplOpenGL3_Shutdown();
    imgui_ClearGLError();
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    ImGui::DestroyContext();
}

static void imgui_ExtensionInit()
{
    dmExtension::RegisterCallback(dmExtension::CALLBACK_POST_RENDER, (FExtensionCallback)imgui_Draw );
    if (g_imgui_TextBuffer)
    {
        free(g_imgui_TextBuffer);
    }
    g_imgui_TextBuffer = (char*)malloc(TEXTBUFFER_SIZE);
}

static void imgui_ExtensionShutdown()
{
    imgui_ClearDrawCallbacks();

    if (g_imgui_TextBuffer)
    {
        free(g_imgui_TextBuffer);
        g_imgui_TextBuffer = 0;
    }

    while (!g_imgui_Images.Empty())
    {
        glDeleteTextures(1, &g_imgui_Images.Back().tid);
        g_imgui_Images.Pop();
    }

    g_imgui_Fonts.SetSize(0);
}

// Functions exposed to Lua
static const luaL_reg Module_methods[] =
{
    {"image_load", imgui_ImageLoad},
    {"image_load_data", imgui_ImageLoadData},
    {"image_get", imgui_ImageGet},
    {"image_free", imgui_ImageFree},
    {"image_add", imgui_ImageAdd},

    {"image_b64_decode", imgui_ImageB64Decode},

    {"font_add_ttf_file", imgui_FontAddTTFFile},
    {"font_add_ttf_data", imgui_FontAddTTFData},
    {"font_push", imgui_FontPush},
    {"font_pop", imgui_FontPop},
    {"font_scale", imgui_FontScale},
    {"get_font_size", imgui_GetFontSize},

    {"set_next_window_size", imgui_SetNextWindowSize},
    {"set_next_window_pos", imgui_SetNextWindowPos},
    {"set_next_window_collapsed", imgui_SetNextWindowCollapsed},
    {"get_window_size", imgui_GetWindowSize},
    {"get_window_pos", imgui_GetWindowPos},
    {"begin_window", imgui_Begin},
    {"end_window", imgui_End},
    {"is_window_focused", imgui_IsWindowFocused},
    {"is_window_hovered", imgui_IsWindowHovered},
    {"is_window_appearing", imgui_IsWindowAppearing},
    {"get_content_region_avail", imgui_GetContentRegionAvail},
    {"get_window_content_region_max", imgui_GetWindowContentRegionMax},

    {"begin_child", imgui_BeginChild},
    {"end_child", imgui_EndChild},

    {"begin_tab_bar", imgui_BeginTabBar},
    {"end_tab_bar", imgui_EndTabBar},

    {"begin_tab_item", imgui_BeginTabItem},
    {"end_tab_item", imgui_EndTabItem},

    {"begin_tooltip", imgui_BeginTooltip},
    {"end_tooltip", imgui_EndTooltip},

    {"begin_combo", imgui_BeginCombo},
    {"end_combo", imgui_EndCombo},
    {"combo", imgui_Combo},

    {"begin_dragdrop_source", imgui_BeginDragDropSource},
    {"end_dragdrop_source", imgui_EndDragDropSource},
    {"begin_dragdrop_target", imgui_BeginDragDropTarget},
    {"end_dragdrop_target", imgui_EndDragDropTarget},
    {"set_dragdrop_payload", imgui_SetDragDropPayload},
    {"accept_dragdrop_payload", imgui_AcceptDragDropPayload},

    {"begin_table", imgui_BeginTable},
    {"end_table", imgui_EndTable},
    {"table_next_row", imgui_TableNextRow},
    {"table_next_column", imgui_TableNextColumn},
    {"table_set_column_index", imgui_TableSetColumnIndex},
    {"table_setup_column", imgui_TableSetupColumn},
    {"table_headers_row", imgui_TableHeadersRow},
    {"table_setup_scroll_freeze", imgui_TableSetupScrollFreeze},

    {"begin_popup_context_item", imgui_BeginPopupContextItem},
    {"begin_popup", imgui_BeginPopup},
    {"begin_popup_modal", imgui_BeginPopupModal},
    {"open_popup", imgui_OpenPopup},
    {"close_current_popup", imgui_CloseCurrentPopup},
    {"end_popup", imgui_EndPopup},

    {"tree_node", imgui_TreeNode},
    {"tree_pop", imgui_TreePop},
    {"collapsing_header", imgui_CollapsingHeader},

    {"push_id", imgui_PushId},
    {"pop_id", imgui_PopId},

    {"color_edit4", imgui_ColorEdit4},

    {"selectable", imgui_Selectable},
    {"text", imgui_Text},
    {"text_colored", imgui_TextColored},
    {"input_text", imgui_InputText},
    {"input_int", imgui_InputInt}, 
    {"input_int2", imgui_InputInt2},  
    {"input_int4", imgui_InputInt4},
    {"input_float", imgui_InputFloat},
    {"input_double", imgui_InputDouble},
    {"input_float3", imgui_InputFloat3},
    {"input_float4", imgui_InputFloat4},
    {"drag_float", imgui_DragFloat},
    {"slider_float", imgui_SliderFloat},
    {"slider_float3", imgui_SliderFloat3},
    {"slider_float4", imgui_SliderFloat4},
    {"button", imgui_Button},
    {"small_button", imgui_SmallButton},
    {"button_image", imgui_ButtonImage},
    {"button_arrow", imgui_ButtonArrow},
    {"checkbox", imgui_Checkbox},
    {"radio_button", imgui_RadioButton},
    {"begin_menu_bar", imgui_BeginMenuBar},
    {"end_menu_bar", imgui_EndMenuBar},
    {"begin_main_menu_bar", imgui_BeginMainMenuBar},
    {"end_main_menu_bar", imgui_EndMainMenuBar},
    {"begin_menu", imgui_BeginMenu},
    {"end_menu", imgui_EndMenu},
    {"menu_item", imgui_MenuItem},

    {"same_line", imgui_SameLine},
    {"new_line", imgui_NewLine},
    {"bullet", imgui_Bullet},
    {"indent", imgui_Indent},
    {"unindent", imgui_Unindent},
    {"spacing", imgui_Spacing},
    {"separator", imgui_Separator},
    {"get_cursor_screen_pos", imgui_GetCursorScreenPos},
    {"get_cursor_pos", imgui_GetCursorPos},

    {"add_line", imgui_DrawListAddLine},
    {"add_rect", imgui_DrawListAddRect},

    {"plot_lines", imgui_PlotLines},
    {"plot_histogram", imgui_PlotHistogram},

    {"text_getsize", imgui_TextGetSize},

    {"draw_rect", imgui_DrawRect},
    {"draw_rect_filled", imgui_DrawRectFilled},
    {"draw_line", imgui_DrawLine},
    {"draw_progress", imgui_DrawProgressBar},
    {"set_rendering_enabled", imgui_SetRenderingEnabled},
    {"demo", imgui_Demo},

    {"set_mouse_input", imgui_SetMouseInput},
    {"set_mouse_pos", imgui_SetMousePos},
    {"set_mouse_button", imgui_SetMouseButton},
    {"set_mouse_wheel", imgui_SetMouseWheel},
    {"set_key_down", imgui_SetKeyDown},
    {"set_key_modifier_ctrl", imgui_SetKeyModifierCtrl},
    {"set_key_modifier_shift", imgui_SetKeyModifierShift},
    {"set_key_modifier_alt", imgui_SetKeyModifierAlt},
    {"set_key_modifier_super", imgui_SetKeyModifierSuper},
    {"add_input_character", imgui_AddInputCharacter},
    {"add_input_characters", imgui_AddInputCharacters},
    {"want_mouse_input", imgui_WantCaptureMouse},
    {"want_keyboard_input", imgui_WantCaptureKeyboard},
    {"want_text_input", imgui_WantCaptureText},
    
    {"is_item_active", imgui_IsItemActive},
    {"is_item_focused", imgui_IsItemFocused},
    {"is_item_clicked", imgui_IsItemClicked},
    {"is_item_double_clicked", imgui_IsItemDoubleClicked},
    {"is_item_hovered", imgui_IsItemHovered},
    {"get_item_rect_max", imgui_GetItemRectMax},
    {"is_mouse_clicked", imgui_IsMouseClicked},
    {"is_mouse_double_clicked", imgui_IsMouseDoubleClicked},
    {"set_keyboard_focus_here", imgui_SetKeyboardFocusHere},
    {"set_item_default_focus", imgui_SetItemDefaultFocus},

    {"set_style", imgui_SetStyle},
    {"get_style", imgui_GetStyle},

    {"set_style_color", imgui_SetStyleColor},
    {"push_style_color", imgui_PushStyleColor},
    {"pop_style_color", imgui_PopStyleColor},

    {"push_style_var", imgui_PushStyleVar},
    {"pop_style_var", imgui_PopStyleVar},

    {"push_item_width", imgui_PushItemWidth},
    {"pop_item_width", imgui_PopItemWidth},
    {"set_next_item_width", imgui_SetNextItemWidth},
    {"calc_item_width", imgui_CalcItemWidth},
    
    {"set_defaults", imgui_SetDefaults},
    {"set_ini_filename", imgui_SetIniFilename},

    {"set_cursor_pos", imgui_SetCursorPos},
    {"set_display_size", imgui_SetDisplaySize},
    {"set_window_font_scale", imgui_SetWindowFontScale},
    {"set_global_font_scale", imgui_SetGlobalFontScale},
    {"scale_all_sizes", imgui_ScaleAllSizes},

    {"get_frame_height", imgui_GetFrameHeight},

    {"set_scroll_here_y", imgui_SetScrollHereY},
    {0, 0}
};

static void lua_setfieldstringstring(lua_State* L, const char* key, const char* value)
{
    int top = lua_gettop(L);
    lua_pushstring(L, value);
    lua_setfield(L, -2, key);
    assert(top == lua_gettop(L));
}
static void lua_setfieldstringint(lua_State* L, const char* key, uint32_t value)
{
    int top = lua_gettop(L);
    lua_pushnumber(L, value);
    lua_setfield(L, -2, key);
    assert(top == lua_gettop(L));
}

static void LuaInit(lua_State* L)
{
    int top = lua_gettop(L);

    // Register lua names
    luaL_register(L, MODULE_NAME, Module_methods);

    /**
     * MOUSEBUTTON_LEFT
     *
     * @field MOUSEBUTTON_LEFT
     */
     lua_setfieldstringint(L, "MOUSEBUTTON_LEFT", ImGuiMouseButton_Left);
    /**
     * MOUSEBUTTON_RIGHT
     *
     * @field MOUSEBUTTON_RIGHT
     */
     lua_setfieldstringint(L, "MOUSEBUTTON_RIGHT", ImGuiMouseButton_Right);
    /**
     * MOUSEBUTTON_MIDDLE
     *
     * @field MOUSEBUTTON_MIDDLE
     */
     lua_setfieldstringint(L, "MOUSEBUTTON_MIDDLE", ImGuiMouseButton_Middle);

    /**
     * SELECTABLE_DONT_CLOSE_POPUPS
     *
     * @field SELECTABLE_DONT_CLOSE_POPUPS
     */
     lua_setfieldstringint(L, "SELECTABLE_DONT_CLOSE_POPUPS", ImGuiSelectableFlags_DontClosePopups);
    /**
     * SELECTABLE_SPAN_ALL_COLUMNS
     *
     * @field SELECTABLE_SPAN_ALL_COLUMNS
     */
     lua_setfieldstringint(L, "SELECTABLE_SPAN_ALL_COLUMNS", ImGuiSelectableFlags_SpanAllColumns);
    /**
     * SELECTABLE_ALLOW_DOUBLE_CLICK
     *
     * @field SELECTABLE_ALLOW_DOUBLE_CLICK
     */
     lua_setfieldstringint(L, "SELECTABLE_ALLOW_DOUBLE_CLICK", ImGuiSelectableFlags_AllowDoubleClick);
    /**
     * SELECTABLE_DISABLED
     *
     * @field SELECTABLE_DISABLED
     */
     lua_setfieldstringint(L, "SELECTABLE_DISABLED", ImGuiSelectableFlags_Disabled);
    /**
     * SELECTABLE_ALLOW_ITEM_OVERLAP
     *
     * @field SELECTABLE_ALLOW_ITEM_OVERLAP
     */
     lua_setfieldstringint(L, "SELECTABLE_ALLOW_ITEM_OVERLAP", ImGuiSelectableFlags_AllowItemOverlap);

    /**
     * TABITEM_UNSAVED_DOCUMENT
     *
     * @field TABITEM_UNSAVED_DOCUMENT
     */
     lua_setfieldstringint(L, "TABITEM_UNSAVED_DOCUMENT", ImGuiTabItemFlags_UnsavedDocument);
    /**
     * TABITEM_SET_SELECTED
     *
     * @field TABITEM_SET_SELECTED
     */
     lua_setfieldstringint(L, "TABITEM_SET_SELECTED", ImGuiTabItemFlags_SetSelected);
    /**
     * TABITEM_NO_CLOSE_WITH_MIDDLE_MOUSE_BUTTON
     *
     * @field TABITEM_NO_CLOSE_WITH_MIDDLE_MOUSE_BUTTON
     */
     lua_setfieldstringint(L, "TABITEM_NO_CLOSE_WITH_MIDDLE_MOUSE_BUTTON", ImGuiTabItemFlags_NoCloseWithMiddleMouseButton);
    /**
     * TABITEM_NO_PUSH_ID
     *
     * @field TABITEM_NO_PUSH_ID
     */
     lua_setfieldstringint(L, "TABITEM_NO_PUSH_ID", ImGuiTabItemFlags_NoPushId);
    /**
     * TABITEM_NO_TOOLTIP
     *
     * @field TABITEM_NO_TOOLTIP
     */
     lua_setfieldstringint(L, "TABITEM_NO_TOOLTIP", ImGuiTabItemFlags_NoTooltip);
    /**
     * TABITEM_NO_REORDER
     *
     * @field TABITEM_NO_REORDER
     */
     lua_setfieldstringint(L, "TABITEM_NO_REORDER", ImGuiTabItemFlags_NoReorder);
    /**
     * TABITEM_LEADING
     *
     * @field TABITEM_LEADING
     */
     lua_setfieldstringint(L, "TABITEM_LEADING", ImGuiTabItemFlags_Leading);
    /**
     * TABITEM_TRAILING
     *
     * @field TABITEM_TRAILING
     */
     lua_setfieldstringint(L, "TABITEM_TRAILING", ImGuiTabItemFlags_Trailing);

    /**
     * FOCUSED_CHILD_WINDOWS
     *
     * @field FOCUSED_CHILD_WINDOWS
     */
     lua_setfieldstringint(L, "FOCUSED_CHILD_WINDOWS", ImGuiFocusedFlags_ChildWindows);
    /**
     * FOCUSED_ROOT_WINDOW
     *
     * @field FOCUSED_ROOT_WINDOW
     */
     lua_setfieldstringint(L, "FOCUSED_ROOT_WINDOW", ImGuiFocusedFlags_RootWindow);
    /**
     * FOCUSED_ANY_WINDOW
     *
     * @field FOCUSED_ANY_WINDOW
     */
     lua_setfieldstringint(L, "FOCUSED_ANY_WINDOW", ImGuiFocusedFlags_AnyWindow);
    /**
     * FOCUSED_ROOT_AND_CHILD_WINDOWS
     *
     * @field FOCUSED_ROOT_AND_CHILD_WINDOWS
     */
     lua_setfieldstringint(L, "FOCUSED_ROOT_AND_CHILD_WINDOWS", ImGuiFocusedFlags_RootAndChildWindows);

    /**
     * TREENODE_SELECTED
     *
     * @field TREENODE_SELECTED
     */
     lua_setfieldstringint(L, "TREENODE_SELECTED", ImGuiTreeNodeFlags_Selected);
    /**
     * TREENODE_FRAMED
     *
     * @field TREENODE_FRAMED
     */
     lua_setfieldstringint(L, "TREENODE_FRAMED", ImGuiTreeNodeFlags_Framed);
    /**
     * TREENODE_ALLOW_ITEM_OVERLAP
     *
     * @field TREENODE_ALLOW_ITEM_OVERLAP
     */
     lua_setfieldstringint(L, "TREENODE_ALLOW_ITEM_OVERLAP", ImGuiTreeNodeFlags_AllowItemOverlap);
    /**
     * TREENODE_NO_TREE_PUSH_ON_OPEN
     *
     * @field TREENODE_NO_TREE_PUSH_ON_OPEN
     */
     lua_setfieldstringint(L, "TREENODE_NO_TREE_PUSH_ON_OPEN", ImGuiTreeNodeFlags_NoTreePushOnOpen);
    /**
     * TREENODE_NO_AUTO_OPEN_ON_LOG
     *
     * @field TREENODE_NO_AUTO_OPEN_ON_LOG
     */
     lua_setfieldstringint(L, "TREENODE_NO_AUTO_OPEN_ON_LOG", ImGuiTreeNodeFlags_NoAutoOpenOnLog);
    /**
     * TREENODE_DEFAULT_OPEN
     *
     * @field TREENODE_DEFAULT_OPEN
     */
     lua_setfieldstringint(L, "TREENODE_DEFAULT_OPEN", ImGuiTreeNodeFlags_DefaultOpen);
    /**
     * TREENODE_OPEN_ON_DOUBLE_CLICK
     *
     * @field TREENODE_OPEN_ON_DOUBLE_CLICK
     */
     lua_setfieldstringint(L, "TREENODE_OPEN_ON_DOUBLE_CLICK", ImGuiTreeNodeFlags_OpenOnDoubleClick);
    /**
     * TREENODE_OPEN_ON_ARROW
     *
     * @field TREENODE_OPEN_ON_ARROW
     */
     lua_setfieldstringint(L, "TREENODE_OPEN_ON_ARROW", ImGuiTreeNodeFlags_OpenOnArrow);
    /**
     * TREENODE_LEAF
     *
     * @field TREENODE_LEAF
     */
     lua_setfieldstringint(L, "TREENODE_LEAF", ImGuiTreeNodeFlags_Leaf);
    /**
     * TREENODE_BULLET
     *
     * @field TREENODE_BULLET
     */
     lua_setfieldstringint(L, "TREENODE_BULLET", ImGuiTreeNodeFlags_Bullet);
    /**
     * TREENODE_FRAME_PADDING
     *
     * @field TREENODE_FRAME_PADDING
     */
     lua_setfieldstringint(L, "TREENODE_FRAME_PADDING", ImGuiTreeNodeFlags_FramePadding);
    /**
     * TREENODE_SPAN_AVAILABLE_WIDTH
     *
     * @field TREENODE_SPAN_AVAILABLE_WIDTH
     */
     lua_setfieldstringint(L, "TREENODE_SPAN_AVAILABLE_WIDTH", ImGuiTreeNodeFlags_SpanAvailWidth);
    /**
     * TREENODE_SPAN_FULL_WIDTH
     *
     * @field TREENODE_SPAN_FULL_WIDTH
     */
     lua_setfieldstringint(L, "TREENODE_SPAN_FULL_WIDTH", ImGuiTreeNodeFlags_SpanFullWidth);
    /**
     * TREENODE_NAV_LEFT_JUMPS_BACK_HERE
     *
     * @field TREENODE_NAV_LEFT_JUMPS_BACK_HERE
     */
     lua_setfieldstringint(L, "TREENODE_NAV_LEFT_JUMPS_BACK_HERE", ImGuiTreeNodeFlags_NavLeftJumpsBackHere);

    /**
     * KEY_TAB
     *
     * @field KEY_TAB
     */
     lua_setfieldstringint(L, "KEY_TAB", ImGuiKey_Tab);
    /**
     * KEY_LEFTARROW
     *
     * @field KEY_LEFTARROW
     */
     lua_setfieldstringint(L, "KEY_LEFTARROW", ImGuiKey_LeftArrow);
    /**
     * KEY_RIGHTARROW
     *
     * @field KEY_RIGHTARROW
     */
     lua_setfieldstringint(L, "KEY_RIGHTARROW", ImGuiKey_RightArrow);
    /**
     * KEY_UPARROW
     *
     * @field KEY_UPARROW
     */
     lua_setfieldstringint(L, "KEY_UPARROW", ImGuiKey_UpArrow);
    /**
     * KEY_DOWNARROW
     *
     * @field KEY_DOWNARROW
     */
     lua_setfieldstringint(L, "KEY_DOWNARROW", ImGuiKey_DownArrow);
    /**
     * KEY_PAGEUP
     *
     * @field KEY_PAGEUP
     */
     lua_setfieldstringint(L, "KEY_PAGEUP", ImGuiKey_PageUp);
    /**
     * KEY_PAGEDOWN
     *
     * @field KEY_PAGEDOWN
     */
     lua_setfieldstringint(L, "KEY_PAGEDOWN", ImGuiKey_PageDown);
    /**
     * KEY_HOME
     *
     * @field KEY_HOME
     */
     lua_setfieldstringint(L, "KEY_HOME", ImGuiKey_Home);
    /**
     * KEY_END
     *
     * @field KEY_END
     */
     lua_setfieldstringint(L, "KEY_END", ImGuiKey_End);
    /**
     * KEY_INSERT
     *
     * @field KEY_INSERT
     */
     lua_setfieldstringint(L, "KEY_INSERT", ImGuiKey_Insert);
    /**
     * KEY_DELETE
     *
     * @field KEY_DELETE
     */
     lua_setfieldstringint(L, "KEY_DELETE", ImGuiKey_Delete);
    /**
     * KEY_BACKSPACE
     *
     * @field KEY_BACKSPACE
     */
     lua_setfieldstringint(L, "KEY_BACKSPACE", ImGuiKey_Backspace);
    /**
     * KEY_SPACE
     *
     * @field KEY_SPACE
     */
     lua_setfieldstringint(L, "KEY_SPACE", ImGuiKey_Space);
    /**
     * KEY_ENTER
     *
     * @field KEY_ENTER
     */
     lua_setfieldstringint(L, "KEY_ENTER", ImGuiKey_Enter);
    /**
     * KEY_ESCAPE
     *
     * @field KEY_ESCAPE
     */
     lua_setfieldstringint(L, "KEY_ESCAPE", ImGuiKey_Escape);
    /**
     * KEY_KEYPADENTER
     *
     * @field KEY_KEYPADENTER
     */
     lua_setfieldstringint(L, "KEY_KEYPADENTER", ImGuiKey_KeypadEnter);
    /**
     * KEY_A
     *
     * @field KEY_A
     */
     lua_setfieldstringint(L, "KEY_A", ImGuiKey_A);
    /**
     * KEY_C
     *
     * @field KEY_C
     */
     lua_setfieldstringint(L, "KEY_C", ImGuiKey_C);
    /**
     * KEY_V
     *
     * @field KEY_V
     */
     lua_setfieldstringint(L, "KEY_V", ImGuiKey_V);
    /**
     * KEY_X
     *
     * @field KEY_X
     */
     lua_setfieldstringint(L, "KEY_X", ImGuiKey_X);
    /**
     * KEY_Y
     *
     * @field KEY_Y
     */
     lua_setfieldstringint(L, "KEY_Y", ImGuiKey_Y);
    /**
     * KEY_Z
     *
     * @field KEY_Z
     */
     lua_setfieldstringint(L, "KEY_Z", ImGuiKey_Z);

    /**
     * ImGuiCol_Text
     *
     * @field ImGuiCol_Text
     */
     lua_setfieldstringint(L, "ImGuiCol_Text", ImGuiCol_Text);
    /**
     * ImGuiCol_TextDisabled
     *
     * @field ImGuiCol_TextDisabled
     */
     lua_setfieldstringint(L, "ImGuiCol_TextDisabled", ImGuiCol_TextDisabled);
    /**
     * ImGuiCol_WindowBg
     *
     * @field ImGuiCol_WindowBg
     */
     lua_setfieldstringint(L, "ImGuiCol_WindowBg", ImGuiCol_WindowBg);
    /**
     * ImGuiCol_ChildBg
     *
     * @field ImGuiCol_ChildBg
     */
     lua_setfieldstringint(L, "ImGuiCol_ChildBg", ImGuiCol_ChildBg);
    /**
     * ImGuiCol_PopupBg
     *
     * @field ImGuiCol_PopupBg
     */
     lua_setfieldstringint(L, "ImGuiCol_PopupBg", ImGuiCol_PopupBg);
    /**
     * ImGuiCol_Border
     *
     * @field ImGuiCol_Border
     */
     lua_setfieldstringint(L, "ImGuiCol_Border", ImGuiCol_Border);
    /**
     * ImGuiCol_BorderShadow
     *
     * @field ImGuiCol_BorderShadow
     */
     lua_setfieldstringint(L, "ImGuiCol_BorderShadow", ImGuiCol_BorderShadow);
    /**
     * ImGuiCol_FrameBg
     *
     * @field ImGuiCol_FrameBg
     */
     lua_setfieldstringint(L, "ImGuiCol_FrameBg", ImGuiCol_FrameBg);
    /**
     * ImGuiCol_FrameBgHovered
     *
     * @field ImGuiCol_FrameBgHovered
     */
     lua_setfieldstringint(L, "ImGuiCol_FrameBgHovered", ImGuiCol_FrameBgHovered);
    /**
     * ImGuiCol_FrameBgActive
     *
     * @field ImGuiCol_FrameBgActive
     */
     lua_setfieldstringint(L, "ImGuiCol_FrameBgActive", ImGuiCol_FrameBgActive);
    /**
     * ImGuiCol_TitleBg
     *
     * @field ImGuiCol_TitleBg
     */
     lua_setfieldstringint(L, "ImGuiCol_TitleBg", ImGuiCol_TitleBg);
    /**
     * ImGuiCol_TitleBgActive
     *
     * @field ImGuiCol_TitleBgActive
     */
     lua_setfieldstringint(L, "ImGuiCol_TitleBgActive", ImGuiCol_TitleBgActive);
    /**
     * ImGuiCol_TitleBgCollapsed
     *
     * @field ImGuiCol_TitleBgCollapsed
     */
     lua_setfieldstringint(L, "ImGuiCol_TitleBgCollapsed", ImGuiCol_TitleBgCollapsed);
    /**
     * ImGuiCol_MenuBarBg
     *
     * @field ImGuiCol_MenuBarBg
     */
     lua_setfieldstringint(L, "ImGuiCol_MenuBarBg", ImGuiCol_MenuBarBg);
    /**
     * ImGuiCol_ScrollbarBg
     *
     * @field ImGuiCol_ScrollbarBg
     */
     lua_setfieldstringint(L, "ImGuiCol_ScrollbarBg", ImGuiCol_ScrollbarBg);
    /**
     * ImGuiCol_ScrollbarGrab
     *
     * @field ImGuiCol_ScrollbarGrab
     */
     lua_setfieldstringint(L, "ImGuiCol_ScrollbarGrab", ImGuiCol_ScrollbarGrab);
    /**
     * ImGuiCol_ScrollbarGrabHovered
     *
     * @field ImGuiCol_ScrollbarGrabHovered
     */
     lua_setfieldstringint(L, "ImGuiCol_ScrollbarGrabHovered", ImGuiCol_ScrollbarGrabHovered);
    /**
     * ImGuiCol_ScrollbarGrabActive
     *
     * @field ImGuiCol_ScrollbarGrabActive
     */
     lua_setfieldstringint(L, "ImGuiCol_ScrollbarGrabActive", ImGuiCol_ScrollbarGrabActive);
    /**
     * ImGuiCol_CheckMark
     *
     * @field ImGuiCol_CheckMark
     */
     lua_setfieldstringint(L, "ImGuiCol_CheckMark", ImGuiCol_CheckMark);
    /**
     * ImGuiCol_SliderGrab
     *
     * @field ImGuiCol_SliderGrab
     */
     lua_setfieldstringint(L, "ImGuiCol_SliderGrab", ImGuiCol_SliderGrab);
    /**
     * ImGuiCol_SliderGrabActive
     *
     * @field ImGuiCol_SliderGrabActive
     */
     lua_setfieldstringint(L, "ImGuiCol_SliderGrabActive", ImGuiCol_SliderGrabActive);
    /**
     * ImGuiCol_Button
     *
     * @field ImGuiCol_Button
     */
     lua_setfieldstringint(L, "ImGuiCol_Button", ImGuiCol_Button);
    /**
     * ImGuiCol_ButtonHovered
     *
     * @field ImGuiCol_ButtonHovered
     */
     lua_setfieldstringint(L, "ImGuiCol_ButtonHovered", ImGuiCol_ButtonHovered);
    /**
     * ImGuiCol_ButtonActive
     *
     * @field ImGuiCol_ButtonActive
     */
     lua_setfieldstringint(L, "ImGuiCol_ButtonActive", ImGuiCol_ButtonActive);
    /**
     * ImGuiCol_Header
     *
     * @field ImGuiCol_Header
     */
     lua_setfieldstringint(L, "ImGuiCol_Header", ImGuiCol_Header);
    /**
     * ImGuiCol_HeaderHovered
     *
     * @field ImGuiCol_HeaderHovered
     */
     lua_setfieldstringint(L, "ImGuiCol_HeaderHovered", ImGuiCol_HeaderHovered);
    /**
     * ImGuiCol_HeaderActive
     *
     * @field ImGuiCol_HeaderActive
     */
     lua_setfieldstringint(L, "ImGuiCol_HeaderActive", ImGuiCol_HeaderActive);
    /**
     * ImGuiCol_Separator
     *
     * @field ImGuiCol_Separator
     */
     lua_setfieldstringint(L, "ImGuiCol_Separator", ImGuiCol_Separator);
    /**
     * ImGuiCol_SeparatorHovered
     *
     * @field ImGuiCol_SeparatorHovered
     */
     lua_setfieldstringint(L, "ImGuiCol_SeparatorHovered", ImGuiCol_SeparatorHovered);
    /**
     * ImGuiCol_SeparatorActive
     *
     * @field ImGuiCol_SeparatorActive
     */
     lua_setfieldstringint(L, "ImGuiCol_SeparatorActive", ImGuiCol_SeparatorActive);
    /**
     * ImGuiCol_ResizeGrip
     *
     * @field ImGuiCol_ResizeGrip
     */
     lua_setfieldstringint(L, "ImGuiCol_ResizeGrip", ImGuiCol_ResizeGrip);
    /**
     * ImGuiCol_ResizeGripHovered
     *
     * @field ImGuiCol_ResizeGripHovered
     */
     lua_setfieldstringint(L, "ImGuiCol_ResizeGripHovered", ImGuiCol_ResizeGripHovered);
    /**
     * ImGuiCol_ResizeGripActive
     *
     * @field ImGuiCol_ResizeGripActive
     */
     lua_setfieldstringint(L, "ImGuiCol_ResizeGripActive", ImGuiCol_ResizeGripActive);
    /**
     * ImGuiCol_Tab
     *
     * @field ImGuiCol_Tab
     */
     lua_setfieldstringint(L, "ImGuiCol_Tab", ImGuiCol_Tab);
    /**
     * ImGuiCol_TabHovered
     *
     * @field ImGuiCol_TabHovered
     */
     lua_setfieldstringint(L, "ImGuiCol_TabHovered", ImGuiCol_TabHovered);
    /**
     * ImGuiCol_TabActive
     *
     * @field ImGuiCol_TabActive
     */
     lua_setfieldstringint(L, "ImGuiCol_TabActive", ImGuiCol_TabActive);
    /**
     * ImGuiCol_TabUnfocused
     *
     * @field ImGuiCol_TabUnfocused
     */
     lua_setfieldstringint(L, "ImGuiCol_TabUnfocused", ImGuiCol_TabUnfocused);
    /**
     * ImGuiCol_TabUnfocusedActive
     *
     * @field ImGuiCol_TabUnfocusedActive
     */
     lua_setfieldstringint(L, "ImGuiCol_TabUnfocusedActive", ImGuiCol_TabUnfocusedActive);
    /**
     * ImGuiCol_PlotLines
     *
     * @field ImGuiCol_PlotLines
     */
     lua_setfieldstringint(L, "ImGuiCol_PlotLines", ImGuiCol_PlotLines);
    /**
     * ImGuiCol_PlotLinesHovered
     *
     * @field ImGuiCol_PlotLinesHovered
     */
     lua_setfieldstringint(L, "ImGuiCol_PlotLinesHovered", ImGuiCol_PlotLinesHovered);
    /**
     * ImGuiCol_PlotHistogram
     *
     * @field ImGuiCol_PlotHistogram
     */
     lua_setfieldstringint(L, "ImGuiCol_PlotHistogram", ImGuiCol_PlotHistogram);
    /**
     * ImGuiCol_PlotHistogramHovered
     *
     * @field ImGuiCol_PlotHistogramHovered
     */
     lua_setfieldstringint(L, "ImGuiCol_PlotHistogramHovered", ImGuiCol_PlotHistogramHovered);
    /**
     * ImGuiCol_TableHeaderBg
     *
     * @field ImGuiCol_TableHeaderBg
     */
     lua_setfieldstringint(L, "ImGuiCol_TableHeaderBg", ImGuiCol_TableHeaderBg);
    /**
     * ImGuiCol_TableBorderStrong
     *
     * @field ImGuiCol_TableBorderStrong
     */
     lua_setfieldstringint(L, "ImGuiCol_TableBorderStrong", ImGuiCol_TableBorderStrong);
    /**
     * ImGuiCol_TableBorderLight
     *
     * @field ImGuiCol_TableBorderLight
     */
     lua_setfieldstringint(L, "ImGuiCol_TableBorderLight", ImGuiCol_TableBorderLight);
    /**
     * ImGuiCol_TableRowBg
     *
     * @field ImGuiCol_TableRowBg
     */
     lua_setfieldstringint(L, "ImGuiCol_TableRowBg", ImGuiCol_TableRowBg);
    /**
     * ImGuiCol_TableRowBgAlt
     *
     * @field ImGuiCol_TableRowBgAlt
     */
     lua_setfieldstringint(L, "ImGuiCol_TableRowBgAlt", ImGuiCol_TableRowBgAlt);
    /**
     * ImGuiCol_TextSelectedBg
     *
     * @field ImGuiCol_TextSelectedBg
     */
     lua_setfieldstringint(L, "ImGuiCol_TextSelectedBg", ImGuiCol_TextSelectedBg);
    /**
     * ImGuiCol_DragDropTarget
     *
     * @field ImGuiCol_DragDropTarget
     */
     lua_setfieldstringint(L, "ImGuiCol_DragDropTarget", ImGuiCol_DragDropTarget);
    /**
     * ImGuiCol_NavHighlight
     *
     * @field ImGuiCol_NavHighlight
     */
     lua_setfieldstringint(L, "ImGuiCol_NavHighlight", ImGuiCol_NavHighlight);
    /**
     * ImGuiCol_NavWindowingHighlight
     *
     * @field ImGuiCol_NavWindowingHighlight
     */
     lua_setfieldstringint(L, "ImGuiCol_NavWindowingHighlight", ImGuiCol_NavWindowingHighlight);
    /**
     * ImGuiCol_NavWindowingDimBg
     *
     * @field ImGuiCol_NavWindowingDimBg
     */
     lua_setfieldstringint(L, "ImGuiCol_NavWindowingDimBg", ImGuiCol_NavWindowingDimBg);
    /**
     * ImGuiCol_ModalWindowDimBg
     *
     * @field ImGuiCol_ModalWindowDimBg
     */
     lua_setfieldstringint(L, "ImGuiCol_ModalWindowDimBg", ImGuiCol_ModalWindowDimBg);

    /**
     * TABLECOLUMN_NONE
     *
     * @field TABLECOLUMN_NONE
     */
     lua_setfieldstringint(L, "TABLECOLUMN_NONE", ImGuiTableColumnFlags_None);
    /**
     * TABLECOLUMN_DEFAULTHIDE
     * Default as a hidden/disabled column.
     * @field TABLECOLUMN_DEFAULTHIDE
     */
     lua_setfieldstringint(L, "TABLECOLUMN_DEFAULTHIDE", ImGuiTableColumnFlags_DefaultHide);
    /**
     * TABLECOLUMN_DEFAULTSORT
     * Default as a sorting column.
     * @field TABLECOLUMN_DEFAULTSORT
     */
     lua_setfieldstringint(L, "TABLECOLUMN_DEFAULTSORT", ImGuiTableColumnFlags_DefaultSort);
    /**
     * TABLECOLUMN_WIDTHSTRETCH
     * Column will stretch. Preferable with horizontal scrolling disabled (default if table sizing policy is _SizingStretchSame or _SizingStretchProp).
     * @field TABLECOLUMN_WIDTHSTRETCH
     */
     lua_setfieldstringint(L, "TABLECOLUMN_WIDTHSTRETCH", ImGuiTableColumnFlags_WidthStretch);
    /**
     * TABLECOLUMN_WIDTHFIXED
     * Column will not stretch. Preferable with horizontal scrolling enabled (default if table sizing policy is _SizingFixedFit and table is resizable).
     * @field TABLECOLUMN_WIDTHFIXED
     */
     lua_setfieldstringint(L, "TABLECOLUMN_WIDTHFIXED", ImGuiTableColumnFlags_WidthFixed);
    /**
     * TABLECOLUMN_NORESIZE
     * Disable manual resizing.
     * @field TABLECOLUMN_NORESIZE
     */
     lua_setfieldstringint(L, "TABLECOLUMN_NORESIZE", ImGuiTableColumnFlags_NoResize);
    /**
     * TABLECOLUMN_NOREORDER
     * Disable manual reordering this column, this will also prevent other columns from crossing over this column.
     * @field TABLECOLUMN_NOREORDER
     */
     lua_setfieldstringint(L, "TABLECOLUMN_NOREORDER", ImGuiTableColumnFlags_NoReorder);
    /**
     * TABLECOLUMN_NOHIDE
     * Disable ability to hide/disable this column.
     * @field TABLECOLUMN_NOHIDE
     */
     lua_setfieldstringint(L, "TABLECOLUMN_NOHIDE", ImGuiTableColumnFlags_NoHide);
    /**
     * TABLECOLUMN_NOCLIP
     * Disable clipping for this column (all NoClip columns will render in a same draw command).
     * @field TABLECOLUMN_NOCLIP
     */
     lua_setfieldstringint(L, "TABLECOLUMN_NOCLIP", ImGuiTableColumnFlags_NoClip);
    /**
     * TABLECOLUMN_NOSORT
     * Disable ability to sort on this field (even if ImGuiTableFlags_Sortable is set on the table).
     * @field TABLECOLUMN_NOSORT
     */
     lua_setfieldstringint(L, "TABLECOLUMN_NOSORT", ImGuiTableColumnFlags_NoSort);
    /**
     * TABLECOLUMN_NOSORTASCENDING
     * Disable ability to sort in the ascending direction.
     * @field TABLECOLUMN_NOSORTASCENDING
     */
     lua_setfieldstringint(L, "TABLECOLUMN_NOSORTASCENDING", ImGuiTableColumnFlags_NoSortAscending);
    /**
     * TABLECOLUMN_NOSORTDESCENDING
     * Disable ability to sort in the descending direction.
     * @field TABLECOLUMN_NOSORTDESCENDING
     */
     lua_setfieldstringint(L, "TABLECOLUMN_NOSORTDESCENDING", ImGuiTableColumnFlags_NoSortDescending);
    /**
     * TABLECOLUMN_NOHEADERWIDTH
     * Disable header text width contribution to automatic column width.
     * @field TABLECOLUMN_NOHEADERWIDTH
     */
     lua_setfieldstringint(L, "TABLECOLUMN_NOHEADERWIDTH", ImGuiTableColumnFlags_NoHeaderWidth);
    /**
     * TABLECOLUMN_PREFERSORTASCENDING
     * Make the initial sort direction Ascending when first sorting on this column (default).
     * @field TABLECOLUMN_PREFERSORTASCENDING
     */
     lua_setfieldstringint(L, "TABLECOLUMN_PREFERSORTASCENDING", ImGuiTableColumnFlags_PreferSortAscending);
    /**
     * TABLECOLUMN_PREFERSORTDESCENDING
     * Make the initial sort direction Descending when first sorting on this column.
     * @field TABLECOLUMN_PREFERSORTDESCENDING
     */
     lua_setfieldstringint(L, "TABLECOLUMN_PREFERSORTDESCENDING", ImGuiTableColumnFlags_PreferSortDescending);
    /**
     * TABLECOLUMN_INDENTENABLE
     * Use current Indent value when entering cell (default for column 0).
     * @field TABLECOLUMN_INDENTENABLE
     */
     lua_setfieldstringint(L, "TABLECOLUMN_INDENTENABLE", ImGuiTableColumnFlags_IndentEnable);
    /**
     * TABLECOLUMN_INDENTDISABLE
     * Ignore current Indent value when entering cell (default for columns > 0). Indentation changes _within_ the cell will still be honored.
     * @field TABLECOLUMN_INDENTDISABLE
     */
     lua_setfieldstringint(L, "TABLECOLUMN_INDENTDISABLE", ImGuiTableColumnFlags_IndentDisable);

    /**
     * TABLE_NONE
     *
     * @field TABLE_NONE
     */
     lua_setfieldstringint(L, "TABLE_NONE", ImGuiTableFlags_None);
    /**
     * TABLE_RESIZABLE
     * Enable resizing columns.
     * @field TABLE_RESIZABLE
     */
     lua_setfieldstringint(L, "TABLE_RESIZABLE", ImGuiTableFlags_Resizable);
    /**
     * TABLE_REORDERABLE
     * Enable reordering columns in header row (need calling TableSetupColumn() + TableHeadersRow() to display headers)
     * @field TABLE_REORDERABLE
     */
     lua_setfieldstringint(L, "TABLE_REORDERABLE", ImGuiTableFlags_Reorderable);
    /**
     * TABLE_HIDEABLE
     * Enable hiding/disabling columns in context menu.
     * @field TABLE_HIDEABLE
     */
     lua_setfieldstringint(L, "TABLE_HIDEABLE", ImGuiTableFlags_Hideable);
    /**
     * TABLE_SORTABLE
     * Enable sorting. Call TableGetSortSpecs() to obtain sort specs. Also see ImGuiTableFlags_SortMulti and ImGuiTableFlags_SortTristate.
     * @field TABLE_SORTABLE
     */
     lua_setfieldstringint(L, "TABLE_SORTABLE", ImGuiTableFlags_Sortable);
    /**
     * TABLE_NOSAVEDSETTINGS
     * Disable persisting columns order, width and sort settings in the .ini file.
     * @field TABLE_NOSAVEDSETTINGS
     */
     lua_setfieldstringint(L, "TABLE_NOSAVEDSETTINGS", ImGuiTableFlags_NoSavedSettings);
    /**
     * TABLE_CONTEXTMENUINBODY
     * Right-click on columns body/contents will display table context menu. By default it is available in TableHeadersRow().
     * @field TABLE_CONTEXTMENUINBODY
     */
     lua_setfieldstringint(L, "TABLE_CONTEXTMENUINBODY", ImGuiTableFlags_ContextMenuInBody);
    /**
     * TABLE_ROWBG
     * Set each RowBg color with ImGuiCol_TableRowBg or ImGuiCol_TableRowBgAlt (equivalent of calling TableSetBgColor with ImGuiTableBgFlags_RowBg0 on each row manually)
     * @field TABLE_ROWBG
     */
     lua_setfieldstringint(L, "TABLE_ROWBG", ImGuiTableFlags_RowBg);
    /**
     * TABLE_BORDERSINNERH
     * Draw horizontal borders between rows.
     * @field TABLE_BORDERSINNERH
     */
     lua_setfieldstringint(L, "TABLE_BORDERSINNERH", ImGuiTableFlags_BordersInnerH);
    /**
     * TABLE_BORDERSOUTERH
     * Draw horizontal borders at the top and bottom.
     * @field TABLE_BORDERSOUTERH
     */
     lua_setfieldstringint(L, "TABLE_BORDERSOUTERH", ImGuiTableFlags_BordersOuterH);
    /**
     * TABLE_BORDERSINNERV
     * Draw vertical borders between columns.
     * @field TABLE_BORDERSINNERV
     */
     lua_setfieldstringint(L, "TABLE_BORDERSINNERV", ImGuiTableFlags_BordersInnerV);
    /**
     * TABLE_BORDERSOUTERV
     * Draw vertical borders on the left and right sides.
     * @field TABLE_BORDERSOUTERV
     */
     lua_setfieldstringint(L, "TABLE_BORDERSOUTERV", ImGuiTableFlags_BordersOuterV);
    /**
     * TABLE_BORDERSH
     * Draw horizontal borders.
     * @field TABLE_BORDERSH
     */
     lua_setfieldstringint(L, "TABLE_BORDERSH", ImGuiTableFlags_BordersH);
    /**
     * TABLE_BORDERSV
     * Draw vertical borders.
     * @field TABLE_BORDERSV
     */
     lua_setfieldstringint(L, "TABLE_BORDERSV", ImGuiTableFlags_BordersV);
    /**
     * TABLE_BORDERSINNER
     * Draw inner borders.
     * @field TABLE_BORDERSINNER
     */
     lua_setfieldstringint(L, "TABLE_BORDERSINNER", ImGuiTableFlags_BordersInner);
    /**
     * TABLE_BORDERSOUTER
     * Draw outer borders.
     * @field TABLE_BORDERSOUTER
     */
     lua_setfieldstringint(L, "TABLE_BORDERSOUTER", ImGuiTableFlags_BordersOuter);
    /**
     * TABLE_BORDERS
     * Draw all borders.
     * @field TABLE_BORDERS
     */
     lua_setfieldstringint(L, "TABLE_BORDERS", ImGuiTableFlags_Borders);
    /**
     * TABLE_NOBORDERSINBODY
     * [ALPHA] Disable vertical borders in columns Body (borders will always appears in Headers). -> May move to style
     * @field TABLE_NOBORDERSINBODY
     */
     lua_setfieldstringint(L, "TABLE_NOBORDERSINBODY", ImGuiTableFlags_NoBordersInBody);
    /**
     * TABLE_NOBORDERSINBODYUNTILRESIZE
     * [ALPHA] Disable vertical borders in columns Body until hovered for resize (borders will always appears in Headers). -> May move to style
     * @field TABLE_NOBORDERSINBODYUNTILRESIZE
     */
     lua_setfieldstringint(L, "TABLE_NOBORDERSINBODYUNTILRESIZE", ImGuiTableFlags_NoBordersInBodyUntilResize);
    /**
     * TABLE_SIZINGFIXEDFIT
     * Columns default to _WidthFixed or _WidthAuto (if resizable or not resizable), matching contents width.
     * @field TABLE_SIZINGFIXEDFIT
     */
     lua_setfieldstringint(L, "TABLE_SIZINGFIXEDFIT", ImGuiTableFlags_SizingFixedFit);
    /**
     * TABLE_SIZINGFIXEDSAME
     * Columns default to _WidthFixed or _WidthAuto (if resizable or not resizable), matching the maximum contents width of all columns. Implicitly enable ImGuiTableFlags_NoKeepColumnsVisible.
     * @field TABLE_SIZINGFIXEDSAME
     */
     lua_setfieldstringint(L, "TABLE_SIZINGFIXEDSAME", ImGuiTableFlags_SizingFixedSame);
    /**
     * TABLE_SIZINGSTRETCHPROP
     * Columns default to _WidthStretch with default weights proportional to each columns contents widths.
     * @field TABLE_SIZINGSTRETCHPROP
     */
     lua_setfieldstringint(L, "TABLE_SIZINGSTRETCHPROP", ImGuiTableFlags_SizingStretchProp);
    /**
     * TABLE_SIZINGSTRETCHSAME
     * Columns default to _WidthStretch with default weights all equal, unless overriden by TableSetupColumn().
     * @field TABLE_SIZINGSTRETCHSAME
     */
     lua_setfieldstringint(L, "TABLE_SIZINGSTRETCHSAME", ImGuiTableFlags_SizingStretchSame);
    /**
     * TABLE_NOHOSTEXTENDX
     * Make outer width auto-fit to columns, overriding outer_size.x value. Only available when ScrollX/ScrollY are disabled and Stretch columns are not used.
     * @field TABLE_NOHOSTEXTENDX
     */
     lua_setfieldstringint(L, "TABLE_NOHOSTEXTENDX", ImGuiTableFlags_NoHostExtendX);
    /**
     * TABLE_NOHOSTEXTENDY
     * Make outer height stop exactly at outer_size.y (prevent auto-extending table past the limit). Only available when ScrollX/ScrollY are disabled. Data below the limit will be clipped and not visible.
     * @field TABLE_NOHOSTEXTENDY
     */
     lua_setfieldstringint(L, "TABLE_NOHOSTEXTENDY", ImGuiTableFlags_NoHostExtendY);
    /**
     * TABLE_NOKEEPCOLUMNSVISIBLE
     * Disable keeping column always minimally visible when ScrollX is off and table gets too small. Not recommended if columns are resizable.
     * @field TABLE_NOKEEPCOLUMNSVISIBLE
     */
     lua_setfieldstringint(L, "TABLE_NOKEEPCOLUMNSVISIBLE", ImGuiTableFlags_NoKeepColumnsVisible);
    /**
     * TABLE_PRECISEWIDTHS
     * Disable distributing remainder width to stretched columns (width allocation on a 100-wide table with 3 columns: Without this flag: 33,33,34. With this flag: 33,33,33). With larger number of columns, resizing will appear to be less smooth.
     * @field TABLE_PRECISEWIDTHS
     */
     lua_setfieldstringint(L, "TABLE_PRECISEWIDTHS", ImGuiTableFlags_PreciseWidths);
    /**
     * TABLE_NOCLIP
     * Disable clipping rectangle for every individual columns (reduce draw command count, items will be able to overflow into other columns). Generally incompatible with TableSetupScrollFreeze().
     * @field TABLE_NOCLIP
     */
     lua_setfieldstringint(L, "TABLE_NOCLIP", ImGuiTableFlags_NoClip);
    /**
     * TABLE_PADOUTERX
     * Default if BordersOuterV is on. Enable outer-most padding. Generally desirable if you have headers.
     * @field TABLE_PADOUTERX
     */
     lua_setfieldstringint(L, "TABLE_PADOUTERX", ImGuiTableFlags_PadOuterX);
    /**
     * TABLE_NOPADOUTERX
     * Default if BordersOuterV is off. Disable outer-most padding.
     * @field TABLE_NOPADOUTERX
     */
     lua_setfieldstringint(L, "TABLE_NOPADOUTERX", ImGuiTableFlags_NoPadOuterX);
    /**
     * TABLE_NOPADINNERX
     * Disable inner padding between columns (double inner padding if BordersOuterV is on, single inner padding if BordersOuterV is off).
     * @field TABLE_NOPADINNERX
     */
     lua_setfieldstringint(L, "TABLE_NOPADINNERX", ImGuiTableFlags_NoPadInnerX);
    /**
     * TABLE_SCROLLX
     * Enable horizontal scrolling. Require 'outer_size' parameter of BeginTable() to specify the container size. Changes default sizing policy. Because this create a child window, ScrollY is currently generally recommended when using ScrollX.
     * @field TABLE_SCROLLX
     */
     lua_setfieldstringint(L, "TABLE_SCROLLX", ImGuiTableFlags_ScrollX);
    /**
     * TABLE_SCROLLY
     * Enable vertical scrolling. Require 'outer_size' parameter of BeginTable() to specify the container size.
     * @field TABLE_SCROLLY
     */
     lua_setfieldstringint(L, "TABLE_SCROLLY", ImGuiTableFlags_ScrollY);
    /**
     * TABLE_SORTMULTI
     * Hold shift when clicking headers to sort on multiple column. TableGetSortSpecs() may return specs where (SpecsCount > 1).
     * @field TABLE_SORTMULTI
     */
     lua_setfieldstringint(L, "TABLE_SORTMULTI", ImGuiTableFlags_SortMulti);
    /**
     * TABLE_SORTTRISTATE
     * Allow no sorting, disable default sorting. TableGetSortSpecs() may return specs where (SpecsCount == 0).
     * @field TABLE_SORTTRISTATE
     */
     lua_setfieldstringint(L, "TABLE_SORTTRISTATE", ImGuiTableFlags_SortTristate);

    /**
     * WINDOWFLAGS_NONE
     *
     * @field WINDOWFLAGS_NONE
     */
     lua_setfieldstringint(L, "WINDOWFLAGS_NONE", ImGuiWindowFlags_None);
    /**
     * WINDOWFLAGS_NOTITLEBAR
     * Disable title-bar
     * @field WINDOWFLAGS_NOTITLEBAR
     */
     lua_setfieldstringint(L, "WINDOWFLAGS_NOTITLEBAR", ImGuiWindowFlags_NoTitleBar);
    /**
     * WINDOWFLAGS_NORESIZE
     * Disable user resizing with the lower-right grip
     * @field WINDOWFLAGS_NORESIZE
     */
     lua_setfieldstringint(L, "WINDOWFLAGS_NORESIZE", ImGuiWindowFlags_NoResize);
    /**
     * WINDOWFLAGS_NOMOVE
     * Disable user moving the window
     * @field WINDOWFLAGS_NOMOVE
     */
     lua_setfieldstringint(L, "WINDOWFLAGS_NOMOVE", ImGuiWindowFlags_NoMove);
    /**
     * WINDOWFLAGS_NOSCROLLBAR
     * Disable scrollbars (window can still scroll with mouse or programmatically)
     * @field WINDOWFLAGS_NOSCROLLBAR
     */
     lua_setfieldstringint(L, "WINDOWFLAGS_NOSCROLLBAR", ImGuiWindowFlags_NoScrollbar);
    /**
     * WINDOWFLAGS_NOSCROLLWITHMOUSE
     * Disable user vertically scrolling with mouse wheel. On child window, mouse wheel will be forwarded to the parent unless NoScrollbar is also set.
     * @field WINDOWFLAGS_NOSCROLLWITHMOUSE
     */
     lua_setfieldstringint(L, "WINDOWFLAGS_NOSCROLLWITHMOUSE", ImGuiWindowFlags_NoScrollWithMouse);
    /**
     * WINDOWFLAGS_NOCOLLAPSE
     * Disable user collapsing window by double-clicking on it
     * @field WINDOWFLAGS_NOCOLLAPSE
     */
     lua_setfieldstringint(L, "WINDOWFLAGS_NOCOLLAPSE", ImGuiWindowFlags_NoCollapse);
    /**
     * WINDOWFLAGS_ALWAYSAUTORESIZE
     * Resize every window to its content every frame
     * @field WINDOWFLAGS_ALWAYSAUTORESIZE
     */
     lua_setfieldstringint(L, "WINDOWFLAGS_ALWAYSAUTORESIZE", ImGuiWindowFlags_AlwaysAutoResize);
    /**
     * WINDOWFLAGS_NOBACKGROUND
     * Disable drawing background color (WindowBg, etc.) and outside border. Similar as using SetNextWindowBgAlpha(0.0f).
     * @field WINDOWFLAGS_NOBACKGROUND
     */
     lua_setfieldstringint(L, "WINDOWFLAGS_NOBACKGROUND", ImGuiWindowFlags_NoBackground);
    /**
     * WINDOWFLAGS_NOSAVEDSETTINGS
     * Never load/save settings in .ini file
     * @field WINDOWFLAGS_NOSAVEDSETTINGS
     */
     lua_setfieldstringint(L, "WINDOWFLAGS_NOSAVEDSETTINGS", ImGuiWindowFlags_NoSavedSettings);
    /**
     * WINDOWFLAGS_NOMOUSEINPUTS
     * Disable catching mouse, hovering test with pass through.
     * @field WINDOWFLAGS_NOMOUSEINPUTS
     */
     lua_setfieldstringint(L, "WINDOWFLAGS_NOMOUSEINPUTS", ImGuiWindowFlags_NoMouseInputs);
    /**
     * WINDOWFLAGS_MENUBAR
     * Has a menu-bar
     * @field WINDOWFLAGS_MENUBAR
     */
     lua_setfieldstringint(L, "WINDOWFLAGS_MENUBAR", ImGuiWindowFlags_MenuBar);
    /**
     * WINDOWFLAGS_HORIZONTALSCROLLBAR
     *  prior to calling Begin() to specify width. Read code in imgui_demo in the "Horizontal Scrolling" section.
     * @field WINDOWFLAGS_HORIZONTALSCROLLBAR
     */
     lua_setfieldstringint(L, "WINDOWFLAGS_HORIZONTALSCROLLBAR", ImGuiWindowFlags_HorizontalScrollbar); // Allow horizontal scrollbar to appear (off by default). You may use SetNextWindowContentSize(ImVec2(width,0.0f));
    /**
     * WINDOWFLAGS_NOFOCUSONAPPEARING
     * Disable taking focus when transitioning from hidden to visible state
     * @field WINDOWFLAGS_NOFOCUSONAPPEARING
     */
     lua_setfieldstringint(L, "WINDOWFLAGS_NOFOCUSONAPPEARING", ImGuiWindowFlags_NoFocusOnAppearing);
    /**
     * WINDOWFLAGS_NOBRINGTOFRONTONFOCUS
     * Disable bringing window to front when taking focus (e.g. clicking on it or programmatically giving it focus)
     * @field WINDOWFLAGS_NOBRINGTOFRONTONFOCUS
     */
     lua_setfieldstringint(L, "WINDOWFLAGS_NOBRINGTOFRONTONFOCUS", ImGuiWindowFlags_NoBringToFrontOnFocus);
    /**
     * WINDOWFLAGS_ALWAYSVERTICALSCROLLBAR
     * Always show vertical scrollbar (even if ContentSize.y < Size.y)
     * @field WINDOWFLAGS_ALWAYSVERTICALSCROLLBAR
     */
     lua_setfieldstringint(L, "WINDOWFLAGS_ALWAYSVERTICALSCROLLBAR", ImGuiWindowFlags_AlwaysVerticalScrollbar);
    /**
     * WINDOWFLAGS_ALWAYSHORIZONTALSCROLLBAR
     * Always show horizontal scrollbar (even if ContentSize.x < Size.x)
     * @field WINDOWFLAGS_ALWAYSHORIZONTALSCROLLBAR
     */
     lua_setfieldstringint(L, "WINDOWFLAGS_ALWAYSHORIZONTALSCROLLBAR", ImGuiWindowFlags_AlwaysHorizontalScrollbar);
    /**
     * WINDOWFLAGS_ALWAYSUSEWINDOWPADDING
     * Ensure child windows without border uses style.WindowPadding (ignored by default for non-bordered child windows, because more convenient)
     * @field WINDOWFLAGS_ALWAYSUSEWINDOWPADDING
     */
     lua_setfieldstringint(L, "WINDOWFLAGS_ALWAYSUSEWINDOWPADDING", ImGuiWindowFlags_AlwaysUseWindowPadding);
    /**
     * WINDOWFLAGS_NONAVINPUTS
     * No gamepad/keyboard navigation within the window
     * @field WINDOWFLAGS_NONAVINPUTS
     */
     lua_setfieldstringint(L, "WINDOWFLAGS_NONAVINPUTS", ImGuiWindowFlags_NoNavInputs);
    /**
     * WINDOWFLAGS_NONAVFOCUS
     * No focusing toward this window with gamepad/keyboard navigation (e.g. skipped by CTRL+TAB)
     * @field WINDOWFLAGS_NONAVFOCUS
     */
     lua_setfieldstringint(L, "WINDOWFLAGS_NONAVFOCUS", ImGuiWindowFlags_NoNavFocus);
    /**
     * WINDOWFLAGS_UNSAVEDDOCUMENT
     * Append '*' to title without affecting the ID, as a convenience to avoid using the ### operator. When used in a tab/docking context, tab is selected on closure and closure is deferred by one frame to allow code to cancel the closure (with a confirmation popup, etc.) without flicker.
     * @field WINDOWFLAGS_UNSAVEDDOCUMENT
     */
     lua_setfieldstringint(L, "WINDOWFLAGS_UNSAVEDDOCUMENT", ImGuiWindowFlags_UnsavedDocument);
    /**
     * WINDOWFLAGS_NONAV
     * ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus,
     * @field WINDOWFLAGS_NONAV
     */
     lua_setfieldstringint(L, "WINDOWFLAGS_NONAV", ImGuiWindowFlags_NoNav);
    /**
     * WINDOWFLAGS_NODECORATION
     * ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse,
     * @field WINDOWFLAGS_NODECORATION
     */
     lua_setfieldstringint(L, "WINDOWFLAGS_NODECORATION", ImGuiWindowFlags_NoDecoration);
    /**
     * WINDOWFLAGS_NOINPUTS
     * ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus,
     * @field WINDOWFLAGS_NOINPUTS
     */
     lua_setfieldstringint(L, "WINDOWFLAGS_NOINPUTS", ImGuiWindowFlags_NoInputs);

    /**
     * POPUPFLAGS_NONE
     *
     * @field POPUPFLAGS_NONE
     */
     lua_setfieldstringint(L, "POPUPFLAGS_NONE", ImGuiPopupFlags_None);
    /**
     * POPUPFLAGS_MOUSEBUTTONLEFT
     *         // For BeginPopupContext*(): open on Left Mouse release. Guaranteed to always be == 0 (same as ImGuiMouseButton_Left)
     * @field POPUPFLAGS_MOUSEBUTTONLEFT
     */
     lua_setfieldstringint(L, "POPUPFLAGS_MOUSEBUTTONLEFT", ImGuiPopupFlags_MouseButtonLeft);
    /**
     * POPUPFLAGS_MOUSEBUTTONRIGHT
     *         // For BeginPopupContext*(): open on Right Mouse release. Guaranteed to always be == 1 (same as ImGuiMouseButton_Right)
     * @field POPUPFLAGS_MOUSEBUTTONRIGHT
     */
     lua_setfieldstringint(L, "POPUPFLAGS_MOUSEBUTTONRIGHT", ImGuiPopupFlags_MouseButtonRight);
    /**
     * POPUPFLAGS_MOUSEBUTTONMIDDLE
     *         // For BeginPopupContext*(): open on Middle Mouse release. Guaranteed to always be == 2 (same as ImGuiMouseButton_Middle)
     * @field POPUPFLAGS_MOUSEBUTTONMIDDLE
     */
     lua_setfieldstringint(L, "POPUPFLAGS_MOUSEBUTTONMIDDLE", ImGuiPopupFlags_MouseButtonMiddle);
    /**
     * POPUPFLAGS_MOUSEBUTTONMASK
     *
     * @field POPUPFLAGS_MOUSEBUTTONMASK
     */
     lua_setfieldstringint(L, "POPUPFLAGS_MOUSEBUTTONMASK", ImGuiPopupFlags_MouseButtonMask_);
    /**
     * POPUPFLAGS_MOUSEBUTTONDEFAULT
     *
     * @field POPUPFLAGS_MOUSEBUTTONDEFAULT
     */
     lua_setfieldstringint(L, "POPUPFLAGS_MOUSEBUTTONDEFAULT", ImGuiPopupFlags_MouseButtonDefault_);
    /**
     * POPUPFLAGS_NOOPENOVEREXISTINGPOPUP
     * For OpenPopup*(), BeginPopupContext*(): don't open if there's already a popup at the same level of the popup stack
     * @field POPUPFLAGS_NOOPENOVEREXISTINGPOPUP
     */
     lua_setfieldstringint(L, "POPUPFLAGS_NOOPENOVEREXISTINGPOPUP", ImGuiPopupFlags_NoOpenOverExistingPopup);
    /**
     * POPUPFLAGS_NOOPENOVERITEMS
     * For BeginPopupContextWindow(): don't return true when hovering items, only when hovering empty space
     * @field POPUPFLAGS_NOOPENOVERITEMS
     */
     lua_setfieldstringint(L, "POPUPFLAGS_NOOPENOVERITEMS", ImGuiPopupFlags_NoOpenOverItems);
    /**
     * POPUPFLAGS_ANYPOPUPID
     * For IsPopupOpen(): ignore the ImGuiID parameter and test for any popup.
     * @field POPUPFLAGS_ANYPOPUPID
     */
     lua_setfieldstringint(L, "POPUPFLAGS_ANYPOPUPID", ImGuiPopupFlags_AnyPopupId);
    /**
     * POPUPFLAGS_ANYPOPUPLEVEL
     * For IsPopupOpen(): search/test at any level of the popup stack (default test in the current level)
     * @field POPUPFLAGS_ANYPOPUPLEVEL
     */
     lua_setfieldstringint(L, "POPUPFLAGS_ANYPOPUPLEVEL", ImGuiPopupFlags_AnyPopupLevel);
    /**
     * POPUPFLAGS_ANYPOPUP
     *
     * @field POPUPFLAGS_ANYPOPUP
     */
     lua_setfieldstringint(L, "POPUPFLAGS_ANYPOPUP", ImGuiPopupFlags_AnyPopup);

    /**
     * DROPFLAGS_NONE
     *
     * @field DROPFLAGS_NONE
     */
     lua_setfieldstringint(L, "DROPFLAGS_NONE", ImGuiDragDropFlags_None);
    /**
     * DROPFLAGS_SOURCENOPREVIEWTOOLTIP
     * By default, a successful call to BeginDragDropSource opens a tooltip so you can display a preview or description of the source contents. This flag disable this behavior.
     * @field DROPFLAGS_SOURCENOPREVIEWTOOLTIP
     */
     lua_setfieldstringint(L, "DROPFLAGS_SOURCENOPREVIEWTOOLTIP", ImGuiDragDropFlags_SourceNoPreviewTooltip);
    /**
     * DROPFLAGS_SOURCENODISABLEHOVER
     * By default, when dragging we clear data so that IsItemHovered() will return false, to avoid subsequent user code submitting tooltips. This flag disable this behavior so you can still call IsItemHovered() on the source item.
     * @field DROPFLAGS_SOURCENODISABLEHOVER
     */
     lua_setfieldstringint(L, "DROPFLAGS_SOURCENODISABLEHOVER", ImGuiDragDropFlags_SourceNoDisableHover);
    /**
     * DROPFLAGS_SOURCENOHOLDTOOPENOTHERS
     * Disable the behavior that allows to open tree nodes and collapsing header by holding over them while dragging a source item.
     * @field DROPFLAGS_SOURCENOHOLDTOOPENOTHERS
     */
     lua_setfieldstringint(L, "DROPFLAGS_SOURCENOHOLDTOOPENOTHERS", ImGuiDragDropFlags_SourceNoHoldToOpenOthers);
    /**
     * DROPFLAGS_SOURCEALLOWNULLID
     * Allow items such as Text(), Image() that have no unique identifier to be used as drag source, by manufacturing a temporary identifier based on their window-relative position. This is extremely unusual within the dear imgui ecosystem and so we made it explicit.
     * @field DROPFLAGS_SOURCEALLOWNULLID
     */
     lua_setfieldstringint(L, "DROPFLAGS_SOURCEALLOWNULLID", ImGuiDragDropFlags_SourceAllowNullID);
    /**
     * DROPFLAGS_SOURCEEXTERN
     * External source (from outside of dear imgui), won't attempt to read current item/window info. Will always return true. Only one Extern source can be active simultaneously.
     * @field DROPFLAGS_SOURCEEXTERN
     */
     lua_setfieldstringint(L, "DROPFLAGS_SOURCEEXTERN", ImGuiDragDropFlags_SourceExtern);
    /**
     * DROPFLAGS_SOURCEAUTOEXPIREPAYLOAD
     * Automatically expire the payload if the source cease to be submitted (otherwise payloads are persisting while being dragged)
     * @field DROPFLAGS_SOURCEAUTOEXPIREPAYLOAD
     */
     lua_setfieldstringint(L, "DROPFLAGS_SOURCEAUTOEXPIREPAYLOAD", ImGuiDragDropFlags_SourceAutoExpirePayload);
    /**
     * DROPFLAGS_ACCEPTBEFOREDELIVERY
     * AcceptDragDropPayload() will returns true even before the mouse button is released. You can then call IsDelivery() to test if the payload needs to be delivered.
     * @field DROPFLAGS_ACCEPTBEFOREDELIVERY
     */
     lua_setfieldstringint(L, "DROPFLAGS_ACCEPTBEFOREDELIVERY", ImGuiDragDropFlags_AcceptBeforeDelivery);
    /**
     * DROPFLAGS_ACCEPTNODRAWDEFAULTRECT
     * Do not draw the default highlight rectangle when hovering over target.
     * @field DROPFLAGS_ACCEPTNODRAWDEFAULTRECT
     */
     lua_setfieldstringint(L, "DROPFLAGS_ACCEPTNODRAWDEFAULTRECT", ImGuiDragDropFlags_AcceptNoDrawDefaultRect);
    /**
     * DROPFLAGS_ACCEPTNOPREVIEWTOOLTIP
     * Request hiding the BeginDragDropSource tooltip from the BeginDragDropTarget site.
     * @field DROPFLAGS_ACCEPTNOPREVIEWTOOLTIP
     */
     lua_setfieldstringint(L, "DROPFLAGS_ACCEPTNOPREVIEWTOOLTIP", ImGuiDragDropFlags_AcceptNoPreviewTooltip);
    /**
     * DROPFLAGS_ACCEPTPEEKONLY
     * For peeking ahead and inspecting the payload before delivery.
     * @field DROPFLAGS_ACCEPTPEEKONLY
     */
     lua_setfieldstringint(L, "DROPFLAGS_ACCEPTPEEKONLY", ImGuiDragDropFlags_AcceptPeekOnly);


    /**
     * INPUTFLAGS_NONE
     *
     * @field INPUTFLAGS_NONE
     */
     lua_setfieldstringint(L, "INPUTFLAGS_NONE", ImGuiInputTextFlags_None);
    /**
     * INPUTFLAGS_CHARSDECIMAL
     * @field INPUTFLAGS_CHARSDECIMAL
     */
     lua_setfieldstringint(L, "INPUTFLAGS_CHARSDECIMAL", ImGuiInputTextFlags_CharsDecimal);
    /**
     * INPUTFLAGS_CHARSHEXADECIMAL
     * @field INPUTFLAGS_CHARSHEXADECIMAL
     */
     lua_setfieldstringint(L, "INPUTFLAGS_CHARSHEXADECIMAL", ImGuiInputTextFlags_CharsHexadecimal);
    /**
     * INPUTFLAGS_CHARSUPPERCASE
     * Turn a..z into A..Z
     * @field INPUTFLAGS_CHARSUPPERCASE
     */
     lua_setfieldstringint(L, "INPUTFLAGS_CHARSUPPERCASE", ImGuiInputTextFlags_CharsUppercase);
    /**
     * INPUTFLAGS_CHARSNOBLANK
     * Filter out spaces, tabs
     * @field INPUTFLAGS_CHARSNOBLANK
     */
     lua_setfieldstringint(L, "INPUTFLAGS_CHARSNOBLANK", ImGuiInputTextFlags_CharsNoBlank);
    /**
     * INPUTFLAGS_AUTOSELECTALL
     * Select entire text when first taking mouse focus
     * @field INPUTFLAGS_AUTOSELECTALL
     */
     lua_setfieldstringint(L, "INPUTFLAGS_AUTOSELECTALL", ImGuiInputTextFlags_AutoSelectAll);
    /**
     * INPUTFLAGS_ENTERRETURNSTRUE
     * Return 'true' when Enter is pressed (as opposed to every time the value was modified). Consider looking at the IsItemDeactivatedAfterEdit() function.
     * @field INPUTFLAGS_ENTERRETURNSTRUE
     */
     lua_setfieldstringint(L, "INPUTFLAGS_ENTERRETURNSTRUE", ImGuiInputTextFlags_EnterReturnsTrue);
    /**
     * INPUTFLAGS_CALLBACKCOMPLETION
     * Callback on pressing TAB (for completion handling)
     * @field INPUTFLAGS_CALLBACKCOMPLETION
     */
     lua_setfieldstringint(L, "INPUTFLAGS_CALLBACKCOMPLETION", ImGuiInputTextFlags_CallbackCompletion);
    /**
     * INPUTFLAGS_CALLBACKHISTORY
     * Callback on pressing Up/Down arrows (for history handling)
     * @field INPUTFLAGS_CALLBACKHISTORY
     */
     lua_setfieldstringint(L, "INPUTFLAGS_CALLBACKHISTORY", ImGuiInputTextFlags_CallbackHistory);
    /**
     * INPUTFLAGS_CALLBACKALWAYS
     * Callback on each iteration. User code may query cursor position, modify text buffer.
     * @field INPUTFLAGS_CALLBACKALWAYS
     */
     lua_setfieldstringint(L, "INPUTFLAGS_CALLBACKALWAYS", ImGuiInputTextFlags_CallbackAlways);
    /**
     * INPUTFLAGS_CALLBACKCHARFILTER
     * Callback on character inputs to replace or discard them. Modify 'EventChar' to replace or discard, or return 1 in callback to discard.
     * @field INPUTFLAGS_CALLBACKCHARFILTER
     */
     lua_setfieldstringint(L, "INPUTFLAGS_CALLBACKCHARFILTER", ImGuiInputTextFlags_CallbackCharFilter);
    /**
     * INPUTFLAGS_ALLOWTABINPUT
     * Pressing TAB input a '\t' character into the text field
     * @field INPUTFLAGS_ALLOWTABINPUT
     */
     lua_setfieldstringint(L, "INPUTFLAGS_ALLOWTABINPUT", ImGuiInputTextFlags_AllowTabInput);
    /**
     * INPUTFLAGS_CTRLENTERFORNEWLINE
     * In multi-line mode, unfocus with Enter, add new line with Ctrl+Enter (default is opposite: unfocus with Ctrl+Enter, add line with Enter).
     * @field INPUTFLAGS_CTRLENTERFORNEWLINE
     */
     lua_setfieldstringint(L, "INPUTFLAGS_CTRLENTERFORNEWLINE", ImGuiInputTextFlags_CtrlEnterForNewLine);
    /**
     * INPUTFLAGS_NOHORIZONTALSCROLL
     * Disable following the cursor horizontally
     * @field INPUTFLAGS_NOHORIZONTALSCROLL
     */
     lua_setfieldstringint(L, "INPUTFLAGS_NOHORIZONTALSCROLL", ImGuiInputTextFlags_NoHorizontalScroll);
    /**
     * INPUTFLAGS_ALWAYSOVERWRITE
     * Insert mode
     * @field INPUTFLAGS_ALWAYSOVERWRITE
     */
     lua_setfieldstringint(L, "INPUTFLAGS_ALWAYSOVERWRITE", ImGuiInputTextFlags_AlwaysOverwrite);
    /**
     * INPUTFLAGS_READONLY
     * Read-only mode
     * @field INPUTFLAGS_READONLY
     */
     lua_setfieldstringint(L, "INPUTFLAGS_READONLY", ImGuiInputTextFlags_ReadOnly);
    /**
     * INPUTFLAGS_PASSWORD
     * Password mode, display all characters as '*'
     * @field INPUTFLAGS_PASSWORD
     */
     lua_setfieldstringint(L, "INPUTFLAGS_PASSWORD", ImGuiInputTextFlags_Password);
    /**
     * INPUTFLAGS_NOUNDOREDO
     * Disable undo/redo. Note that input text owns the text data while active, if you want to provide your own undo/redo stack you need e.g. to call ClearActiveID().
     * @field INPUTFLAGS_NOUNDOREDO
     */
     lua_setfieldstringint(L, "INPUTFLAGS_NOUNDOREDO", ImGuiInputTextFlags_NoUndoRedo);
    /**
     * INPUTFLAGS_CHARSSCIENTIFIC
     * Allow 0123456789.+-*\/eE (Scientific notation input)
     * @field INPUTFLAGS_CHARSSCIENTIFIC
     */
     lua_setfieldstringint(L, "INPUTFLAGS_CHARSSCIENTIFIC", ImGuiInputTextFlags_CharsScientific);
    /**
     * INPUTFLAGS_CALLBACKRESIZE
     * Callback on buffer capacity changes request (beyond 'buf_size' parameter value), allowing the string to grow. Notify when the string wants to be resized (for string types which hold a cache of their Size). You will be provided a new BufSize in the callback and NEED to honor it. (see misc/cpp/imgui_stdlib.h for an example of using this)
     * @field INPUTFLAGS_CALLBACKRESIZE
     */
     lua_setfieldstringint(L, "INPUTFLAGS_CALLBACKRESIZE", ImGuiInputTextFlags_CallbackResize);
    /**
     * INPUTFLAGS_CALLBACKEDIT
     * Callback on any edit (note that InputText() already returns true on edit, the callback is useful mainly to manipulate the underlying buffer while focus is active)
     * @field INPUTFLAGS_CALLBACKEDIT
     */
     lua_setfieldstringint(L, "INPUTFLAGS_CALLBACKEDIT", ImGuiInputTextFlags_CallbackEdit);
    /**
     * INPUTFLAGS_ESCAPECLEARSALL
     * Callback on any edit (note that InputText() already returns true on edit, the callback is useful mainly to manipulate the underlying buffer while focus is active)
     * @field INPUTFLAGS_ESCAPECLEARSALL
     */
     lua_setfieldstringint(L, "INPUTFLAGS_ESCAPECLEARSALL", ImGuiInputTextFlags_EscapeClearsAll);

    /**
     * COLOREDITFLAGS_NONE
     *
     * @field COLOREDITFLAGS_NONE
     */
     lua_setfieldstringint(L, "COLOREDITFLAGS_NONE", ImGuiColorEditFlags_None);
    /**
     * COLOREDITFLAGS_NOALPHA
     *              // ColorEdit, ColorPicker, ColorButton: ignore Alpha component (will only read 3 components from the input pointer).
     * @field COLOREDITFLAGS_NOALPHA
     */
     lua_setfieldstringint(L, "COLOREDITFLAGS_NOALPHA", ImGuiColorEditFlags_NoAlpha);
    /**
     * COLOREDITFLAGS_NOPICKER
     *              // ColorEdit: disable picker when clicking on color square.
     * @field COLOREDITFLAGS_NOPICKER
     */
     lua_setfieldstringint(L, "COLOREDITFLAGS_NOPICKER", ImGuiColorEditFlags_NoPicker);
    /**
     * COLOREDITFLAGS_NOOPTIONS
     *              // ColorEdit: disable toggling options menu when right-clicking on inputs/small preview.
     * @field COLOREDITFLAGS_NOOPTIONS
     */
     lua_setfieldstringint(L, "COLOREDITFLAGS_NOOPTIONS", ImGuiColorEditFlags_NoOptions);
    /**
     * COLOREDITFLAGS_NOSMALLPREVIEW
     *              // ColorEdit, ColorPicker: disable color square preview next to the inputs. (e.g. to show only the inputs)
     * @field COLOREDITFLAGS_NOSMALLPREVIEW
     */
     lua_setfieldstringint(L, "COLOREDITFLAGS_NOSMALLPREVIEW", ImGuiColorEditFlags_NoSmallPreview);
    /**
     * COLOREDITFLAGS_NOINPUTS
     *              // ColorEdit, ColorPicker: disable inputs sliders/text widgets (e.g. to show only the small preview color square).
     * @field COLOREDITFLAGS_NOINPUTS
     */
     lua_setfieldstringint(L, "COLOREDITFLAGS_NOINPUTS", ImGuiColorEditFlags_NoInputs);
    /**
     * COLOREDITFLAGS_NOTOOLTIP
     *              // ColorEdit, ColorPicker, ColorButton: disable tooltip when hovering the preview.
     * @field COLOREDITFLAGS_NOTOOLTIP
     */
     lua_setfieldstringint(L, "COLOREDITFLAGS_NOTOOLTIP", ImGuiColorEditFlags_NoTooltip);
    /**
     * COLOREDITFLAGS_NOLABEL
     *              // ColorEdit, ColorPicker: disable display of inline text label (the label is still forwarded to the tooltip and picker).
     * @field COLOREDITFLAGS_NOLABEL
     */
     lua_setfieldstringint(L, "COLOREDITFLAGS_NOLABEL", ImGuiColorEditFlags_NoLabel);
    /**
     * COLOREDITFLAGS_NOSIDEPREVIEW
     *              // ColorPicker: disable bigger color preview on right side of the picker, use small color square preview instead.
     * @field COLOREDITFLAGS_NOSIDEPREVIEW
     */
     lua_setfieldstringint(L, "COLOREDITFLAGS_NOSIDEPREVIEW", ImGuiColorEditFlags_NoSidePreview);
    /**
     * COLOREDITFLAGS_NODRAGDROP
     *              // ColorEdit: disable drag and drop target. ColorButton: disable drag and drop source.
     * @field COLOREDITFLAGS_NODRAGDROP
     */
     lua_setfieldstringint(L, "COLOREDITFLAGS_NODRAGDROP", ImGuiColorEditFlags_NoDragDrop);
    /**
     * COLOREDITFLAGS_NOBORDER
     *              // ColorButton: disable border (which is enforced by default)
     * @field COLOREDITFLAGS_NOBORDER
     */
     lua_setfieldstringint(L, "COLOREDITFLAGS_NOBORDER", ImGuiColorEditFlags_NoBorder);
    /**
     * COLOREDITFLAGS_ALPHABAR
     *              // ColorEdit, ColorPicker: show vertical alpha bar/gradient in picker.
     * @field COLOREDITFLAGS_ALPHABAR
     */
     lua_setfieldstringint(L, "COLOREDITFLAGS_ALPHABAR", ImGuiColorEditFlags_AlphaBar);
    /**
     * COLOREDITFLAGS_ALPHAPREVIEW
     *              // ColorEdit, ColorPicker, ColorButton: display preview as a transparent color over a checkerboard, instead of opaque.
     * @field COLOREDITFLAGS_ALPHAPREVIEW
     */
     lua_setfieldstringint(L, "COLOREDITFLAGS_ALPHAPREVIEW", ImGuiColorEditFlags_AlphaPreview);
    /**
     * COLOREDITFLAGS_ALPHAPREVIEWHALF
     *              // ColorEdit, ColorPicker, ColorButton: display half opaque / half checkerboard, instead of opaque.
     * @field COLOREDITFLAGS_ALPHAPREVIEWHALF
     */
     lua_setfieldstringint(L, "COLOREDITFLAGS_ALPHAPREVIEWHALF", ImGuiColorEditFlags_AlphaPreviewHalf);
    /**
     * COLOREDITFLAGS_HDR
     *              // (WIP) ColorEdit: Currently only disable 0.0f..1.0f limits in RGBA edition (note: you probably want to use ImGuiColorEditFlags_Float flag as well).
     * @field COLOREDITFLAGS_HDR
     */
     lua_setfieldstringint(L, "COLOREDITFLAGS_HDR", ImGuiColorEditFlags_HDR);
    /**
     * COLOREDITFLAGS_DISPLAYRGB
     * [Display]    // ColorEdit: override _display_ type among RGB/HSV/Hex. ColorPicker: select any combination using one or more of RGB/HSV/Hex.
     * @field COLOREDITFLAGS_DISPLAYRGB
     */
     lua_setfieldstringint(L, "COLOREDITFLAGS_DISPLAYRGB", ImGuiColorEditFlags_DisplayRGB);
    /**
     * COLOREDITFLAGS_DISPLAYHSV
     * [Display]    // "
     * @field COLOREDITFLAGS_DISPLAYHSV
     */
     lua_setfieldstringint(L, "COLOREDITFLAGS_DISPLAYHSV", ImGuiColorEditFlags_DisplayHSV);
    /**
     * COLOREDITFLAGS_DISPLAYHEX
     * [Display]    // "
     * @field COLOREDITFLAGS_DISPLAYHEX
     */
     lua_setfieldstringint(L, "COLOREDITFLAGS_DISPLAYHEX", ImGuiColorEditFlags_DisplayHex);
    /**
     * COLOREDITFLAGS_UINT8
     * [DataType]   // ColorEdit, ColorPicker, ColorButton: _display_ values formatted as 0..255.
     * @field COLOREDITFLAGS_UINT8
     */
     lua_setfieldstringint(L, "COLOREDITFLAGS_UINT8", ImGuiColorEditFlags_Uint8);
    /**
     * COLOREDITFLAGS_FLOAT
     * [DataType]   // ColorEdit, ColorPicker, ColorButton: _display_ values formatted as 0.0f..1.0f floats instead of 0..255 integers. No round-trip of value via integers.
     * @field COLOREDITFLAGS_FLOAT
     */
     lua_setfieldstringint(L, "COLOREDITFLAGS_FLOAT", ImGuiColorEditFlags_Float);
    /**
     * COLOREDITFLAGS_PICKERHUEBAR
     * [Picker]     // ColorPicker: bar for Hue, rectangle for Sat/Value.
     * @field COLOREDITFLAGS_PICKERHUEBAR
     */
     lua_setfieldstringint(L, "COLOREDITFLAGS_PICKERHUEBAR", ImGuiColorEditFlags_PickerHueBar);
    /**
     * COLOREDITFLAGS_PICKERHUEWHEEL
     * [Picker]     // ColorPicker: wheel for Hue, triangle for Sat/Value.
     * @field COLOREDITFLAGS_PICKERHUEWHEEL
     */
     lua_setfieldstringint(L, "COLOREDITFLAGS_PICKERHUEWHEEL", ImGuiColorEditFlags_PickerHueWheel);
    /**
     * COLOREDITFLAGS_INPUTRGB
     * [Input]      // ColorEdit, ColorPicker: input and output data in RGB format.
     * @field COLOREDITFLAGS_INPUTRGB
     */
     lua_setfieldstringint(L, "COLOREDITFLAGS_INPUTRGB", ImGuiColorEditFlags_InputRGB);
    /**
     * COLOREDITFLAGS_INPUTHSV
     * [Input]      // ColorEdit, ColorPicker: input and output data in HSV format.
     * @field COLOREDITFLAGS_INPUTHSV
     */
     lua_setfieldstringint(L, "COLOREDITFLAGS_INPUTHSV", ImGuiColorEditFlags_InputHSV);


    /**
     * COND_NONE
     * No condition (always set the variable), same as _Always
     * @field COND_NONE
     */
     lua_setfieldstringint(L, "COND_NONE", ImGuiCond_None);
    /**
     * COND_ALWAYS
     * No condition (always set the variable)
     * @field COND_ALWAYS
     */
     lua_setfieldstringint(L, "COND_ALWAYS", ImGuiCond_Always);
    /**
     * COND_ONCE
     * Set the variable once per runtime session (only the first call will succeed)
     * @field COND_ONCE
     */
     lua_setfieldstringint(L, "COND_ONCE", ImGuiCond_Once);
    /**
     * COND_FIRSTUSEEVER
     * Set the variable if the object/window has no persistently saved data (no entry in .ini file)
     * @field COND_FIRSTUSEEVER
     */
     lua_setfieldstringint(L, "COND_FIRSTUSEEVER", ImGuiCond_FirstUseEver);
    /**
     * COND_APPEARING
     * Set the variable if the object/window is appearing after being hidden/inactive (or the first time)
     * @field COND_APPEARING
     */
     lua_setfieldstringint(L, "COND_APPEARING", ImGuiCond_Appearing);

    /**
     * DIR_NONE
     *
     * @field DIR_NONE
     */
     lua_setfieldstringint(L, "DIR_NONE", ImGuiDir_None);
    /**
     * DIR_LEFT
     *
     * @field DIR_LEFT
     */
     lua_setfieldstringint(L, "DIR_LEFT", ImGuiDir_Left);
    /**
     * DIR_RIGHT
     *
     * @field DIR_RIGHT
     */
     lua_setfieldstringint(L, "DIR_RIGHT", ImGuiDir_Right);
    /**
     * DIR_UP
     *
     * @field DIR_UP
     */
     lua_setfieldstringint(L, "DIR_UP", ImGuiDir_Up);
    /**
     * DIR_DOWN
     *
     * @field DIR_DOWN
     */
     lua_setfieldstringint(L, "DIR_DOWN", ImGuiDir_Down);

    /**
     * GLYPH_RANGES_DEFAULT
     *                 // Basic Latin, Extended Latin
     * @field GLYPH_RANGES_DEFAULT
     */
     lua_setfieldstringint(L, "GLYPH_RANGES_DEFAULT", ExtImGuiGlyphRanges_Default);
    /**
     * GLYPH_RANGES_GREEK
     *                   // Default + Greek and Coptic
     * @field GLYPH_RANGES_GREEK
     */
     lua_setfieldstringint(L, "GLYPH_RANGES_GREEK", ExtImGuiGlyphRanges_Greek);
    /**
     * GLYPH_RANGES_KOREAN
     *                  // Default + Korean characters
     * @field GLYPH_RANGES_KOREAN
     */
     lua_setfieldstringint(L, "GLYPH_RANGES_KOREAN", ExtImGuiGlyphRanges_Korean);
    /**
     * GLYPH_RANGES_JAPANESE
     *                // Default + Hiragana, Katakana, Half-Width, Selection of 2999 Ideographs
     * @field GLYPH_RANGES_JAPANESE
     */
     lua_setfieldstringint(L, "GLYPH_RANGES_JAPANESE", ExtImGuiGlyphRanges_Japanese);
    /**
     * GLYPH_RANGES_CHINESEFULL
     *             // Default + Half-Width + Japanese Hiragana/Katakana + full set of about 21000 CJK Unified Ideographs
     * @field GLYPH_RANGES_CHINESEFULL
     */
     lua_setfieldstringint(L, "GLYPH_RANGES_CHINESEFULL", ExtImGuiGlyphRanges_ChineseFull);
    /**
     * GLYPH_RANGES_CHINESESIMPLIFIEDCOMMON
     * // Default + Half-Width + Japanese Hiragana/Katakana + set of 2500 CJK Unified Ideographs for common simplified Chinese
     * @field GLYPH_RANGES_CHINESESIMPLIFIEDCOMMON
     */
     lua_setfieldstringint(L, "GLYPH_RANGES_CHINESESIMPLIFIEDCOMMON", ExtImGuiGlyphRanges_ChineseSimplifiedCommon);
    /**
     * GLYPH_RANGES_CYRILLIC
     *                // Default + about 400 Cyrillic characters
     * @field GLYPH_RANGES_CYRILLIC
     */
     lua_setfieldstringint(L, "GLYPH_RANGES_CYRILLIC", ExtImGuiGlyphRanges_Cyrillic);
    /**
     * GLYPH_RANGES_THAI
     *                    // Default + Thai characters
     * @field GLYPH_RANGES_THAI
     */
     lua_setfieldstringint(L, "GLYPH_RANGES_THAI", ExtImGuiGlyphRanges_Thai);
    /**
     * GLYPH_RANGES_VIETNAMESE
     *              // Default + Vietnamese characters
     * @field GLYPH_RANGES_VIETNAMESE
     */
     lua_setfieldstringint(L, "GLYPH_RANGES_VIETNAMESE", ExtImGuiGlyphRanges_Vietnamese);

    // For use with push_style_var / Imgui::PushStyleVar
    /**
     * STYLEVAR_ALPHA
     *                                              // float  Alpha
     * @field STYLEVAR_ALPHA
     */
     lua_setfieldstringint(L, "STYLEVAR_ALPHA", ImGuiStyleVar_Alpha);
    /**
     * STYLEVAR_DISABLEDALPHA
     * float  DisabledAlpha
     * @field STYLEVAR_DISABLEDALPHA
     */
     lua_setfieldstringint(L, "STYLEVAR_DISABLEDALPHA", ImGuiStyleVar_DisabledAlpha);
    /**
     * STYLEVAR_WINDOWPADDING
     * ImVec2 WindowPadding
     * @field STYLEVAR_WINDOWPADDING
     */
     lua_setfieldstringint(L, "STYLEVAR_WINDOWPADDING", ImGuiStyleVar_WindowPadding);
    /**
     * STYLEVAR_WINDOWROUNDING
     *                            // float  WindowRounding
     * @field STYLEVAR_WINDOWROUNDING
     */
     lua_setfieldstringint(L, "STYLEVAR_WINDOWROUNDING", ImGuiStyleVar_WindowRounding);
    /**
     * STYLEVAR_WINDOWBORDERSIZE
     *                        // float  WindowBorderSize
     * @field STYLEVAR_WINDOWBORDERSIZE
     */
     lua_setfieldstringint(L, "STYLEVAR_WINDOWBORDERSIZE", ImGuiStyleVar_WindowBorderSize);
    /**
     * STYLEVAR_WINDOWMINSIZE
     * ImVec2 WindowMinSize
     * @field STYLEVAR_WINDOWMINSIZE
     */
     lua_setfieldstringint(L, "STYLEVAR_WINDOWMINSIZE", ImGuiStyleVar_WindowMinSize);
    /**
     * STYLEVAR_WINDOWTITLEALIGN
     *                        // ImVec2 WindowTitleAlign
     * @field STYLEVAR_WINDOWTITLEALIGN
     */
     lua_setfieldstringint(L, "STYLEVAR_WINDOWTITLEALIGN", ImGuiStyleVar_WindowTitleAlign);
    /**
     * STYLEVAR_CHILDROUNDING
     * float  ChildRounding
     * @field STYLEVAR_CHILDROUNDING
     */
     lua_setfieldstringint(L, "STYLEVAR_CHILDROUNDING", ImGuiStyleVar_ChildRounding);
    /**
     * STYLEVAR_CHILDBORDERSIZE
     *                          // float  ChildBorderSize
     * @field STYLEVAR_CHILDBORDERSIZE
     */
     lua_setfieldstringint(L, "STYLEVAR_CHILDBORDERSIZE", ImGuiStyleVar_ChildBorderSize);
    /**
     * STYLEVAR_POPUPROUNDING
     * float  PopupRounding
     * @field STYLEVAR_POPUPROUNDING
     */
     lua_setfieldstringint(L, "STYLEVAR_POPUPROUNDING", ImGuiStyleVar_PopupRounding);
    /**
     * STYLEVAR_POPUPBORDERSIZE
     *                          // float  PopupBorderSize
     * @field STYLEVAR_POPUPBORDERSIZE
     */
     lua_setfieldstringint(L, "STYLEVAR_POPUPBORDERSIZE", ImGuiStyleVar_PopupBorderSize);
    /**
     * STYLEVAR_FRAMEPADDING
     *                                // ImVec2 FramePadding
     * @field STYLEVAR_FRAMEPADDING
     */
     lua_setfieldstringint(L, "STYLEVAR_FRAMEPADDING", ImGuiStyleVar_FramePadding);
    /**
     * STYLEVAR_FRAMEROUNDING
     * float  FrameRounding
     * @field STYLEVAR_FRAMEROUNDING
     */
     lua_setfieldstringint(L, "STYLEVAR_FRAMEROUNDING", ImGuiStyleVar_FrameRounding);
    /**
     * STYLEVAR_FRAMEBORDERSIZE
     *                          // float  FrameBorderSize
     * @field STYLEVAR_FRAMEBORDERSIZE
     */
     lua_setfieldstringint(L, "STYLEVAR_FRAMEBORDERSIZE", ImGuiStyleVar_FrameBorderSize);
    /**
     * STYLEVAR_ITEMSPACING
     *                                  // ImVec2 ItemSpacing
     * @field STYLEVAR_ITEMSPACING
     */
     lua_setfieldstringint(L, "STYLEVAR_ITEMSPACING", ImGuiStyleVar_ItemSpacing);
    /**
     * STYLEVAR_ITEMINNERSPACING
     *                        // ImVec2 ItemInnerSpacing
     * @field STYLEVAR_ITEMINNERSPACING
     */
     lua_setfieldstringint(L, "STYLEVAR_ITEMINNERSPACING", ImGuiStyleVar_ItemInnerSpacing);
    /**
     * STYLEVAR_INDENTSPACING
     * float  IndentSpacing
     * @field STYLEVAR_INDENTSPACING
     */
     lua_setfieldstringint(L, "STYLEVAR_INDENTSPACING", ImGuiStyleVar_IndentSpacing);
    /**
     * STYLEVAR_CELLPADDING
     *                                  // ImVec2 CellPadding
     * @field STYLEVAR_CELLPADDING
     */
     lua_setfieldstringint(L, "STYLEVAR_CELLPADDING", ImGuiStyleVar_CellPadding);
    /**
     * STYLEVAR_SCROLLBARSIZE
     * float  ScrollbarSize
     * @field STYLEVAR_SCROLLBARSIZE
     */
     lua_setfieldstringint(L, "STYLEVAR_SCROLLBARSIZE", ImGuiStyleVar_ScrollbarSize);
    /**
     * STYLEVAR_SCROLLBARROUNDING
     *                      // float  ScrollbarRounding
     * @field STYLEVAR_SCROLLBARROUNDING
     */
     lua_setfieldstringint(L, "STYLEVAR_SCROLLBARROUNDING", ImGuiStyleVar_ScrollbarRounding);
    /**
     * STYLEVAR_GRABMINSIZE
     *                                  // float  GrabMinSize
     * @field STYLEVAR_GRABMINSIZE
     */
     lua_setfieldstringint(L, "STYLEVAR_GRABMINSIZE", ImGuiStyleVar_GrabMinSize);
    /**
     * STYLEVAR_GRABROUNDING
     *                                // float  GrabRounding
     * @field STYLEVAR_GRABROUNDING
     */
     lua_setfieldstringint(L, "STYLEVAR_GRABROUNDING", ImGuiStyleVar_GrabRounding);
    /**
     * STYLEVAR_TABROUNDING
     *                                  // float  TabRounding
     * @field STYLEVAR_TABROUNDING
     */
     lua_setfieldstringint(L, "STYLEVAR_TABROUNDING", ImGuiStyleVar_TabRounding);
    /**
     * STYLEVAR_TABBORDERSIZE
     * float  TabBorderSize
     * @field STYLEVAR_TABBORDERSIZE
     */
     lua_setfieldstringint(L, "STYLEVAR_TABBORDERSIZE", ImGuiStyleVar_TabBorderSize);
    /**
     * STYLEVAR_TABBARBORDERSIZE
     *                        // float  TabBarBorderSize
     * @field STYLEVAR_TABBARBORDERSIZE
     */
     lua_setfieldstringint(L, "STYLEVAR_TABBARBORDERSIZE", ImGuiStyleVar_TabBarBorderSize);
    /**
     * STYLEVAR_TABLEANGLEDHEADERSANGLE
     *          // float  TableAngledHeadersAngle
     * @field STYLEVAR_TABLEANGLEDHEADERSANGLE
     */
     lua_setfieldstringint(L, "STYLEVAR_TABLEANGLEDHEADERSANGLE", ImGuiStyleVar_TableAngledHeadersAngle);
    /**
     * STYLEVAR_TABLEANGLEDHEADERSTEXTALIGN
     * ImVec2 TableAngledHeadersTextAlign
     * @field STYLEVAR_TABLEANGLEDHEADERSTEXTALIGN
     */
     lua_setfieldstringint(L, "STYLEVAR_TABLEANGLEDHEADERSTEXTALIGN", ImGuiStyleVar_TableAngledHeadersTextAlign);
    /**
     * STYLEVAR_BUTTONTEXTALIGN
     *                          // ImVec2 ButtonTextAlign
     * @field STYLEVAR_BUTTONTEXTALIGN
     */
     lua_setfieldstringint(L, "STYLEVAR_BUTTONTEXTALIGN", ImGuiStyleVar_ButtonTextAlign);
    /**
     * STYLEVAR_SELECTABLETEXTALIGN
     *                  // ImVec2 SelectableTextAlign
     * @field STYLEVAR_SELECTABLETEXTALIGN
     */
     lua_setfieldstringint(L, "STYLEVAR_SELECTABLETEXTALIGN", ImGuiStyleVar_SelectableTextAlign);
    /**
     * STYLEVAR_SEPARATORTEXTBORDERSIZE
     *          // float  SeparatorTextBorderSize
     * @field STYLEVAR_SEPARATORTEXTBORDERSIZE
     */
     lua_setfieldstringint(L, "STYLEVAR_SEPARATORTEXTBORDERSIZE", ImGuiStyleVar_SeparatorTextBorderSize);
    /**
     * STYLEVAR_SEPARATORTEXTALIGN
     *                    // ImVec2 SeparatorTextAlign
     * @field STYLEVAR_SEPARATORTEXTALIGN
     */
     lua_setfieldstringint(L, "STYLEVAR_SEPARATORTEXTALIGN", ImGuiStyleVar_SeparatorTextAlign);
    /**
     * STYLEVAR_SEPARATORTEXTPADDING
     *                // ImVec2 SeparatorTextPadding
     * @field STYLEVAR_SEPARATORTEXTPADDING
     */
     lua_setfieldstringint(L, "STYLEVAR_SEPARATORTEXTPADDING", ImGuiStyleVar_SeparatorTextPadding);

    lua_pop(L, 1);
    assert(top == lua_gettop(L));
}

static dmExtension::Result AppInitializeDefoldImGui(dmExtension::AppParams* params)
{
    imgui_ExtensionInit();
    return dmExtension::RESULT_OK;
}

static dmExtension::Result InitializeDefoldImGui(dmExtension::Params* params)
{
    // This is actually more complex than this,
    // but that value is buried deep in the private internals of dmGraphics_OpenGL
    #ifdef DM_RELEASE
    g_VerifyGraphicsCalls = false;
    #else
    g_VerifyGraphicsCalls = true;
    #endif

    LuaInit(params->m_L);
    float displayWidth = dmConfigFile::GetFloat(params->m_ConfigFile, "display.width", 960.0f);
    float displayHeight = dmConfigFile::GetFloat(params->m_ConfigFile, "display.height", 540.0f);
    imgui_Init(displayWidth, displayHeight);

    return dmExtension::RESULT_OK;
}

static dmExtension::Result AppFinalizeDefoldImGui(dmExtension::AppParams* params)
{
    return dmExtension::RESULT_OK;
}

static dmExtension::Result FinalizeDefoldImGui(dmExtension::Params* params)
{
    dmLogInfo("FinalizeDefoldImGui");
    imgui_Shutdown();
    imgui_ExtensionShutdown();
    return dmExtension::RESULT_OK;
}

static dmExtension::Result OnUpdateDefoldImGui(dmExtension::Params* params)
{
    return dmExtension::RESULT_OK;
}

static void OnEventDefoldImGui(dmExtension::Params* params, const dmExtension::Event* event)
{
    switch(event->m_Event)
    {
        case EXTENSION_EVENT_ID_ACTIVATEAPP:
        //dmLogInfo("OnEventDefoldImGui - EVENT_ID_ACTIVATEAPP\n");
        break;
        case EXTENSION_EVENT_ID_DEACTIVATEAPP:
        //dmLogInfo("OnEventDefoldImGui - EVENT_ID_DEACTIVATEAPP\n");
        break;
        case EXTENSION_EVENT_ID_ICONIFYAPP:
        //dmLogInfo("OnEventDefoldImGui - EVENT_ID_ICONIFYAPP\n");
        break;
        case EXTENSION_EVENT_ID_DEICONIFYAPP:
        //dmLogInfo("OnEventDefoldImGui - EVENT_ID_DEICONIFYAPP\n");
        break;
        default:
        //dmLogWarning("OnEventDefoldImGui - Unknown event id\n");
        break;
    }
}

#else

extern "C" DM_DLLEXPORT bool DefoldImGui_RegisterDrawCallback(DefoldImGuiDrawCallback callback, void* user_data)
{
    return false;
}

extern "C" DM_DLLEXPORT void DefoldImGui_UnregisterDrawCallback(DefoldImGuiDrawCallback callback, void* user_data)
{
}

static dmExtension::Result AppInitializeDefoldImGui(dmExtension::AppParams* params)
{
    return dmExtension::RESULT_OK;
}

static dmExtension::Result AppFinalizeDefoldImGui(dmExtension::AppParams* params)
{
    return dmExtension::RESULT_OK;
}

static dmExtension::Result InitializeDefoldImGui(dmExtension::Params* params)
{
    return dmExtension::RESULT_OK;
}

static dmExtension::Result FinalizeDefoldImGui(dmExtension::Params* params)
{
    return dmExtension::RESULT_OK;
}

static dmExtension::Result OnUpdateDefoldImGui(dmExtension::Params* params)
{
    return dmExtension::RESULT_OK;
}

static void OnEventDefoldImGui(dmExtension::Params* params, const dmExtension::Event* event)
{
}

#endif

DM_DECLARE_EXTENSION(DefoldImGui, LIB_NAME, AppInitializeDefoldImGui, AppFinalizeDefoldImGui, InitializeDefoldImGui, OnUpdateDefoldImGui, OnEventDefoldImGui, FinalizeDefoldImGui)
