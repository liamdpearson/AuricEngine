local mesh
local capsule
local rig

local health = 5

local player
local playerCapsule
local playerScript
local toPlayer

local state = 0 -- 0: idle, 1: walking towards player, 2: attacking player, 3: dead
local moveDir
local time = 0
local attackTimer = 0.7

local soundPlayer


function start()
    mesh = self:getMesh()
    capsule = self:getCapsuleCollider()
    rig = self:getRig()
    rig:setAnim(0)

    player = find.obj("player")
    playerCapsule = player:getCapsuleCollider()
    playerScript = player:getScript()

    sound = find.childObj("sound", self)
    soundPlayer = sound:getAudioSource()
end

function hit(damage)
    if state == 3 then
        return
    end

    damage = damage or 1
    health = health - damage
    capsule.velocity = capsule.velocity - toPlayer:normalized() * 10.0
    if health <= 0 then
        state = 3
        time = 0
        soundPlayer.volume = 10
        soundPlayer:Play("assets/sounds/zombiedeath.mp3")
        rig:setAnim(3, 0.1, -2)
    else
        num = math.random(1, 3)
        soundPlayer.volume = 5
        soundPlayer:Play("assets/sounds/zombiehit" .. tostring(num) .. ".mp3")
        rig:setAnim(4, 0.1, state)
    end
end

function attack()
    playerScript.hit(1)
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
    time = time + deltaTime
    
    if state ~= 3 then
        if state == 2 then
            attackTimer = attackTimer - deltaTime
            if attackTimer <= 0.0 then
                attack()
                attackTimer = 1.625
            end
        else
           attackTimer = 1.0
        end

        head = self.transform:pos()
        head.y = head.y + capsule.height - capsule.radius

        playerHead = player.transform:pos()
        playerHead.y = playerHead.y + playerCapsule.height - playerCapsule.radius

        moveDir = Vec3.new(0)

        toPlayer = playerHead - head
        distance = toPlayer:length()

        state = 0
        if distance < 50 then
            if not raycast.occluded(head, playerHead) then
                if not soundPlayer:isPlaying() then
                    soundPlayer.volume = 8
                    soundPlayer:Play("assets/sounds/zombiegroan.mp3")
                end
                moveDir = toPlayer
                moveDir.y = 0.0
                moveDir:normalize()
                
                angle = math.deg(math.atan(moveDir.x / moveDir.z))
                if (moveDir.z < 0.0) then
                    angle = angle + 180.0
                end

                self.transform.yaw = angle

                if distance < 1 then
                    state = 2
                    moveDir = Vec3.new(0)
                else
                    state = 1
                end
            end
        end

        capsule.velocity = capsule.velocity + moveDir * 33 * deltaTime
    end

    resolveMovement(deltaTime)
    
    if rig.currentAnim ~= state and rig.currentAnim ~= 4 then
        rig:setAnim(state, 0.5)
    end
end
