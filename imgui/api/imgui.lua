--[[
Dear ImGUI API
Functions and constants for interacting with Dear ImGUI
--]]

---@meta
---@diagnostic disable: lowercase-global
---@diagnostic disable: missing-return

---@class defold_api.imgui
---@
---@field MOUSEBUTTON_LEFT integer
---@field MOUSEBUTTON_RIGHT integer
---@field MOUSEBUTTON_MIDDLE integer
---@
---@field KEY_TAB integer
---@field KEY_LEFTARROW integer
---@field KEY_RIGHTARROW integer
---@field KEY_UPARROW integer
---@field KEY_DOWNARROW integer
---@field KEY_PAGEUP integer
---@field KEY_PAGEDOWN integer
---@field KEY_HOME integer
---@field KEY_END integer
---@field KEY_INSERT integer
---@field KEY_DELETE integer
---@field KEY_BACKSPACE integer
---@field KEY_SPACE integer
---@field KEY_ENTER integer
---@field KEY_ESCAPE integer
---@field KEY_KEYPADENTER integer
---@field KEY_A integer
---@field KEY_C integer
---@field KEY_V integer
---@field KEY_X integer
---@field KEY_Y integer
---@field KEY_Z integer
---@
---@field TREENODE_SELECTED integer
---@field TREENODE_FRAMED integer
---@field TREENODE_ALLOW_ITEM_OVERLAP integer
---@field TREENODE_NO_TREE_PUSH_ON_OPEN integer
---@field TREENODE_NO_AUTO_OPEN_ON_LOG integer
---@field TREENODE_DEFAULT_OPEN integer
---@field TREENODE_OPEN_ON_DOUBLE_CLICK integer
---@field TREENODE_OPEN_ON_ARROW integer
---@field TREENODE_LEAF integer
---@field TREENODE_BULLET integer
---@field TREENODE_FRAME_PADDING integer
---@field TREENODE_SPAN_AVAILABLE_WIDTH integer
---@field TREENODE_SPAN_FULL_WIDTH integer
---@field TREENODE_NAV_LEFT_JUMPS_BACK_HERE integer
---@
---@field WINDOWFLAGS_NONE integer
---@field WINDOWFLAGS_NOTITLEBAR integer
---@field WINDOWFLAGS_NORESIZE integer
---@field WINDOWFLAGS_NOMOVE integer
---@field WINDOWFLAGS_NOSCROLLBAR integer
---@field WINDOWFLAGS_NOSCROLLWITHMOUSE integer
---@field WINDOWFLAGS_NOCOLLAPSE integer
---@field WINDOWFLAGS_ALWAYSAUTORESIZE integer
---@field WINDOWFLAGS_NOBACKGROUND integer
---@field WINDOWFLAGS_NOSAVEDSETTINGS integer
---@field WINDOWFLAGS_NOMOUSEINPUTS integer
---@field WINDOWFLAGS_MENUBAR integer
---@field WINDOWFLAGS_HORIZONTALSCROLLBAR integer
---@field WINDOWFLAGS_NOFOCUSONAPPEARING integer
---@field WINDOWFLAGS_NOBRINGTOFRONTONFOCUS integer
---@field WINDOWFLAGS_ALWAYSVERTICALSCROLLBAR integer
---@field WINDOWFLAGS_ALWAYSHORIZONTALSCROLLBAR integer
---@field WINDOWFLAGS_ALWAYSUSEWINDOWPADDING integer
---@field WINDOWFLAGS_NONAVINPUTS integer
---@field WINDOWFLAGS_NONAVFOCUS integer
---@field WINDOWFLAGS_UNSAVEDDOCUMENT integer
---@field WINDOWFLAGS_NONAV integer
---@field WINDOWFLAGS_NODECORATION integer
---@field WINDOWFLAGS_NOINPUTS integer
---@
---@field COND_NONE integer
---@field COND_ALWAYS integer
---@field COND_ONCE integer
---@field COND_FIRSTUSEEVER integer
---@field COND_APPEARING integer
---@
---@field FOCUSED_CHILD_WINDOWS integer
---@field FOCUSED_ROOT_WINDOW integer
---@field FOCUSED_ANY_WINDOW integer
---@field FOCUSED_ROOT_AND_CHILD_WINDOWS integer
---@
---@field POPUPFLAGS_NONE integer
---@field POPUPFLAGS_MOUSEBUTTONLEFT integer
---@field POPUPFLAGS_MOUSEBUTTONRIGHT integer
---@field POPUPFLAGS_MOUSEBUTTONMIDDLE integer
---@field POPUPFLAGS_MOUSEBUTTONMASK integer
---@field POPUPFLAGS_MOUSEBUTTONDEFAULT integer
---@field POPUPFLAGS_NOOPENOVEREXISTINGPOPUP integer
---@field POPUPFLAGS_NOOPENOVERITEMS integer
---@field POPUPFLAGS_ANYPOPUPID integer
---@field POPUPFLAGS_ANYPOPUPLEVEL integer
---@field POPUPFLAGS_ANYPOPUP integer
---@
---@field DROPFLAGS_NONE integer
---@field DROPFLAGS_SOURCENOPREVIEWTOOLTIP integer
---@field DROPFLAGS_SOURCENODISABLEHOVER integer
---@field DROPFLAGS_SOURCENOHOLDTOOPENOTHERS integer
---@field DROPFLAGS_SOURCEALLOWNULLID integer
---@field DROPFLAGS_SOURCEEXTERN integer
---@field DROPFLAGS_SOURCEAUTOEXPIREPAYLOAD integer
---@field DROPFLAGS_ACCEPTBEFOREDELIVERY integer
---@field DROPFLAGS_ACCEPTNODRAWDEFAULTRECT integer
---@field DROPFLAGS_ACCEPTNOPREVIEWTOOLTIP integer
---@field DROPFLAGS_ACCEPTPEEKONLY integer
---@
---@field TABLE_NONE integer
---@field TABLE_RESIZABLE integer
---@field TABLE_REORDERABLE integer
---@field TABLE_HIDEABLE integer
---@field TABLE_SORTABLE integer
---@field TABLE_NOSAVEDSETTINGS integer
---@field TABLE_CONTEXTMENUINBODY integer
---@field TABLE_ROWBG integer
---@field TABLE_BORDERSINNERH integer
---@field TABLE_BORDERSOUTERH integer
---@field TABLE_BORDERSINNERV integer
---@field TABLE_BORDERSOUTERV integer
---@field TABLE_BORDERSH integer
---@field TABLE_BORDERSV integer
---@field TABLE_BORDERSINNER integer
---@field TABLE_BORDERSOUTER integer
---@field TABLE_BORDERS integer
---@field TABLE_NOBORDERSINBODY integer
---@field TABLE_NOBORDERSINBODYUNTILRESIZE integer
---@field TABLE_SIZINGFIXEDFIT integer
---@field TABLE_SIZINGFIXEDSAME integer
---@field TABLE_SIZINGSTRETCHPROP integer
---@field TABLE_SIZINGSTRETCHSAME integer
---@field TABLE_NOHOSTEXTENDX integer
---@field TABLE_NOHOSTEXTENDY integer
---@field TABLE_NOKEEPCOLUMNSVISIBLE integer
---@field TABLE_PRECISEWIDTHS integer
---@field TABLE_NOCLIP integer
---@field TABLE_PADOUTERX integer
---@field TABLE_NOPADOUTERX integer
---@field TABLE_NOPADINNERX integer
---@field TABLE_SCROLLX integer
---@field TABLE_SCROLLY integer
---@field TABLE_SORTMULTI integer
---@field TABLE_SORTTRISTATE integer
---@
---@field TABLECOLUMN_NONE integer
---@field TABLECOLUMN_DEFAULTHIDE integer
---@field TABLECOLUMN_DEFAULTSORT integer
---@field TABLECOLUMN_WIDTHSTRETCH integer
---@field TABLECOLUMN_WIDTHFIXED integer
---@field TABLECOLUMN_NORESIZE integer
---@field TABLECOLUMN_NOREORDER integer
---@field TABLECOLUMN_NOHIDE integer
---@field TABLECOLUMN_NOCLIP integer
---@field TABLECOLUMN_NOSORT integer
---@field TABLECOLUMN_NOSORTASCENDING integer
---@field TABLECOLUMN_NOSORTDESCENDING integer
---@field TABLECOLUMN_NOHEADERWIDTH integer
---@field TABLECOLUMN_PREFERSORTASCENDING integer
---@field TABLECOLUMN_PREFERSORTDESCENDING integer
---@field TABLECOLUMN_INDENTENABLE integer
---@field TABLECOLUMN_INDENTDISABLE integer
---@
---@field TABITEM_UNSAVED_DOCUMENT integer
---@field TABITEM_SET_SELECTED integer
---@field TABITEM_NO_CLOSE_WITH_MIDDLE_MOUSE_BUTTON integer
---@field TABITEM_NO_PUSH_ID integer
---@field TABITEM_NO_TOOLTIP integer
---@field TABITEM_NO_REORDER integer
---@field TABITEM_LEADING integer
---@field TABITEM_TRAILING integer
---@
---@field COLOREDITFLAGS_NONE integer
---@field COLOREDITFLAGS_NOALPHA integer
---@field COLOREDITFLAGS_NOPICKER integer
---@field COLOREDITFLAGS_NOOPTIONS integer
---@field COLOREDITFLAGS_NOSMALLPREVIEW integer
---@field COLOREDITFLAGS_NOINPUTS integer
---@field COLOREDITFLAGS_NOTOOLTIP integer
---@field COLOREDITFLAGS_NOLABEL integer
---@field COLOREDITFLAGS_NOSIDEPREVIEW integer
---@field COLOREDITFLAGS_NODRAGDROP integer
---@field COLOREDITFLAGS_NOBORDER integer
---@field COLOREDITFLAGS_ALPHABAR integer
---@field COLOREDITFLAGS_ALPHAPREVIEW integer
---@field COLOREDITFLAGS_ALPHAPREVIEWHALF integer
---@field COLOREDITFLAGS_HDR integer
---@field COLOREDITFLAGS_DISPLAYRGB integer
---@field COLOREDITFLAGS_DISPLAYHSV integer
---@field COLOREDITFLAGS_DISPLAYHEX integer
---@field COLOREDITFLAGS_UINT8 integer
---@field COLOREDITFLAGS_FLOAT integer
---@field COLOREDITFLAGS_PICKERHUEBAR integer
---@field COLOREDITFLAGS_PICKERHUEWHEEL integer
---@field COLOREDITFLAGS_INPUTRGB integer
---@field COLOREDITFLAGS_INPUTHSV integer
---@
---@field INPUTFLAGS_NONE integer
---@field INPUTFLAGS_CHARSDECIMAL integer
---@field INPUTFLAGS_CHARSHEXADECIMAL integer
---@field INPUTFLAGS_CHARSUPPERCASE integer
---@field INPUTFLAGS_CHARSNOBLANK integer
---@field INPUTFLAGS_AUTOSELECTALL integer
---@field INPUTFLAGS_ENTERRETURNSTRUE integer
---@field INPUTFLAGS_CALLBACKCOMPLETION integer
---@field INPUTFLAGS_CALLBACKHISTORY integer
---@field INPUTFLAGS_CALLBACKALWAYS integer
---@field INPUTFLAGS_CALLBACKCHARFILTER integer
---@field INPUTFLAGS_ALLOWTABINPUT integer
---@field INPUTFLAGS_CTRLENTERFORNEWLINE integer
---@field INPUTFLAGS_NOHORIZONTALSCROLL integer
---@field INPUTFLAGS_ALWAYSOVERWRITE integer
---@field INPUTFLAGS_READONLY integer
---@field INPUTFLAGS_PASSWORD integer
---@field INPUTFLAGS_NOUNDOREDO integer
---@field INPUTFLAGS_CHARSSCIENTIFIC integer
---@field INPUTFLAGS_CALLBACKRESIZE integer
---@field INPUTFLAGS_CALLBACKEDIT integer
---@field INPUTFLAGS_ESCAPECLEARSALL integer
---@
---@field SELECTABLE_DONT_CLOSE_POPUPS integer
---@field SELECTABLE_SPAN_ALL_COLUMNS integer
---@field SELECTABLE_ALLOW_DOUBLE_CLICK integer
---@field SELECTABLE_DISABLED integer
---@field SELECTABLE_ALLOW_ITEM_OVERLAP integer
---@
---@field DIR_NONE integer
---@field DIR_LEFT integer
---@field DIR_RIGHT integer
---@field DIR_UP integer
---@field DIR_DOWN integer
---@
---@field ImGuiCol_Text integer
---@field ImGuiCol_TextDisabled integer
---@field ImGuiCol_WindowBg integer
---@field ImGuiCol_ChildBg integer
---@field ImGuiCol_PopupBg integer
---@field ImGuiCol_Border integer
---@field ImGuiCol_BorderShadow integer
---@field ImGuiCol_FrameBg integer
---@field ImGuiCol_FrameBgHovered integer
---@field ImGuiCol_FrameBgActive integer
---@field ImGuiCol_TitleBg integer
---@field ImGuiCol_TitleBgActive integer
---@field ImGuiCol_TitleBgCollapsed integer
---@field ImGuiCol_MenuBarBg integer
---@field ImGuiCol_ScrollbarBg integer
---@field ImGuiCol_ScrollbarGrab integer
---@field ImGuiCol_ScrollbarGrabHovered integer
---@field ImGuiCol_ScrollbarGrabActive integer
---@field ImGuiCol_CheckMark integer
---@field ImGuiCol_SliderGrab integer
---@field ImGuiCol_SliderGrabActive integer
---@field ImGuiCol_Button integer
---@field ImGuiCol_ButtonHovered integer
---@field ImGuiCol_ButtonActive integer
---@field ImGuiCol_Header integer
---@field ImGuiCol_HeaderHovered integer
---@field ImGuiCol_HeaderActive integer
---@field ImGuiCol_Separator integer
---@field ImGuiCol_SeparatorHovered integer
---@field ImGuiCol_SeparatorActive integer
---@field ImGuiCol_ResizeGrip integer
---@field ImGuiCol_ResizeGripHovered integer
---@field ImGuiCol_ResizeGripActive integer
---@field ImGuiCol_Tab integer
---@field ImGuiCol_TabHovered integer
---@field ImGuiCol_TabActive integer
---@field ImGuiCol_TabUnfocused integer
---@field ImGuiCol_TabUnfocusedActive integer
---@field ImGuiCol_PlotLines integer
---@field ImGuiCol_PlotLinesHovered integer
---@field ImGuiCol_PlotHistogram integer
---@field ImGuiCol_PlotHistogramHovered integer
---@field ImGuiCol_TableHeaderBg integer
---@field ImGuiCol_TableBorderStrong integer
---@field ImGuiCol_TableBorderLight integer
---@field ImGuiCol_TableRowBg integer
---@field ImGuiCol_TableRowBgAlt integer
---@field ImGuiCol_TextSelectedBg integer
---@field ImGuiCol_DragDropTarget integer
---@field ImGuiCol_NavHighlight integer
---@field ImGuiCol_NavWindowingHighlight integer
---@field ImGuiCol_NavWindowingDimBg integer
---@field ImGuiCol_ModalWindowDimBg integer
---@
---@field STYLEVAR_ALPHA integer
---@field STYLEVAR_DISABLEDALPHA integer
---@field STYLEVAR_WINDOWPADDING integer
---@field STYLEVAR_WINDOWROUNDING integer
---@field STYLEVAR_WINDOWBORDERSIZE integer
---@field STYLEVAR_WINDOWMINSIZE integer
---@field STYLEVAR_WINDOWTITLEALIGN integer
---@field STYLEVAR_CHILDROUNDING integer
---@field STYLEVAR_CHILDBORDERSIZE integer
---@field STYLEVAR_POPUPROUNDING integer
---@field STYLEVAR_POPUPBORDERSIZE integer
---@field STYLEVAR_FRAMEPADDING integer
---@field STYLEVAR_FRAMEROUNDING integer
---@field STYLEVAR_FRAMEBORDERSIZE integer
---@field STYLEVAR_ITEMSPACING integer
---@field STYLEVAR_ITEMINNERSPACING integer
---@field STYLEVAR_INDENTSPACING integer
---@field STYLEVAR_CELLPADDING integer
---@field STYLEVAR_SCROLLBARSIZE integer
---@field STYLEVAR_SCROLLBARROUNDING integer
---@field STYLEVAR_GRABMINSIZE integer
---@field STYLEVAR_GRABROUNDING integer
---@field STYLEVAR_TABROUNDING integer
---@field STYLEVAR_TABBORDERSIZE integer
---@field STYLEVAR_TABBARBORDERSIZE integer
---@field STYLEVAR_TABLEANGLEDHEADERSANGLE integer
---@field STYLEVAR_TABLEANGLEDHEADERSTEXTALIGN integer
---@field STYLEVAR_BUTTONTEXTALIGN integer
---@field STYLEVAR_SELECTABLETEXTALIGN integer
---@field STYLEVAR_SEPARATORTEXTBORDERSIZE integer
---@field STYLEVAR_SEPARATORTEXTALIGN integer
---@field STYLEVAR_SEPARATORTEXTPADDING integer
---@
---@field GLYPH_RANGES_DEFAULT integer
---@field GLYPH_RANGES_GREEK integer
---@field GLYPH_RANGES_KOREAN integer
---@field GLYPH_RANGES_JAPANESE integer
---@field GLYPH_RANGES_CHINESEFULL integer
---@field GLYPH_RANGES_CHINESESIMPLIFIEDCOMMON integer
---@field GLYPH_RANGES_CYRILLIC integer
---@field GLYPH_RANGES_THAI integer
---@field GLYPH_RANGES_VIETNAMESE integer
--- 本扩展自定义：CHINESEFULL + Korean + Cyrillic + Latin 扩展 A/B 的并集。
--- CHINESEFULL 单独用会缺谚文 / 西里尔 / Latin 扩展 A，16 语种界面须用这一档。
---@field GLYPH_RANGES_MULTILINGUAL integer
---@
imgui = {}

