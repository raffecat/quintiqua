--[[

	User Interface module

--]]

local class = class
local print = print
local printf = printf
local format = string.format
local pairs = pairs
local tostring = tostring
local unpack = unpack
local error = error
local assert = assert
local type = type
local sqrt = math.sqrt
local dump = dump
local insert = table.insert
local remove = table.remove
local ipairs = ipairs
local sort = table.sort
local ord = string.byte
local substr = string.sub
local floor = math.floor

local Node = Node
local Frame = Frame
local Texture = Texture
local Graphic = Graphic
local connect = connect
local disconnect = disconnect
local signal = signal
local keyboard = keyboard
local mouse = mouse

module 'ui'

local WHITE = {1,1,1,1}


--[[ utilities ]]--

function draggable(drag)
	local ox, oy
	local function dragMove(meh, x, y)
		-- move our origin in parent space
		drag:setPosition(ox + x, oy + y)
	end
	local function endDrag()
		disconnect(mouse, 'move', dragMove)
		disconnect(mouse, 'buttonUp', endDrag)
	end
	local function startDrag(meh, btn, x, y)
		if btn == 1 then
			-- map local mouse coords to parent space
			-- and make relative to our origin in parent space
			local px, py = drag:localToParent(x, y)
			local rx, ry = drag:getPosition()
			ox = rx - px
			oy = ry - py
			connect(mouse, 'move', dragMove)
			connect(mouse, 'buttonUp', endDrag)
		end
	end
	connect(drag, 'mouseDown', startDrag)
end


--[[ layer ]]--

Layer = class {
	__base = Frame,
	width = 0,
	height = 0,
}

function Layer:init(args)
	Frame.init(self)
	self.ids = {}
	if args then self:setup(args) end
end

function Layer:setup(args)
	-- set up whatever is passed in the dict of args
	if args.parent then self:setParent(args.parent) end
	if args.x or args.y then self:setPosition(args.x or 0, args.y or 0) end
	if args.angle then self:setAngle(args.angle) end
	if args.scale then self:setScale(args.scale, args.scale) end
	if args.colour then self:setColour(unpack(args.colour)) end
	if args.texture then self:setTexture(Texture(args.texture)) end
	self:setSize(args.width or self.width or 0, args.height or self.height or 0)
end

function Layer:setSize(width, height)
	self.width = width
	self.height = height
	local w = width * 0.5
	local h = height * 0.5
	self:setShape(-w, -h, w, h)
end

-- override
function Layer:addChild(child)
	--print("adding "..tostring(child).." to "..tostring(self))
	if child.id then
		--print("adding '"..child.id.."' to "..tostring(self))
		self.ids[child.id] = child
	end
	Frame.addChild(self, child)
end

function Layer:findChild(id, opt)
	child = self.ids[id]
	if child or not opt then return child end
	error("no such child: "..id)
end

function Layer:hitTestSelf(x, y)
	-- hit test against width and height
	if x >= 0 and x < self.width and
	   y <= 0 and y > -self.height then
	   --printf("** hit on Layer %s %d %d", tostring(self), x, y)
	   return self, x, y
	end
end


--[[ clip box ]]--

clip = class {
	__base = Layer,
}

function clip:init(args)
	self.node = ClipView()
	self:setup(args)
	self.ids = {}
end

function clip:setup(args)
	-- set up whatever is passed in the dict of args
	if args.parent then args.parent:addChild(self) end
	self:setShape(args.left or 0, args.bottom or 0, args.right or 0, args.top or 0)
end

function clip:setShape(left, bottom, right, top)
	self.width = right - left
	self.height = top - bottom
	self.node:setShape(left, bottom, right, top)
end


--[[ sprite layer ]]--

SpriteLayer = class {
	__base = Layer,
}

function SpriteLayer:init(args)
	self.children = {}
	Layer.init(self, args)
	-- schedule(self)
end

function SpriteLayer:animate(dt)
	-- going to do this in C anyway
	-- sort(self.children, function(l,r) return l.y < r.y end)
end


--[[ window ]]--

Window = class {
	__base = Layer,
}

-- these should come from a theme or something
local _windowTitleLeft = Texture('ui/window-title-left.png')
local _windowTitleBar = Texture('ui/window-title-bar.png')
local _windowTitleRight = Texture('ui/window-title-right.png')
local _windowBorder = Texture('ui/window-border-left.png')
local _windowBottom = Texture('ui/window-border-bottom.png')
local _windowCornerLeft = Texture('ui/window-corner-left.png')
local _windowCornerRight = Texture('ui/window-corner-right.png')
local _windowCloseHilight = Texture('ui/close-button-hilight.png')

