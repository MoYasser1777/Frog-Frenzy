#pragma once

#include "../ecs/world.hpp"
#include "../components/camera.hpp"
#include "../components/free-camera-controller.hpp"
#include "../components/movement.hpp"

#include "../application.hpp"
#include "forward-renderer.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>
#include <glm/gtx/fast_trigonometry.hpp>

#include <thread>
#include <chrono>
#include <irrKlang.h>
using namespace irrklang;

namespace our
{

    // The free camera controller system is responsible for moving every entity which contains a FreeCameraControllerComponent.
    // This system is added as a slightly complex example for how use the ECS framework to implement logic. 
    // For more information, see "common/components/free-camera-controller.hpp"
    class FreeCameraControllerSystem {
        Application* app; // The application in which the state runs
        bool mouse_locked = false; // Is the mouse locked
        int maxHeightWin = 10;
        bool frogAboveTrunk = false;
        bool frogAboveGrass = false;
        float levelWidth = 17.0f; 
        float levelStart = 15.0f;                                     // The start of the level
        float levelEnd[5] = {-200.0f, -16.0f, -16.0f, -16.0f, -25.0f}; // The end of the levels
        int enteredStars = 1; 
        float lastTimeTakenPostPreprocessed = 0.0f;
        ForwardRenderer *renderer = nullptr; 
        Entity * skull= nullptr;
        std::vector<Entity *> stars;
        bool repositionFrogCheck = false;
        bool validStar[2]={true, true};
        MovementComponent * skullMover = nullptr; 
        bool skullMoving = false;
        
    public:
        // When a state enters, it should call this function and give it the pointer to the application
        void enter(Application* app){
            this->app = app;
            ISoundEngine *soundEngine = app->getSoundEngine();
            if (soundEngine == nullptr)
                app->setSoundEngine(createIrrKlangDevice()); 
            
        }

