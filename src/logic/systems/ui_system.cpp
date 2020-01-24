#include "ui_system.h"

#include "renderer/windu.h"

#include "logic/game.h"
#include "logic/components/unit_component.h"
#include "logic/components/position_component.h"
#include "logic/components/player_component.h"
#include "logic/components/hexagon_component.h"
#include "logic/components/possession_component.h"
#include "logic/components/move_component.h"
#include "logic/components/attack_component.h"
#include "logic/components/city_component.h"
#include "logic/components/ai_component.h"
#include "logic/tags/selected_tag.h"

#include "util/position_converter.h"


#include "imgui/imgui.h"

#include "entt/entt.hpp"

#include "glm/glm.hpp"

#include <QDebug>
#include <QCursor>
#include <QApplication>

UISystem::UISystem(Game* game, Windu* win) : System(game), win(win) {
    game->setUISystem(this);
    win->setUISystem(this);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
}

UISystem::~UISystem() {
    ImGui::DestroyContext();
}



void UISystem::update(float delta) {
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(win->width(), win->height());
    io.MousePos = ImVec2(win->camera.mouseX, win->camera.mouseY);
    io.IniFilename = nullptr;
    
    bool clicked = win->isClicked;

    io.MouseDown[0] = clicked;
    if(io.WantCaptureMouse) {
        win->wasClicked = false;
    }
    
    io.DeltaTime = delta;
    ImGui::NewFrame();
    


    if(menu) {
        renderMenu(delta);
        win->wasClicked = false;
    } else {
    
        renderGameUI(delta);
        
    }

    ImGui::Render();
}

void UISystem::renderMenu(float delta) {
    static float animate = 0;
    animate += delta/16.f;

    {
        ImGui::Begin("TitreSlay", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings |  ImGuiWindowFlags_NoMove);
        ImGui::SetWindowFontScale(2);
        ImGui::Text("Slay ");
        ImVec2 size = ImGui::GetWindowSize();
        ImGui::SetWindowPos(ImVec2(sinAnimate(animate)*win->width()/2-size.x, win->height()/10));
        ImGui::End();
    }
    
    {
        ImGui::Begin("Titre&", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings |  ImGuiWindowFlags_NoMove);
        ImGui::SetWindowFontScale(2);
        ImGui::Text("&");
        ImVec2 size = ImGui::GetWindowSize();
        ImGui::SetWindowPos(ImVec2(win->width()/2-size.x/2, sinAnimate(animate)*win->height()/9));
        ImGui::End();
    }
    
    {
        ImGui::Begin("TitreConquer", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings |  ImGuiWindowFlags_NoMove);
        ImGui::SetWindowFontScale(2);
        ImGui::Text("Conquer");
        ImVec2 size = ImGui::GetWindowSize();
        ImGui::SetWindowPos(ImVec2(win->width()-(sinAnimate(animate)*win->width()/2-size.x/10), win->height()/7));
        ImGui::End();
    }
    
    float height = 1./7;
    const float inc = 0.17f;
    
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(30, 30));
    
    //Bouton Jouer
    {
        ImGui::Begin("BoutonJouer", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration);

        if(ImGui::Button("Jouer")) {
            if(game->isInitialized) {
                menu = false;
            } else {
                game->init();
            }
        }

        height += inc;
        ImVec2 size = ImGui::GetWindowSize();
        ImGui::SetWindowPos(ImVec2(sinAnimate(animate,23)*win->width()/2-size.x/2, win->height()*height));
        ImGui::End();
    }

    //Bouton Charger
    {
        ImGui::Begin("BoutonCharger", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration);

        if(ImGui::Button("Charger")) {
            game->loadState();
        }

        height += inc;
        ImVec2 size = ImGui::GetWindowSize();
        ImGui::SetWindowPos(ImVec2(sinAnimate(animate,30)*win->width()/2-size.x/2, win->height()*height));
        ImGui::End();
    }

    //Bouton Sauvegarder
    if(game->isInitialized) {
        ImGui::Begin("BoutonSauvegarder", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration);

        if(ImGui::Button("Sauvegarder")) {
            game->saveState();
        }

        height += inc;
        ImVec2 size = ImGui::GetWindowSize();
        ImGui::SetWindowPos(ImVec2(sinAnimate(animate,30)*win->width()/2-size.x/2, win->height()*height));
        ImGui::End();
    }

    {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.7f,0.01f,0.02f,1.f));
        ImGui::Begin("BoutonQuitter" , nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration);

        if(ImGui::Button("Quitter")) {
            QApplication::exit();
        }

        height += inc;
        ImVec2 size = ImGui::GetWindowSize();
        ImGui::SetWindowPos(ImVec2(sinAnimate(animate,50)*win->width()/2-size.x/2, win->height()*height));

        ImGui::End();
        ImGui::PopStyleColor();

    }
    
    ImGui::PopStyleVar();
    
}