local function _windowPart(wnd, tex)
	local f = Graphic()
	f:setTexture(tex)
	f.width, f.height = tex:getSize()
	wnd:addChild(f)
	return f
end

local function _makeHilight(wnd, tex)
	local f = Frame()
	wnd:addChild(f)
	f:setTexture(tex)
	f:setBlendMode('add')
	f:setColour(0,0,0,0) -- hide
	connect(f, 'mouseEnter', f, function(self) self:setColour(1,1,1,1) end)
	connect(f, 'mouseLeave', f, function(self) self:setColour(0,0,0,0) end)
	return f
end

function Window:init(args)
	Layer.init(self)
	self.background = Graphic()
	self:addChild(self.background)
	self.background:setColour(0.05, 0.21, 0.25, 0.6)
	self.titleLeft = _windowPart(self, _windowTitleLeft)
	self.titleBar = _windowPart(self, _windowTitleBar)
	self.titleRight = _windowPart(self, _windowTitleRight)
	self.borderLeft = _windowPart(self, _windowBorder)
	self.borderRight = _windowPart(self, _windowBorder)
	self.borderBottom = _windowPart(self, _windowBottom)
	self.cornerLeft = _windowPart(self, _windowCornerLeft)
	self.cornerRight = _windowPart(self, _windowCornerRight)
	self.closeHilight = _makeHilight(self, _windowCloseHilight)
	self.barTrim = 10 -- transparent area in bar textures
	self.barHeight = self.titleBar.height - self.barTrim
	self.borderWidth = self.borderLeft.width
	self.borderHeight = self.borderBottom.height
	self:setup(args)
	self.ids = {}
end

function Window:setSize(width, height)
	-- origin is at top-left, invert y for down
	self.width = width
	self.height = height
	local btrim = self.barTrim
	local bh = -self.barHeight
	local lw = self.titleLeft.width
	local rw = self.titleRight.width
	local bx = self.borderWidth
	local by = self.borderHeight
	local bot = -(height - by)
	self.background:setShape(bx, bh, width - bx, bot)
	self.titleLeft:setShape(0, bh, lw, btrim)
	self.titleBar:setShape(lw, bh, width - rw, btrim)
	self.titleRight:setShape(width - rw, bh, width, btrim)
	self.borderLeft:setShape(0, bh, bx, bot)
	self.borderRight:setShape(width, bh, width - bx, bot)
	self.borderBottom:setShape(bx, -height, width - bx, bot)
	self.cornerLeft:setShape(0, -height, bx, bot)
	self.cornerRight:setShape(width - bx, -height, width, bot)
	self.closeHilight:setShape(width - 22, -3-16, width - 22 + 16, -3)
end

function Window:hitTestSelf(x, y)
	-- hit test against the title bar
	if x >= 0 and x < self.width and
	   y > -self.barHeight and y <= 0 then
	   return self, x, y
	end
end


--[[ text ]]--

Text = class {
	__base = Graphic,
}

-- only supports ascii texture-based fonts
local defaultFont = Texture('ui/fontBookMan.png')
defaultFont.cellWidth = 32
defaultFont.cellHeight = 48
defaultFont.fontSize = 24

function Text:init(text, font, size, x, y)
	Graphic.init(self)
	self:setPosition(x or 0, y or 0)
	self:setFont(font or defaultFont, size)
	self:setText(text)
	self:setBlendMode('add')
end

function Text:setFont(font, size)
	self.font = font
	self:setTexture(font)
	self.lineHeight = size or font.fontSize
	local aspect = font.cellWidth / font.cellHeight
	self.charWidth = self.lineHeight * aspect
end

function Text:setText(text)
	self.text = text
	local fw, fh = self.font:getSize()
	local cols = fw / self.font.cellWidth
	local cw = 1 / cols
	local ch = self.font.cellHeight / fh
	local rw = self.charWidth
	local rh = self.lineHeight
	local x = 0
	local y = 0
	local p = 1
	local verts = {}
	local coords = {}
	local indices = {}
	for i = 1, #text do
		-- ascii conversion
		local c = ord(text, i) - 32
		-- generate texture coords
		local u = (c % cols) * cw
		local v = floor(c/cols) * ch
		coords[p] = u
		coords[p+1] = v
		coords[p+2] = u
		coords[p+3] = v + ch
		coords[p+4] = u + cw
		coords[p+5] = v + ch
		coords[p+6] = u + cw
		coords[p+7] = v
		-- generate vertices
		verts[p] = x
		verts[p+1] = y
		verts[p+2] = x
		verts[p+3] = y - rh
		verts[p+4] = x + rw
		verts[p+5] = y - rh
		verts[p+6] = x + rw
		verts[p+7] = y
		p = p + 8
		x = x + rw
	end
	-- build indices for quads
	for i = 1, #text * 4 do indices[i] = i-1 end
	self:setGeometry(indices, verts, coords)
