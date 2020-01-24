#ifndef WORLD_GENERATOR_H
#define WORLD_GENERATOR_H

#include "logic/ecs.h"
#include "logic/components/position_component.h"

class Game;

namespace WorldGenerator {
    
    void generate(Game& game);
    
    double distance(PositionComponent pc1, PositionComponent pc2);
    
}

#endif