---@alias imgui.MOUSEBUTTON
---| `imgui.MOUSEBUTTON_LEFT`
---| `imgui.MOUSEBUTTON_RIGHT`
---| `imgui.MOUSEBUTTON_MIDDLE`

---@alias imgui.KEY
---| `imgui.KEY_TAB`
---| `imgui.KEY_LEFTARROW`
---| `imgui.KEY_RIGHTARROW`
---| `imgui.KEY_UPARROW`
---| `imgui.KEY_DOWNARROW`
---| `imgui.KEY_PAGEUP`
---| `imgui.KEY_PAGEDOWN`
---| `imgui.KEY_HOME`
---| `imgui.KEY_END`
---| `imgui.KEY_INSERT`
---| `imgui.KEY_DELETE`
---| `imgui.KEY_BACKSPACE`
---| `imgui.KEY_SPACE`
---| `imgui.KEY_ENTER`
---| `imgui.KEY_ESCAPE`
---| `imgui.KEY_KEYPADENTER`
---| `imgui.KEY_A`
---| `imgui.KEY_C`
---| `imgui.KEY_V`
---| `imgui.KEY_X`
---| `imgui.KEY_Y`
---| `imgui.KEY_Z`

---@alias imgui.TREENODE
---| `imgui.TREENODE_SELECTED`
---| `imgui.TREENODE_FRAMED`
---| `imgui.TREENODE_ALLOW_ITEM_OVERLAP`
---| `imgui.TREENODE_NO_TREE_PUSH_ON_OPEN`
---| `imgui.TREENODE_NO_AUTO_OPEN_ON_LOG`
---| `imgui.TREENODE_DEFAULT_OPEN`
---| `imgui.TREENODE_OPEN_ON_DOUBLE_CLICK`
---| `imgui.TREENODE_OPEN_ON_ARROW`
---| `imgui.TREENODE_LEAF`
---| `imgui.TREENODE_BULLET`
---| `imgui.TREENODE_FRAME_PADDING`
---| `imgui.TREENODE_SPAN_AVAILABLE_WIDTH`
---| `imgui.TREENODE_SPAN_FULL_WIDTH`
---| `imgui.TREENODE_NAV_LEFT_JUMPS_BACK_HERE`

