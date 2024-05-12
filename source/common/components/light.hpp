#pragma once 

#include "../ecs/component.hpp" 

#include <glm/mat4x4.hpp> 

namespace our {

    // Enum that defines the type of the light (DIRECTIONAL, POINT, or SPOT) 
    enum class LightType {
        DIRECTIONAL, // Represents a light source emitting light in a specific direction
        POINT,       // Represents a light source emitting light equally in all directions from a point
        SPOT         // Represents a light source emitting light within a cone-shaped region
    };

    // Component representing a light source in the scene
    class LightComponent : public Component { // Inherits from Component class
    public:
        LightType LightType; // Type of the light source (DIRECTIONAL, POINT, or SPOT)
        // glm::vec3 position; // Position of the light (currently unused)
        glm::vec3 direction;  // Direction of the light
        // glm::vec3 color; // Color of the light (currently unused)
        glm::vec3 attenuation; // Attenuation parameters affecting light intensity
        glm::vec2 cone_angles; // Cone angles for spotlight type
        glm::vec3 diffuse;     // Diffuse color of the light
        glm::vec3 specular;    // Specular color of the light
        
        // Static function to get the ID of this component type
        static std::string getID() { return "Light"; } // Returns the ID as "Light"
        
        // Deserialize function to read light parameters from JSON
        void deserialize(const nlohmann::json& data) override;
    };

} // End of namespace 'our'
