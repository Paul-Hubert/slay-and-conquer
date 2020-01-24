#ifndef WATERRENDER_H
#define WATERRENDER_H

#include "include_vk.h"
#include <QString>


class Windu;
struct Image;

class WaterRender {
public:
    WaterRender(Windu *win);
    ~WaterRender();
    
    void preinit();
    void init(vk::RenderPass);
    void setup();
    void cleanup();
    
    Windu *win;
    
    Image* normalMap;
    Image* dudvMap;
    
    vk::Sampler sampler;
    vk::DescriptorSetLayout descriptorLayout;
    vk::DescriptorSet descriptorSet;
    vk::PipelineLayout pipelineLayout;
    vk::Pipeline graphicsPipeline;
    
private:
    
    void initPipeline(vk::RenderPass);
    
};


#endif

