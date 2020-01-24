#include "attack_system.h"

#include <QDebug>

#include "logic/game.h"

#include "util/position_converter.h"
#include "logic/components/hexagon_component.h"
#include "logic/components/unit_component.h"
#include "logic/components/attack_component.h"
#include "logic/components/move_component.h"
#include "logic/tags/selected_tag.h"
#include "logic/components/possession_component.h"
#include "logic/components/pathfinding_attack_component.h"
#include "logic/components/render_update_component.h"
#include "logic/components/animation_components.h"
#include "logic/components/ranged_unit_component.h"
#include "logic/components/pathfinding_move_component.h"
#include "logic/components/destroy_component.h"

AttackSystem::AttackSystem(Game* game) : System(game) {}

bool AttackSystem::attack(Entity attackedHexagon) {
    
    Entity selectedHexagon = game->reg.attachee<SelectedTag>();
    
    if(selectedHexagon == entt::null || !game->reg.has<PathfindingAttackComponent>(attackedHexagon)) {
        return false;
    }
    
    auto& attackedHexagonComp = game->reg.get<HexagonComponent>(attackedHexagon);
    
    auto& selectedHexagonComp = game->reg.get<HexagonComponent>(selectedHexagon);
    
    Entity attacked = attackedHexagonComp.unit;
    
    Entity attacker = selectedHexagonComp.unit;
    
    game->reg.remove<AttackComponent>(attacker);
    if(game->reg.has<MoveComponent>(attacker)) {
        game->reg.remove<MoveComponent>(attacker);
    }
    
    
    // FIGHT
    
    if(game->reg.has<RangedUnitComponent>(attacker)) { // if ranged, create arrow animation
        
        Entity arrow = game->factorySystem.createArrow();
        
        game->reg.assign<ParabolaAnimationComponent>(arrow,
                    PositionConverter::hexToOrtho(game->reg.get<PositionComponent>(attacker)), // start
                    PositionConverter::hexToOrtho(game->reg.get<PositionComponent>(attacked)),  // end
                    [=]() {
                        this->game->reg.assign<DestroyComponent>(arrow);
                        this->firstAttack(attacker, attacked, selectedHexagon, attackedHexagon);
                    });
        
    } else { // if not, immediately proceed
        this->firstAttack(attacker, attacked, selectedHexagon, attackedHexagon);
    }
    
    game->reg.reset<PathfindingAttackComponent>();
    
    return true;
}

void AttackSystem::firstAttack(Entity attacker, Entity attacked, Entity attackerHexagon, Entity attackedHexagon) {
    
    auto& attackerComp = game->reg.get<UnitComponent>(attacker);
    
    auto& attackedComp = game->reg.get<UnitComponent>(attacked);
    
    attackedComp.healthPoints -= std::abs(attackerComp.attack - attackedComp.defence);
    if(attackedComp.healthPoints <= 0) {
        
        game->factorySystem.killUnit(attacked);
        
        // Move to killed unit position
        if(!game->reg.has<RangedUnitComponent>(attacker)) { // Except if attacker is ranged
            
            game->moveSystem.pathfinding(attacker); // redo movement pathfinding
            
            if(game->reg.has<PathfindingMoveComponent>(attackedHexagon)) {
                
                game->selectionSystem.unselect();
                
                // Make animation to new position
                game->reg.assign<ParabolaAnimationComponent>(attacker,
                    PositionConverter::hexToOrtho(game->reg.get<PositionComponent>(attackerHexagon)), // start
                    PositionConverter::hexToOrtho(game->reg.get<PositionComponent>(attackedHexagon)),  // end
                    [=]() {
                        game->moveSystem.move(attackerHexagon, attackedHexagon);
                        if(!game->reg.has<SelectedTag>()) game->click(game->reg.get<PositionComponent>(attacker));
                        game->endAction();
                    }
                );
                
                
            }
            
            game->reg.reset<PathfindingMoveComponent>();
            
        } else {
            game->endAction();
        }
        
        return;

    }
    
    
    
    // redo attack pathfinding of defending unit for retaliation
    pathfinding(attacked);
    
    if(game->reg.has<PathfindingAttackComponent>(attackerHexagon)) {
        
        // if ranged, create arrow animation
        if(game->reg.has<RangedUnitComponent>(attacked)) {
        
            Entity arrow = game->factorySystem.createArrow();
            
            game->reg.assign<ParabolaAnimationComponent>(arrow,
                        PositionConverter::hexToOrtho(game->reg.get<PositionComponent>(attacked)), // start
                        PositionConverter::hexToOrtho(game->reg.get<PositionComponent>(attacker)),  // end
                        [=]() {
                            this->game->reg.assign<DestroyComponent>(arrow);
                            this->secondAttack(attacker, attacked);
                        });
            
        } else { // if not, immediately proceed
            this->secondAttack(attacker, attacked);
        }
        
    } else {
        game->endAction();
    }
    
    game->reg.reset<PathfindingAttackComponent>();
}

