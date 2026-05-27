local JEDI = { "Luke", "Obi-Wan", "Yoda" }
local SITH = { "Maul", "Vader", "Palpatine" }

local COLOR = vmath.vector4()
local COLOR2 = vmath.vector4(255, 0, 0, 1)

return function(self)
	local selected = imgui.tree_node("root")
	if selected then
		imgui.bullet()
		self.foo_selected = imgui.selectable("foo", self.foo_selected)
		imgui.bullet()
		self.bar_selected = imgui.selectable("bar", self.bar_selected)
		imgui.tree_pop()
	end


	-- this is the old way of creating combo boxes
	-- it still exists in the ImGUI API since it's a quick way to create a combo box
	local changed, jedi = imgui.combo("Jedi##array", self.selected_jedi or 1, JEDI)
	if changed then
		self.selected_jedi = jedi
	end

	-- new way of creating combo boxes with a begin and end function
	local combo_text = self.selected_jedi and JEDI[self.selected_jedi] or "Select a Jedi"
	if imgui.begin_combo("Jedi##selectable", combo_text) then
		for i=1,#JEDI do
			local currently_selected = i == (self.selected_jedi or 1)
			local _, pushed = imgui.selectable(JEDI[i], currently_selected)
			if pushed then
				self.selected_jedi = i
			end
		end
		imgui.end_combo()
	end

	imgui.separator()

	if imgui.button("Push me to focus on test double") then
		imgui.set_keyboard_focus_here(0)
	end
	local testdbl = self.testdbl or 0
	local changed, value = imgui.input_double("test double", testdbl, 0.0001, 0.01, 5)
	if changed then
		self.testdbl = value
	end

	local progress = self.progress or 0
	local changed, p = imgui.input_float("progress float", progress, 0.01, 1.0, 3)
	if changed then
		self.progress = p
		progress = p
	end

	imgui.separator()
	imgui.draw_progress(progress, -1, 0.0)
	imgui.separator()

	local changed, p = imgui.drag_float("drag float", self.drag_float or 0, 0.2, 0.0, 100.0)
	if changed then
		self.drag_float = p
	end

	local changed, p = imgui.slider_float("slider float", self.slider_float or 0, 0.0, 100.0)
	if changed then
		self.slider_float = p
	end
	
	imgui.push_id("first")
	if imgui.button("Same name") then
		pprint("first same button")
	end
	imgui.pop_id()

	imgui.push_id("second")
	if imgui.button("Same name") then
		pprint("second same button")
	end
	imgui.pop_id()

	self.color = self.color or vmath.vector4()
	imgui.color_edit4("coloredit4", self.color)
	imgui.color_edit4("other color", COLOR2)
end
