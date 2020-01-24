#include "ai_system.h"

#include "logic/game.h"

#include "logic/components/hexagon_component.h"
#include "logic/components/unit_component.h"
#include "logic/components/position_component.h"
#include "logic/components/player_component.h"
#include "logic/components/possession_component.h"
#include "logic/components/city_component.h"
#include "logic/components/attack_component.h"
#include "logic/components/move_component.h"
#include "logic/components/ranged_unit_component.h"
#include "logic/components/ai_component.h"
#include "logic/components/pathfinding_move_component.h"
#include "logic/components/pathfinding_attack_component.h"
#include "logic/components/marker_component.h"

#include "logic/systems/render_system.h"

#include "util/position_converter.h"

#include <random>

AISystem::AISystem(Game* game) : System(game) {}

void AISystem::turn() {
    
    action();
    
}


void AISystem::action() {
    
    auto& player = game->reg.get<PlayerComponent>(game->currentPlayer());
    
    { // iterate on units
    
        auto units = game->reg.view<UnitComponent, PossessionComponent, PositionComponent>();
        
        for(auto unit : units) {
            if(units.get<PossessionComponent>(unit).owner == game->currentPlayer() && (game->reg.has<MoveComponent>(unit) || game->reg.has<AttackComponent>(unit))) {
                // only owned units
                
                auto pos = units.get<PositionComponent>(unit);

                game->selectionSystem.select(game->board.get(pos)); // select unit
                game->moveSystem.pathfinding();
                game->attackSystem.pathfinding();
                
                Entity closest = entt::null;
                float minEnemy = 10000000.f;
                
                
                // get closest enemy
                auto enemyUnits = game->reg.view<UnitComponent, PossessionComponent, PositionComponent>();
                for(auto enemyUnit : enemyUnits) {
                    if(enemyUnits.get<PossessionComponent>(enemyUnit).owner != game->currentPlayer()) {
                        // only enemy units
                        float distance2 = game->board.distance2(pos, enemyUnits.get<PositionComponent>(enemyUnit));
                        
                        if(minEnemy > distance2) {
                            minEnemy = distance2;
                            closest = enemyUnit;
                        }
                        
                    }
                }
                
                
                    
                // get closest enemy city
                auto enemyCities = game->reg.view<CityComponent, PossessionComponent, PositionComponent>();
                for(auto enemyCity : enemyCities) {
                    if(enemyCities.get<PossessionComponent>(enemyCity).owner != game->currentPlayer()) {
                        // only enemy units
                        float distance2 = game->board.distance2(pos, enemyCities.get<PositionComponent>(enemyCity));
                        
                        if(minEnemy > distance2) {
                            minEnemy = distance2;
                            closest = enemyCity;
                            
                        }
                        
                    }
                }
                
                if(closest == entt::null) {
                    continue;
                }
                
                {
                    auto view = game->reg.view<PathfindingMoveComponent, PositionComponent>();
                    
                    Entity best = entt::null;
                    float min = 1000.f;
                    for(auto hex : view) {
                        float dist2 = game->board.distance2(view.get<PositionComponent>(hex), game->reg.get<PositionComponent>(closest));
                        if(min > dist2) {
                            min = dist2;
                            best = hex;
                        }
                    }
                    
                    
                    
                    if(best != entt::null) {
                        auto newPos = game->reg.get<PositionComponent>(best);
                        if(!(newPos.x == pos.x && newPos.y == pos.y)) {
                            game->render->setCameraPos(PositionConverter::hexToOrtho(newPos));
                            game->moveSystem.move(game->board.get(newPos));
                            game->selectionSystem.unselect();
                            return;
                        }
                    }
                }
                
                
                {
                    auto view = game->reg.view<PathfindingAttackComponent, HexagonComponent>();
                    
                    Entity best = entt::null;
                    float min = 1000.f;
                    for(auto hex : view) {
                        auto& hp = game->reg.get<UnitComponent>(view.get<HexagonComponent>(hex).unit).healthPoints;
                        if(min > hp) {
                            min = hp;
                            best = hex;
                        }
                    }
                    
                    
                    
                    if(best != entt::null) {
                        auto newPos = game->reg.get<PositionComponent>(best);
                        if(!(newPos.x == pos.x && newPos.y == pos.y)) {
                            game->render->setCameraPos(PositionConverter::hexToOrtho(newPos));
                            game->attackSystem.attack(game->board.get(newPos));
                            game->selectionSystem.unselect();
                            return;
                        }
                    }
                    
                }
                
                /*
                auto view = game->reg.view<PathfindingMoveComponent>();
                
                for(auto hex : view) {
                    if(!game->reg.has<MarkerComponent>(hex)) game->reg.assign<MarkerComponent>(hex, game->reg.get<PlayerComponent>(game->currentPlayer()).index+1);
                }*/
                
            }
        }
    
    }
    
    
    
    
    { // iterate on towns, if no unit can move
    
        auto cities = game->reg.view<CityComponent, PossessionComponent, PositionComponent>();
        
        for(auto city : cities) {
            auto owner = cities.get<PossessionComponent>(city).owner;
            if(owner == game->currentPlayer()) {
                
                auto pos = cities.get<PositionComponent>(city);
                
                if(game->reg.get<HexagonComponent>(game->board.get(pos)).unit == entt::null) {
                    if(player.goldPlus > player.goldMinus+2) {
                        
                        std::knuth_b generator{game->factorySystem.r()};
                        std::uniform_int_distribution<int> size_rand(1,3);
                        int index = size_rand(generator);
                        qDebug() << index;
                        
                        if(index == 1) {
                            game->factorySystem.createWarrior(pos.x, pos.y, owner);
                        } else if(index == 2) {
                            game->factorySystem.createArcher(pos.x, pos.y, owner);
                        } else if(index == 3) {
                            game->factorySystem.createRider(pos.x, pos.y, owner);
                        }
                    }
                }
                
            }
        }
        
    }
    
    
    game->selectionSystem.unselect();
    
    game->toTurn = true;
    
}
