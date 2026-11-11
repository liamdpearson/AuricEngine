local cam1
local cam2

function start()
    camera = find.obj("camera")
    cam1 = camera:getCamera()
    testcam = find.obj("testcam")
    cam2 = testcam:getCamera()
end

function update(deltaTime)
    if input.keyPressed(key.Q) then
        cam2:setCurrent()
    end
    if input.keyPressed(key.E) then
        cam1:setCurrent()
    end
    if input.keyPressed(key.T) then
        swapScene("assets/scenes/scene1.json")
    end
end
