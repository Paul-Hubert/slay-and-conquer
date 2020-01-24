#ifndef RENDERER_H
#define RENDERER_H

#include "include_vk.h"
#include <QString>

#include "obj_render.h"
#include "water_render.h"
#include "ui_render.h"
#include "fonode.h"
#include "object_system.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <logic/components/render_component.h>

class Windu;


struct perObject {
    glm::mat4 model;
    int highlightcolor;
    float ambient;
    float diffuse;
    float specular;
};

struct Transform {
    float time;
    glm::vec3 pad;
    glm::mat4 viewproj;
    glm::vec4 viewpos;
    perObject obj[800];
};

class Renderer : public foNode {
public:
    
    Renderer(Windu *win);
    ~Renderer();
    
    void preinit();
    void init();
    void setup();
    void cleanup();
    void reset();
    void render(uint32_t i);
    
    ObjRender objrender;
    
    WaterRender waterrender;
    
    UIRender uirender;
    
    std::vector<vk::ImageView> depthImageViews;
    
    Windu *win;
    
    glm::vec3 highlight_pos;
    int highlightX = 3, highlightY = 2;
    int clickX = 3, clickY = 2;
    
private:
    
    void initDescriptors();
    
    void initRenderPass();
    
    void initRest();
    
    vk::RenderPass renderPass;
    vk::CommandPool commandPool;
    std::vector<vk::Framebuffer> framebuffers;
    std::vector<vk::CommandBuffer> commandBuffers;
    std::vector<vk::Fence> fences;
    
    Object hex;
    
    Object skybox;
    
    Object footman;
    
    Object castle;
    
    Object arrow;
    
    std::vector<vk::DeviceMemory> depthMemories;
    std::vector<vk::Image> depthImages;
    void transitionDepthImage(vk::CommandBuffer &commandBuffer);
    void cull(uint32_t& index, Transform* ubo, uint (&sizes)[RenderComponent::meshCount], uint (&offsets)[RenderComponent::meshCount]);
    
    float* readbackMapped;
    Transform* uniformMapped;
    
    
    vk::QueryPool queryPool;
    
    bool wasRecreated = true;
    bool isFirst = true;
    
};

#endif /* RENDERER_H */
