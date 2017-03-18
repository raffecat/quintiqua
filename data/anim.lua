--[[

	Animation Behaviours

--]]

local class = class
local print = print
local sqrt = math.sqrt
local tostring = tostring
local dump = dump

module 'anim'



-- the 'rotate' behaviour

rotate = class()

function rotate:init(actor, rate)
	-- rate is pixels per second
	self.actor = actor
	self.ppm = (rate or 1) / 1000 -- pels per milli
end

function rotate:animate(dt)
	local move = self.ppm * dt
	self.actor:setAngle(self.actor:getAngle() + move)
end


-- the 'zoom' behaviour

zoom = class()

function zoom:init(actor, rate)
	-- rate is pixels per second
	self.actor = actor
	self.ppm = (rate or 1) / 1000 -- pels per milli
	self.shrink = true
end

function zoom:animate(dt)
	local move = self.ppm * dt
	local scale = self.actor:getScale()
	while move > 0 do
		if self.shrink then
			scale = scale - move
			if scale < 0 then
				-- begin growing
				move = -scale
				scale = 0
				self.shrink = false
			else
				move = 0
			end
		else
			scale = scale + move
			if scale > 1 then
				-- begin shrinking
				move = scale - 1
				scale = 1
				self.shrink = true
			else
				move = 0
			end
		end
	end
	self.actor:setScale(scale)
end


-- the 'follow path' behaviour

follow = class()

function follow:init(actor, id, rate)
	-- rate is pixels per second
	self.actor = actor
	self.ppm = (rate or 10) / 1000 -- pels per milli
	local to = actor:getParent():findChild(id)
	actor:setPosition(to:getPosition())
	self:beginSeek(to.next)
end

function follow:beginSeek(id)
	-- begin moving toward target 'id'
	local to = self.actor:getParent():findChild(id)
	local tx, ty = to:getPosition()
	local ox, oy = self.actor:getPosition()
	local dx, dy = tx - ox, ty - oy
	local dist = sqrt(dx*dx+dy*dy)
	local time = dist / self.ppm
	self.to = to
	self.dx = dx / time
	self.dy = dy / time
	self.remain = dist
end

function follow:animate(dt)
	-- loop because one dt can take us around corners
	while dt > 0 do
		local move = self.ppm * dt
		if not (self.remain > 0) then
			break
		end
		-- print("dt "..dt..", move "..move..", remain "..self.remain..", dx "..self.dx)
		if move > self.remain then
			-- use remaining time on next target
			if self.remain > 0 then
				timeUsed = dt * (self.remain / move)
				dt = dt - timeUsed
				-- print("*** NEXT timeUsed: "..timeUsed..", dt: "..dt)
			end
			-- snap to target, begin next move
			self.actor:setPosition(self.to:getPosition())
			self:beginSeek(self.to.next)
		else
			-- advance current move
			local x, y = self.actor:getPosition()
			self.actor:setPosition(x + self.dx * dt, y + self.dy * dt)
			self.remain = self.remain - move
			dt = 0
		end
		-- print("x = "..self.actor.x..", y = "..self.actor.y)
	end
end

