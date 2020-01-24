#ifndef GOLD_SYSTEM_H
#define GOLD_SYSTEM_H

#include "system.h"
#include "logic/ecs.h"

class GoldSystem : public System {
public:
    
    GoldSystem(Game* game);
    
    void turn();
    
    void calculate();
    
};


#endif
