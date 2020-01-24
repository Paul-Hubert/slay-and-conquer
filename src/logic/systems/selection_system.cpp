#include "selection_system.h"

#include <QDebug>

#include "logic/components/hexagon_component.h"
#include "logic/components/render_update_component.h"
#include "logic/components/possession_component.h"
#include "logic/components/pathfinding_attack_component.h"
#include "logic/components/pathfinding_move_component.h"
#include "logic/tags/selected_tag.h"

#include "logic/game.h"

SelectionSystem::SelectionSystem(Game* game) : System(game) {}

void SelectionSystem::select(Entity clickedHexagon) {
    
    HexagonComponent clickedComp = game->reg.get<HexagonComponent>(clickedHexagon);
    
    Entity selectedHexagon = game->reg.attachee<SelectedTag>();
    
    SelectedTag tag = selectedHexagon != entt::null ?  game->reg.get<SelectedTag>() : SelectedTag();
    
    unselect();
    
    if(clickedHexagon == selectedHexagon) { // L'hexagone à déja été selectionné
        
        if(tag.state == SelectedTag::unit && clickedComp.structure != entt::null && game->reg.get<PossessionComponent>(clickedComp.structure).owner == game->currentPlayer()) {
            game->reg.assign<SelectedTag>(entt::tag_t{}, selectedHexagon, SelectedTag::structure);
        }
        
    } else {
        
        if(clickedComp.unit != entt::null && game->reg.get<PossessionComponent>(clickedComp.unit).owner == game->currentPlayer()) { // Une unité a été selectionné
            game->reg.assign<SelectedTag>(entt::tag_t{}, clickedHexagon, SelectedTag::unit);
        } else if(clickedComp.structure != entt::null && game->reg.get<PossessionComponent>(clickedComp.structure).owner == game->currentPlayer()) {
            game->reg.assign<SelectedTag>(entt::tag_t{}, clickedHexagon, SelectedTag::structure);
        }
        
    }
    
    if(game->reg.has<SelectedTag>(entt::tag_t{}, clickedHexagon) && !game->reg.has<RenderUpdateComponent>(clickedHexagon)) {
        game->reg.assign<RenderUpdateComponent>(clickedHexagon);
    }
}

void SelectionSystem::unselect() {
    Entity selected = game->reg.attachee<SelectedTag>();
    if(selected == entt::null) return;
    if(!game->reg.has<RenderUpdateComponent>(selected)) game->reg.assign<RenderUpdateComponent>(selected);
    game->reg.reset<PathfindingMoveComponent>();
    game->reg.reset<PathfindingAttackComponent>();
    game->reg.remove<SelectedTag>();
}


