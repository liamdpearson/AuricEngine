local ammo
local rig
local zombies = {}

function start()
    rig = self:getRig()
    rig:setAnim(0)
    ammo = 17

    cam = find.obj("camera"):getCamera()
    for _, z in ipairs(find.allWithTag("zombie")) do
        table.insert(zombies, { obj = z, capsule = z:getCapsuleCollider(), script = z:getScript() })
    end
end

local function shoot()
    local origin = cam.pos
    local dir = cam.front:normalized()

    for _, z in ipairs(zombies) do
        local d = raycast.hitCapsule(origin, dir, 100, z.capsule)
        if d and not raycast.occluded(origin, dir, d) then
            z.script.hit()
        end
    end
end

function update(deltaTime)
    if input.mousePressed(mouse.LEFT) and rig.currentAnim ~= 5 then
        if ammo > 1 then
            rig:setAnim(2, 0.01, 0)
            ammo = ammo - 1
            playSound2D("assets/sounds/gunshot.mp3", 1.0)
            shoot()
        elseif ammo == 1 then
            rig:setAnim(3, 0.01, 1)
            ammo = ammo - 1
            playSound2D("assets/sounds/gunshot.mp3", 1.0)
            shoot()
        else
            rig:setAnim(4, 0.05, 1)
            playSound2D("assets/sounds/noammo.wav", 0.3)
        end
    end

    if input.keyPressed(key.R) and rig.currentAnim ~= 5 then
        rig:setAnim(5, 0.05, 0)
        ammo = 17
    end
end
