#include "move_system.h"

#include <QDebug>

#include "logic/game.h"

#include "logic/components/position_component.h"
#include "logic/components/hexagon_component.h"
#include "logic/components/unit_component.h"
#include "logic/components/pathfinding_move_component.h"
#include "logic/components/render_update_component.h"
#include "logic/components/move_component.h"
#include "logic/tags/selected_tag.h"
#include "logic/components/animation_components.h"

#include "util/position_converter.h"

MoveSystem::MoveSystem(Game* game) : System(game) {}

bool MoveSystem::move(Entity clickedHexagonEntity) {
    
    if(!game->reg.has<PathfindingMoveComponent>(clickedHexagonEntity)) return false;

    HexagonComponent& clickedHexagon = game->reg.get<HexagonComponent>(clickedHexagonEntity);

    Entity selectedHexagonEntity = game->reg.attachee<SelectedTag>();

    if(clickedHexagonEntity == selectedHexagonEntity) return false;

    move(selectedHexagonEntity, clickedHexagonEntity);
    
    game->reg.move<SelectedTag>(clickedHexagonEntity);
    
    if(game->reg.has<MoveComponent>(clickedHexagon.unit)) {
        game->reg.remove<MoveComponent>(clickedHexagon.unit);
    }

    { // Add path to animation component
        
        std::vector<glm::vec3> positions;
        PathfindingMoveComponent pathfindingComp(0, entt::null);
        Entity ent = clickedHexagonEntity;
        do {
            pathfindingComp = game->reg.get<PathfindingMoveComponent>(ent);
            positions.push_back(PositionConverter::hexToOrtho(game->reg.get<PositionComponent>(ent)));
        } while((ent = pathfindingComp.previous) != entt::null);
        std::reverse(positions.begin(), positions.end());
        
        
        if(game->reg.has<PathAnimationComponent>(clickedHexagon.unit)) {
            auto& path = game->reg.get<PathAnimationComponent>(clickedHexagon.unit);
            path.positions.insert(path.positions.end(), positions.begin(), positions.end());
        } else {
            game->reg.assign<PathAnimationComponent>(clickedHexagon.unit, positions, [=]() {
                if(!game->reg.has<SelectedTag>()) game->click(game->reg.get<PositionComponent>(clickedHexagon.unit));
                game->endAction();
            });
        }
        
        
        game->selectionSystem.unselect();
        
    }
    
    if(!game->reg.has<RenderUpdateComponent>(clickedHexagon.unit)) {
        game->reg.assign<RenderUpdateComponent>(clickedHexagon.unit);
    }

    return true;
}

void MoveSystem::move(Entity moveHex, Entity targetHex) {
    
    HexagonComponent& moveHexagon = game->reg.get<HexagonComponent>(moveHex);
    HexagonComponent& targetHexagon = game->reg.get<HexagonComponent>(targetHex);

    targetHexagon.unit = moveHexagon.unit;
    moveHexagon.unit = entt::null;

    PositionComponent& target = game->reg.get<PositionComponent>(targetHexagon.unit);
    PositionComponent pos = game->reg .get<PositionComponent>(targetHex);
    target.x = pos.x;
    target.y = pos.y;
    target.height = pos.height;
    
}

void MoveSystem::pathfinding() {
    
    auto selected = game->reg.attachee<SelectedTag>();
    if(selected == entt::null || game->reg.get<SelectedTag>().state != SelectedTag::unit) return;
    auto selectedUnit = game->reg.get<HexagonComponent>(selected).unit;
    
    if(!game->reg.has<MoveComponent>(selectedUnit)) return;

    pathfinding(selectedUnit);
    
}

void MoveSystem::pathfinding(Entity selectedUnit) {
    
    game->reg.reset<PathfindingMoveComponent>();
    
    UnitComponent& unit = game->reg.get<UnitComponent>(selectedUnit);
    PositionComponent& pos =  game->reg.get<PositionComponent>(selectedUnit);
    float mp = unit.range;
    hPathFinding(mp+1, pos, entt::null);
    
}

void MoveSystem::hPathFinding(float mp, PositionComponent pos, Entity previous) {
    
    Entity caze = game->board.get(pos.x, pos.y);
    if (mp <= 0)
        return ;
    if (game->reg.has<PathfindingMoveComponent>(caze)) {
        PathfindingMoveComponent& pComp = game->reg.get<PathfindingMoveComponent>(caze);
        if (pComp.maxWeight < mp) {
            pComp.previous = previous;
            pComp.maxWeight = mp;
        }
        else return;
    } else {
        game->reg.assign<PathfindingMoveComponent>(caze, mp, previous);
    }
    for (PositionComponent pos2 : game->board.neighbours(pos)) {
        Entity target = game->board.get(pos2.x, pos2.y);
        if(target == entt::null) continue;
        hPathFinding(mp - weight(caze, target), pos2, caze);
    }
    
}

float MoveSystem::weight(Entity current, Entity target) {
    
    PositionComponent& currentPos =  game->reg.get<PositionComponent>(current);
    PositionComponent& targetPos =  game->reg.get<PositionComponent>(target);
    auto& hexComp = game->reg.get<HexagonComponent>(target);
    if( hexComp.type == HexagonComponent::water ||
        hexComp.unit != entt::null
    ) return 1000.f;
    return std::max(targetPos.height - currentPos.height, 0.f) + 1.; // Placeholder
    
}
