local camnhands
local rig
local capsule
local crosshair
local walking
local ads

function start()
    camnhands = find.obj("camnhands")
    rig = camnhands:getRig()
    capsule = self:getCapsuleCollider()
    rig:setAnim(0)
    crosshair = find.ui("crosshair")
    walking = false
    ads = false
end

function update(deltaTime)
    self.transform.yaw = self.transform.yaw - input.mouseDX() * 0.05
    camnhands.transform.pitch = camnhands.transform.pitch + input.mouseDY() * 0.05
    if camnhands.transform.pitch > 90.0 then
        camnhands.transform.pitch = 90.0
    end

    if camnhands.transform.pitch < -90.0 then
        camnhands.transform.pitch = -90.0
    end

    acceleration = 30.0 * deltaTime

    moveDir = Vec3.new(0)

    forward = Vec3.new(
        -math.sin(math.rad(self.transform.yaw)),
        0.0,
        -math.cos(math.rad(self.transform.yaw))
    )

    right = Vec3.new(
        -math.sin(math.rad(self.transform.yaw + 90.0)),
        0.0,
        -math.cos(math.rad(self.transform.yaw + 90.0))
    )

    if input.keyHeld(key.W) then
        moveDir = moveDir + forward
    end
    if input.keyHeld(key.A) then
        moveDir = moveDir + right
    end
    if input.keyHeld(key.S) then
        moveDir = moveDir - forward
    end
    if input.keyHeld(key.D) then
        moveDir = moveDir - right
    end
    if input.keyHeld(key.LSHIFT) then
        acceleration = acceleration * 1.5
    end

    pos = self.transform:pos()
    pos.y = pos.y + 0.1
    down = Vec3.new(0, -1, 0)
    grounded = raycast.occluded(pos, down, 0.2)
    if input.keyPressed(key.SPACE) and grounded then
        capsule.velocity.y = capsule.velocity.y + 5.0
    end

    moveDirLen = moveDir:length()
    if (moveDirLen > 0.0) then
        moveDir:normalize()
        rig.animSpeed = 1.5
        walking = true
    else
        rig.animSpeed = 1.0
        walking = false
    end

    capsule.velocity.y = capsule.velocity.y - 9.81 * deltaTime
    if (capsule.velocity.y < -50.0) then
        capsule.velocity.y = -50.0
    end

    damping = 0.00005 ^ deltaTime
    capsule.velocity.x = capsule.velocity.x * damping
    capsule.velocity.z = capsule.velocity.z * damping
    capsule.velocity = capsule.velocity + moveDir * acceleration

    capsule:resolveCollisions(deltaTime)

    if input.mousePressed(mouse.RIGHT) then
        ads = true
        crosshair.draw = false
    end
    if input.mouseReleased(mouse.RIGHT) then
        ads = false
        crosshair.draw = true
    end

    w = 0
    if walking then
        w = 1
    end
    a = 0
    if ads then
        a = 1
    end
    shouldBe = w + a * 2
    if rig.currentAnim ~= shouldBe then
        rig:setAnim(shouldBe, 0.1)
    end
end
