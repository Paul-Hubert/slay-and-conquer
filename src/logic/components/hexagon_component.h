#ifndef HEXAGON_COMPONENT_H
#define HEXAGON_COMPONENT_H

#include "logic/ecs.h"

class HexagonComponent final {
public:
    enum Terrain {plains = 0, water = 1};
    
    HexagonComponent(Terrain terrain = plains) : type(terrain) {};
    
    Terrain type;
    
    Entity unit = entt::null;
    Entity structure = entt::null;
    
    template<class Archive> 
    void serialize(Archive & ar) 
    { 
        ar(type, unit, structure);
    }
};

#endif
