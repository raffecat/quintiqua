--[[

	Exported to the global namespace

--]]

local _print = print
local format = string.format
local setmetatable = setmetatable
local concat = table.concat
local gsub = string.gsub
local package = package
local require = require
local tostring = tostring
local pairs = pairs
local tinsert = table.insert
local tremove = table.remove
local join = string.join

traceback = debug.traceback
debug = nil -- dangerous

function print(...)
	local n = select('#', ...)
	local s = ""
	for i = 1,n do
		s = s .. tostring(select(i, ...)) .. ' '
	end
	_print(s) -- print() in client expects a string
end

function printf(...) print(format(...)) end

function class(dict)
	local cls = dict or {}
	cls.__class = cls
	local mt = { __index = cls }
	function new(cls, ...)
		local o = {}
		setmetatable(o, mt)
		local init = o.init
		if init then init(o, ...) end
		return o
	end
	local clsmt = { __call = new }
	if cls.__base then
		clsmt.__index = cls.__base
	end
	setmetatable(cls, clsmt)
	return cls
end

function typeof(x)
	return x.__class or type(x)
end

function partial(f, ...)
	local bound = {...}
	return function(...)
		return f(unpack(bound), ...)
	end
end

local function _tdump(t, indent, done)
  done = done or {}
  local ind = indent or ""
  local s = ""
  for key, value in pairs(t) do
    if type(value) == "table" then
      if not done[value] then
        done[value] = true
        if next(value) then
		  s = s..format("%s%s = %s {\n%s%s}\n", ind, tostring(key), tostring(value), _tdump(value, ind..'  ', done), ind)
        else
      	  s = s..format("%s%s = %s {}\n", ind, tostring(key), tostring(value))
      	end
      else
      	s = s..format("%s%s = %s { ... }\n", ind, tostring(key), tostring(value))
      end
    elseif type(value) == "string" then
      local v = gsub(tostring(value), "\n", "\\n")
      s = s..format("%s%s = \"%s\"\n", ind, tostring(key), v)
    else
      s = s..format("%s%s = %s\n", ind, tostring(key), tostring(value))
    end
  end
  return s
end

function dump(t)
	print(_tdump(t))
end

function reload(name)
	package.loaded[name] = nil
	return require(name)
end


-- animation schedule

local _animations = {}
local _scene
local _capture
local width, height = 0,0
local mx, my = 0,0

function schedule(controller)
	-- add an animation controller to the schedule
	_animations[controller] = 1
end

function deschedule(controller)
	-- remove a scheduled animation
	_animations[controller] = nil
end


-- debugging spam

local hitDebug = false
local overDebug = false
local keyDebug = false
local eventDebug = false



-- Events system

-- Note: connecting a handler to an event source will prevent that
-- event source from being garbage collected until the listener
-- disconnects the handler or the listener itself is collected.

function connect(obj, name, who, func) -- 'who' is optional!
	if not func then func = who ; who = nil end
	local subs = obj.__events
	if not subs then
		subs = {}
		obj.__events = subs
	end
	local handlers = subs[name]
	if not handlers then
		handlers = {}
		subs[name] = handlers
	end
	tinsert(handlers, { func=func, who=who })
	if eventDebug then
		local s = "++ connect: "..tostring(obj).."."..tostring(name).." -> "
		if who then s=s..tostring(who).."." end
		print(s..tostring(func))
	end
end

function disconnect(obj, name, who, func) -- 'who' is optional!
	if not func then func = who ; who = nil end
	if eventDebug then
		local s = "++ disconnect: "..tostring(obj).."."..tostring(name).." -> "
		if who then s=s..tostring(who).."." end
		print(s..tostring(func))
	end
	local subs = obj.__events
	if subs then
		local handlers = subs[name]
		if handlers then
			for i, hand in ipairs(handlers) do
				if hand.func == func and hand.who == who then
					tremove(handlers, i)
					return
				end
			end
		end
	end
	if eventDebug then print("!! handler was not registered") end