---@alias imgui.WINDOWFLAGS
---| `imgui.WINDOWFLAGS_NONE`
---| `imgui.WINDOWFLAGS_NOTITLEBAR`
---| `imgui.WINDOWFLAGS_NORESIZE`
---| `imgui.WINDOWFLAGS_NOMOVE`
---| `imgui.WINDOWFLAGS_NOSCROLLBAR`
---| `imgui.WINDOWFLAGS_NOSCROLLWITHMOUSE`
---| `imgui.WINDOWFLAGS_NOCOLLAPSE`
---| `imgui.WINDOWFLAGS_ALWAYSAUTORESIZE`
---| `imgui.WINDOWFLAGS_NOBACKGROUND`
---| `imgui.WINDOWFLAGS_NOSAVEDSETTINGS`
---| `imgui.WINDOWFLAGS_NOMOUSEINPUTS`
---| `imgui.WINDOWFLAGS_MENUBAR`
---| `imgui.WINDOWFLAGS_HORIZONTALSCROLLBAR`
---| `imgui.WINDOWFLAGS_NOFOCUSONAPPEARING`
---| `imgui.WINDOWFLAGS_NOBRINGTOFRONTONFOCUS`
---| `imgui.WINDOWFLAGS_ALWAYSVERTICALSCROLLBAR`
---| `imgui.WINDOWFLAGS_ALWAYSHORIZONTALSCROLLBAR`
---| `imgui.WINDOWFLAGS_ALWAYSUSEWINDOWPADDING`
---| `imgui.WINDOWFLAGS_NONAVINPUTS`
---| `imgui.WINDOWFLAGS_NONAVFOCUS`
---| `imgui.WINDOWFLAGS_UNSAVEDDOCUMENT`
---| `imgui.WINDOWFLAGS_NONAV`
---| `imgui.WINDOWFLAGS_NODECORATION`
---| `imgui.WINDOWFLAGS_NOINPUTS`

