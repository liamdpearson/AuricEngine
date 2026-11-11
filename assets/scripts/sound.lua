local sound

function start()
    testsound = find.obj("test sound")
    sound = testsound:getAudioSource()
end

function update(deltaTime)
    if input.keyPressed(key.Y) then
        sound:Play()
    end
    if input.keyPressed(key.U) then
        sound:Stop()
    end
end
