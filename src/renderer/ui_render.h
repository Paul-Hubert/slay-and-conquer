#ifndef UI_RENDER_H
#define UI_RENDER_H

#include "include_vk.h"

#include "object_system.h"

#include "imgui/imgui.h"

class Windu;

// Frame data
class FrameDataForRender {
public:
    vk::DeviceMemory  vertexBufferMemory;
    vk::DeviceMemory  indexBufferMemory;
    vk::DeviceSize    vertexBufferSize;
    vk::DeviceSize    indexBufferSize;
    vk::Buffer        vertexBuffer;
    vk::Buffer        indexBuffer;
};

class UIRender {
public:
    UIRender(Windu *win);
    ~UIRender();
    
    void preinit();
    void init(vk::RenderPass);
    
    void createOrResizeBuffer(vk::Buffer& buffer, vk::DeviceMemory& buffer_memory, vk::DeviceSize& p_buffer_size, size_t new_size,vk::BufferUsageFlagBits usage);
    
    void render(vk::CommandBuffer commandBuffer, uint32_t i);
    
    Windu *win;
    
    vk::DescriptorSet set;
    vk::PipelineLayout pipelineLayout;
    vk::Pipeline graphicsPipeline;
    
    std::vector<FrameDataForRender> g_FramesDataBuffers;
    
    Image* fontAtlas;
    
    
private:
    
    void initPipeline(vk::RenderPass);
    
};

#endif