---@alias imgui.COND
---| `imgui.COND_NONE`
---| `imgui.COND_ALWAYS`
---| `imgui.COND_ONCE`
---| `imgui.COND_FIRSTUSEEVER`
---| `imgui.COND_APPEARING`

---@alias imgui.FOCUSED
---| `imgui.FOCUSED_CHILD_WINDOWS`
---| `imgui.FOCUSED_ROOT_WINDOW`
---| `imgui.FOCUSED_ANY_WINDOW`
---| `imgui.FOCUSED_ROOT_AND_CHILD_WINDOWS`

---@alias imgui.POPUPFLAGS
---| `imgui.POPUPFLAGS_NONE`
---| `imgui.POPUPFLAGS_MOUSEBUTTONLEFT`
---| `imgui.POPUPFLAGS_MOUSEBUTTONRIGHT`
---| `imgui.POPUPFLAGS_MOUSEBUTTONMIDDLE`
---| `imgui.POPUPFLAGS_MOUSEBUTTONMASK`
---| `imgui.POPUPFLAGS_MOUSEBUTTONDEFAULT`
---| `imgui.POPUPFLAGS_NOOPENOVEREXISTINGPOPUP`
---| `imgui.POPUPFLAGS_NOOPENOVERITEMS`
---| `imgui.POPUPFLAGS_ANYPOPUPID`
---| `imgui.POPUPFLAGS_ANYPOPUPLEVEL`
---| `imgui.POPUPFLAGS_ANYPOPUP`

---@alias imgui.DROPFLAGS
---| `imgui.DROPFLAGS_NONE`
---| `imgui.DROPFLAGS_SOURCENOPREVIEWTOOLTIP`
---| `imgui.DROPFLAGS_SOURCENODISABLEHOVER`
---| `imgui.DROPFLAGS_SOURCENOHOLDTOOPENOTHERS`
---| `imgui.DROPFLAGS_SOURCEALLOWNULLID`
---| `imgui.DROPFLAGS_SOURCEEXTERN`
---| `imgui.DROPFLAGS_SOURCEAUTOEXPIREPAYLOAD`
---| `imgui.DROPFLAGS_ACCEPTBEFOREDELIVERY`
---| `imgui.DROPFLAGS_ACCEPTNODRAWDEFAULTRECT`
---| `imgui.DROPFLAGS_ACCEPTNOPREVIEWTOOLTIP`
---| `imgui.DROPFLAGS_ACCEPTPEEKONLY`

---@alias imgui.TABLE
---| `imgui.TABLE_NONE`
---| `imgui.TABLE_RESIZABLE`
---| `imgui.TABLE_REORDERABLE`
---| `imgui.TABLE_HIDEABLE`
---| `imgui.TABLE_SORTABLE`
---| `imgui.TABLE_NOSAVEDSETTINGS`
---| `imgui.TABLE_CONTEXTMENUINBODY`
---| `imgui.TABLE_ROWBG`
---| `imgui.TABLE_BORDERSINNERH`
---| `imgui.TABLE_BORDERSOUTERH`
---| `imgui.TABLE_BORDERSINNERV`
---| `imgui.TABLE_BORDERSOUTERV`
---| `imgui.TABLE_BORDERSH`
---| `imgui.TABLE_BORDERSV`
---| `imgui.TABLE_BORDERSINNER`
---| `imgui.TABLE_BORDERSOUTER`
---| `imgui.TABLE_BORDERS`
---| `imgui.TABLE_NOBORDERSINBODY`
---| `imgui.TABLE_NOBORDERSINBODYUNTILRESIZE`
---| `imgui.TABLE_SIZINGFIXEDFIT`
---| `imgui.TABLE_SIZINGFIXEDSAME`
---| `imgui.TABLE_SIZINGSTRETCHPROP`
---| `imgui.TABLE_SIZINGSTRETCHSAME`
---| `imgui.TABLE_NOHOSTEXTENDX`
---| `imgui.TABLE_NOHOSTEXTENDY`
---| `imgui.TABLE_NOKEEPCOLUMNSVISIBLE`
---| `imgui.TABLE_PRECISEWIDTHS`
---| `imgui.TABLE_NOCLIP`
---| `imgui.TABLE_PADOUTERX`
---| `imgui.TABLE_NOPADOUTERX`
---| `imgui.TABLE_NOPADINNERX`
---| `imgui.TABLE_SCROLLX`
---| `imgui.TABLE_SCROLLY`
---| `imgui.TABLE_SORTMULTI`
---| `imgui.TABLE_SORTTRISTATE`

---@alias imgui.TABLECOLUMN
---| `imgui.TABLECOLUMN_NONE`
---| `imgui.TABLECOLUMN_DEFAULTHIDE`
---| `imgui.TABLECOLUMN_DEFAULTSORT`
---| `imgui.TABLECOLUMN_WIDTHSTRETCH`
---| `imgui.TABLECOLUMN_WIDTHFIXED`
---| `imgui.TABLECOLUMN_NORESIZE`
---| `imgui.TABLECOLUMN_NOREORDER`
---| `imgui.TABLECOLUMN_NOHIDE`
---| `imgui.TABLECOLUMN_NOCLIP`
---| `imgui.TABLECOLUMN_NOSORT`
---| `imgui.TABLECOLUMN_NOSORTASCENDING`
---| `imgui.TABLECOLUMN_NOSORTDESCENDING`
---| `imgui.TABLECOLUMN_NOHEADERWIDTH`
---| `imgui.TABLECOLUMN_PREFERSORTASCENDING`
---| `imgui.TABLECOLUMN_PREFERSORTDESCENDING`
---| `imgui.TABLECOLUMN_INDENTENABLE`
---| `imgui.TABLECOLUMN_INDENTDISABLE`

---@alias imgui.TABITEM
---| `imgui.TABITEM_UNSAVED_DOCUMENT`
---| `imgui.TABITEM_SET_SELECTED`
---| `imgui.TABITEM_NO_CLOSE_WITH_MIDDLE_MOUSE_BUTTON`
---| `imgui.TABITEM_NO_PUSH_ID`
---| `imgui.TABITEM_NO_TOOLTIP`
---| `imgui.TABITEM_NO_REORDER`
---| `imgui.TABITEM_LEADING`
---| `imgui.TABITEM_TRAILING`

