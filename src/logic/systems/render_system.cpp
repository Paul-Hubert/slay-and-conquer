#include "render_system.h"

#include "renderer/windu.h"

#include "logic/game.h"

#include "logic/components/render_component.h"

#include "util/position_converter.h"
#include "logic/tags/selected_tag.h"
#include "logic/components/pathfinding_move_component.h"
#include "logic/components/render_update_component.h"
#include "logic/components/pathfinding_attack_component.h"
#include "logic/components/ai_component.h"
#include <logic/components/marker_component.h>

RenderSystem::RenderSystem(Game* game, Windu* win) : System(game) {
    this->win = win;
    this->win->setRenderSystem(this);
    this->game->setRenderSystem(this);
    game->reg.construction<RenderComponent>().connect<RenderSystem, &RenderSystem::construction>(this);
    game->reg.destruction<RenderComponent>().connect<RenderSystem, &RenderSystem::destruction>(this);
    game->reg.construction<PathfindingAttackComponent>().connect<RenderSystem, &RenderSystem::modChange>(this);
    game->reg.destruction<PathfindingAttackComponent>().connect<RenderSystem, &RenderSystem::modChange>(this);
    game->reg.construction<PathfindingMoveComponent>().connect<RenderSystem, &RenderSystem::modChange>(this);
    game->reg.destruction<PathfindingMoveComponent>().connect<RenderSystem, &RenderSystem::modChange>(this);
    game->reg.construction<SelectedTag>().connect<RenderSystem, &RenderSystem::modChange>(this);
    game->reg.destruction<SelectedTag>().connect<RenderSystem, &RenderSystem::modChange>(this);
    
    completeUpdate();
}

RenderSystem::~RenderSystem() {
    
}

void RenderSystem::construction(Registry& reg, Entity ent) {
    numObjects++;
    toCompleteUpdate = true;
}

void RenderSystem::destruction(Registry& reg, Entity ent) {
    numObjects--;
    toCompleteUpdate = true;
}

void RenderSystem::modChange(Registry& reg, Entity ent) {
    if(!reg.has<RenderUpdateComponent>(ent)) reg.assign<RenderUpdateComponent>(ent);
}

void RenderSystem::click(glm::vec3 vec) {
    if(!game->reg.has<AIComponent>(game->currentPlayer())) game->click(vec);
}

void RenderSystem::tick(float delta) {
    game->tick(delta);
}

void RenderSystem::update() {
    
    if(toCompleteUpdate) {
        completeUpdate();
    } else {
        
        auto view = game->reg.view<RenderUpdateComponent, RenderComponent>();
        
        for (auto entity : view) {
            auto& render = view.get<RenderComponent>(entity);
            updateMesh(entity, render);
            game->reg.remove<RenderUpdateComponent>(entity);
        }
    }
    
}

void RenderSystem::completeUpdate() {
    
    if(obj.size() < numObjects) {
        obj.resize(numObjects);
    }
    
    for(uint i = 0; i < RenderComponent::meshCount; i++) {
        numOfMesh[i] = 0;
    }
    
    {
        auto view = game->reg.view<RenderComponent>();
        
        for(auto entity : view) {
            numOfMesh[view.get(entity).mesh]++;
        }
    }
    
    uint offsets[RenderComponent::meshCount];
    
    offsets[0] = 0;
    for(uint i = 1; i < RenderComponent::meshCount; i++) {
        offsets[i] = offsets[i-1] + numOfMesh[i-1];
    }
    
    
    auto view = game->reg.view<RenderComponent>();
    
    for(auto entity : view) {
        auto& render = view.get(entity);

        render.index = offsets[render.mesh];
        
        updateMesh(entity, render);
        
        offsets[render.mesh]++;
    }
    
    toCompleteUpdate = false;
    
}

void RenderSystem::updateMesh(Entity entity, RenderComponent& render) {
    
    auto& object = obj[render.index];
    
    glm::vec3 pos = render.pos;
    
    auto mat = glm::mat4(1.0);
    
    int color_index = 0;
    if(game->reg.has<PathfindingMoveComponent>(entity)) {
        color_index = 2;
    } if(game->reg.has<SelectedTag>(entt::tag_t{}, entity)) {
        color_index = 1;
    } if(game->reg.has<PathfindingAttackComponent>(entity)) {
        color_index = 3;
    } if(game->reg.has<MarkerComponent>(entity)) {
        color_index = game->reg.get<MarkerComponent>(entity).color_index;
    }
    
    
    if(render.mesh == RenderComponent::hexagon) {
        mat = glm::scale(glm::translate(mat, glm::vec3(pos.x, -3, pos.z)), glm::vec3(1, (pos.y+3)/0.237646, 1));
        object.ambient = 0.5;
        object.diffuse = 0.8;
        object.specular = 0.;
    } else if (render.mesh == RenderComponent::unit) {
        mat = glm::scale(glm::translate(mat, glm::vec3(pos.x, pos.y+0.6, pos.z)), glm::vec3(0.6));
        object.ambient = 0.5;
        object.diffuse = 0.5;
        object.specular = 0.;
    } else if (render.mesh == RenderComponent::water) {
        mat = glm::translate(mat, glm::vec3(pos.x, pos.y - 0.236646, pos.z));
        object.ambient = 0.5;
        object.diffuse = 0.8;
        object.specular = 0.5;
    } else if (render.mesh == RenderComponent::castle) {
        mat = glm::scale(glm::translate(mat, glm::vec3(pos.x, pos.y, pos.z)), glm::vec3(0.03, 0.03*1.3, 0.03));
        object.ambient = 0.5;
        object.diffuse = 0.8;
        object.specular = 0.6;
    } else if (render.mesh == RenderComponent::arrow) {
        mat = glm::scale(glm::translate(mat, glm::vec3(pos.x, pos.y, pos.z)), glm::vec3(1,1,1));
        object.ambient = 0.5;
        object.diffuse = 0.8;
        object.specular = 0.6;
    } else {
        qDebug("Mesh not renderable");
    }
    
    if(game->reg.has<RotationYRenderComponent>(entity)) {
        auto rot = game->reg.get<RotationYRenderComponent>(entity);
        mat = glm::rotate(mat, rot.angle, glm::vec3(0, 1, 0));
    } if(game->reg.has<RotationRenderComponent>(entity)) {
        auto rot = game->reg.get<RotationRenderComponent>(entity);
        mat = glm::rotate(mat, -rot.angleX, glm::vec3(1, 0, 0));
        mat = glm::rotate(mat, rot.angleZ, glm::vec3(0, 0, 1));
    }
    
    object.highlightcolor = color_index;
    object.model = mat;
    
}

void RenderSystem::setCameraPos(glm::vec3 pos) {
    win->camera.setTargetCameraPos(pos);
}

glm::vec3 RenderSystem::getCameraPos() {
    return win->camera.getTargetCameraPos();
}

