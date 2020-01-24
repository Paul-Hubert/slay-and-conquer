#ifndef MOVE_SYSTEM_H
#define MOVE_SYSTEM_H

#include "system.h"
#include "logic/ecs.h"
#include "logic/components/position_component.h"

class MoveSystem : public System {
public:
    MoveSystem(Game* game);
    
    bool move(Entity clickedHexagon);
    
    void move(Entity moveHex, Entity targetHex);
    
    void pathfinding();
    
    void pathfinding(Entity selectedUnit);
    
    void hPathFinding(float mp, PositionComponent pos, Entity previous);
    
    float weight(Entity current, Entity target);
};

#endif
