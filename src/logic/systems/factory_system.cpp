#include "factory_system.h"

#include <random>

#include "logic/game.h"
#include "logic/components/render_component.h"
#include "logic/components/hexagon_component.h"
#include "logic/components/unit_component.h"
#include "logic/components/possession_component.h"
#include "logic/components/player_component.h"
#include "logic/components/city_component.h"
#include "logic/components/ranged_unit_component.h"
#include "logic/components/capital_component.h"
#include "logic/components/position_component.h"

#include <QDebug>
#include <QDateTime>

FactorySystem::FactorySystem(Game* game) : System(game) {
    
}

Entity FactorySystem::createHexagon(uint x, uint y, float height) {
    
    Entity ret = game->reg.create();
    game->reg.assign<HexagonComponent>(ret);
    game->reg.assign<PositionComponent>(ret, x, y, height);
    game->reg.assign<RenderComponent>(ret, RenderComponent::hexagon);
    
    return ret;
}

Entity FactorySystem::createWater(uint x, uint y, float height) {
    
    Entity ret = game->reg.create();
    game->reg.assign<HexagonComponent>(ret, HexagonComponent::water);
    game->reg.assign<PositionComponent>(ret, x, y, height);
    game->reg.assign<RenderComponent>(ret, RenderComponent::water);
    
    return ret;
}

Entity FactorySystem::createBasicUnit(uint x, uint y, Entity owner, uint goldCost) {
    
    
    auto& player = game->reg.get<PlayerComponent>(owner);
    if(player.gold - static_cast<int>(goldCost) < 0) return entt::null;
    
    
    Entity hex = game->board.get(x, y);
    auto& hexComp = game->reg.get<HexagonComponent>(hex);
    
    if(hexComp.unit != entt::null) return entt::null;
    
    
    
    Entity ret = game->reg.create();
    
    game->reg.assign<PossessionComponent>(ret, owner);
    game->reg.assign<PositionComponent>(ret, x, y, game->reg.get<PositionComponent>(game->board.get(x, y)).height);
    game->reg.assign<RotationYRenderComponent>(ret);
    
    player.gold -= goldCost;

    hexComp.unit = ret;
    
    return ret;

}

// range = 2, attack = 6, defence = 1, attackRange = 2, retaliation = 3, dodge = 1, healthPoints = 10, goldCostPerTurn = 1, name

Entity FactorySystem::createWarrior(uint x, uint y, Entity owner) {
    
    Entity ret = createBasicUnit(x, y, owner, 3);
    
    if(ret == entt::null) return ret;
    
    game->reg.assign<UnitComponent>(ret, 2, 6, 2, 1, 5, 1, 10, 1, "Guerrier");
    game->reg.assign<RenderComponent>(ret, RenderComponent::unit);
    
    return ret;
}

Entity FactorySystem::createArcher(uint x, uint y, Entity owner) {
    
    Entity ret = createBasicUnit(x, y, owner, 5);
    
    if(ret == entt::null) return ret;
    
    game->reg.assign<UnitComponent>(ret, 2, 6, 0, 4, 1, 3, 10, 1, "Archer");
    game->reg.assign<RangedUnitComponent>(ret);
    game->reg.assign<RenderComponent>(ret, RenderComponent::unit);
    
    return ret;
}

Entity FactorySystem::createRider(uint x, uint y, Entity owner) {
    
    Entity ret = createBasicUnit(x, y, owner, 5);
    
    if(ret == entt::null) return ret;
    
    game->reg.assign<UnitComponent>(ret, 4, 6, 1, 1, 5, 3, 15, 2, "Cavalier");
    game->reg.assign<RenderComponent>(ret, RenderComponent::unit);
    
    return ret;
}


Entity FactorySystem::createCity(uint x, uint y, Entity owner) {
    
    Entity ret = game->reg.create();
    game->reg.assign<CityComponent>(ret, 3, 3, createWeebName());
    game->reg.assign<PossessionComponent>(ret, owner);

    game->reg.assign<PositionComponent>(ret, x, y, game->reg.get<PositionComponent>(game->board.get(x, y)).height);
    game->reg.assign<RenderComponent>(ret, RenderComponent::castle);

    game->reg.get<HexagonComponent>(game->board.get(x, y)).structure = ret;
    
    return ret;
}

std::string FactorySystem::createRandomName() {
    
    const std::string voy[] = {"a","e","i","o","u","é","è","en","an","eau","oi","un"};
    const uint voy_size = 12;
    const std::string cons[] = {"b","c","ch","d","f","g","h","j","k","l","m","n","p","qu","r","s","t","v","y","z"};
    const uint cons_size = 20;

    std::knuth_b generator1{r()};
    std::knuth_b generator2{r()};
    std::uniform_int_distribution<int> size_rand(2,4);

    int size = size_rand(generator1);

    std::uniform_int_distribution<int> voy_rand(0, voy_size-1);
    auto voy_dice = std::bind(voy_rand, generator1);

    std::uniform_int_distribution<int> cons_rand(0, cons_size-1);
    auto cons_dice = std::bind(cons_rand, generator2);

    std::string ret = "";
    for(int i = 0; i < size; ++i) {
        ret.append(cons[cons_dice()] + voy[voy_dice()]);
    }
    ret[0] = toupper(ret[0]);

    return ret;
}

