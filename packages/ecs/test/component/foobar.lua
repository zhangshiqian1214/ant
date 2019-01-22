local ecs = ...

local foobar = ecs.component "foobar" {
	x = 0.0,
	y = 0.0,	
}

function foobar:init()
	print("New component foobar")
	self.temp = 0
end

function foobar:delete()
	print("Delete", self.x, self.y)
end

function foobar:print()
	print(self.x, self.y, self.temp)
end
