#ifndef PATHFINDING_MOVE_COMPONENT_H
#define PATHFINDING_MOVE_COMPONENT_H

#include <logic/ecs.h>

class PathfindingMoveComponent final {
    public :
        float maxWeight;
        Entity previous;
        PathfindingMoveComponent(float maxWeight, Entity previous) : maxWeight(maxWeight), previous(previous) {}
};

#endif // PATHFINDINGMOVECOMPONENT_H
