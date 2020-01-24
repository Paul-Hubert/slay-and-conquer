#ifndef BOARD_H
#define BOARD_H

#include "logic/ecs.h"

#include "components/position_component.h"

#include <vector>

#include "cereal/types/vector.hpp"

class Board {

public:
    Board(unsigned int width, unsigned int height);
    ~Board();
    
    unsigned int width;
    unsigned int height;
    
    Entity get(unsigned int x, unsigned int y);
    Entity get(PositionComponent comp);
    
    void set(unsigned int x, unsigned int y, Entity e);
    void set(PositionComponent comp, Entity e);
    
    std::vector<PositionComponent> neighbours(PositionComponent pos);
    
    float distance(PositionComponent a, PositionComponent b);
    float distance2(PositionComponent a, PositionComponent b);
    
    template <class Archive>
    void serialize(Archive& ar) {
        ar(grid, width, height);
    }

private:
    std::vector<Entity> grid;
};

#endif
