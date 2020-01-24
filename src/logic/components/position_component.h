#ifndef POSITION_COMPONENT_H
#define POSITION_COMPONENT_H

#include <qglobal.h>

class PositionComponent final {
public:
    PositionComponent(uint x = 0, uint y = 0, float height = 0) {
        this->x = x;
        this->y = y;
        this->height = height;
    }
    uint x, y;
    float height;
    
    template<class Archive> 
    void serialize(Archive & ar) 
    { 
        ar(x, y, height); 
    }
};

#endif // POSITION_COMPONENT_H
