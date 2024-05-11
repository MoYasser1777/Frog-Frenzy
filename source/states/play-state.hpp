#pragma once

#include <application.hpp>

#include <ecs/world.hpp>
#include <systems/forward-renderer.hpp>
#include <systems/free-camera-controller.hpp>
#include <systems/movement.hpp>
#include <asset-loader.hpp>

// This state shows how to use the ECS framework and deserialization.
class Playstate: public our::State {

    our::World world;
    our::ForwardRenderer renderer;
    our::FreeCameraControllerSystem cameraController;
    our::MovementSystem movementSystem;

    void onInitialize() override {
        // First of all, we get the scene configuration from the app config
        auto& config = getApp()->getConfig()["scene"];
        // If we have assets in the scene config, we deserialize them
        if(config.contains("assets")){
            our::deserializeAllAssets(config["assets"]);
        }
        // If we have a world in the scene config, we use it to populate our world
        if(config.contains("world")){
            world.deserialize(config["world"]);
        }
        // We initialize the camera controller system since it needs a pointer to the app
        cameraController.enter(getApp());
        // Then we initialize the renderer
        auto size = getApp()->getFrameBufferSize();
        renderer.initialize(size, config["renderer"]);
    }
    void onImmediateGui() override
    {

        //HEALTH//
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        // Box 1
        ImGui::SetNextWindowPos(ImVec2(20, 20)); // Set position of Box 1
        ImGui::Begin("HealthName", 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
        ImGui::SetWindowSize(ImVec2(250, 80));
        ImGui::SetWindowFontScale(3.0f);
        ImGui::TextColored(ImVec4(0.0, 1.0, 0.0, 1.0), "Your Health: ");
        ImGui::End();

        // // Box 2
        ImGui::SetNextWindowPos(ImVec2(270, 20)); // Set position of Box 2
        ImGui::Begin("HealthValue", 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
        ImGui::SetWindowSize(ImVec2(100, 50));
        ImGui::SetWindowFontScale(3.0f);
        ImGui::TextColored(ImVec4(0.0, 1.0, 0.0, 1.0), std::to_string(getApp()->getLives()).c_str());
        ImGui::End();
        ImGui::PopStyleColor();

        //CHECKPOINT//

        // // Box 3
        ImGui::SetNextWindowPos(ImVec2(500, 20));
        ImGui::Begin("CheckPoint", 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
        ImGui::SetWindowSize(ImVec2(250, 50));
        ImGui::SetWindowFontScale(3.0f);
        ImGui::TextColored(ImVec4(0.0, 1.0, 0.0, 1.0), "Checkpoint");
        ImGui::End();

        // // Box 4
        ImGui::SetNextWindowPos(ImVec2(575, 65));
        ImGui::Begin("CheckPointValue", 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
        ImGui::SetWindowSize(ImVec2(100, 50));
        ImGui::SetWindowFontScale(3.0f);
        std::string checkpointString = std::to_string(getApp()->getChecks()) + "/3";
        ImGui::TextColored(ImVec4(0.0, 1.0, 0.0, 1.0), checkpointString.c_str());
        ImGui::End();

        //TIMER//

        // Box 5
        ImGui::SetNextWindowPos(ImVec2(900, 20));
        ImGui::Begin("Timer", 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
        ImGui::SetWindowSize(ImVec2(150, 50));
        ImGui::SetWindowFontScale(3.0f);
        ImGui::TextColored(ImVec4(0.0, 1.0, 0.0, 1.0), "Timer");
        ImGui::End();

        // // Box 6
        ImGui::SetNextWindowPos(ImVec2(920, 65));
        ImGui::Begin("TimerValue", 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
        ImGui::SetWindowSize(ImVec2(100, 50));
        ImGui::SetWindowFontScale(3.0f);
        std::string TimerString = std::to_string(getApp()->getTimeDiff());
        ImGui::TextColored(ImVec4(0.0, 1.0, 0.0, 1.0), TimerString.c_str());
        ImGui::End();

        //PAUSE//

        ImGui::SetNextWindowPos(ImVec2(1100, 20));
        ImGui::Begin("Pause", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
        ImGui::SetWindowSize(ImVec2(150, 100)); 
        ImGui::SetWindowFontScale(4.0f);
        if (ImGui::Button("||", ImVec2(-1, 80))) { 
                getApp()->setGameState(our::GameState::PAUSE);
                getApp()->setTimeDiffOnPause(getApp()->getTimeDiff());
                getApp()->changeState("pause");
            
            
        }
        ImGui::End();

        
    }
    void onDraw(double deltaTime) override {
        our::GameState state = getApp()->getGameState();
        if (state != our::GameState::PAUSE){
        // Here, we just run a bunch of systems to control the world logic
        movementSystem.update(&world, (float)deltaTime);
        cameraController.update(&world, (float)deltaTime,&renderer);
        // And finally we use the renderer system to draw the scene
        renderer.render(&world);
        }
        // Get a reference to the keyboard object
        auto& keyboard = getApp()->getKeyboard();
        

        if(keyboard.justPressed(GLFW_KEY_ESCAPE)){
            if (getApp()->getGameState() == our::GameState::PLAYING)
            {
                getApp()->setGameState(our::GameState::PAUSE);
                getApp()->setTimeDiffOnPause(getApp()->getTimeDiff());
                // If the escape  key is pressed in this frame, go to the play state
                getApp()->changeState("pause");
            }

        }
        }

    void onDestroy() override {
        // Don't forget to destroy the renderer
        renderer.destroy();
        // On exit, we call exit for the camera controller system to make sure that the mouse is unlocked
        cameraController.exit();
        // Clear the world
        world.clear();
        // and we delete all the loaded assets to free memory on the RAM and the VRAM
        our::clearAllAssets();
   }
};