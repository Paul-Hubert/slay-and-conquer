#ifndef POSSESSION_COMPONENT_H
#define POSSESSION_COMPONENT_H

#include <logic/ecs.h>

#include <glm/glm.hpp>


class PossessionComponent final {
public:
    
    Entity owner;
    
    PossessionComponent(Entity index = entt::null) : owner(index) {}
    
    template<class Archive> 
    void serialize(Archive & ar) 
    { 
        ar(owner); 
    }
    
};

#endif