        // This should be called every frame to update all entities containing a FreeCameraControllerComponent 
        void update(World* world, float deltaTime,ForwardRenderer *renderer) {
            // First of all, we search for an entity containing both a CameraComponent and a FreeCameraControllerComponent
            // As soon as we find one, we break
            this->renderer = renderer;
            CameraComponent* camera = nullptr;
            FreeCameraControllerComponent *controller = nullptr;
            for(auto entity : world->getEntities()){
                camera = entity->getComponent<CameraComponent>();
                controller = entity->getComponent<FreeCameraControllerComponent>();
                if(camera && controller) break;
            }
            // If there is no entity with both a CameraComponent and a FreeCameraControllerComponent, we can do nothing so we return
            if(!(camera && controller)) return;
            // Get the entity that we found via getOwner of camera (we could use controller->getOwner())
            Entity* entity = camera->getOwner();

            // If the left mouse button is pressed, we lock and hide the mouse. This common in First Person Games.
            if(app->getMouse().isPressed(GLFW_MOUSE_BUTTON_1) && !mouse_locked){
                app->getMouse().lockMouse(app->getWindow());
                mouse_locked = true;
            // If the left mouse button is released, we unlock and unhide the mouse.
            } else if(!app->getMouse().isPressed(GLFW_MOUSE_BUTTON_1) && mouse_locked) {
                app->getMouse().unlockMouse(app->getWindow());
                mouse_locked = false;
            }

            // We get a reference to the entity's position and rotation
            glm::vec3& position = entity->localTransform.position;
            glm::vec3& rotation = entity->localTransform.rotation;

            // If the left mouse button is pressed, we get the change in the mouse location
            // and use it to update the camera rotation
            if(app->getMouse().isPressed(GLFW_MOUSE_BUTTON_1)){
                glm::vec2 delta = app->getMouse().getMouseDelta();
                rotation.x -= delta.y * controller->rotationSensitivity; // The y-axis controls the pitch
                rotation.y -= delta.x * controller->rotationSensitivity; // The x-axis controls the yaw
            }

            // We prevent the pitch from exceeding a certain angle from the XZ plane to prevent gimbal locks
            if(rotation.x < -glm::half_pi<float>() * 0.99f) rotation.x = -glm::half_pi<float>() * 0.99f;
            if(rotation.x >  glm::half_pi<float>() * 0.99f) rotation.x  = glm::half_pi<float>() * 0.99f;
            // This is not necessary, but whenever the rotation goes outside the 0 to 2*PI range, we wrap it back inside.
            // This could prevent floating point error if the player rotates in single direction for an extremely long time. 
            rotation.y = glm::wrapAngle(rotation.y);

            // We update the camera fov based on the mouse wheel scrolling amount
            float fov = camera->fovY + app->getMouse().getScrollOffset().y * controller->fovSensitivity;
            fov = glm::clamp(fov, glm::pi<float>() * 0.01f, glm::pi<float>() * 0.99f); // We keep the fov in the range 0.01*PI to 0.99*PI
            camera->fovY = fov;

            // We get the camera model matrix (relative to its parent) to compute the front, up and right directions
            glm::mat4 matrix = entity->localTransform.toMat4();

            glm::vec3 front = glm::vec3(matrix * glm::vec4(0, 0, -1, 0)),
                      up = glm::vec3(matrix * glm::vec4(0, 1, 0, 0)), 
                      right = glm::vec3(matrix * glm::vec4(1, 0, 0, 0));

            glm::vec3 current_sensitivity = controller->positionSensitivity;
            // If the LEFT SHIFT key is pressed, we multiply the position sensitivity by the speed up factor
            if(app->getKeyboard().isPressed(GLFW_KEY_LEFT_SHIFT)) current_sensitivity *= controller->speedupFactor;

            // We change the camera position based on the keys WASD/QE
            // S & W moves the player back and forth
            if(app->getKeyboard().isPressed(GLFW_KEY_W)) position += front * (deltaTime * current_sensitivity.z);
            if(app->getKeyboard().isPressed(GLFW_KEY_S)) position -= front * (deltaTime * current_sensitivity.z);
            // Q & E moves the player up and down
            if(app->getKeyboard().isPressed(GLFW_KEY_Q)) position += up * (deltaTime * current_sensitivity.y);
            if(app->getKeyboard().isPressed(GLFW_KEY_E)) position -= up * (deltaTime * current_sensitivity.y);
            // A & D moves the player left or right 
            if(app->getKeyboard().isPressed(GLFW_KEY_D)) position += right * (deltaTime * current_sensitivity.x);
            if(app->getKeyboard().isPressed(GLFW_KEY_A)) position -= right * (deltaTime * current_sensitivity.x);

            Entity * frog = nullptr;
            Entity *woodenBox = nullptr;
            Entity *brickWall = nullptr;
            Entity *pipe = nullptr;

            std::vector<Entity *> logs;
            std::vector<Entity *> cars;
            std::vector<Entity *> water;
            std::vector<Entity *> mazeGrass;

            

            for (auto entity : world->getEntities())
            {
                std::string name = entity->name;
                 if (name == "frog")
                {
                    frog = entity;
                }else if (name == "log")
                {
                    logs.push_back(entity);
                }else if (name == "woodenBox")
                {
                    woodenBox = entity;
                }else if (name == "brickWall")
                {
                    brickWall = entity;
                }else if (name == "pipe")
                {
                    pipe = entity;
                }else if (name == "floatingCar" || name == "floatingCarReversed")
                {
                    cars.push_back(entity);
                }else if (name == "water")
                {
                    water.push_back(entity);
                }else if (name == "mazeGrass")
                {
                    mazeGrass.push_back(entity);
                }else if (name == "star")
                {
                    stars.push_back(entity);
                }else if (name == "skull"){
                    skull = entity;
                    skullMover = entity->getComponent<MovementComponent>();
                }
                
            }

            

            

            if (!frog)
                return;
            if (app->getGameState() == GameState::WIN)
            {
                // make wooden box flying when collision with frog
                glm::vec3 deltaPosition = glm::vec3(0.0f, 5 * deltaTime , 0.0f); 
                woodenBox->localTransform.position += deltaPosition; // update position of wooden box
                frog->localTransform.position += deltaPosition;      // update position of frog
                frog->localTransform.rotation += deltaPosition;
                frog->localTransform.rotation += deltaPosition;
                position += deltaPosition;                           // update position of camera

                if (position.y >= maxHeightWin)
                {
                    app->changeState("win");
                    
                }
                return;
            }

            if (skull->localTransform.position.y >= 0.1f && skullMoving)
            {  
                skullMoving = false;
                lastTimeTakenPostPreprocessed = (float)glfwGetTime();                  
            }

            if (app->getGameState() == GameState::GAME_OVER && !skullMoving)
            {
                restartCheckpoint(world);
                return;
            }



            // frog movement
            if ((app->getKeyboard().isPressed(GLFW_KEY_UP) || app->getKeyboard().isPressed(GLFW_KEY_DOWN) || app->getKeyboard().isPressed(GLFW_KEY_LEFT) || app->getKeyboard().isPressed(GLFW_KEY_RIGHT)) && !skullMoving)
            {
                // // MOVING   =>  Jump Effect
                frog->localTransform.position.y = float(0.05f * sin(glfwGetTime() * 10) + 0.05f) - 1.05f;           // make the frog jump

                // UP
                if (app->getKeyboard().isPressed(GLFW_KEY_UP))
                {
                    if (frog->localTransform.position.z < levelEnd[0] &&  !(frog->localTransform.position.x - woodenBox->localTransform.position.x < 1.0f &&
                frog->localTransform.position.x - woodenBox->localTransform.position.x > -1.0f) )
                        return;
                     // update the camera position
                    position += front * (deltaTime * current_sensitivity.z);
                    // update the frog position
                    frog->localTransform.position += front * (deltaTime * current_sensitivity.z);
                    // update the frog direction
                    frog->localTransform.rotation.y = glm::pi<float>();
                }
                // DOWN
                else if (app->getKeyboard().isPressed(GLFW_KEY_DOWN))
                {
                    if (frog->localTransform.position.z > levelStart)
                        return;
                    position -= front * (deltaTime * current_sensitivity.z);
                    frog->localTransform.position -= front * (deltaTime * current_sensitivity.z);
                    frog->localTransform.rotation.y = 0;
                }
                // RIGHT
                else if (app->getKeyboard().isPressed(GLFW_KEY_RIGHT))
                {
                    if (frog->localTransform.position.x > levelWidth / 2 )
                        return;
                    position += right * (deltaTime * current_sensitivity.x);
                    frog->localTransform.position += right * (deltaTime * current_sensitivity.x);
                    frog->localTransform.rotation.y = glm::pi<float>() * 0.5f;
                }
                // LEFT
                else if (app->getKeyboard().isPressed(GLFW_KEY_LEFT))
                {
                    if (frog->localTransform.position.x < -levelWidth / 2)
                        return;
                    position -= right * (deltaTime * current_sensitivity.x);
                    frog->localTransform.position -= right * (deltaTime * current_sensitivity.x);
                    frog->localTransform.rotation.y = glm::pi<float>() * -0.5f;
                }

            }
            frogAboveTrunk = false;
            for (auto log : logs)
            {
                if ((frog->localTransform.position.x < log->localTransform.position.x + log->localTransform.scale[0] &&
                    frog->localTransform.position.x > log->localTransform.position.x - log->localTransform.scale[0] &&
                    frog->localTransform.position.z < log->localTransform.position.z + log->localTransform.scale[2] &&
                    frog->localTransform.position.z > log->localTransform.position.z - log->localTransform.scale[2]) && !skullMoving)
                {
                    frogAboveTrunk = true;
                    MovementComponent * movement = log->getComponent<MovementComponent>();
                    // take frog right and make it with limits from right
                    if(movement->id == "right" && frog->localTransform.position.x < levelWidth / 2  ) { 
                        position += right * (deltaTime * movement->linearVelocity.x);
                        frog->localTransform.position += deltaTime * movement->linearVelocity; 
                        
                    }
                    // take frog left and make it with limits from left
                    else if (frog->localTransform.position.x > -levelWidth / 2){  
                        position -= right * (deltaTime * movement->linearVelocity.x);
                        frog->localTransform.position -= deltaTime * movement->linearVelocity;
                    }
                }
            }
            frogAboveGrass = false;
            for (auto maze : mazeGrass)
            {
                if (frog->localTransform.position.x < maze->localTransform.position.x + maze->localTransform.scale[1] &&
                    frog->localTransform.position.x > maze->localTransform.position.x - maze->localTransform.scale[1] &&

                    frog->localTransform.position.z < maze->localTransform.position.z + maze->localTransform.scale[0] &&
                    frog->localTransform.position.z > maze->localTransform.position.z - maze->localTransform.scale[0])
                {
                    frogAboveGrass = true;
                }
            }


             if (!frogAboveGrass && !frogAboveTrunk && !skullMoving)
                for (auto wat : water)
                {
                    if (frog->localTransform.position.z  < (wat->localTransform.scale[1]) + wat->localTransform.position.z &&
                    frog->localTransform.position.z   >  wat->localTransform.position.z - (wat->localTransform.scale[1]) ){

                        playAudio("splash.mp3");
                    
                    //  if(app->getLives() == 1)
                    //  {
                    //     app->changeState("lose");
                    //     return;
                    // }else{
                    //     gameOver();
                    // }
                    
                    }
                        
                }

                // check if a car hits the frog
            for (auto car : cars)
            {
                glm::mat4 carTransformationMatrix = car->getLocalToWorldMatrix();
                glm::vec3 carPosition = glm::vec3(carTransformationMatrix[3]);
                bool carRight= frog->localTransform.position.x < carPosition.x + 0.5f &&
                    frog->localTransform.position.x > carPosition.x - 2.0f &&
                    frog->localTransform.position.z < carPosition.z + 0.5f &&
                    frog->localTransform.position.z > carPosition.z - 1.1f;


                bool carLeft=frog->localTransform.position.x < carPosition.x + 2.0f &&
                    frog->localTransform.position.x > carPosition.x - 0.5f &&
                    frog->localTransform.position.z < carPosition.z + 0.75f &&
                    frog->localTransform.position.z > carPosition.z - 0.85f;
                if ((car->name =="floatingCar" && carRight )  || (car->name=="floatingCarReversed" && carLeft ))
                {
                        if(app->getLives() == 1){
                            app->changeState("lose");
                            return;

                        }else{
                            playAudio("car.mp3");
                            gameOver();
                        }
                    
                }
            }

            if (app->getTimeDiff() <= 0)
            {
                app->changeState("lose");
                return;
            }
            if (skullMoving == true)
            {       
                skull->localTransform.position += deltaTime * skullMover->linearVelocity; 
                skull->localTransform.rotation += deltaTime * skullMover->angularVelocity;                          
            }
            if(repositionFrogCheck){
                repositionFrog(frog,position, world);
                repositionFrogCheck = false;
            }
            for (auto star : stars)
            {
                if ((int(frog->localTransform.position.z) == int(star->localTransform.position.z)) &&
                 (int(frog->localTransform.position.x) == int(star->localTransform.position.x)))
                {
                
                    world->markForRemoval(star); //? removing star after collision detection
                    world->deleteMarkedEntities();
                    //star->localTransform.position.y = -5;
                    // if(validStar[app->getChecks()-1]){
                    playAudio("stars.mp3");      //? playing audio at collision detection
                    renderer->effectTwo = true;   
                    lastTimeTakenPostPreprocessed = (float)glfwGetTime();             
                    app->upgradeCheck();
                    //validStar[app->getChecks()-1] = false;
                    // }
                    
                }
            }
            if (glfwGetTime() - lastTimeTakenPostPreprocessed >= 0.25f && renderer->effectTwo)
            {
                renderer->effectTwo = false;
                lastTimeTakenPostPreprocessed = 0.0f;
            }


            if (
                frog->localTransform.position.z - woodenBox->localTransform.position.z < 1.0f &&
                frog->localTransform.position.z - woodenBox->localTransform.position.z > -1.0f &&
                frog->localTransform.position.x - woodenBox->localTransform.position.x < 1.0f &&
                frog->localTransform.position.x - woodenBox->localTransform.position.x > -1.0f)
            {
                playAudio("riseGlory.mp3");
                app->setGameState(GameState::WIN);
            }



        }

