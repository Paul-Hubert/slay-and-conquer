#ifndef PLAYER_COMPONENT_H
#define PLAYER_COMPONENT_H

#include <cmath>
#include "imgui/imgui.h"
#include "logic/ecs.h"

class PlayerComponent final {
public:
    
    uint index;
    ImVec4 color;
    
    PlayerComponent(uint index = 0, ImVec4 color = ImVec4(0,0,0,0), Entity capital = entt::null) : index(index), color(color), capital(capital) {}
    
    int gold = 5;
    int goldPlus = 0; //Généré par les villes
    int goldMinus = 0; //Consommé par les unités

    Entity capital;
    
    glm::vec3 cameraPos;
    
    template<class Archive> 
    void serialize(Archive & ar) 
    {
        ar(index, color.x, color.y, color.z, color.w, gold, capital, cameraPos.x, cameraPos.y, cameraPos.z); 
    }
};

#endif
