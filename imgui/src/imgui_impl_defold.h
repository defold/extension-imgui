#pragma once

#include "imgui/imgui.h"

#include <dmsdk/graphics/graphics.h>
#include <dmsdk/resource/resource.hpp>

bool ImGui_ImplDefold_Init(dmResource::HFactory resource_factory);
void ImGui_ImplDefold_Shutdown();
void ImGui_ImplDefold_NewFrame();
void ImGui_ImplDefold_RenderDrawData(ImDrawData* draw_data);

dmGraphics::HTexture ImGui_ImplDefold_CreateTexture(int width, int height, const void* rgba_pixels);
void ImGui_ImplDefold_DestroyTexture(dmGraphics::HTexture texture);

uint32_t ImGui_ImplDefold_RegisterTexture(dmGraphics::HTexture texture);
void ImGui_ImplDefold_UnregisterTexture(uint32_t texture_id);
dmGraphics::HTexture ImGui_ImplDefold_GetTexture(uint32_t texture_id);