end

function signal(obj, name, ...)
	local eventDebug = eventDebug
	if eventDebug then
		if obj == display and name == 'update' then
			eventDebug = false -- too much spam!
		else
			print("++ signal: "..tostring(obj).."."..tostring(name))
		end
	end
	local e = { source=obj, name=name }
	--while obj do
		local subs = obj.__events
		if subs then
			local handlers = subs[name]
			if handlers then
				for i, hand in ipairs(handlers) do
					local callback, who = hand.func, hand.who
					if who then
						if eventDebug then print("-> "..tostring(who).."."..tostring(callback)) end
						callback(who, e, ...)
					else
						if eventDebug then print("-> "..tostring(callback)) end
						callback(e, ...)
					end
				end
			end
		end
		--obj = obj.parent -- walk up tree
	--end
end



-- Global event sources
-- Connect to signals on these objects to handle display resize,
-- mouse movement, mouse buttons, key presses, etc.

display = {}
keyboard = {}
mouse = {}

local _setBackground = sg.setBackground
local _setScene = sg.setScene
local _setWindowTitle = SetWindowTitle
SetWindowTitle = nil

function display:getSize()
	return width, height
end

function display:setWindowTitle(caption)
	_setWindowTitle(caption)
end

function mouse:getPosition()
	return mx, my
end

function display:setBackground(r, g, b)
	_setBackground(r, g, b)
end

function display:setScene(scene)
	_scene = scene
	_setScene(scene.__id)
end

function display:getScene()
	return _scene
end

function keyboard:setFocus(obj)
	self._focus = obj
end

function keyboard:getFocus()
	return self._focus
end


-- Hooks called from the C engine
-- Eventually replace these with something else,
-- probably direct calls to signal() from C

function sg_update(t)
	-- called every frame, t is time passed in milliseconds
	signal(display, "update", t)
	for k,v in pairs(_animations) do
		k:animate(t)
	end
end

function sg_size_change(w, h)
	if width ~= w or height ~= h then
		width = w
		height = h
		--printf("size is %d, %d", w, h)
		signal(display, "resize", w, h)
		if _scene and _scene.setSize then
			_scene:setSize(w, h)
		end
	end
end

function sg_mouse_move(x, y)
	-- make position relative to center
	mx = x - width/2
	my = -y + height/2
	--printf("move %d, %d", x, y)
	signal(mouse, "move", mx, my)
end

function sg_mouse_button(btn, down)
	if keyDebug then printf("button %d %d", btn, down) end
	signal(mouse, (down==1 and "buttonDown" or "buttonUp"), btn)
	if _scene then
		local e, x, y = _scene:hitTest(mx, my)
		if e then
			signal(e, (down==1 and "mouseDown" or "mouseUp"), btn, x, y)
		end
	end
end

function sg_key_press(key, down)
	if keyDebug then printf("key %d, %d", key, down) end
	if down == 1 then
		signal(keyboard, "keyDown", key)
	else
		signal(keyboard, "keyUp", key)
	end
end

function sg_key_char(text)
	if keyDebug then print("char: "..text) end
	signal(keyboard, "input", text)
	local focus = keyboard._focus
	if focus then
		focus:onTextInput(text)
	end
end


-- enter and leave events

-- TODO: this needs to keep an ordered list of frames
-- that the mouse is over and needs to send leave events to
-- them in reverse order before checking for new enter events

local _mouseOver

local function _checkEnter()
	if _scene then
		local e, x, y = _scene:hitTest(mx, my)
		local over = _mouseOver
		if e ~= over then
			if over then
				if overDebug then print("** leaving "..tostring(over)) end
				_mouseOver = nil -- first, in case of error
				signal(over, 'mouseLeave')
			end
			if e then
				if overDebug then print("** entering "..tostring(e)) end
				_mouseOver = e -- first, in case of error
				signal(e, 'mouseEnter')
			end
		end
	end