---@alias imgui.COLOREDITFLAGS
---| `imgui.COLOREDITFLAGS_NONE`
---| `imgui.COLOREDITFLAGS_NOALPHA`
---| `imgui.COLOREDITFLAGS_NOPICKER`
---| `imgui.COLOREDITFLAGS_NOOPTIONS`
---| `imgui.COLOREDITFLAGS_NOSMALLPREVIEW`
---| `imgui.COLOREDITFLAGS_NOINPUTS`
---| `imgui.COLOREDITFLAGS_NOTOOLTIP`
---| `imgui.COLOREDITFLAGS_NOLABEL`
---| `imgui.COLOREDITFLAGS_NOSIDEPREVIEW`
---| `imgui.COLOREDITFLAGS_NODRAGDROP`
---| `imgui.COLOREDITFLAGS_NOBORDER`
---| `imgui.COLOREDITFLAGS_ALPHABAR`
---| `imgui.COLOREDITFLAGS_ALPHAPREVIEW`
---| `imgui.COLOREDITFLAGS_ALPHAPREVIEWHALF`
---| `imgui.COLOREDITFLAGS_HDR`
---| `imgui.COLOREDITFLAGS_DISPLAYRGB`
---| `imgui.COLOREDITFLAGS_DISPLAYHSV`
---| `imgui.COLOREDITFLAGS_DISPLAYHEX`
---| `imgui.COLOREDITFLAGS_UINT8`
---| `imgui.COLOREDITFLAGS_FLOAT`
---| `imgui.COLOREDITFLAGS_PICKERHUEBAR`
---| `imgui.COLOREDITFLAGS_PICKERHUEWHEEL`
---| `imgui.COLOREDITFLAGS_INPUTRGB`
---| `imgui.COLOREDITFLAGS_INPUTHSV`

---@alias imgui.INPUTFLAGS
---| `imgui.INPUTFLAGS_NONE`
---| `imgui.INPUTFLAGS_CHARSDECIMAL`
---| `imgui.INPUTFLAGS_CHARSHEXADECIMAL`
---| `imgui.INPUTFLAGS_CHARSUPPERCASE`
---| `imgui.INPUTFLAGS_CHARSNOBLANK`
---| `imgui.INPUTFLAGS_AUTOSELECTALL`
---| `imgui.INPUTFLAGS_ENTERRETURNSTRUE`
---| `imgui.INPUTFLAGS_CALLBACKCOMPLETION`
---| `imgui.INPUTFLAGS_CALLBACKHISTORY`
---| `imgui.INPUTFLAGS_CALLBACKALWAYS`
---| `imgui.INPUTFLAGS_CALLBACKCHARFILTER`
---| `imgui.INPUTFLAGS_ALLOWTABINPUT`
---| `imgui.INPUTFLAGS_CTRLENTERFORNEWLINE`
---| `imgui.INPUTFLAGS_NOHORIZONTALSCROLL`
---| `imgui.INPUTFLAGS_ALWAYSOVERWRITE`
---| `imgui.INPUTFLAGS_READONLY`
---| `imgui.INPUTFLAGS_PASSWORD`
---| `imgui.INPUTFLAGS_NOUNDOREDO`
---| `imgui.INPUTFLAGS_CHARSSCIENTIFIC`
---| `imgui.INPUTFLAGS_CALLBACKRESIZE`
---| `imgui.INPUTFLAGS_CALLBACKEDIT`
---| `imgui.INPUTFLAGS_ESCAPECLEARSALL`

---@alias imgui.SELECTABLE
---| `imgui.SELECTABLE_DONT_CLOSE_POPUPS`
---| `imgui.SELECTABLE_SPAN_ALL_COLUMNS`
---| `imgui.SELECTABLE_ALLOW_DOUBLE_CLICK`
---| `imgui.SELECTABLE_DISABLED`
---| `imgui.SELECTABLE_ALLOW_ITEM_OVERLAP`

---@alias imgui.DIR
---| `imgui.DIR_NONE`
---| `imgui.DIR_LEFT`
---| `imgui.DIR_RIGHT`
---| `imgui.DIR_UP`
---| `imgui.DIR_DOWN`

---@alias imgui.ImGuiCol
---| `imgui.ImGuiCol_Text`
---| `imgui.ImGuiCol_TextDisabled`
---| `imgui.ImGuiCol_WindowBg`
---| `imgui.ImGuiCol_ChildBg`
---| `imgui.ImGuiCol_PopupBg`
---| `imgui.ImGuiCol_Border`
---| `imgui.ImGuiCol_BorderShadow`
---| `imgui.ImGuiCol_FrameBg`
---| `imgui.ImGuiCol_FrameBgHovered`
---| `imgui.ImGuiCol_FrameBgActive`
---| `imgui.ImGuiCol_TitleBg`
---| `imgui.ImGuiCol_TitleBgActive`
---| `imgui.ImGuiCol_TitleBgCollapsed`
---| `imgui.ImGuiCol_MenuBarBg`
---| `imgui.ImGuiCol_ScrollbarBg`
---| `imgui.ImGuiCol_ScrollbarGrab`
---| `imgui.ImGuiCol_ScrollbarGrabHovered`
---| `imgui.ImGuiCol_ScrollbarGrabActive`
---| `imgui.ImGuiCol_CheckMark`
---| `imgui.ImGuiCol_SliderGrab`
---| `imgui.ImGuiCol_SliderGrabActive`
---| `imgui.ImGuiCol_Button`
---| `imgui.ImGuiCol_ButtonHovered`
---| `imgui.ImGuiCol_ButtonActive`
---| `imgui.ImGuiCol_Header`
---| `imgui.ImGuiCol_HeaderHovered`
---| `imgui.ImGuiCol_HeaderActive`
---| `imgui.ImGuiCol_Separator`
---| `imgui.ImGuiCol_SeparatorHovered`
---| `imgui.ImGuiCol_SeparatorActive`
---| `imgui.ImGuiCol_ResizeGrip`
---| `imgui.ImGuiCol_ResizeGripHovered`
---| `imgui.ImGuiCol_ResizeGripActive`
---| `imgui.ImGuiCol_Tab`
---| `imgui.ImGuiCol_TabHovered`
---| `imgui.ImGuiCol_TabActive`
---| `imgui.ImGuiCol_TabUnfocused`
---| `imgui.ImGuiCol_TabUnfocusedActive`
---| `imgui.ImGuiCol_PlotLines`
---| `imgui.ImGuiCol_PlotLinesHovered`
---| `imgui.ImGuiCol_PlotHistogram`
---| `imgui.ImGuiCol_PlotHistogramHovered`
---| `imgui.ImGuiCol_TableHeaderBg`
---| `imgui.ImGuiCol_TableBorderStrong`
---| `imgui.ImGuiCol_TableBorderLight`
---| `imgui.ImGuiCol_TableRowBg`
---| `imgui.ImGuiCol_TableRowBgAlt`
---| `imgui.ImGuiCol_TextSelectedBg`
---| `imgui.ImGuiCol_DragDropTarget`
---| `imgui.ImGuiCol_NavHighlight`
---| `imgui.ImGuiCol_NavWindowingHighlight`
---| `imgui.ImGuiCol_NavWindowingDimBg`
---| `imgui.ImGuiCol_ModalWindowDimBg`

