#ifndef FACTORY_SYSTEM_H
#define FACTORY_SYSTEM_H

#include "system.h"
#include "logic/ecs.h"

#include <QtGlobal>
#include <random>

class FactorySystem : public System {
public:
    
    FactorySystem(Game* game);
    
    /* Création d'entités */
    Entity createHexagon(uint x, uint y, float height);
    Entity createWater(uint x, uint y, float height);
    Entity createWarrior(uint x, uint y, Entity owner);
    Entity createArcher(uint x, uint y, Entity owner);
    Entity createRider(uint x, uint y, Entity owner);
    Entity createCity(uint x, uint y, Entity owner);
    Entity createCapital(uint x, uint y, Entity owner);
    Entity createArrow();
    
    void killUnitOnHexagon(Entity hexagon);
    void killUnit(Entity unit);
    std::string createRandomName();
    std::string createWeebName();

    std::random_device r;
    
private:
    
    Entity createBasicUnit(uint x, uint y, Entity owner, uint goldCost);
};

#endif

