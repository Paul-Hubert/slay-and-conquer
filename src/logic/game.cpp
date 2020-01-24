#include "game.h"

#include "components/hexagon_component.h"
#include "components/unit_component.h"
#include "components/render_component.h"
#include "components/position_component.h"
#include "components/player_component.h"
#include "components/possession_component.h"
#include "components/city_component.h"
#include "components/capital_component.h"
#include "components/attack_component.h"
#include "components/move_component.h"
#include "components/animation_components.h"
#include "components/ranged_unit_component.h"
#include "components/destroy_component.h"
#include "components/ai_component.h"
#include "components/marker_component.h"
#include "tags/selected_tag.h"


#include "util/position_converter.h"

#include "systems/ui_system.h"
#include "systems/render_system.h"

#include "logic/world_generator.h"

#include "entt/entity/utility.hpp"

#include <QDebug>
#include <QMessageBox>
#include <QApplication>
#include <QUrl>

#include <cereal/types/vector.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <fstream>

#include "util/settings.h"


Game::Game() : factorySystem(this), moveSystem(this), attackSystem(this), selectionSystem(this), goldSystem(this), animationSystem(this), aiSystem(this), board(Settings::getUInt("game/MapLength"), Settings::getUInt("game/MapWidth")), players(Settings::getUInt("game/NumPlayers") + Settings::getUInt("game/NumComputers")) {}

Game::~Game() {
    
}


void Game::init() {
    // Changer l'UI en mode jeu
    ui->menu = false;

    // Créer les joueurs
    for(uint i = 0; i < players.size(); i++) {
        Entity player = reg.create();
        reg.assign<PlayerComponent>(player, i, ImVec4(i/(float)players.size(), std::sin((float)i), std::cos((float)i), 1.), entt::null);
        if(i >= Settings::getUInt("game/NumPlayers")) reg.assign<AIComponent>(player);
        players[i] = player;
    }
    
    //Initialise la musique de fin
    victoryTune.setMedia(QUrl("qrc:/resources/audio/victory/victory.mp3"));
    victoryTune.setVolume(100);

    // Générer le terrain et les villes
    WorldGenerator::generate(*this);
    
    // Commencer le premier tour
    startTurn();

    isInitialized = true;
}

void Game::click(glm::vec3 pos) {
    click(PositionConverter::orthoToHex(pos)); 
}

void Game::click(PositionComponent pos) {
    
    Entity clickedHexagon = board.get(pos);
    
    if(clickedHexagon == entt::null) {
        
        selectionSystem.unselect();
        
    } else {
        
        auto& clickedComp = reg.get<HexagonComponent>(clickedHexagon);
        
        Entity selectedHexagon = reg.attachee<SelectedTag>();
        
        bool action = false;
        if(selectedHexagon != entt::null) {
            
            auto& tag = reg.get<SelectedTag>();
            auto& selectedHexagonComponent = reg.get<HexagonComponent>(selectedHexagon);
            
            if(selectedHexagonComponent.unit != entt::null && tag.state == SelectedTag::unit) { // if you have selected a unit
                if(clickedComp.unit != entt::null ? attackSystem.attack(clickedHexagon) : moveSystem.move(clickedHexagon)) {
                    action = true;
                }
            }
        }
        
        if(!action) {
            selectionSystem.select(clickedHexagon);
        }
        
    }
    
    update();

}

void Game::tick(float delta) {
    
    if(toTurn) {
        toTurn = false;
        nextTurn();
    }
    
    animationSystem.tick(delta);
    
    auto toDestroy = reg.view<DestroyComponent>();
    for(Entity ent : toDestroy) {
        reg.destroy(ent);
    }
    
    render->update();
    
}

void Game::nextTurn() {
    
    endTurn();
    
    currentPlayerIndex = (currentPlayerIndex+1)%players.size();
    
    startTurn();
    
}