std::string FactorySystem::createWeebName() {
    
    const std::string romajis[] = {"u", "n'", "i", "shi", "ki", "ni", "ka", "xyo", "no", "chi", "ku", "to", "ha", "ta", "xyu", "ko", "te", "sa", "tsu", "na", "se", "ji", "ga", "ru", "ro", "ri", "ke", "wo", "do", "t", "yo", "ze", "de", "o", "ra", "go", "su", "a", "mo", "da", "me", "ma", "re", "e", "gi", "hi", "ho", "mi", "so", "ne", "ge", "xya", "wa", "hu", "bu", "ya", "n'", "ba", "pa", "ru", "su", "be", "mu", "za", "to", "bi", "i", "he", "yu", "po", "zo", "bo", "ra", "hu", "ku", "do", "zu", "ri", "gu", "re", "zu", "t", "pi", "a", "ta", "ko", "ji", "po", "shi", "ma", "bi", "mu", "e", "ro", "ba", "pu", "chi", "u", "te", "na", "me", "e", "ka", "ni", "o", "de", "ha", "sa", "be", "da", "xyo", "bu", "xyu", "i", "mi", "so", "se", "e", "ne", "ki", "pa", "gu", "no", "zu", "wa", "pe", "xya", "a", "pe", "ke", "ga", "tsu", "mo", "ho", "i", "go", "pi", "pu", "ze", "yu", "nu", "he", "bo", "ya", "o", "za", "hi", "u", "zi", "gi", "a", "nu", "yo", "ge", "u", "o", "wo"};
    const double probs[] = {0.074, 0.069, 0.066, 0.041, 0.031, 0.030, 0.030, 0.029, 0.027, 0.026, 0.026, 0.025, 0.021, 0.020, 0.018, 0.018, 0.017, 0.017, 0.016, 0.015, 0.014, 0.014, 0.014, 0.014, 0.013, 0.012, 0.011, 0.011, 0.010, 0.010, 0.010, 0.010, 0.010, 0.009, 0.008, 0.008, 0.008, 0.008, 0.008, 0.008, 0.007, 0.007, 0.007, 0.007, 0.006, 0.006, 0.005, 0.005, 0.005, 0.005, 0.004, 0.004, 0.004, 0.004, 0.004, 0.004, 0.004, 0.003, 0.003, 0.002, 0.002, 0.002, 0.002, 0.002, 0.002, 0.002, 0.002, 0.002, 0.002, 0.002, 0.002, 0.001, 0.001, 0.001, 0.001, 0.001, 0.001, 0.001, 0.001, 0.001, 0.001, 0.001, 0.001, 0.001, 0.001, 0.001, 0.001, 0.001, 0.001, 0.001, 0.001, 0.001, 0.001, 0.001, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000};
    const uint romajis_size = sizeof(romajis)/sizeof(std::string);
    const double e = 1.-0.967;

    std::chrono::time_point<std::chrono::system_clock> timestamp =
        std::chrono::system_clock::now();


    std::knuth_b generator{r() + QDateTime::currentDateTime().toMSecsSinceEpoch()};
    std::uniform_int_distribution<int> size_rand(2,5);

    int size = size_rand(generator);

    std::uniform_int_distribution<int> rand(0, 1000000);
    auto dice = std::bind(rand, generator);

    std::string ret = "";
    for(int i = 0; i < size; ++i) {
        int index = 0;
        double fl = dice()/1000000.;
        for(uint j = 0; j < romajis_size; j++) {
            fl -= probs[j] + e/romajis_size;
            if(fl < 0) {
                index = j;
                break;
            }
        }
        ret.append(romajis[index]);
        if(dice()/1000000. < 0.01f) ret.append("n");
    }
    ret[0] = toupper(ret[0]);
    
    return ret;
}

Entity FactorySystem::createCapital(uint x, uint y, Entity owner) {
    
    Entity ret = createCity(x,y,owner);
    game->reg.assign<CapitalComponent>(ret);
    game->reg.get<PlayerComponent>(owner).capital = ret;
    
    return ret;
}

Entity FactorySystem::createArrow() {
    
    Entity ret = game->reg.create();
    game->reg.assign<RenderComponent>(ret, RenderComponent::arrow);
    game->reg.assign<RotationYRenderComponent>(ret);
    game->reg.assign<RotationRenderComponent>(ret);
    return ret;
    
}

void FactorySystem::killUnitOnHexagon(Entity hexagon) {
    
    auto& hex = game->reg.get<HexagonComponent>(hexagon);
    game->reg.destroy(hex.unit);
    hex.unit = entt::null;
    
    game->update();
}
void FactorySystem::killUnit(Entity unit) {
    
    killUnitOnHexagon(game->board.get(game->reg.get<PositionComponent>(unit)));
    
}