void AttackSystem::secondAttack(Entity attacker, Entity attacked) {
    
    auto& attackerComp = game->reg.get<UnitComponent>(attacker);
    
    auto& attackedComp = game->reg.get<UnitComponent>(attacked);
    
    // do retaliation
    attackerComp.healthPoints -= std::abs(attackedComp.retaliation - attackerComp.defence);
    if(attackerComp.healthPoints <= 0) {
        
        game->selectionSystem.unselect();
        game->factorySystem.killUnit(attacker);
        
    }
    
    game->endAction();
}



void AttackSystem::pathfinding() {
    
    game->reg.reset<PathfindingAttackComponent>();
    
    auto selected = game->reg.attachee<SelectedTag>();
    if(selected == entt::null || game->reg.get<SelectedTag>().state != SelectedTag::unit) return;
    auto selectedUnit = game->reg.get<HexagonComponent>(selected).unit;
    
    if(!game->reg.has<AttackComponent>(selectedUnit)) return;
    
    pathfinding(selectedUnit);
    
}

void AttackSystem::pathfinding(Entity attacker) {
    
    UnitComponent& unit = game->reg.get<UnitComponent>(attacker);
    PositionComponent& pos =  game->reg.get<PositionComponent>(attacker);
    float mp = unit.attackRange;
    hPathFinding(mp+1, pos);

    Entity owner = game->reg.get<PossessionComponent>(attacker).owner;
    
    auto view = game->reg.view<PathfindingAttackComponent, HexagonComponent>();
    
    for(auto entity : view) {
        Entity unt = view.get<HexagonComponent>(entity).unit;
        if(unt == entt::null || game->reg.get<PossessionComponent>(unt).owner == owner) {
            game->reg.remove<PathfindingAttackComponent>(entity);
        }
    }
    
}

void AttackSystem::hPathFinding(float mp, PositionComponent pos) {
    Entity caze = game->board.get(pos.x, pos.y);
    if (mp <= 0)
        return ;
    if (game->reg.has<PathfindingAttackComponent>(caze)) {
        PathfindingAttackComponent& pComp = game->reg.get<PathfindingAttackComponent>(caze);
        if (pComp.maxWeight < mp)
            pComp.maxWeight = mp;
        else return;
    } else {
        game->reg.assign<PathfindingAttackComponent>(caze, mp);
    }
    for (PositionComponent pos2 : game->board.neighbours(pos)) {
        Entity target = game->board.get(pos2.x, pos2.y);
        if(target == entt::null) continue;
        hPathFinding(mp - weight(caze, target), pos2);
    }
}

float AttackSystem::weight(Entity current, Entity target) {
    PositionComponent& currentPos =  game->reg.get<PositionComponent>(current);
    PositionComponent& targetPos =  game->reg.get<PositionComponent>(target);
    return std::max(targetPos.height - currentPos.height, 0.f) + 1.; // Placeholder
}
