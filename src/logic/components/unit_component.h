#ifndef FIGHT_COMPONENT_H
#define FIGHT_COMPONENT_H

#include <cmath>

#include <cereal/types/string.hpp>

class UnitComponent final {
public:

    UnitComponent(float range = 2, float attack = 6, float defence = 1, float attackRange = 1, float retaliation = 3, float dodge = 1, float healthPoints = 10, uint goldCostPerTurn = 1, std::string name = "undefined") {
        this->range = range;
        this->attack = attack;
        this->attackRange = attackRange;
        this->defence = defence;
        this->retaliation = retaliation;
        this->dodge = dodge;
        this->healthPoints = healthPoints;
        this->maxHealthPoints = healthPoints;
        this->goldCostPerTurn = goldCostPerTurn;
        this->name = name;
    }
    
    float range;

    float attack;

    float attackRange;

    float defence;

    float retaliation;

    float dodge;

    float maxHealthPoints;

    float healthPoints;

    uint goldCostPerTurn;
    
    std::string name;
    
    template<class Archive> 
    void serialize(Archive & ar) 
    { 
        ar(range, attack, attackRange, defence, retaliation, dodge, maxHealthPoints, healthPoints, goldCostPerTurn, name); 
    }
};

#endif
