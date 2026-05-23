#pragma once

#include <stdint.h>
#include <dmsdk/dlib/shared_library.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*DefoldImGuiDrawCallback)(void* user_data);

// Register a native draw callback. The callback is invoked once per rendered
// ImGui frame after ImGui::NewFrame() and before ImGui::Render().
// Returns true if registered, false if callback is null or the callback table is full.
DM_DLLEXPORT bool DefoldImGui_RegisterDrawCallback(DefoldImGuiDrawCallback callback, void* user_data);

// Remove a previously registered draw callback. Safe to call even if not registered.
DM_DLLEXPORT void DefoldImGui_UnregisterDrawCallback(DefoldImGuiDrawCallback callback, void* user_data);

#ifdef __cplusplus
}
#endif
