local mesh
local capsule
local rig
local health = 3
local player

function start()
    mesh = self:getMesh()
    capsule = self:getCapsuleCollider()
    rig = self:getRig()
    rig:setAnim(0)
    player = find.obj("player")
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
    moveDir = player.transform:pos() - self.transform:pos()
    moveDir:normalize()

    capsule.velocity = capsule.velocity + moveDir * 30 * deltaTime

    resolveMovement(deltaTime)
end
