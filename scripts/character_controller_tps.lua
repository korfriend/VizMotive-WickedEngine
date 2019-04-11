-- Lua Third person camera and character controller script
--	This script will load a simple scene with a character that can be controlled
--
-- 	CONTROLS:
--		WASD: walk
--		SHIFT: speed
--		SPACE: Jump
--		Right Mouse Button: rotate camera

local scene = GetScene()

Character = {
	model = INVALID_ENTITY,
	target = INVALID_ENTITY, -- Camera will look at this location, rays will be started from this location, etc.
	idle_anim = INVALID_ENTITY,
	walk_anim = INVALID_ENTITY,
	face = Vector(0,0,1), -- forward direction
	velocity = Vector(),
	ray = Ray(Vector(),Vector()),
	o = INVALID_ENTITY, -- collision prop with scene (entity)
	p,n = Vector(), -- collision props with scene (position,normal)
	savedPointerPos = Vector(),
	moveSpeed = 0.2,
	layerMask = 0x2, -- The character will be tagged to use this layer, so scene Picking can filter out the character
	scale = Vector(0.8,0.8,0.8),
	rotation = Vector(0,3.1415,0),
	
	states = {
		STAND = 0,
		WALK = 1,
		JUMP = 2,
	},
	state = STAND,
	
	Create = function(self,entity)
		self.model = entity
		local layer = scene.Component_GetLayer(self.model)
		layer.SetLayerMask(self.layerMask)
		
		self.idle_anim = scene.Entity_FindByName("idle")
		self.walk_anim = scene.Entity_FindByName("walk")
		
		local model_transform = scene.Component_GetTransform(self.model)
		model_transform.ClearTransform()
		model_transform.Scale(self.scale)
		model_transform.Rotate(self.rotation)
		model_transform.UpdateTransform()
		
		self.target = CreateEntity()
		local target_transform = scene.Component_CreateTransform(self.target)
		target_transform.ClearTransform()
		target_transform.Translate(Vector(0,2))
		
		scene.Component_Attach(self.target, self.model)
	end,
	
	MoveForward = function(self,f)
		local model_transform = scene.Component_GetTransform(self.model)
		local target_transform = scene.Component_GetTransform(self.target)
		
		-- avoid falling outside the world
		local velocityPrev = self.velocity;
		self.velocity = self.face:Multiply(Vector(f,f,f))
		self.velocity.SetY(velocityPrev.GetY())
		self.ray = Ray(target_transform.GetPosition():Add(self.velocity),Vector(0,-1,0))
		self.o,self.p,self.n = Pick(self.ray, PICK_OPAQUE, ~self.layerMask)
		if(self.o ~= INVALID_ENTITY) then
			self.state = self.states.WALK
		else
			self.state = self.states.STAND
			self.velocity = velocityPrev
		end
		
		-- front block
		local ray2 = Ray(target_transform.GetPosition(),self.face)
		local o2,p2,n2 = Pick(ray2, PICK_OPAQUE, ~self.layerMask)
		local dist = vector.Subtract(target_transform.GetPosition(),p2):Length()
		if(o2 ~= INVALID_ENTITY and dist < 1.5) then
			-- run along wall instead of going through it
			local velocityLen = self.velocity.Length()
			local velocityNormalized = self.velocity.Normalize()
			local undesiredMotion = n2:Multiply(vector.Dot(velocityNormalized, n2))
			local desiredMotion = vector.Subtract(velocityNormalized, undesiredMotion)
			self.velocity = vector.Multiply(desiredMotion, velocityLen)
		end
		
	end,
	Jump = function(self,f)
		self.velocity = self.velocity:Add(Vector(0,f,0))
		self.state = self.states.JUMP
	end,
	MoveDirection = function(self,dir,f)
		local model_transform = scene.Component_GetTransform(self.model)
		local target_transform = scene.Component_GetTransform(self.target)
		local savedPos = model_transform.GetPosition()
		model_transform.ClearTransform()
		self.face = dir:Normalize().Transform(target_transform.GetMatrix())
		self.face.SetY(0)
		self.face = self.face.Normalize()
		model_transform.MatrixTransform(matrix.LookTo(Vector(),self.face):Inverse())
		model_transform.Scale(self.scale)
		model_transform.Rotate(self.rotation)
		model_transform.Translate(savedPos)
		model_transform.UpdateTransform()
		scene.Component_Detach(self.target)
		scene.Component_Attach(self.target, self.model)
		self:MoveForward(f)
	end,
	
	Input = function(self)
		
		if(self.state==self.states.STAND) then
			local lookDir = Vector()
			if(input.Down(VK_LEFT) or input.Down(string.byte('A'))) then
				lookDir = lookDir:Add( Vector(-1) )
			end
			if(input.Down(VK_RIGHT) or input.Down(string.byte('D'))) then
				lookDir = lookDir:Add( Vector(1) )
			end
		
			if(input.Down(VK_UP) or input.Down(string.byte('W'))) then
				lookDir = lookDir:Add( Vector(0,0,1) )
			end
			if(input.Down(VK_DOWN) or input.Down(string.byte('S'))) then
				lookDir = lookDir:Add( Vector(0,0,-1) )
			end
			
			if(lookDir:Length()>0) then
				if(input.Down(VK_LSHIFT)) then
					self:MoveDirection(lookDir,self.moveSpeed*2)
				else
					self:MoveDirection(lookDir,self.moveSpeed)
				end
			end
		
		end
		
		if( input.Press(string.byte('J'))  or input.Press(VK_SPACE) ) then
			self:Jump(0.6)
		end
		
		-- mouse control
		if(input.Down(VK_RBUTTON)) then
			local mousePosNew = input.GetPointer()
			local mouseDif = vector.Subtract(mousePosNew,self.savedPointerPos)
			mouseDif = mouseDif:Multiply(getDeltaTime() * 0.3)
			local target_transform = scene.Component_GetTransform(self.target)
			target_transform.Rotate(Vector(mouseDif.GetY(),mouseDif.GetX()))
			self.face.SetY(0)
			self.face=self.face:Normalize()
			input.SetPointer(self.savedPointerPos)
		else
			self.savedPointerPos = input.GetPointer()
		end
		
	end,
	
	Update = function(self)
		local model_transform = scene.Component_GetTransform(self.model)
		local target_transform = scene.Component_GetTransform(self.target)
		
		-- state and animation update
		if(self.state == self.states.STAND) then
			scene.Component_GetAnimation(self.idle_anim).Play()
			scene.Component_GetAnimation(self.walk_anim).Stop()
			self.state = self.states.STAND
		elseif(self.state == self.states.WALK) then
			scene.Component_GetAnimation(self.idle_anim).Stop()
			scene.Component_GetAnimation(self.walk_anim).Play()
			self.state = self.states.STAND
		elseif(self.state == self.states.JUMP) then
			scene.Component_GetAnimation(self.idle_anim).Play()
			scene.Component_GetAnimation(self.walk_anim).Stop()
			self.state = self.states.STAND
		end
		
		-- check what is below the character
		self.ray = Ray(target_transform.GetPosition(),Vector(0,-1,0))
		local pPrev = self.p
		self.o,self.p,self.n = Pick(self.ray, PICK_OPAQUE, ~self.layerMask)
		if(self.o == INVALID_ENTITY) then
			self.p=pPrev -- if nothing, go back to previous position
		end
		
		-- try to put water ripple if character is directly above water
		local w,wp,wn = Pick(self.ray,PICK_WATER)
		if(w ~= INVALID_ENTITY and self.velocity.Length()>0.1) then
			PutWaterRipple("../Editor/images/ripple.png",wp)
		end
		
		-- add gravity:
		self.velocity = vector.Add(self.velocity, Vector(0,-0.04,0))
		
		-- check if we are below the ground:
		if(model_transform.GetPosition().GetY() <= self.p.GetY() and self.velocity.GetY()<=0) then
			self.state = self.states.STAND
			model_transform.Translate(vector.Subtract(self.p,model_transform.GetPosition()))
			self.velocity.SetY(0) -- don't fall below ground
			self.velocity = vector.Multiply(self.velocity, 0.8) -- slow down gradually on ground
		end
		
		-- apply velocity:
		model_transform.Translate(vector.Multiply(getDeltaTime() * 60, self.velocity))
		
	end
}

