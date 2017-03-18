local ui = require 'ui'
local anim = require 'anim'

function createDemo()

	local root = Node()

	local scene = ui.Layer()
	root:addChild(scene)

	local bg = ui.Layer {width=1024, height=1024, texture='COL01B-761779.JPG'}
	scene:addChild(bg)

	local function box(x, y, w, h, r, g, b, a)
		-- make a box with a silly texture
		local f = ui.Layer {x=x, y=y, width=w, height=h, colour={r,g,b,a},
						 texture='S200N802.BMP'}
		scene:addChild(f)
		ui.draggable(f)
		return f
	end

	local function pom(id, x, y, next)
		-- make a node for pathing
		local f = ui.Layer {x=x, y=y, width=5, height=5, colour={0,1,0}}
		f.id, f.next = id, next
		scene:addChild(f)
		ui.draggable(f)
		return f
	end

	local b2 = box(-150, 0, 100, 100, 0.7, 0.7, 0)
	local b1 = box(0, 0, 100, 100, 1, 0, 0)
	local b3 = box(150, 0, 100, 100, 0, 1, 1)
	local b4 = box(-75, 50, 100, 100, 0, 1, 0, 1)

	connect(b2, 'mouseEnter', b2, function(self) self:setColour(1,1,0) end)
	connect(b2, 'mouseLeave', b2, function(self) self:setColour(0.7,0.7,0) end)

	--d = encode("bhisBHIS", -18, -20000, -1048576, "foo", 200, 65000, 3145728, "argh!")
	--p = encode("S", d)

	--r = decode("S", p)
	--print(decode("bhisBHIS", r))

	display:setWindowTitle("Quintiqua")
	display:setBackground(0.63, 0.84, 0.99)
	display:setScene(root)


	-- rotate b2 quickly
	schedule(anim.rotate(b2, 10))

	-- rotate b1 slowly ccw
	schedule(anim.rotate(b1, -5))

	-- and spin up b3
	schedule(anim.rotate(b3, 60))
	schedule(anim.zoom(b3, 0.25))

	-- zoomify b4
	schedule(anim.zoom(b4, 1))

	-- give the whole scene a lean
	-- schedule(anim.rotate(scene, 3))


	-- well okay this could just be a list of points!
	local n1 = pom("tram", -200, -100, "tram-2")
	local n2 = pom("tram-2", 200, -100, "tram-3")
	local n3 = pom("tram-3", 200, 100, "tram-4")
	local n4 = pom("tram-4", -200, 100, "tram")

	local foo = box(0, 0, 20, 20, 0, 0, 1)


	-- make the box follow the tram path
	schedule(anim.follow(foo, 'tram', 200))
	schedule(anim.rotate(foo, 40))
	schedule(anim.zoom(foo, 2))


	-- ui stuff
	local w = ui.Window {x=-100, y=180, width=400, height=400}
	scene:addChild(w)
	ui.draggable(w)

	local t = ui.Text("Welcome to Quintiqua")
	t:setPosition(-350, 200)
	scene:addChild(t)


	-- chat box
	-- yes this should be a class. later.

	local inp = ui.TextInput {width=600, fontSize=24}
	scene:addChild(inp)
	keyboard:setFocus(inp)

	local out = ui.TextArea {width=600, lines=10, fontSize=24}
	scene:addChild(out)

	local function sendLine()
		out:addLine(inp.value)
		inp:setText("")
	end

	connect(inp, "enter", sendLine)

	local function reposition()
		local w, h = display:getSize()
		local left = -w/2 + 10
		local inpTop = -h/2 + 10 + inp.height
		inp:setPosition(left, inpTop)
		out:setPosition(left, inpTop + out.height + 5)
	end

	connect(display, "resize", reposition)
	reposition()
end

function createLoginScene()
	local scene = Frame()
	local bg = ui.Layer {width=1024, height=1024, texture='COL01B-761779.JPG', parent=scene}
	local box = ui.Window {x=-250, y=150, width=470, height=210, parent=scene}
	local x, y = 15, -30
	local loginLabel = ui.Label {text="Quintiqua Login", x=x, y=y, parent=box}
	local userLabel = ui.Label {text="Username:", x=x, y=y-50, parent=box}
	local passLabel = ui.Label {text="Password:", x=x, y=y-80, parent=box}
	local userBox = ui.TextInput {x=x+160, y=y-50, width=270, parent=box}
	local passBox = ui.TextInput {x=x+160, y=y-80, width=270, parent=box}
	local loginButton = ui.Button {label="log in", x=x+470-40-100, y=y-120, width=100, height=35, parent=box}
	connect(userBox, "tab", function() keyboard:setFocus(passBox) end)
	connect(passBox, "tab", function() keyboard:setFocus(userBox) end)
	local function submit()
		doStuff()
	end
	connect(passBox, "enter", submit)
	connect(loginButton, "press", submit)
	ui.draggable(box)
	display:setBackground(0,0,0)
	display:setScene(scene)
	keyboard:setFocus(userBox)
end


createLoginScene()
--createDemo()


require 'socket'

function doStuff()
	s = socket.tcp()
	con, err = s:connect("pomke.com", 7971)
	if not con then
		print("connect failed: "..err)
	end
	s:send("flibble")
	s:receive(4)
end

--connect(inp, "enter", doStuff)
