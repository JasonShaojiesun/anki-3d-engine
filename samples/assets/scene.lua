local scene = getSceneGraph()
local events = getEventManager()
local rot
local node
local inst
local lcomp

node = scene:newSector("sector0", "samples/assets/sector.ankimesh")
trf = Transform.new()
trf:setOrigin(Vec4.new(0, 0, 0, 0))
rot = Mat3x4.new()
rot:setAll(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0)
trf:setRotation(rot)
trf:setScale(1.16458)
node:getSceneNodeBase():getMoveComponent():setLocalTransform(trf)

node = scene:newModelNode("roomroom-materialnone0", "samples/assets/roomroom-material.ankimdl")
trf = Transform.new()
trf:setOrigin(Vec4.new(0, 0, 0, 0))
rot = Mat3x4.new()
rot:setAll(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0)
trf:setRotation(rot)
trf:setScale(1)
node:getSceneNodeBase():getMoveComponent():setLocalTransform(trf)

node = scene:newModelNode("columnroom-materialnone1", "samples/assets/columnroom-material.ankimdl")
trf = Transform.new()
trf:setOrigin(Vec4.new(5.35225, 5.06618, 5.43441, 0))
rot = Mat3x4.new()
rot:setAll(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0)
trf:setRotation(rot)
trf:setScale(1)
node:getSceneNodeBase():getMoveComponent():setLocalTransform(trf)

node = scene:newPointLight("Point")
lcomp = node:getSceneNodeBase():getLightComponent()
lcomp:setDiffuseColor(Vec4.new(2, 2, 2, 1))
lcomp:setSpecularColor(Vec4.new(2, 2, 2, 1))
lcomp:setRadius(12.77)
trf = Transform.new()
trf:setOrigin(Vec4.new(0.0680842, -0.0302386, 9.57987, 0))
rot = Mat3x4.new()
rot:setAll(1, 0, 0, 0, 0, 4.63287e-05, 1, 0, 0, -1, 4.63287e-05, 0)
trf:setRotation(rot)
trf:setScale(1)
node:getSceneNodeBase():getMoveComponent():setLocalTransform(trf)
lcomp:setShadowEnabled(1)

node = scene:newPerspectiveCamera("Camera")
scene:setActiveCamera(node:getSceneNodeBase())
node:setAll(1.5708, 1.0 / getMainRenderer():getAspectRatio() * 1.5708, 0.1, 100)
trf = Transform.new()
trf:setOrigin(Vec4.new(5.65394, -5.98675, 3.80237, 0))
rot = Mat3x4.new()
rot:setAll(1, 0, 0, 0, 0, 1, -4.63724e-05, 0, 0, 4.63724e-05, 1, 0)
trf:setRotation(rot)
trf:setScale(1)
node:getSceneNodeBase():getMoveComponent():setLocalTransform(trf)