-- Third person camera controller class:
ThirdPersonCamera = {
	camera = INVALID_ENTITY,
	character = nil,
	side_offset = 1.4,
	height = 3,
	rest_distance = 6,
	correction_speed = 1.4,
	
	Create = function(self, character)
		self.character = character
		
		self.camera = CreateEntity()
		local camera_transform = scene.Component_CreateTransform(self.camera)
		camera_transform.ClearTransform()
		camera_transform.Translate(Vector(self.side_offset,self.height,-self.rest_distance))
		scene.Component_Attach(self.camera, character.target)
	end,
	
	Update = function(self)
		if(self.character ~= nil) then
			local camera_transform = scene.Component_GetTransform(self.camera)
			local target_transform = scene.Component_GetTransform(self.character.target)
		
			local camTargetDiff = vector.Subtract(target_transform.GetPosition(), camera_transform.GetPosition())
			local camTargetDistance = camTargetDiff.Length()
			
			local use_correction = true

			-- Cast a ray from the camera eye and check if it hits something other than the player...
			local camRay = Ray(camera_transform.GetPosition(),camTargetDiff.Normalize())
			local camCollObj,camCollPos,camCollNor = Pick(camRay, PICK_OPAQUE, ~self.character.layerMask)
			if(camCollObj ~= INVALID_ENTITY) then
				-- It hit something, see if it is between the player and camera:
				local camCollDiff = vector.Subtract(camCollPos, camera_transform.GetPosition())
				local camCollDistance = camCollDiff.Length()
				if(camCollDistance < camTargetDistance) then
					-- If there was something between player and camera, clamp camera position inside:
					camera_transform.Translate(Vector(0,0,camCollDistance-0.1))
					use_correction = false
				end
			end
			
			if(use_correction and camTargetDistance < self.rest_distance) then
				-- Camera is closer to target than rest distance, push it back some amount...
				camera_transform.Translate(vector.Multiply(getDeltaTime() * 60, Vector(0,0,-self.correction_speed)))
			end
			
			local cam = GetCamera()
			cam.TransformCamera(camera_transform)
			cam.UpdateCamera()
			
		end
	end,
}


