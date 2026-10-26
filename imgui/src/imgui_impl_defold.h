#pragma once

#include "imgui/imgui.h"

#include <dmsdk/graphics/graphics.h>
#include <dmsdk/resource/resource.hpp>

bool ImGui_ImplDefold_Init(dmResource::HFactory resource_factory);
void ImGui_ImplDefold_Shutdown();
void ImGui_ImplDefold_NewFrame();
void ImGui_ImplDefold_RenderDrawData(ImDrawData* draw_data);

// Draws the current ImGui frame into an offscreen render target owned by this
// extension and returns its color texture handle (0 when there is nothing to
// render or creation failed). The handle can be passed to render.enable_texture
// so the render script can composite the UI inside its own pipeline.
dmGraphics::HTexture ImGui_ImplDefold_RenderToTexture(uint32_t* out_width, uint32_t* out_height);

dmGraphics::HTexture ImGui_ImplDefold_CreateTexture(int width, int height, const void* rgba_pixels);
void ImGui_ImplDefold_DestroyTexture(dmGraphics::HTexture texture);

uint32_t ImGui_ImplDefold_RegisterTexture(dmGraphics::HTexture texture);
void ImGui_ImplDefold_UnregisterTexture(uint32_t texture_id);
dmGraphics::HTexture ImGui_ImplDefold_GetTexture(uint32_t texture_id);