---@alias imgui.STYLEVAR
---| `imgui.STYLEVAR_ALPHA`
---| `imgui.STYLEVAR_DISABLEDALPHA`
---| `imgui.STYLEVAR_WINDOWPADDING`
---| `imgui.STYLEVAR_WINDOWROUNDING`
---| `imgui.STYLEVAR_WINDOWBORDERSIZE`
---| `imgui.STYLEVAR_WINDOWMINSIZE`
---| `imgui.STYLEVAR_WINDOWTITLEALIGN`
---| `imgui.STYLEVAR_CHILDROUNDING`
---| `imgui.STYLEVAR_CHILDBORDERSIZE`
---| `imgui.STYLEVAR_POPUPROUNDING`
---| `imgui.STYLEVAR_POPUPBORDERSIZE`
---| `imgui.STYLEVAR_FRAMEPADDING`
---| `imgui.STYLEVAR_FRAMEROUNDING`
---| `imgui.STYLEVAR_FRAMEBORDERSIZE`
---| `imgui.STYLEVAR_ITEMSPACING`
---| `imgui.STYLEVAR_ITEMINNERSPACING`
---| `imgui.STYLEVAR_INDENTSPACING`
---| `imgui.STYLEVAR_CELLPADDING`
---| `imgui.STYLEVAR_SCROLLBARSIZE`
---| `imgui.STYLEVAR_SCROLLBARROUNDING`
---| `imgui.STYLEVAR_GRABMINSIZE`
---| `imgui.STYLEVAR_GRABROUNDING`
---| `imgui.STYLEVAR_TABROUNDING`
---| `imgui.STYLEVAR_TABBORDERSIZE`
---| `imgui.STYLEVAR_TABBARBORDERSIZE`
---| `imgui.STYLEVAR_TABLEANGLEDHEADERSANGLE`
---| `imgui.STYLEVAR_TABLEANGLEDHEADERSTEXTALIGN`
---| `imgui.STYLEVAR_BUTTONTEXTALIGN`
---| `imgui.STYLEVAR_SELECTABLETEXTALIGN`
---| `imgui.STYLEVAR_SEPARATORTEXTBORDERSIZE`
---| `imgui.STYLEVAR_SEPARATORTEXTALIGN`
---| `imgui.STYLEVAR_SEPARATORTEXTPADDING`

---@alias imgui.GLYPH_RANGES
---| `imgui.GLYPH_RANGES_DEFAULT`
---| `imgui.GLYPH_RANGES_GREEK`
---| `imgui.GLYPH_RANGES_KOREAN`
---| `imgui.GLYPH_RANGES_JAPANESE`
---| `imgui.GLYPH_RANGES_CHINESEFULL`
---| `imgui.GLYPH_RANGES_CHINESESIMPLIFIEDCOMMON`
---| `imgui.GLYPH_RANGES_CYRILLIC`
---| `imgui.GLYPH_RANGES_THAI`
---| `imgui.GLYPH_RANGES_VIETNAMESE`
---| `imgui.GLYPH_RANGES_MULTILINGUAL`

---
--- IMAGES

---@param data string
---@param length number
---@return string decoded_data
function imgui.image_b64_decode(data, length) end

---@param filename string
---@param data string
---@param data_length integer
---@return integer texture_id
---@return integer width
---@return integer height
function imgui.image_load_data(filename, data, data_length) end

---@param filename string
---@return integer texture_id
---@return integer width
---@return integer height
function imgui.image_load(filename) end

---@param texture_id integer
---@return integer texture_id_result
---@return integer width
---@return integer height
function imgui.image_get(texture_id) end

---@param texture_id integer
---@param width number
---@param height number
---@param uv0x? number
---@param uv0y? number
---@param uv1x? number
---@param uv1y? number
function imgui.image_add(texture_id, width, height, uv0x, uv0y, uv1x, uv1y) end

---@param texture_id integer
function imgui.image_free(texture_id) end

---
--- PRIMITIVES

---@param x1 number
---@param y1 number
---@param x2 number
---@param y2 number
---@param r number
---@param g number
---@param b number
---@param a number
---@param thickness? number
function imgui.add_line(x1, y1, x2, y2, r, g, b, a, thickness) end

---@param x1 number
---@param y1 number
---@param x2 number
---@param y2 number
---@param r number
---@param g number
---@param b number
---@param a number
---@param thickness? number
function imgui.add_rect(x1, y1, x2, y2, r, g, b, a, thickness) end

---
--- FRAMES

---@param w number
---@param h number
function imgui.set_display_size(w, h) end

---
--- INPUT

---@param x number
---@param y number
---@param down1 number
---@param down2 number
---@param down3 number
---@param wheel number
function imgui.set_mouse_input(x, y, down1, down2, down3, wheel) end

---@param x number
---@param y number
function imgui.set_mouse_pos(x, y) end

---@param index number|imgui.MOUSEBUTTON
---@param down number
function imgui.set_mouse_button(index, down) end

---@param wheel number
function imgui.set_mouse_wheel(wheel) end

---@param key integer|imgui.KEY
---@param down boolean
function imgui.set_key_down(key, down) end

---@param button integer|imgui.MOUSEBUTTON
---@return boolean clicked
function imgui.is_mouse_double_clicked(button) end

---@param button integer|imgui.MOUSEBUTTON
---@return boolean clicked
function imgui.is_mouse_clicked(button) end

---@return boolean active
function imgui.is_item_active() end

---@return boolean focused
function imgui.is_item_focused() end

---@param button integer|imgui.MOUSEBUTTON
---@return boolean clicked
function imgui.is_item_clicked(button) end

---@param button integer|imgui.MOUSEBUTTON
---@return boolean double_clicked
function imgui.is_item_double_clicked(button) end

---@return boolean hovered
function imgui.is_item_hovered() end

---@return number x
---@return number y
function imgui.get_item_rect_max() end

---@param offset integer
function imgui.set_keyboard_focus_here(offset) end

function imgui.set_item_default_focus() end

---
--- KEY MODIFIERS

---@param down boolean
function imgui.set_key_modifier_ctrl(down) end

---@param down boolean
function imgui.set_key_modifier_shift(down) end

---@param down boolean
function imgui.set_key_modifier_alt(down) end

---@param down boolean
function imgui.set_key_modifier_super(down) end

---
--- TEXT INPUT

---@param character string
function imgui.add_input_character(character) end

---@param characters string
function imgui.add_input_characters(characters) end

---
--- TEXT INPUT

---@param character string
function imgui.add_input_character(character) end

---@param characters string
function imgui.add_input_characters(characters) end


---
--- TREE

---@param label string
---@param flags? integer|imgui.TREENODE
---@return boolean open
function imgui.tree_node(label, flags) end

---@param label string
---@param open boolean
---@param flags? integer|imgui.TREENODE
---@return boolean open
function imgui.tree_node_ex(label, open, flags) end

function imgui.tree_pop() end

---@param label string
---@param visible? boolean
---@param flags? integer|imgui.TREENODE
---@return boolean open
function imgui.collapsing_header(label, visible, flags) end

---
--- PUSH/POP ID

---@param id string
function imgui.push_id(id) end

function imgui.pop_id() end

---
--- WINDOW

---@param title string
---@param is_open? boolean
---@param flags? integer|imgui.WINDOWFLAGS
---@return boolean result
---@return boolean is_open
function imgui.begin_window(title, is_open, flags) end

function imgui.end_window() end

---@param width number
---@param height number
---@param cond? integer
function imgui.set_next_window_size(width, height, cond) end

---声明窗口内容尺寸（Begin 前调用）：显式滚动判定基准，0 = 该轴不约束。
---窗口高 < 内容高 + WindowPadding×2 时自动出现竖向滚动条。
---@param width number
---@param height number
function imgui.set_next_window_content_size(width, height) end

---设置窗口初始滚动量（Begin 前调用；用于打开面板时归零滚动位置）。
---@param x number
---@param y number
function imgui.set_next_window_scroll(x, y) end

