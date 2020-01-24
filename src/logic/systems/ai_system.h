#ifndef AI_SYSTEM_H
#define AI_SYSTEM_H

#include "logic/systems/system.h"
#include "logic/ecs.h"

class AISystem : public System {
public:
    AISystem(Game* game);
    void turn();
    void action();
};

#endif
