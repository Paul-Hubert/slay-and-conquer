#ifndef GAME_H
#define GAME_H

#include "logic/ecs.h"

#include <QMediaPlayer>
#include <string>
#include <glm/glm.hpp>

#include "components/position_component.h"

#include "board.h"
#include "systems/move_system.h"
#include "systems/attack_system.h"
#include "systems/selection_system.h"
#include "systems/gold_system.h"
#include "systems/animation_system.h"
#include "systems/factory_system.h"
#include "systems/ai_system.h"

class RenderSystem;
class UISystem;
class Game {

public:
    Game();
    ~Game();
    
    void init();
    
    Registry reg;
    
    /* Accesseurs */
    void setRenderSystem(RenderSystem* render);
    void setUISystem(UISystem* ui);
    
    /* Reception évenements */
    void click(glm::vec3 pos);
    void click(PositionComponent pos);
    void nextTurn();
    void tick(float delta);

    void update();
    void startTurn();
    void endTurn();
    
    void endAction();
    
    void saveState();
    void loadState();

    Entity currentPlayer();

    /* Systèmes */
    RenderSystem* render;
    UISystem* ui;
    FactorySystem factorySystem;
    MoveSystem moveSystem;
    AttackSystem attackSystem;
    SelectionSystem selectionSystem;
    GoldSystem goldSystem;
    AnimationSystem animationSystem;
    AISystem aiSystem;
    
    Board board;
    std::vector<Entity> players;
    uint currentPlayerIndex = 0;
    bool isInitialized = false;
    QMediaPlayer victoryTune;
    
    bool toTurn = false;
};

#endif // GAME_H
