#ifndef OBJRENDER_H
#define OBJRENDER_H

#include "include_vk.h"
#include <QString>

#include "object_system.h"

class Windu;

class ObjRender{
public:
    ObjRender(Windu *win);
    ~ObjRender();
    
    void preinit();
    void init(vk::RenderPass);
    
    Windu *win;
    
    vk::DescriptorPool descriptorPool;
    vk::DescriptorSetLayout descriptorSetLayout;
    vk::PipelineLayout pipelineLayout;
    std::vector<vk::DescriptorSet> descriptorSets;
    vk::Pipeline graphicsPipeline;
    
private:
    
    void initDescriptors();
    
    void initPipeline(vk::RenderPass);
    
};


#endif