end

if hitDebug then
	connect(mouse, 'move', _checkEnter)
else
	connect(display, 'update', _checkEnter)
end


-- Some of this will eventually move into C.
-- Anything starting with __ is an implementation detail;
-- code that accesses these attrs will break one day.

local insert = table.insert
local remove = table.remove
local ipairs = ipairs

local createTransform = sg.createTransform
local createFrame = sg.createFrame
local createGraphic = sg.createGraphic
local setParent = sg.setParent
local setPosition = sg.setPosition
local setAngle = sg.setAngle
local setScale = sg.setScale
local setColour = sg.setColour
local setShape = sg.setShape
local setTexture = sg.setTexture
local loadTexture = sg.loadTexture
local setOutline = sg.setOutline
local setBlendMode = sg.setBlendMode
local getTextureSize = sg.getTextureSize
local setGeometry = sg.setGeometry
local setGfxTexture = sg.setGfxTexture

sg = nil


-------------------------------------------

Node = class {
	__x = 0,
	__y = 0,
	__angle = 0,
	__xscale = 1,
	__yscale = 1,
}

function Node:init()
	self.__id = createTransform()
end

function Node:addChild(child)
	local op = child.parent
	if op then
		op:removeChild(child)
	end
	child.parent = self
	setParent(child.__id, self.__id)
	local children = self.children
	if not children then
		children = {}
		self.children = children
	end
	insert(children, child)
end

function Node:removeChild(child)
	local children = self.children
	if children then
		for i,c in ipairs(children) do
			if c == child then
				remove(children, i)
				setParent(child.__id)
				return
			end
		end
	end
	print("child", child, "not found in ", self)
end

function Node:setParent(parent)
	parent:addChild(self)
end

function Node:getParent()
	return self.parent
end

function Node:setPosition(x, y)
	self.__x, self.__y = x, y
	setPosition(self.__id, x, y)
end

function Node:getPosition()
	return self.__x, self.__y
end

function Node:setAngle(a)
	self.__angle = a
	setAngle(self.__id, a)
end

function Node:getAngle()
	return self.__angle
end

function Node:setScale(x, y)
	if not y then y = x end
	self.__xscale, self.__yscale = x, y
	setScale(self.__id, x, y)
end

function Node:getScale()
	return self.__xscale, self.__yscale
end

function Node:setColour(r, g, b, a)
	setColour(self.__id, r, g, b, a)
end

function Node:setBlendMode(mode)
	setBlendMode(self.__id, mode)
end

function Node:hitTestChildren(x, y)
	-- check collision against all children of this node
	-- x,y must be in local coordinates
	if hitDebug then printf("Node:hitTestChildren %d %d", x, y) end
	local children = self.children
	if children then
		local n = #children
		while n > 0 do
			local c = children[n]
			local cc, cx, cy = c:hitTest(x, y)
			if cc then
				return cc, cx, cy
			end
			n = n - 1
		end
	end
end

function Node:hitTestSelf(x, y)
	-- do nothing; no content to hit-test
end

function Node:hitTest(x, y)
	if hitDebug then printf("Node:hitTest %d %d", x, y) end
	x, y = self:parentToLocal(x, y)
	return self:hitTestChildren(x, y)
end

local sin = math.sin
local cos = math.cos
local rad = math.rad

function Node:parentToLocal(x, y)
	-- untransform point: parent coords to local coords
	x = x - self.__x
	y = y - self.__y
	local xs = self.__xscale
	local ys = self.__yscale
	-- this might do funny things to hitTest...
	if xs ~= 0 then x = x / xs end
	if ys ~= 0 then y = y / ys end
	local t = rad(-self.__angle)
	if t ~= 0 then
		local st, ct = sin(t), cos(t)
		x, y = ct * x - st * y, st * x + ct * y
	end
	return x, y
end