void UISystem::renderGameUI(float delta) {
    static float animate = -M_PI*2;
    if((animate += 0.1) > M_PI*2) animate = -M_PI*2 * delta/16.f;

    /* Villes */
    {
        auto view = game->reg.view<CityComponent, PositionComponent, PossessionComponent>();
        for(auto entity: view) {
            auto pos = view.get<PositionComponent>(entity);
            pos.height += 3.5f;
            ImVec2 screen = hexToScreen(pos);

            //Ne dessine pas si le personnage n'est pas visible sur l'écran
            if(screen.x < -win->width()/12.f || screen.x > win->width() + win->width()/12.f || screen.y < -win->height()/12.f || screen.y > win->height() + win->height()/12.f) {
                continue;
            }

            auto& owner = view.get<PossessionComponent>(entity);
            auto& city = view.get<CityComponent>(entity);

            Entity unitOnTop = game->reg.get<HexagonComponent>(game->board.get(pos.x, pos.y)).unit;
            bool isBeingCaptured = unitOnTop != entt::null && game->reg.get<PossessionComponent>(unitOnTop).owner != owner.owner;

            if(isBeingCaptured) {
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(std::sin(animate/2)/2, 0., 0., 0.3f));
            } else {
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,0.3f));
            }

            ImGui::PushStyleColor(ImGuiCol_Text, owner.owner == entt::null ? ImVec4(1.f,1.f,1.f,1.f) : game->reg.get<PlayerComponent>(owner.owner).color);
            ImGui::Begin(std::to_string(entity).c_str(), nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings |  ImGuiWindowFlags_NoMove);

            ImGui::Text("%s", city.name.c_str());

            ImVec2 size = ImGui::GetWindowSize();
            screen.x -= size.x/2.f;
            screen.y -= size.y/2.f;
            ImGui::SetWindowPos(screen);
            
            ImGui::End();
            ImGui::PopStyleColor(2);
        }
    }

    /* Unités */
    {
        
        auto view = game->reg.view<UnitComponent, RenderComponent, PossessionComponent>();
        
        for(auto entity: view) {
            auto pos = view.get<RenderComponent>(entity).pos;
            pos.y += 2.5f;
            ImVec2 screen = orthoToScreen(pos);

            //Ne dessine pas si le personnage n'est pas visible sur l'écran
            if(screen.x < -win->width()/12.f || screen.x > win->width() + win->width()/12.f || screen.y < -win->height()/12.f || screen.y > win->height() + win->height()/12.f) {
                continue;
            }

            auto& owner = view.get<PossessionComponent>(entity);
            auto& unit = view.get<UnitComponent>(entity);
            bool isLow = unit.healthPoints < unit.maxHealthPoints/3.f;
            if(isLow) {
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(std::sin(animate/2)/2, 0., 0., 1));
            }

            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, game->reg.get<PlayerComponent>(owner.owner).color);
            ImGui::Begin(std::to_string(entity).c_str(), nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings |  ImGuiWindowFlags_NoMove);

            ImGui::ProgressBar(unit.healthPoints/float(unit.maxHealthPoints), ImVec2(100, 20));
            
            ImVec2 size = ImGui::GetWindowSize();
            screen.x -= size.x/2.f;
            screen.y -= size.y/2.f;
            ImGui::SetWindowPos(screen);
            
            ImGui::PopStyleColor(1 + (isLow ? 1 : 0));
            
            ImGui::End();
        }
    }

    {
        
        ImGui::Begin("Joueur ", nullptr,  ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
        
        ImGui::SetWindowPos(ImVec2(win->width()/30, win->height()/30));
        
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(30, 30));
        ImGui::PushStyleColor(ImGuiCol_Text, game->reg.get<PlayerComponent>(game->currentPlayer()).color);
        ImGui::Text("Joueur n°%d", game->currentPlayerIndex+1);
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        ImGui::End();
    }
    
    
    if(!game->reg.has<AIComponent>(game->currentPlayer())) {

        //Bouton fin de tour
        {
            
            ImGui::Begin("FinDeTour", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(30, 30));
            if(ImGui::Button("Fin du tour")) {
                game->nextTurn();
                win->wasClicked = false;
            }
            ImGui::PopStyleVar();
            
            ImVec2 endTurnSize = ImGui::GetWindowSize();
            ImGui::SetWindowPos(ImVec2(win->width()-endTurnSize.x-win->width()/30, win->height()-endTurnSize.y-win->height()/30.));
            
            ImGui::End();
        }

        {
            //Statistique (Or, coût unité, etc)
            ImGui::Begin("Finances", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoFocusOnAppearing);


            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1., 1., 0., 1.));
            ImGui::Text("Or: %d", game->reg.get<PlayerComponent>(game->currentPlayer()).gold);
            ImGui::PopStyleColor();

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0., 1., 0., 1.));
            ImGui::Text("Bénéfice: + %d", game->reg.get<PlayerComponent>(game->currentPlayer()).goldPlus);
            ImGui::PopStyleColor();

            auto& playerComponent = game->reg.get<PlayerComponent>(game->currentPlayer());

            if(playerComponent.goldMinus >= playerComponent.goldPlus) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(std::sin(animate), 0, 0.f, 1.f));
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1., 0.25f, 0., 1.));
            }

            ImGui::Text("Coût: - %d", playerComponent.goldMinus);
            ImGui::PopStyleColor();
            
            ImVec2 size = ImGui::GetWindowSize();
            ImGui::SetWindowPos(ImVec2(win->width()-size.x-win->width()/30, win->height()/30));
            
            ImGui::End();
        }


        

        { // Selection Window
            auto selected = game->reg.attachee<SelectedTag>();
            if(selected != entt::null) {

                auto& state = game->reg.get<SelectedTag>();
                auto& hex = game->reg.get<HexagonComponent>(selected);
                auto& pos = game->reg.get<PositionComponent>(selected);


                if(state.state == SelectedTag::unit) {
                    auto& unit = game->reg.get<UnitComponent>(hex.unit);
                    ImGui::Begin(unit.name.c_str(), nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoFocusOnAppearing);
                    
                    if(ImGui::CollapsingHeader("Statistiques:")) {
                        ImGui::Text("   Coût par tour: %d", unit.goldCostPerTurn);
                        ImGui::Text("   Point de vie: %3.1f/%3.1f", unit.healthPoints, unit.maxHealthPoints);
                        ImGui::Text("   Attaque: %3.1f", unit.attack);
                        ImGui::Text("   Défense: %3.1f", unit.defence);
                        ImGui::Text("   Esquive: %3.1f", unit.dodge);
                        ImGui::Text("   Mobilité: %3.1f", unit.range);
                        ImGui::Text("   Portée: %3.1f", unit.attackRange);
                        ImGui::Text("   Riposte: %3.1f", unit.retaliation);
                    }

                    ImGui::Separator();

                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1., 0., 0., 1.));
                    if(ImGui::Button("Licencier")) {
                        game->selectionSystem.unselect();
                        game->factorySystem.killUnitOnHexagon(selected);
                    }
                    ImGui::PopStyleColor();

                } else {
                    ImGui::Begin("Structure", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoFocusOnAppearing);
                    auto& owner = game->reg.get<PossessionComponent>(hex.structure);
                    ImGui::Text("Structure");
                    if(ImGui::Button("Créer guerrier (-3)")) {
                        game->factorySystem.createWarrior(pos.x, pos.y, owner.owner);
                        game->selectionSystem.unselect();
                        game->click(pos);
                    }
                    if(ImGui::Button("Créer archer (-5)")) {
                        game->factorySystem.createArcher(pos.x, pos.y, owner.owner);
                        game->selectionSystem.unselect();
                        game->click(pos);
                    }
                    if(ImGui::Button("Créer cavalier (-5)")) {
                        game->factorySystem.createRider(pos.x, pos.y, owner.owner);
                        game->selectionSystem.unselect();
                        game->click(pos);
                    }
                }

                ImVec2 size = ImGui::GetWindowSize();
                ImGui::SetWindowPos(ImVec2(win->width()/30, win->height()-size.y-win->height()/30));
                ImGui::End();
            }
        }
    
    }
}

float UISystem::sinAnimate(float animate) {
    return sinAnimate(animate, 42);
}

float UISystem::sinAnimate(float animate, float divider) {
    return std::sin(std::min(animate/divider, 3.14f/2));
}

ImVec2 UISystem::hexToScreen(PositionComponent pos) {
    return orthoToScreen(PositionConverter::hexToOrtho(pos));
}


ImVec2 UISystem::orthoToScreen(glm::vec3 pos) {
    glm::vec4 tr = win->camera.getViewProj() * glm::vec4(pos, 1);
    tr /= tr.w;
    return (tr.z < 0 || tr.z > 1) ? ImVec2(-100, -100) : ImVec2((tr.x + 1) * win->width()/2, (tr.y + 1) * win->height()/2);
}