end


--[[ Label ]]--

Label = class {
	__base = Text,
}

function Label:init(args)
	Text.init(self, args.text, args.font, args.size, args.x, args.y)
	if args.parent then
		args.parent:addChild(self)
	end
end



--[[ TextArea ]]--

TextArea = class {
	__base = Layer,
}

local function makeBackground(owner, width, height)
	local background = Graphic()
	background:setColour(0, 0, 0, 0.5)
	background:setShape(0, 0, width or owner.width,
						-(height or owner.height))
	owner:addChild(background)
	return background
end

function TextArea:init(args)
	Layer.init(self, args)
	self.font = args.font or defaultFont
	self.fontSize = args.fontSize or self.font.fontSize
	self.rowStride = self.fontSize
	self.lines = args.lines or 10
	self.height = self.lines * self.rowStride
	self.background = makeBackground(self)
	self._lines = {}
end

function TextArea:addLine(text)
	local rows = #self._lines
	if rows >= self.lines then
		local old = remove(self._lines, 1)
		self:removeChild(old)
		rows = rows - 1
	end
	-- scroll the remaining rows up
	for i = 1, rows do
		self._lines[i]:setPosition(2, -i * self.rowStride)
	end
	-- add the new row at the bottom
	local row = Text(text, self.font, self.fontSize)
	row:setPosition(2, -(rows + 1) * self.rowStride)
	self:addChild(row)
	insert(self._lines, row)
end


--[[ TextInput ]]--

TextInput = class {
	__base = Layer,
}

function TextInput:init(args)
	Layer.init(self, args)
	self.font = args.font or defaultFont
	self.fontSize = args.fontSize or self.font.fontSize
	self.height = self.fontSize
	self.background = makeBackground(self)
	self.value = ""
	self._text = Text("", self.font, self.fontSize, 2, -2)
	self:addChild(self._text)
	connect(self, "mouseDown", self, function(self)
		keyboard:setFocus(self)
	end)
end

function TextInput:onTextInput(text)
	if text == "\008" then -- backspace
		self.value = substr(self.value, 1, -2)
	elseif text == "\013" then -- enter
		signal(self, "enter", value)
	elseif text == "\09" then -- tab
		signal(self, "tab")
	else
		self.value = self.value .. text
	end
	self:redraw()
end

function TextInput:setText(text)
	self.value = text
	self:redraw()
end

function TextInput:redraw()
	self._text:setText(self.value)
end


--[[ button ]]--

Button = class {
	__base = Layer,
	width = 80,
	height = 35,
	isover = false,
}

function Button:init(args)
	Layer.init(self)
	self.background = Graphic()
	self.hilight = Graphic()
	self.pressed = Graphic()
	self:addChild(self.background)
	self:addChild(self.pressed)
	self:addChild(self.hilight)
	self.background:setTexture(Texture('ui/button-up.png'))
	self.pressed:setTexture(Texture('ui/button-down.png'))
	self.pressed:setColour(0,0,0,0)
	self.hilight:setColour(0,0,0,0)
	self.hilight:setBlendMode('add')
	self:setup(args)
	connect(self, "mouseDown", self, function(self)
		self.background:setColour(0,0,0,0)
		self.pressed:setColour(1,1,1,1)
		self.hilight:setColour(0,0,0,0)
	end)
	connect(self, "mouseUp", self, function(self)
		self.background:setColour(1,1,1,1)
		self.pressed:setColour(0,0,0,0)
		if self.isover then
			self:showHilight()
			signal(self, "press")
		end
	end)
	connect(self, "mouseEnter", self, function(self)
		self.isover = true
		self:showHilight()
	end)
	connect(self, "mouseLeave", self, function(self)
		self.isover = false
		self.hilight:setColour(0,0,0,0)
	end)
end

function Button:setSize(width, height)
	self.width = width
	self.height = height
	if self.hilight then
		self.background:setShape(0, -height, width, 0)
		self.pressed:setShape(0, -height, width, 0)
		self.hilight:setShape(0, -height, width, 0)
	end
end

function Button:showHilight()
	self.hilight:setColour(0.1,0.1,0.1,0)
end

-- sioreth