function Node:localToParent(x, y)
	-- transform point: local coords to parent coords
	local t = rad(self.__angle)
	if t ~= 0 then
		local st, ct = sin(t), cos(t)
		x, y = ct * x - st * y, st * x + ct * y
	end
	return (x * self.__xscale) + self.__x, (y * self.__yscale) + self.__y
end


-------------------------------------------

Frame = class {
	__base = Node,
	__left = 0,
	__bottom = 0,
	__right = 0,
	__top = 0,
}

function Frame:init()
	self.__id = createFrame()
end

function Frame:setShape(left, bottom, right, top)
	self.__left, self.__bottom, self.__right, self.__top = left, bottom, right, top
	setShape(self.__id, left, bottom, right, top)
end

function Frame:setTexture(tex)
	-- need to keep a ref in self
	if not tex or not tex.__id then
		error("expecting a Texture object")
	end
	self._tex_ref = tex
	setTexture(self.__id, tex.__id)
end

function Frame:setOutline(thickness)
	setOutline(self.__id, thickness)
end

function Frame:hitTestSelf(x, y)
	-- check collision against frame surface
	-- x,y must be in local coordinates
	if hitDebug then printf("Frame:hitTestSelf %d %d", x, y) end
	-- check collision against frame
	local left, bottom, right, top = self.__left, self.__bottom, self.__right, self.__top
	if left < right and bottom < top and
	   x >= left and x <= right and
	   y >= bottom and y <= top then
	   if hitDebug then printf("** hit on Frame %s %d %d", tostring(self), x, y) end
	   return self, x, y
	end
end

function Frame:hitTest(x, y)
	if hitDebug then printf("Frame:hitTest %d %d", x, y) end
	x, y = self:parentToLocal(x, y)
	-- check collision against children first
	local c, cx, cy = self:hitTestChildren(x, y)
	if c then
		return c, cx, cy
	else
		return self:hitTestSelf(x, y)
	end
end


-------------------------------------------

-- a graphic is meant to have no transform or children,
-- only a rendered shape

Graphic = class {
	__base = Node,
}

function Graphic:init()
	self.__id = createGraphic()
end

function Graphic:setShape(left, bottom, right, top)
	verts = { left, bottom, right, bottom, right, top, left, top }
	coords = { 0, 1, 1, 1, 1, 0, 0, 0 }
	indices = { 0, 1, 2, 3 }
	setGeometry(self.__id, indices, verts, coords)
end

function Graphic:setGeometry(indices, verts, coords)
	setGeometry(self.__id, indices, verts, coords)
end

function Graphic:setTexture(tex)
	-- need to keep a ref in self
	if not tex or not tex.__id then
		error("expecting a Texture object")
	end
	self._tex_ref = tex
	setGfxTexture(self.__id, tex.__id)
end

function Graphic:setColour(r, g, b, a)
	setColour(self.__id, r, g, b, a)
end

function Graphic:setBlendMode(mode)
	setBlendMode(self.__id, mode)
end

function Graphic:hitTestSelf(x, y)
	-- never hit-test a graphic
end

function Graphic:hitTest(x, y)
	-- never hit-test a graphic
end


-------------------------------------------

Texture = class {}

function Texture:init(name)
	self.__id = loadTexture(name)
end

function Texture:getSize()
	return getTextureSize(self.__id)
end


-------------------------------------------

--[[

local function lockClass(cls)
	local mt = getmetatable(cls)
	mt.__newindex = function(t,k,v)
		error("cannot modify built-in classes")
	end
end

lockClass(Node)
lockClass(Frame)

]]--



--[[
a = class{foo=1}
b = class{__base=a, bar=1}

function a:init(x, y, z)
	print("x: "..tostring(x))
	print("y: "..tostring(y))
	print("z: "..tostring(z))
end

print(tostring(a))
dump(a)
print(tostring(b))
dump(b)
c = b(5,6)
print(tostring(c))
dump(c)
]]--