-- Player Controller
local player = Character
-- Third Person camera
local camera = ThirdPersonCamera

-- Main loop:
runProcess(function()

	-- We will override the render path so we can invoke the script from Editor and controls won't collide with editor scripts
	--	Also save the active component that we can restore when ESCAPE is pressed
	local prevPath = main.GetActivePath()
	local path = RenderPath3D_TiledForward()
	path.SetLightShaftsEnabled(true)
	main.SetActivePath(path)

	LoadModel("../models/playground.wiscene")
	
	player:Create(LoadModel("../models/girl.wiscene"))
	camera:Create(player)
	
	while true do
	
		if(input.Press(VK_ESCAPE)) then
			-- restore previous component
			--	so if you loaded this script from the editor, you can go back to the editor with ESC
			backlog_post("EXIT")
			killProcesses()
			main.SetActivePath(prevPath)
			return
		end

		player:Input()
		
		player:Update()
		
		camera:Update()
		
		-- Wait for Engine update tick
		update()
		
	end
end)




-- Debug draw:

-- Draw Helpers
local DrawAxis = function(point,f)
	DrawLine(point,point:Add(Vector(f,0,0)),Vector(1,0,0,1))
	DrawLine(point,point:Add(Vector(0,f,0)),Vector(0,1,0,1))
	DrawLine(point,point:Add(Vector(0,0,f)),Vector(0,0,1,1))
end

-- Draw
runProcess(function()
	local head = scene.Entity_FindByName("testa")
	local left_hand = scene.Entity_FindByName("mano_L")
	local right_hand = scene.Entity_FindByName("mano_R")
	local left_foot = scene.Entity_FindByName("avampiede_L")
	local right_foot = scene.Entity_FindByName("avampiede_R")
	
	while true do
	
		while( backlog_isactive() ) do
			waitSeconds(1)
		end
		
		local model_transform = scene.Component_GetTransform(player.model)
		local target_transform = scene.Component_GetTransform(player.target)
		
		-- Drawing additional render data (slow, only for debug purposes)
		
		--velocity
		DrawLine(target_transform.GetPosition(),target_transform.GetPosition():Add(player.velocity))
		--face
		DrawLine(target_transform.GetPosition(),target_transform.GetPosition():Add(player.face:Normalize()),Vector(0,1,0,1))
		--intersection
		DrawAxis(player.p,0.5)
		
		-- camera target box and axis
		DrawBox(target_transform.GetMatrix())
		
		-- Head bone
		DrawPoint(scene.Component_GetTransform(head).GetPosition(),0.2, Vector(0,1,1,1))
		-- Left hand bone
		DrawPoint(scene.Component_GetTransform(left_hand).GetPosition(),0.2, Vector(0,1,1,1))
		-- Right hand bone
		DrawPoint(scene.Component_GetTransform(right_hand).GetPosition(),0.2, Vector(0,1,1,1))
		-- Left foot bone
		DrawPoint(scene.Component_GetTransform(left_foot).GetPosition(),0.2, Vector(0,1,1,1))
		-- Right foot bone
		DrawPoint(scene.Component_GetTransform(right_foot).GetPosition(),0.2, Vector(0,1,1,1))
		
		
		-- Wait for the engine to render the scene
		render()
	end
end)
