#ifndef RENDERSYSTEM_H
#define RENDERSYSTEM_H

#include "system.h"

#include <glm/glm.hpp>

#include "logic/ecs.h"

#include "logic/components/render_component.h"
#include "logic/components/position_component.h"

#include "renderer/renderer.h"

class RenderSystem : public System {
public:
    
    RenderSystem(Game* game, Windu* win);
    
    ~RenderSystem();
    
    void construction(Registry& reg, Entity ent);
    
    void destruction(Registry& reg, Entity ent);
    
    void modChange(Registry& reg, Entity ent);
    
    void update();
    
    void click(glm::vec3 vec);
    
    void tick(float delta);

    uint numOfMesh[RenderComponent::meshCount];
    
    std::vector<perObject> obj;
    
    uint numObjects = 0;
    
    void setCameraPos(glm::vec3 pos);
    
    glm::vec3 getCameraPos();
    
private:
    
    Windu* win;

    bool toCompleteUpdate = true;
    
    void completeUpdate();
    
    void updateMesh(Entity, RenderComponent&);
    
};

#endif // RENDERSYSTEM_H
