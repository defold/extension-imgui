# Dear ImGUI extension for Defold
This project adds support for [Dear ImGUI](https://github.com/ocornut/imgui) in Defold. Dear ImGUI is described as a "Bloat-free Graphical User interface for C++ with minimal dependencies". It is perfect for quickly creating a debug interface or for creation of in-game tools and settings.

![imgui1](https://user-images.githubusercontent.com/1300688/118636082-983cc000-b7d4-11eb-9a02-55785e4651ac.png)
![imgui2](https://user-images.githubusercontent.com/1300688/118636087-996ded00-b7d4-11eb-88e6-e90d15ede189.png)


## Why no auto-generated bindings?
Projects such as [cimgui](https://github.com/cimgui/cimgui) offers Dear ImGUI bindings for a number of different languages, including Lua. It could be used  to generate the bindings for this extension, but this extension was initially created as a way to learn more about the Dear ImGUI API and as such it made more sense to write the bindings by hand.


## Platforms support
The current state of platform support:

* ✅ macOS
* ✅ Linux
* ✅ Windows
* ✅ Android
* ✅ iOS
* ✅ HTML5


## Usage
Refer to [example/example.script](/example/example.script) to learn how to use the extension. Also check the bindings in `LuaInit()` in `extension_imgui.cpp`.


### Input
You need to update the input state in Dear ImGUI each frame to accurately reflect user input:

* `imgui.set_key_down(key, pressed)` - Set the pressed state of a key. The key must be one of the `imgui.KEY_` constants
* `imgui.set_key_modifier_super(pressed)`
* `imgui.set_key_modifier_alt(pressed)`
* `imgui.set_key_modifier_ctrl(pressed)`
* `imgui.set_key_modifier_shift(pressed)`
* `imgui.add_input_character(text)` - Add keyboard input
* `imgui.set_mouse_input(x, y, left, right, middle, wheel)` - Set mouse state (position, buttons and wheel)

You can add the `imgui/imgui.script` to a game object and let that handle input for you (also make sure to use imgui/bindings/imgui.input_bindings).


### Display size
You need to let Dear ImGUI know of any changes to the window size by calling `imgui.set_display_size(w, h)` when the screen size changes.

### Native C/C++ draw callbacks
Native extensions can register a draw callback invoked after `ImGui::NewFrame()` and before `ImGui::Render()`:

```cpp
#include "extension_imgui.h"
#include "imgui/src/imgui/imgui.h"

static void Draw(void* user_data) {
    ImGui::Begin("Native Window");
    ImGui::Text("Hello");
    ImGui::End();
}

DefoldImGui_RegisterDrawCallback(Draw, 0);
```

Callbacks run on the render thread/post-render callback where extension-imgui renders. Do not call `ImGui::NewFrame()`, `ImGui::Render()`, or backend render functions from the callback.

### Rendering the UI inside the engine render pipeline

By default the ImGui frame is drawn in the `POST_RENDER` extension callback, i.e.
straight to the default target after the render script finished. The UI therefore
bypasses every render script pass (post processing, transitions, blur, ...).

To let the engine pipeline see the UI, draw the frame into an offscreen target
owned by the extension and composite it from the render script:

```lua
-- render script update(), before recording any render command
local ui_texture = imgui.render_to_texture()   -- 0 when nothing was drawn
...
render.set_render_target(self.target)          -- any render target of your pipeline
render.enable_texture(0, ui_texture)
-- the offscreen target contains premultiplied alpha, so blend premultiplied
render.set_blend_func(graphics.BLEND_FACTOR_ONE, graphics.BLEND_FACTOR_ONE_MINUS_SRC_ALPHA)
render.draw(self.fullscreen_quad)              -- samples the UI texture
render.disable_texture(0)
```

Notes:

* Call it from the render script update. The extension issues `dmGraphics` calls
  immediately, so the UI has to be drawn before the command stream that
  composites it is recorded.
* `POST_RENDER` skips its own draw when the frame was already rendered offscreen,
  so the UI never shows up twice.
* The offscreen target is allocated at `DisplaySize * FramebufferScale` and is
  recreated whenever that size changes.
* Scissor coordinates are flipped when drawing into the offscreen target: unlike
  the swapchain there is no negative-height viewport, so the scissor origin is the
  bottom-left corner. Without the flip, small clip rects (popups, tooltips) are
  clipped at their mirrored position.

### How to generate API docs

* Download and unpack https://github.com/britzl/generatedocs/archive/refs/heads/main.zip
* From the root of this project run `path/to/generatedocs.py imgui extension_imgui.cpp`
