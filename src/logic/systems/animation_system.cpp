#include "animation_system.h"

#include <logic/game.h>
#include <logic/components/animation_components.h>
#include <logic/components/render_component.h>
#include <logic/components/render_update_component.h>
#include <logic/tags/selected_tag.h>

#include <util/position_converter.h>

#include <QDebug>

AnimationSystem::AnimationSystem(Game* game) : System(game) {
    game->reg.construction<RenderComponent>().connect<AnimationSystem, &AnimationSystem::construction>(this);
}

void AnimationSystem::tick(float delta) {
    
    { // Pathfinding Animation
        
        auto view = game->reg.view<PathAnimationComponent, RenderComponent>();
        
        for(const auto entity : view) {
            
            auto& path = view.get<PathAnimationComponent>(entity);
            
            auto& render = view.get<RenderComponent>(entity);
            
            if(!game->reg.has<RenderUpdateComponent>(entity)) game->reg.assign<RenderUpdateComponent>(entity);
            
            path.t += delta/500.;
            if(path.t >= 1.0) {
                path.index++;
                path.t -= 1.;
                if(path.index + 1 >= path.positions.size()) {
                    render.pos = path.positions[path.positions.size()-1];
                    if(path.callback) path.callback();
                    if(game->reg.valid(entity)) game->reg.remove<PathAnimationComponent>(entity);
                    continue;
                }
            }
            
            auto first = path.positions[path.index];
            auto next = path.positions[path.index+1];
            auto mid = (first + next) / 2.f;
            mid.y += 6.f*std::abs(first.y - next.y);
            render.pos = next * path.t*path.t + mid * 2.f*path.t*(1.f-path.t) + first * (1.f-path.t)*(1.f-path.t);
            glm::vec3 derivative = 2.f*path.t * (next - 2.f*mid + first) + 2.f*mid - 2.f*first;
            
            if(game->reg.has<RotationYRenderComponent>(entity)) {
                game->reg.get<RotationYRenderComponent>(entity).angle = std::atan2(next.x - first.x, next.z - first.z);
            } if(game->reg.has<RotationRenderComponent>(entity)) {
                game->reg.get<RotationRenderComponent>(entity).angleX = std::atan2(derivative.y, std::sqrt(derivative.x*derivative.x + derivative.z*derivative.z));
            }
        }
        
    }
    
    { // Parabola Animation
        
        auto view = game->reg.view<ParabolaAnimationComponent, RenderComponent>();
        
        for(const auto entity : view) {
            
            auto& path = view.get<ParabolaAnimationComponent>(entity);
            
            auto& render = view.get<RenderComponent>(entity);
            
            if(!game->reg.has<RenderUpdateComponent>(entity)) game->reg.assign<RenderUpdateComponent>(entity);
            
            path.t += delta/500.;
            if(path.t >= 1.0) {
                render.pos = path.end;
                if(path.callback) path.callback();
                if(game->reg.valid(entity)) game->reg.remove<ParabolaAnimationComponent>(entity);
                continue;
            }
            
            auto mid = (path.start + path.end) / 2.f;
            mid.y += 4.f + 6.f*std::abs(path.start.y - path.end.y);
            render.pos = path.end * path.t*path.t + mid * 2.f*path.t*(1.f-path.t) + path.start * (1.f-path.t)*(1.f-path.t);
            glm::vec3 derivative = 2.f*path.t * (path.end - 2.f*mid + path.start) + 2.f*mid - 2.f*path.start;
            
            if(game->reg.has<RotationYRenderComponent>(entity)) {
                game->reg.get<RotationYRenderComponent>(entity).angle = std::atan2(path.end.x - path.start.x, path.end.z - path.start.z);
            } if(game->reg.has<RotationRenderComponent>(entity)) {
                game->reg.get<RotationRenderComponent>(entity).angleX = std::atan2(derivative.y, std::sqrt(derivative.x*derivative.x + derivative.z*derivative.z));
            }
            
        }
        
    }
    
    { // No Animation
        
        auto view = game->reg.view<NoAnimationComponent, RenderUpdateComponent, RenderComponent, PositionComponent>();
        
        for(const auto entity : view) {
            auto& render = view.get<RenderComponent>(entity);
            auto pos = view.get<PositionComponent>(entity);
            glm::vec3 newPos = PositionConverter::hexToOrtho(pos);
            if(render.pos == newPos) {
                game->reg.remove<RenderUpdateComponent>(entity);
            } else {
                render.pos = newPos;
            }
        }
        
    }
    
}

void AnimationSystem::construction(Registry& reg, Entity ent) {
    if(reg.has<PositionComponent>(ent)) reg.get<RenderComponent>(ent).pos = PositionConverter::hexToOrtho(reg.get<PositionComponent>(ent));
}
