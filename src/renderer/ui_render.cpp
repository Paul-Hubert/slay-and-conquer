#include "ui_render.h"

#include "windu.h"

#include "helper.h"

#include "imgui/imgui.h"
#include "util/resource_copy.h"


UIRender::UIRender(Windu *win) {
    this->win = win;
}

void UIRender::preinit() {
    
    auto& io = ImGui::GetIO(); (void) io;
    //io.Fonts->AddFontDefault();
    io.Fonts->AddFontFromFileTTF(qUtf8Printable(ResourceCopy::loadResource(":/resources/fonts/aniron.ttf")), 50.0f, NULL, io.Fonts->GetGlyphRangesDefault());
    unsigned char* tex_pixels = NULL;
    int tex_w, tex_h;
    io.Fonts->GetTexDataAsRGBA32(&tex_pixels, &tex_w, &tex_h);
    
    fontAtlas = win->objects.initImage(tex_pixels, tex_w, tex_h, STBI_rgb_alpha, vk::Format::eR8G8B8A8Unorm);
    
}


void UIRender::init(vk::RenderPass pass) {
    // Création de ressources statiques, qui ne dépendent pas de la swapchain, qui ne sont donc pas recréés
    
    // RENDERPASS
    
    g_FramesDataBuffers.resize(3);
    
    set = win->device.logical.allocateDescriptorSets(vk::DescriptorSetAllocateInfo(win->objects.pool, 1, &win->objects.layout))[0];
    
    auto info = vk::DescriptorImageInfo(win->objects.sampler, fontAtlas->view, vk::ImageLayout::eShaderReadOnlyOptimal);
        
    win->device.logical.updateDescriptorSets({vk::WriteDescriptorSet(set, 0, 0, 1, vk::DescriptorType::eCombinedImageSampler, &info, 0, 0)}, {});
    
    initPipeline(pass);
    
}


void UIRender::createOrResizeBuffer(vk::Buffer& buffer, vk::DeviceMemory& buffer_memory, vk::DeviceSize& p_buffer_size, size_t new_size, vk::BufferUsageFlagBits usage) {
    
    if (buffer)
        win->device.logical.destroyBuffer(buffer);
    if (buffer_memory)
        win->device.logical.freeMemory(buffer_memory);
    
    buffer = win->device.logical.createBuffer(vk::BufferCreateInfo({}, new_size, usage, vk::SharingMode::eExclusive));

    vk::MemoryRequirements req = win->device.logical.getBufferMemoryRequirements(buffer);
    
    buffer_memory = win->device.logical.allocateMemory(
        vk::MemoryAllocateInfo(req.size,
                               win->device.getMemoryType(req.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)));
    
    win->device.logical.bindBufferMemory(buffer, buffer_memory, 0);
    p_buffer_size = new_size;
    
}

void UIRender::render(vk::CommandBuffer commandBuffer, uint32_t i) {
    
    ImDrawData* draw_data = ImGui::GetDrawData();
    
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0 || draw_data->TotalVtxCount == 0)
        return;

    FrameDataForRender* fd = &g_FramesDataBuffers[i];

    // Create the Vertex and Index buffers:
    size_t vertex_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
    size_t index_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);
    
    if (!fd->vertexBuffer || fd->vertexBufferSize < vertex_size)
        createOrResizeBuffer(fd->vertexBuffer, fd->vertexBufferMemory, fd->vertexBufferSize, vertex_size, vk::BufferUsageFlagBits::eVertexBuffer);
    if (!fd->indexBuffer || fd->indexBufferSize < index_size)
        createOrResizeBuffer(fd->indexBuffer, fd->indexBufferMemory, fd->indexBufferSize, index_size, vk::BufferUsageFlagBits::eIndexBuffer);

    // Upload Vertex and index Data:
    {
        ImDrawVert* vtx_dst = (ImDrawVert*) win->device.logical.mapMemory(fd->vertexBufferMemory, 0, vertex_size, {});
        ImDrawIdx* idx_dst = (ImDrawIdx*) win->device.logical.mapMemory(fd->indexBufferMemory, 0, index_size, {});
        
        for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = draw_data->CmdLists[n];
            memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
            vtx_dst += cmd_list->VtxBuffer.Size;
            idx_dst += cmd_list->IdxBuffer.Size;
        }
        
        win->device.logical.unmapMemory(fd->vertexBufferMemory);
        win->device.logical.unmapMemory(fd->indexBufferMemory);
    }
    
    
    
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);
    
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, {set}, {});
    
    
    commandBuffer.bindVertexBuffers(0, {fd->vertexBuffer}, {0});
    commandBuffer.bindIndexBuffer(fd->indexBuffer, 0, vk::IndexType::eUint16);

    commandBuffer.setViewport(0, vk::Viewport(0, 0, (float)fb_width, (float)fb_height, 0.0f, 1.0f));
    

    // Setup scale and translation:
    // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayMin is typically (0,0) for single viewport apps.
    
    float scale[4];
    scale[0] = 2.0f / draw_data->DisplaySize.x;
    scale[1] = 2.0f / draw_data->DisplaySize.y;
    scale[2] = -1.0f - draw_data->DisplayPos.x * scale[0];
    scale[3] = -1.0f - draw_data->DisplayPos.y * scale[1];
    commandBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eVertex, sizeof(float) * 0, sizeof(float) * 4, scale);
    

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
    ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    // Render command lists
    int vtx_offset = 0;
    int idx_offset = 0;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                if(pcmd->TextureId != NULL) {
                    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, {*static_cast<vk::DescriptorSet*>(pcmd->TextureId)}, {});
                }
                // Project scissor/clipping rectangles into framebuffer space
                ImVec4 clip_rect;
                clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
                clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
                clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
                clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

                if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
                {
                    // Apply scissor/clipping rectangle
                    commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D( (int32_t)(clip_rect.x), (int32_t)(clip_rect.y) ),
                                                           vk::Extent2D( (uint32_t)(clip_rect.z - clip_rect.x), (uint32_t)(clip_rect.w - clip_rect.y))));

                    // Draw
                    commandBuffer.drawIndexed(pcmd->ElemCount, 1, idx_offset, vtx_offset, 0);
                }
                
                if(pcmd->TextureId != NULL) {
                    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, {set}, {});
                }
                
            }
            idx_offset += pcmd->ElemCount;
        }
        vtx_offset += cmd_list->VtxBuffer.Size;
    }
    
}