---@return number y 当前竖向滚动量
--- 空白区拖拽滚动：在当前窗口 Begin 之后每帧调用。按住窗口体空白处拖动 = 竖向
--- 平移滚动（内容未超出窗口时无效果）；标题栏按下去移动窗口、滚动条按下走原生
--- 拖拽、控件上按下走控件自身交互，均不接管。按下瞬间的命中判定只在按压开始
--- 生效，拖拽中途越界不中断。仅改造版扩展有此 API。
function imgui.drag_scroll_window() end

--- @return number 当前竖向滚动量
function imgui.get_scroll_y() end

---@return number max_y 最大竖向滚动量（0 = 内容未超出窗口）
function imgui.get_scroll_max_y() end

---@param x number
---@param y number
---@param cond? integer|imgui.COND
---@param pivot_x? number
---@param pivot_y? number
function imgui.set_next_window_pos(x, y, cond, pivot_x, pivot_y) end

---@param collapsed boolean
---@param cond? integer|imgui.COND
function imgui.set_next_window_collapsed(collapsed, cond) end

---@return number width
---@return number height
function imgui.get_window_size() end

---@return number x
---@return number y
function imgui.get_window_pos() end

---@param flags? integer|imgui.FOCUSED
---@return boolean focused
function imgui.is_window_focused(flags) end

---@return boolean result
function imgui.is_window_hovered() end

---@return boolean result
function imgui.is_window_appearing() end

---@return number x
---@return number y
function imgui.get_content_region_avail() end

---@return number x
---@return number y
function imgui.get_window_content_region_max() end

---
--- CHILD WINDOW

---@param title string
---@param width number
---@param height number
---@param border? integer
---@param flags? integer|imgui.WINDOWFLAGS
---@return boolean result
function imgui.begin_child(title, width, height, border, flags) end

function imgui.end_child() end

---
--- POPUP FLAGS

---@param id? string
---@return boolean result
function imgui.begin_popup_context_item(id) end

---@param id string
---@param flags? integer|imgui.POPUPFLAGS
---@return boolean result
function imgui.begin_popup(id, flags) end

---@param id string
---@param flags? integer|imgui.POPUPFLAGS
---@return boolean result
function imgui.begin_popup_modal(id, flags) end

---@param id string
---@param flags? integer|imgui.POPUPFLAGS
function imgui.open_popup(id, flags) end

function imgui.close_current_popup() end

function imgui.end_popup() end

---
--- DRAG AND DROP

---@param flags? integer|imgui.DROPFLAGS
---@return boolean result
function imgui.begin_dragdrop_source(flags) end

function imgui.end_dragdrop_source() end

---@return boolean result
function imgui.begin_dragdrop_target() end

function imgui.end_dragdrop_target() end

---@param type string
---@param payload string
---@return boolean result
function imgui.set_dragdrop_payload(type, payload) end

---@param type string
---@param flags? integer|imgui.DROPFLAGS
---@return string? payload
function imgui.accept_dragdrop_payload(type, flags) end

---
--- COMBO

---@param label string
---@param preview string
---@return boolean result
function imgui.begin_combo(label, preview) end

function imgui.end_combo() end

---@param label string
---@param current integer
---@param items string[]
---@return boolean result
---@return integer current
function imgui.combo(label, current, items) end

---
--- TABLE
---

---@param id string
---@param columns integer
---@param flags? integer|imgui.TABLE
---@param outer_width? number
---@param outer_height? number
---@return boolean result
function imgui.begin_table(id, columns, flags, outer_width, outer_height) end

function imgui.end_table() end

function imgui.table_headers_row() end

---@param label string
---@param flags? integer|imgui.TABLECOLUMN
---@param weight? number
function imgui.table_setup_column(label, flags, weight) end

---@param column integer
function imgui.table_set_column_index(column) end

function imgui.table_next_column() end

function imgui.table_next_row() end

---@param freeze_cols integer
---@param freeze_rows integer
function imgui.table_setup_scroll_freeze(freeze_cols, freeze_rows) end

---
--- TOOLTIP

function imgui.begin_tooltip() end

function imgui.end_tooltip() end

---
--- TAB BAR

---@param id string
---@return boolean result
function imgui.begin_tab_bar(id) end

function imgui.end_tab_bar() end

---@param label string
---@param open? boolean
---@param flags? integer|imgui.TABITEM
---@return boolean result
---@return boolean open
function imgui.begin_tab_item(label, open, flags) end

function imgui.end_tab_item() end

---
--- WIDGETS

---@param text string
---@param wrapped? integer
function imgui.text(text, wrapped) end

---@param text string
---@param font_size number
---@param fontid? integer
---@return number width
---@return number height
function imgui.text_getsize(text, font_size, fontid) end

---@param text string
---@param r number
---@param g number
---@param b number
---@param a number
function imgui.text_colored(text, r, g, b, a) end

---@param label string
---@param text string
---@param flags? integer|imgui.INPUTFLAGS
---@return boolean changed
---@return string? value
function imgui.input_text(label, text, flags) end

---@param label string
---@param value integer
---@return boolean changed
---@return integer? value
function imgui.input_int(label, value) end

---@param label string
---@param x number
---@param y number
---@return boolean changed
---@return number? x
---@return number? y
function imgui.input_int2(label, x, y) end

---@param label string
---@param value number
---@param step? number
---@param step_fast? number
---@param precision? integer
---@return boolean changed
---@return number? value
function imgui.input_float(label, value, step, step_fast, precision) end

---@param label string
---@param value number
---@param step? number
---@param step_fast? number
---@param precision? integer
---@return boolean changed
---@return number? value
function imgui.input_double(label, value, step, step_fast, precision) end

---@param label string
---@param x integer
---@param y integer
---@param z integer
---@param w integer
---@return boolean changed
---@return integer? x
---@return integer? y
---@return integer? z
---@return integer? w
function imgui.input_int4(label, x, y, z, w) end

---@param label string
---@param x number
---@param y number
---@param z number
---@return boolean changed
---@return number? x
---@return number? y
---@return number? z
function imgui.input_float3(label, x, y, z) end

---@param label string
---@param x number
---@param y number
---@param z number
---@param w number
---@return boolean changed
---@return number? x
---@return number? y
---@return number? z
---@return number? w
function imgui.input_float4(label, x, y, z, w) end

---@param label string
---@param value number
---@param speed number
---@param min number
---@param max number
---@param precision? integer
---@return boolean changed
---@return number? value
function imgui.drag_float(label, value, speed, min, max, precision) end

---@param label string
---@param value number
---@param min number
---@param max number
---@param precision? integer|string 数字 = 值文本小数位数（%.Nf）；字符串 = 直接作为 printf format（传 "" 隐藏值文本）
---@return boolean changed
---@return number? value
function imgui.slider_float(label, value, min, max, precision) end

---@param label string
---@param x number
---@param y number
---@param z number
---@param min number
---@param max number
---@param precision? integer
---@return boolean changed
---@return number? x
---@return number? y
---@return number? z
function imgui.slider_float3(label, x, y, z, min, max, precision) end

---@param label string
---@param x number
---@param y number
---@param z number
---@param w number
---@param min number
---@param max number
---@param precision? integer
---@return boolean changed
---@return number? x
---@return number? y
---@return number? z
---@return number? w
function imgui.slider_float4(label, x, y, z, w, min, max, precision) end

