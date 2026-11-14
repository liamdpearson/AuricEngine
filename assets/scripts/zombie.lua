local mesh
local capsule
local rig
local health = 3
local player
local playerCapsule
local state = 0 -- 0: idle, 1: walking towards player, 2: attacking player, 3: dead

function start()
    mesh = self:getMesh()
    capsule = self:getCapsuleCollider()
    rig = self:getRig()
    rig:setAnim(0)

    player = find.obj("player")
    playerCapsule = player:getCapsuleCollider()

end

function hit()
    health = health - 1
    if health <= 0 then
        mesh.draw = false
    end
end

function resolveMovement(deltaTime)
    damping = 0.00005 ^ deltaTime
    capsule.velocity.x = capsule.velocity.x * damping
    capsule.velocity.z = capsule.velocity.z * damping

    capsule.velocity.y = capsule.velocity.y - 9.81 * deltaTime
    if (capsule.velocity.y < -50.0) then
        capsule.velocity.y = -50.0
    end

    capsule:resolveCollisions(deltaTime)
end

function update(deltaTime)

    head = self.transform:pos()
    head.y = head.y + capsule.height - capsule.radius

    playerHead = player.transform:pos()
    playerHead.y = playerHead.y + playerCapsule.height - playerCapsule.radius

    moveDir = Vec3.new(0)

    if not raycast.occluded(head, playerHead) then
        moveDir = player.transform:pos() - self.transform:pos()
        moveDir.y = 0.0
        moveDir:normalize()
        
        angle = math.deg(math.atan(moveDir.x / moveDir.z))
        if (moveDir.z < 0.0) then
            angle = angle + 180.0
        end

        self.transform.yaw = angle

        state = 1
    else
        state = 0
    end
    
    capsule.velocity = capsule.velocity + moveDir * 10 * deltaTime

    resolveMovement(deltaTime)

    if rig.currentAnim ~= state then
        rig:setAnim(state, 0.5)
    end
end
