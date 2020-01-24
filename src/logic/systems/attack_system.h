#ifndef ATTACK_SYSTEM_H
#define ATTACK_SYSTEM_H

#include "system.h"
#include "logic/ecs.h"
#include "logic/components/position_component.h"

class AttackSystem : public System {
public:
    AttackSystem(Game* game);
    
    bool attack(Entity clicked);
    
    void firstAttack(Entity attacker, Entity attacked, Entity attackerHexagon, Entity attackedHexagon);
    
    void secondAttack(Entity attacker, Entity attacked);
    
    void pathfinding();
    
    void pathfinding(Entity selectedUnit);
    
    void hPathFinding(float mp, PositionComponent pos);
    
    float weight(Entity current, Entity target);
};

#endif
