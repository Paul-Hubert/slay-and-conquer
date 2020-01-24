#ifndef ANIMATION_SYSTEM_H
#define ANIMATION_SYSTEM_H

#include "system.h"
#include "logic/ecs.h"

#include <vector>

#include <glm/glm.hpp>

class AnimationSystem : public System {
public:
    
    AnimationSystem(Game* game);
    
    void tick(float delta);
    
    void construction(Registry& reg, Entity ent);
    
};

#endif
