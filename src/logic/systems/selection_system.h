#ifndef SELECTION_SYSTEM_H
#define SELECTION_SYSTEM_H

#include "system.h"
#include "logic/ecs.h"

class SelectionSystem : public System {
public:
    
    SelectionSystem(Game* game);
    
    void select(Entity clickedHexagon);
    
    void unselect();
    
};

#endif

