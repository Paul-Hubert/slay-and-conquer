#include "gold_system.h"

#include "logic/game.h"
#include "logic/components/player_component.h"
#include "logic/components/city_component.h"
#include "logic/components/possession_component.h"
#include "logic/components/unit_component.h"

GoldSystem::GoldSystem(Game* game) : System(game) {}

void GoldSystem::calculate() {
    
    auto& player = game->reg.get<PlayerComponent>(game->currentPlayer());
    {
        player.goldPlus = 0;
        auto view = game->reg.view<CityComponent, PossessionComponent>();
        for(auto c : view) {
            auto owner = view.get<PossessionComponent>(c);
            if(owner.owner == game->currentPlayer()) {
                auto& city = view.get<CityComponent>(c);
                player.goldPlus += city.goldPerTurn;
            }
        }
    }

    {
        player.goldMinus = 0;
        auto view = game->reg.view<UnitComponent, PossessionComponent>();
        for(auto c : view) {
            auto owner = view.get<PossessionComponent>(c);
            if(owner.owner == game->currentPlayer()) {
                auto& unit = view.get<UnitComponent>(c);
                player.goldMinus += unit.goldCostPerTurn;
                
            }
        }
    }
    
}

void GoldSystem::turn() {
    auto& player = game->reg.get<PlayerComponent>(game->currentPlayer());
    player.gold += player.goldPlus - player.goldMinus;

    if(player.gold < 0) {
        auto view = game->reg.view<UnitComponent, PossessionComponent, PositionComponent>();
        for(auto c : view) {
            auto& unit = view.get<UnitComponent>(c);
            auto& owner = view.get<PossessionComponent>(c);
            if(owner.owner == game->currentPlayerIndex) {
                auto& pos = view.get<PositionComponent>(c);
                game->factorySystem.killUnit(c);
                break;
            }
        }
    }
}
