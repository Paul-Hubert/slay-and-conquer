#ifndef UI_SYSTEM_H
#define UI_SYSTEM_H

#include "logic/systems/system.h"

#include "logic/ecs.h"

#include "imgui/imgui.h"

#include "renderer/object_system.h"

#include "renderer/include_vk.h"

#include "logic/components/position_component.h"

#include <QWindow>

class Windu;
class UISystem : public System {
public:
    
    UISystem(Game* game, Windu* win);
    ~UISystem();
    void update(float delta);
    bool menu = true;
    Windu* win;

private:
    
    void renderMenu(float delta);
    void renderGameUI(float delta);
    float sinAnimate(float animate);
    float sinAnimate(float animate, float divider);



    ImVec2 hexToScreen(PositionComponent pos);
    ImVec2 orthoToScreen(glm::vec3 pos);
    
};

#endif // UI_SYSTEM_H
