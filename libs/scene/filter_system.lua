local ecs = ...
local world = ecs.world

ecs.import "render.math3d.math_component"

local cu = require "render.components.util"

local function insert_primitive(eid, result)
	local entity = world[eid]

	local mesh = assert(entity.mesh.assetinfo)
	
	local materialcontent = entity.material.content
	assert(#materialcontent >= 1)

	local srt ={s=entity.scale, r=entity.rotation, t=entity.position}
	local mgroups = mesh.handle.groups
	for i=1, #mgroups do
		local g = mgroups[i]
		local mc = materialcontent[i] or materialcontent[1]
		local material = mc.materialinfo
		local properties = mc.properties

		table.insert(result, {
			eid = eid,
			mgroup = g,
			material = material,
			properties = properties,
			srt = srt,
		})
	end
end

local function append_lighting_properties(ms, result)

	local function gen_directional_light_properties()
		local properties = {}

		local dlight_info = {
			dir = {},
			color = {},
			intensity = {}
		}

		for _, l_eid in world:each("directional_light") do
			local dlight = world[l_eid]
			local l = dlight.light.v
		
			-- point from vertex position to light position			
			table.insert(dlight_info.dir, ms(dlight.rotation, "dim"))
			table.insert(dlight_info.color, l.color)
			table.insert(dlight_info.intensity, {l.intensity, 0.28, 0, 0})

		end

		properties["directional_lightdir"] 	= {name="Light Direction", type="v4", value = dlight_info.dir}
		properties["directional_color"] 	= {name="Light Color", type="color", value = dlight_info.color}
		properties["directional_intensity"] = {name="Light Intensity", type="v4", value = dlight_info.intensity}

		return properties
	end

	--add ambient properties
	local function gen_ambient_light_propertices()
		local properties = {} 
		local ambient_data = {		
			-- mode = { 0, 0.3, 0, 0},   -- transfer and combine
			-- 							 -- mode :=   0 = "factor" , 1= "color" ,2 = "gradient"
			-- skycolor = {1,1,1,1},
			-- midcolor = {1,1,1,1},
			-- groundcolor = {1,1,1,1},

			-- 流程看来时需要，数据作为表中第一个子表，因此，按这个方法组织
			mode = {},
			skycolor = {},
			midcolor = {},
			groundcolor = {},
		}


		for _,l_eid in world:each("ambient_light") do 
			local  am_ent = world[l_eid]
			local  data = am_ent.ambient_light.data 
			-- print("s data ",data.mode,data.factor)
			-- print("s skycolor..",data.skycolor[1],data.skycolor[2],data.skycolor[3],data.skycolor[4])
			-- print("s midcolor..",data.midcolor[1],data.midcolor[2],data.midcolor[3],data.midcolor[4])
			-- print("s groundcolor..",data.groundcolor[1],data.groundcolor[2],data.groundcolor[3],data.groundcolor[4])

			local type = 1   -- default = "color"    	
			if data.mode == "factor" then 	
				type = 0
			elseif data.mode == "gradient" then 
				type = 2 
			end 
			table.insert( ambient_data.mode, {type, data.factor, 0, 0} )   
			table.insert( ambient_data.skycolor,  data.skycolor )
			table.insert( ambient_data.midcolor, data.midcolor )
			table.insert( ambient_data.groundcolor, data.groundcolor )

			-- print("t data ",ambient_data.mode[1][1],ambient_data.mode[1][2],ambient_data.mode[3],ambient_data.mode[4])
			-- print("t skycolor..",ambient_data.skycolor[1][1],ambient_data.skycolor[1][2],ambient_data.skycolor[1][3],ambient_data.skycolor[1][4])
			-- print("t midcolor..",ambient_data.midcolor[1][1],ambient_data.midcolor[1][2],ambient_data.midcolor[1][3],ambient_data.midcolor[1][4])
			-- print("t groundcolor..",ambient_data.groundcolor[1][1],ambient_data.groundcolor[1][2],ambient_data.groundcolor[1][3],ambient_data.groundcolor[1][4])
		end 

		properties["ambient_mode"] = { name ="ambient_mode",type="v4",value = ambient_data.mode }
		properties["ambient_skycolor"] = { name ="ambient_skycolor",type="color",value=ambient_data.skycolor}
		properties["ambient_midcolor"] = { name ="ambient_midcolor",type="color",value=ambient_data.midcolor}
		properties["ambient_groundcolor"] = { name ="ambient_groundcolor",type="color",value=ambient_data.groundcolor}

		--print("gen ambient light propertices")

		return properties 
	end 


	local lighting_properties = gen_directional_light_properties()
	-- add tested for ambient 
	local ambient_properties = gen_ambient_light_propertices()


	local camera = world:first_entity("main_camera")
	local eyepos = ms(camera.position, "m")
	lighting_properties["u_eyepos"] = {name = "Eye Position", type="v4", value=eyepos}

	for _, r in ipairs(result) do
		local material = r.material       
		local properties = r.properties   
		local surface_type = material.surface_type
		if surface_type.lighting == "on" then
			for k, v in pairs(lighting_properties) do
				properties[k] = v
			end	
			-- add ambient propertices
			for k,v in pairs(ambient_properties) do 	
				properties[k] = v
			end 		
		end
	end
end

--- scene filter system----------------------------------
local primitive_filter_sys = ecs.system "primitive_filter_system"
primitive_filter_sys.singleton "math_stack"

function primitive_filter_sys:update()
	for _, eid in world:each("primitive_filter") do
		local e = world[eid]
		local filter = e.primitive_filter
		filter.result = {}
		for _, eid in world:each("can_render") do
			local ce = world[eid]
			if cu.is_entity_visible(ce) then
				if (not filter.filter_select) or ce.can_select then
					insert_primitive(eid, filter.result)
				end
			end
		end
	end
end

--- scene lighting fitler system ------------------------
local lighting_primitive_filter_sys = ecs.system "lighting_primitive_filter_system"
lighting_primitive_filter_sys.singleton "math_stack"

lighting_primitive_filter_sys.depend "primitive_filter_system"

function lighting_primitive_filter_sys:update()
	local ms = self.math_stack
	for _, eid in world:each("primitive_filter") do
		local e = world[eid]
		local filter = e.primitive_filter
		if not filter.no_lighting then
			append_lighting_properties(ms, filter.result)
		end
	end
end

----for transparency filter system-------------------------------
local transparency_filter_sys = ecs.system "transparency_filter_system"
transparency_filter_sys.singleton "math_stack"
transparency_filter_sys.depend "lighting_primitive_filter_system"

local function split_transparent_filter_result(result, opaticy_result, transparent_result)
	for _, r in ipairs(result) do
		local material = r.material
		local surface_type = material.surface_type
		if surface_type.transparency == "transparent" then
			table.insert(transparent_result, r)
		else
			assert(surface_type.transparency == "opaticy")
			table.insert(opaticy_result, r)
		end
	end
end

function transparency_filter_sys:update()
	for _, eid in world:each("primitive_filter") do
		local e = world[eid]
		local filter = e.primitive_filter
		local transparent_result, opaticy_result= {}, {}
		split_transparent_filter_result(filter.result, opaticy_result, transparent_result)
		filter.result = opaticy_result
		filter.transparent_result = transparent_result
	end
end