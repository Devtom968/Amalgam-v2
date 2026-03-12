-- Amalgam V2 Master Cheat Script
-- This script demonstrates UI, Engine access and Math libraries

local menu_open = true
local spin_yaw = 0
local spin_speed = 10
local spin_enabled = false

-- This function is called every frame by the cheat
function OnRender()
    -- 1. Create a Cheat Menu using ImGui
    if ui.Begin("Master Cheat Hub") then
        ui.Text("Welcome to Amalgam Lua!")
        ui.Text("Local Player Index: " .. tostring(engine.GetLocalPlayer()))
        
        local connected = engine.IsConnected()
        ui.Text("Status: " .. (connected and "Connected" or "Disconnected"))
        
        ui.SameLine()
        if ui.Button("Disconnect") then
            engine.ExecuteClientCmd("disconnect")
        end
        
        -- Toggle Spinbot
        local _, new_spin = ui.Checkbox("Spinbot Enabled", spin_enabled)
        spin_enabled = new_spin
        
        -- Spin Speed Slider
        local _, new_speed = ui.Slider("Spin Speed", spin_speed, 1, 50)
        spin_speed = new_speed
        
        -- Custom command button
        if ui.Button("Say Hello in Chat") then
            engine.ExecuteClientCmd("say Hello from Master Lua Script!")
        end
        
        ui.End()
    end

    -- 2. Engine Logic: Spinbot Example
    if spin_enabled and engine.IsInGame() then
        local x, y, z = engine.GetViewAngles()
        
        -- Increase yaw by speed
        spin_yaw = (spin_yaw + spin_speed) % 360
        
        -- Set the new view angles
        engine.SetViewAngles(x, spin_yaw, z)
    end
    
    -- 3. Math Test: Floating Text (just print to console for now)
    -- This shows we have math.pi, math.sin etc
    -- local pulse = math.sin(os.clock())
end

print("Master Cheat Script Loaded!")
print("Use ui.Begin to create menus in OnRender.")