void Game::startTurn() {
    
    { // Vérifier si ce joueur a des unités/structures à lui, sinon il passe son tour.
        
        bool hasStuff = false;
        
        auto view = reg.view<PossessionComponent>();
        
        for(Entity ent : view) {
            if(currentPlayer() == view.get(ent).owner) {
                hasStuff = true;
                break;
            }
        }
        
        if(!hasStuff) {
            nextTurn();
            return;
        }
        
    }
    
    selectionSystem.unselect();
    
    { // Ajoute un move component et un attack component à chaque unité du joueur (afin qu'il puisse les déplacer)
        
        auto view = reg.view<UnitComponent, PossessionComponent, PositionComponent>();
        for(auto c : view) {
            auto& owner = view.get<PossessionComponent>(c);
            if(owner.owner == currentPlayer()) {

                { // Si une unité est sur une ville, on capture la ville et on n'assigne pas de move component ou d'attack component
                    Entity structure = reg.get<HexagonComponent>(board.get(view.get<PositionComponent>(c))).structure;
                    if(structure != entt::null && reg.get<PossessionComponent>(structure).owner != currentPlayer()) {
                        reg.get<PossessionComponent>(structure).owner = currentPlayer();
                        continue;
                    }
                }

                if(!reg.has<MoveComponent>(c)) reg.assign<MoveComponent>(c);
                if(!reg.has<AttackComponent>(c)) reg.assign<AttackComponent>(c);
            }
        }
        
    }

    { // Centre la caméra sur la capitale du joueur
        
        render->setCameraPos(reg.get<PlayerComponent>(currentPlayer()).cameraPos);
        
    }

    update();
    
    if(reg.has<AIComponent>(currentPlayer())) aiSystem.turn();
    
}


void Game::endTurn() {
    
    //reg.reset<MarkerComponent>();
    
    reg.get<PlayerComponent>(currentPlayer()).cameraPos = render->getCameraPos();
    
    auto view = reg.view<UnitComponent, MoveComponent, AttackComponent>();
    for(auto e : view) {
        auto& unit = view.get<UnitComponent>(e);
        unit.healthPoints += (unit.maxHealthPoints - unit.healthPoints)/3;
    }

    reg.reset<MoveComponent>();
    reg.reset<AttackComponent>();
    
    { // Condition de victoire : tout lui appartient
        
        Entity winner = entt::null;
        
        {
            auto view = reg.view<PossessionComponent>();
            
            for(Entity ent : view) {
                Entity new_owner = view.get(ent).owner;
                if(winner != new_owner && new_owner != entt::null) {
                    if(winner == entt::null) winner = new_owner;
                    else {
                        winner = entt::null;
                        break;
                    }
                }
            }
        }
        
        if(winner != entt::null) {

            victoryTune.play();
            PlayerComponent p = reg.get<PlayerComponent>(winner);
            QMessageBox::information(nullptr, QString("Victoire!"), QString("Le joueur n°%1 à gagné! Bravo!").arg(p.index+1));
            QApplication::exit();
        }
    
    }
    
    goldSystem.turn();
    
}

void Game::endAction() {
    
    update();
    
    if(reg.has<AIComponent>(currentPlayer())) aiSystem.action();
    
}



void Game::update() {
    
    moveSystem.pathfinding();
    
    attackSystem.pathfinding();
    
    goldSystem.calculate();
    
}



void Game::saveState() {
    
    QString path = Settings::getString("save/Path");
    
    std::ofstream os(path.toStdString(), std::ios::binary);
    cereal::PortableBinaryOutputArchive ar( os );

    reg.snapshot()
    .entities(ar)
    .destroyed(ar)
    .component<HexagonComponent, PositionComponent,
    RenderComponent, RotationRenderComponent, RotationYRenderComponent,
    AttackComponent, MoveComponent, CapitalComponent, CityComponent, PlayerComponent, AIComponent, PossessionComponent, UnitComponent, RangedUnitComponent>(ar);
    
    ar(board, players, currentPlayerIndex);
    
}

void Game::loadState() {
    
    QString path = Settings::getString("save/Path");
    
    std::ifstream is(path.toStdString(), std::ios::binary);
    
    if(!is.good()) return; // fichier non existant
    
    cereal::PortableBinaryInputArchive ar(is);
    
    players.clear();
    
    reg.restore()
    .entities(ar)
    .destroyed(ar)
    .component<HexagonComponent, PositionComponent,
    RenderComponent, RotationRenderComponent, RotationYRenderComponent,
    AttackComponent, MoveComponent, CapitalComponent, CityComponent, PlayerComponent, AIComponent, PossessionComponent, UnitComponent, RangedUnitComponent>(ar);
    
    ar(board, players, currentPlayerIndex);
    
    render->setCameraPos(reg.get<PlayerComponent>(currentPlayer()).cameraPos);
    ui->menu = false;
    isInitialized = true;
    
    update();
    
}


Entity Game::currentPlayer() {
    return players[currentPlayerIndex];
}


void Game::setRenderSystem(RenderSystem* render) {
    this->render = render;
}

void Game::setUISystem(UISystem* ui) {
    this->ui = ui;
}