---@param label string
---@param color vector4
---@param flags? integer|imgui.COLOREDITFLAGS
function imgui.color_edit4(label, color, flags) end

---@param text string
---@param selected boolean?
---@param flags? integer|imgui.SELECTABLE
---@return boolean selected
---@return boolean pushed
function imgui.selectable(text, selected, flags) end

---@param text string
---@param width? integer
---@param height? integer
---@return boolean pushed
function imgui.button(text, width, height) end

---@param text string
---@return boolean pushed
function imgui.small_button(text) end

---@param texture_id number
---@param width? integer
---@param height? integer
---@return boolean pushed
function imgui.button_image(texture_id, width, height) end

---@param label string
---@param direction integer|imgui.DIR
---@return boolean pushed
function imgui.button_arrow(label, direction) end

---@param text string
---@param checked boolean?
---@return boolean changed
---@return boolean checked
function imgui.checkbox(text, checked) end

---@param text string
---@param checked boolean?
---@return boolean changed
---@return boolean checked
function imgui.radio_button(text, checked) end

---@return boolean result
function imgui.begin_menu_bar() end

function imgui.end_menu_bar() end

---@return boolean result
function imgui.begin_main_menu_bar() end

function imgui.end_main_menu_bar() end

---@param label string
---@param enabled? boolean
---@return boolean result
function imgui.begin_menu(label, enabled) end

function imgui.end_menu() end

---@param label string
---@param shortcut? string
---@param selected? boolean
---@param enabled? boolean
---@return boolean result
---@return boolean selected
function imgui.menu_item(label, shortcut, selected, enabled) end

---
--- LAYOUT

---@param offset? number
function imgui.same_line(offset) end

function imgui.new_line() end

function imgui.bullet() end

function imgui.indent() end

function imgui.unindent() end

function imgui.spacing() end

function imgui.separator() end

---@return number x
---@return number y
function imgui.get_cursor_screen_pos() end

---@return number x
---@return number y
function imgui.get_cursor_pos() end

---
--- IMGUI PLOT

---@param label string
---@param offset integer
---@param width integer
---@param height integer
---@param values number[]
function imgui.plot_lines(label, offset, width, height, values) end

---@param label string
---@param offset integer
---@param width integer
---@param height integer
---@param values number[]
function imgui.plot_histogram(label, offset, width, height, values) end


---
--- IMGUI DEMO

function imgui.demo() end

---
--- STYLE

---@return table style
function imgui.get_style() end

---@param style table
function imgui.set_style(style) end

---@param index integer|imgui.ImGuiCol
---@param r number
---@param g number
---@param b number
---@param a number
function imgui.set_style_color(index, r, g, b, a) end

---@param index integer|imgui.ImGuiCol
---@param r number
---@param g number
---@param b number
---@param a number
function imgui.push_style_color(index, r, g, b, a) end

---@param count? integer
function imgui.pop_style_color(count) end

---@param style_var integer|imgui.STYLEVAR
---@param value number|vector3
function imgui.push_style_var(style_var, value) end

---@param count? integer
function imgui.pop_style_var(count) end

---@param scale number
function imgui.set_window_font_scale(scale) end

---@param scale number
function imgui.set_global_font_scale(scale) end

---@param scale number
function imgui.scale_all_sizes(scale) end

---@param x integer
---@param y integer
function imgui.set_cursor_pos(x, y) end

---
--- ITEM WIDTH

---@param width number
function imgui.push_item_width(width) end

function imgui.pop_item_width() end

---@param width number
function imgui.set_next_item_width(width) end

---@return number width
function imgui.calc_item_width() end

---
--- NAVIGATION

---@param center_y_ratio number
function imgui.set_scroll_here_y(center_y_ratio) end

---
--- FONT
---

---@param filename string
---@param size number
---@param glyph_ranges? integer|imgui.GLYPH_RANGES
---@return integer? font_id
function imgui.font_add_ttf_file(filename, size, glyph_ranges) end

--- 注册字体。字号由 font_pixels 决定（写入 ImFont::LegacySize，运行时再乘 font_scale）；
--- font_size 形参保留只为对齐签名，扩展不读它。
---@param data string
---@param data_size integer
---@param font_size number
---@param font_pixels integer
---@param glyph_ranges? integer|imgui.GLYPH_RANGES
---@return integer? font_id
function imgui.font_add_ttf_data(data, data_size, font_size, font_pixels, glyph_ranges) end

---@param font_id integer
function imgui.font_push(font_id) end

function imgui.font_pop() end

---@param font_id integer
---@param scale number
---@return number old_scale
function imgui.font_scale(font_id, scale) end

--- 当前引擎的图形后端家族（dmGraphics::GetInstalledAdapterFamily）。
--- 2=OpenGL 3=OpenGL ES 4=Vulkan 8=Metal；非 2/3 时扩展的 GL 渲染路径不可用，
--- Lua 侧应跳过 imgui 绘制。
---@return integer adapter_family
function imgui.get_adapter_family() end

--- 物理屏幕密度（dpi，进程内查询一次后缓存）。
--- Android 走 JNI 读 DisplayMetrics.densityDpi；其他平台返回 96（Lua 侧按 1x 处理）。
--- 供 theme.compute_scale 做高密度屏补偿。
---@return number dpi
function imgui.get_screen_density() end

--- 把当前 ImGui 帧画进扩展自建的 GL FBO（render script 里调用），
--- POST_RENDER 时扩展用内置 CRT 合成程序（扫描线+暗角）上屏。
--- 仅改造版扩展有此 API，调用前用 `if imgui.render_draw_data then` 检测。
function imgui.render_draw_data() end

--- 显隐安卓软键盘。经引擎 DefoldActivity 的隐藏输入框路径（隐形 EditText +
--- DefoldInputWrapper），退格 / 回车 / 文本由此转成引擎标准输入事件。
--- 仅移动端有效。
---@param show boolean
---@param keyboard_type number|nil 0 通用 / 1 数字 / 2 邮箱 / 3 密码（仅在 show 时读取）
---@return boolean ok
function imgui.show_soft_keyboard(show, keyboard_type) end

---@return number font_size
function imgui.get_font_size() end

---@return number height
function imgui.get_frame_height() end

---
--- DRAW
---

---@param x1 integer
---@param y1 integer
---@param x2 integer
---@param y2 integer
---@param color integer
function imgui.draw_line(x1, y1, x2, y2, color) end

---@param x integer
---@param y integer
---@param width integer
---@param height integer
---@param color integer
function imgui.draw_rect(x, y, width, height, color) end

---@param x integer
---@param y integer
---@param width integer
---@param height integer
---@param color integer
function imgui.draw_rect_filled(x, y, width, height, color) end

---@param progress number
---@param width number
---@param height number
function imgui.draw_progress(progress, width, height) end

---@param enabled boolean
function imgui.set_rendering_enabled(enabled) end

---
--- INPUT CAPTURE
---

---@return boolean want_mouse
function imgui.want_mouse_input() end

---@return boolean want_keyboard
function imgui.want_keyboard_input() end

---@return boolean want_text
function imgui.want_text_input() end

---
--- CONFIG
---

function imgui.set_defaults() end

---@param filename string?
function imgui.set_ini_filename(filename) end