void UIRender::initPipeline(vk::RenderPass renderpass) {
    
    // PIPELINE INFO
    
    auto vertShaderCode = foLoad(":/build/shaders/uirender.vert.glsl.spv");
    auto fragShaderCode = foLoad(":/build/shaders/uirender.frag.glsl.spv");
    
    VkShaderModuleCreateInfo moduleInfo = {};
    moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    
    moduleInfo.codeSize = vertShaderCode.size();
    moduleInfo.pCode = reinterpret_cast<const uint32_t*>(vertShaderCode.constData());
    VkShaderModule vertShaderModule = static_cast<VkShaderModule> (win->device.logical.createShaderModule(moduleInfo));
    
    moduleInfo.codeSize = fragShaderCode.size();
    moduleInfo.pCode = reinterpret_cast<const uint32_t*>(fragShaderCode.constData());
    VkShaderModule fragShaderModule = static_cast<VkShaderModule> (win->device.logical.createShaderModule(moduleInfo));

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
    
    // VERTEX INPUT
    
    auto vertexInputBindings = std::vector<vk::VertexInputBindingDescription> {
        vk::VertexInputBindingDescription(0, sizeof(ImDrawVert), vk::VertexInputRate::eVertex),
    };
    // Inpute attribute bindings describe shader attribute locations and memory layouts
    auto vertexInputAttributs = std::vector<vk::VertexInputAttributeDescription> {
        {0, 0, vk::Format::eR32G32Sfloat, offsetof(ImDrawVert, pos)},
        {1, 0, vk::Format::eR32G32Sfloat, offsetof(ImDrawVert, uv)},
        {2, 0, vk::Format::eR8G8B8A8Unorm, offsetof(ImDrawVert, col)}
    };

    auto vertexInputState = vk::PipelineVertexInputStateCreateInfo({}, vertexInputBindings.size(), vertexInputBindings.data(), vertexInputAttributs.size(), vertexInputAttributs.data());
    
    
    

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) win->swap.extent.width;
    viewport.height = (float) win->swap.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = win->swap.extent;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;
    
    
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_ALWAYS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f; // Optional
    depthStencil.maxDepthBounds = 1.0f; // Optional
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {}; // Optional
    depthStencil.back = {}; // Optional
    
    
    VkDynamicState states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynInfo = {};
    dynInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynInfo.dynamicStateCount = 2;
    dynInfo.pDynamicStates = &states[0];
    
    
    
    
    auto layouts = std::vector<vk::DescriptorSetLayout> {win->objects.layout};
    
    auto range = vk::PushConstantRange(vk::ShaderStageFlagBits::eVertex, 0, 4 * sizeof(float));
    
    pipelineLayout = win->device.logical.createPipelineLayout(vk::PipelineLayoutCreateInfo(
        {}, layouts.size(), layouts.data(), 1, &range
    ));
    
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputState.operator const VkPipelineVertexInputStateCreateInfo &();
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynInfo;
    pipelineInfo.layout = static_cast<VkPipelineLayout>(pipelineLayout);
    pipelineInfo.renderPass = static_cast<VkRenderPass>(renderpass);
    pipelineInfo.subpass = 0;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    graphicsPipeline = win->device.logical.createGraphicsPipelines(nullptr, {pipelineInfo})[0];

    win->device.logical.destroyShaderModule(static_cast<vk::ShaderModule> (fragShaderModule));
    win->device.logical.destroyShaderModule(static_cast<vk::ShaderModule> (vertShaderModule));
    
}

UIRender::~UIRender() {
    
    // Free/Destroy ce qui est créé dans init();
    
    for(FrameDataForRender& fd : g_FramesDataBuffers) {
        win->device.logical.destroy(fd.vertexBuffer);
        win->device.logical.destroy(fd.indexBuffer);
        win->device.logical.free(fd.vertexBufferMemory);
        win->device.logical.free(fd.indexBufferMemory);
    }
    
    win->device.logical.destroy(graphicsPipeline);
    
    win->device.logical.destroy(pipelineLayout);
    
}
