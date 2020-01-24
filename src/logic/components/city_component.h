#ifndef CITY_COMPONENT_H
#define CITY_COMPONENT_H

#include <cmath>
#include <string>

#include <cereal/types/string.hpp>

class CityComponent {
public:
    CityComponent(int goldPerTurn = 3, uint populationNeededForNextLevel = 3, std::string name = "") {
        this->goldPerTurn = goldPerTurn;
        this->populationNeededForNextLevel = populationNeededForNextLevel;
        this->name = name;
    }
    
    int goldPerTurn;
    uint populationNeededForNextLevel;
    int population = 0;
    std::string name;
    
    template<class Archive> 
    void serialize(Archive & ar) 
    { 
        ar(goldPerTurn, populationNeededForNextLevel, population, name);
    }
};

#endif // STRUCTURECOMPONENT_H