        void playAudio(std::string audioFileName, bool repeat = false, bool stopAll = false)
        {
            ISoundEngine *soundEngine = app->getSoundEngine();
            std::string audioPath = "sounds/" + audioFileName;
            if (!soundEngine)
                return;
            if (stopAll)
            {
                soundEngine->stopAllSounds();
            }
            if (!soundEngine->isCurrentlyPlaying(audioPath.c_str()))
            {
                // repeat is a boolean
                // when this boolean is true the audio repeats after it's finished
                soundEngine->play2D(audioPath.c_str(), repeat);
            }
        }


        void restartCheckpoint(World *world)
        {
            this->renderer->effectOne = false;
            //std::this_thread::sleep_for(std::chrono::milliseconds(3000));

            app->setGameState(GameState::PLAYING);
            int currentLives = app->getLives();
            
            //app->setLives(currentLives - 1);

            //auto &config = app->getConfig()["scene"];
            // If we have a world in the scene config, we use it to populate our world
            // if(config.contains("world")){
            //     world->clear();
            //     world->deserialize(config["world"]);
            repositionFrogCheck = true;
        }
        void repositionFrog( Entity * frog, glm::vec3 & position, World* world){
                int currentCheck = app->getChecks();
                if(currentCheck==1){
                    frog->localTransform.position.z = 10;
                    frog->localTransform.position.x = 0;
                }else if(currentCheck == 2){
                    frog->localTransform.position.z = -9;
                    frog->localTransform.position.x = 3;
                }else if(currentCheck == 3){
                    frog->localTransform.position.z = -39;
                    frog->localTransform.position.x = 0;
                }
                frog->localTransform.rotation.y = glm::pi<float>();
                position = frog->localTransform.position;
                position.y += 2;
                position.z += 3;
                skull->localTransform.position.y = position.y - 4;
                skull->localTransform.position.z = position.z - 18;
                skull->localTransform.rotation.z = 0;
                skullMoving = false;
        }
        //  When the frog hits the water, collides with a car, or runs out of time, the game is over.
        void gameOver()
        {

            this->renderer->effectOne = true;
            skullMoving = true;

            lastTimeTakenPostPreprocessed = (float)glfwGetTime();
            
            app->setGameState(GameState::GAME_OVER);

            playAudio("die.mp3");
        }
        // When the state exits, it should call this function to ensure the mouse is unlocked
        void exit(){
            if(mouse_locked) {
                mouse_locked = false;
                app->getMouse().unlockMouse(app->getWindow());
            }
        }

    };

}
