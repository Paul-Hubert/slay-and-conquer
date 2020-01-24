#ifndef ATTACKCOMPONENT_H
#define ATTACKCOMPONENT_H

#include <vector>
#include "logic/ecs.h"

#include <cereal/types/vector.hpp>

//Défini si une entité peut attaquer
class AttackComponent {
public:
    bool hidden = true;
    std::vector<Entity> attackers;
    
    template<class Archive> 
    void serialize(Archive & ar) 
    { 
        ar(hidden, attackers); 
    }
};

#endif // ATTACKCOMPONENT_H
