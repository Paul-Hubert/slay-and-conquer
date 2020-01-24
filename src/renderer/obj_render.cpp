#include "obj_render.h"

#include "windu.h"

#include "helper.h"

ObjRender::ObjRender(Windu *win) {
    this->win = win;
}

void ObjRender::preinit() {
    
    
}


void ObjRender::init(vk::RenderPass pass) {
    
    // Création de ressources statiques, qui ne dépendent pas de la swapchain, qui ne sont donc pas recréés
    
    // RENDERPASS
    
    initDescriptors();
    
    initPipeline(pass);
    
}




void ObjRender::initDescriptors() {
    
    // CREATE DESCRIPTORS
    
    // POOL
    auto size = std::vector<vk::DescriptorPoolSize> {{vk::DescriptorType::eUniformBuffer, win->swap.NUM_FRAMES}};
    descriptorPool = win->device.logical.createDescriptorPool(vk::DescriptorPoolCreateInfo({}, win->swap.NUM_FRAMES, size.size(), size.data()));
    
    // LAYOUT
    std::vector<vk::DescriptorSetLayoutBinding> bindings = {
        {0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment}
    };
    descriptorSetLayout = win->device.logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo({}, bindings.size(), bindings.data()));

    std::vector<vk::DescriptorSetLayout> layouts(win->swap.NUM_FRAMES);
    for(uint32_t i = 0; i < layouts.size(); i++) {
        layouts[i] = descriptorSetLayout;
    }
    
    // SET
    descriptorSets = win->device.logical.allocateDescriptorSets(vk::DescriptorSetAllocateInfo(descriptorPool, layouts.size(), layouts.data()));
    
    // WRITE
    FoBuffer* uniform = win->resman.getBuffer(FO_RESOURCE_UNIFORM_BUFFER);
    
    vk::DescriptorBufferInfo uniformInfo(uniform->buffer, 0, uniform->size);
    
    VkDeviceSize alignment = win->device.properties.limits.minUniformBufferOffsetAlignment;
    std::vector<vk::DescriptorBufferInfo> infos(win->swap.NUM_FRAMES);
    std::vector<vk::WriteDescriptorSet> writes(win->swap.NUM_FRAMES);
    for(uint32_t i = 0; i < layouts.size(); i++) {
        infos[i] = vk::DescriptorBufferInfo(uniform->buffer, std::ceil(sizeof(Transform) / float(alignment)) * alignment * i, sizeof(Transform));
        writes[i] = vk::WriteDescriptorSet(descriptorSets[i], 0, 0, 1, vk::DescriptorType::eUniformBuffer, 0, &infos[i], 0);
    }
    
    win->device.logical.updateDescriptorSets(writes, {});
    
}


void ObjRender::initPipeline(vk::RenderPass renderpass) {
    
    // PIPELINE INFO
    
    auto vertShaderCode = foLoad(":/build/shaders/objrender.vert.glsl.spv");
    auto fragShaderCode = foLoad(":/build/shaders/objrender.frag.glsl.spv");
    
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
        vk::VertexInputBindingDescription(0, sizeof(Vertex), vk::VertexInputRate::eVertex),
    };
    // Inpute attribute bindings describe shader attribute locations and memory layouts
    auto vertexInputAttributs = std::vector<vk::VertexInputAttributeDescription> {
        {0, 0, vk::Format::eR32G32B32Sfloat, 0},
        {1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normals)},
        {2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord)}
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
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcColorBlendFactor=VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor=VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp=VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor=VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor=VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp=VK_BLEND_OP_ADD;

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
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
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
    
    
    
    
    auto layouts = std::vector<vk::DescriptorSetLayout> {descriptorSetLayout, win->objects.layout};
    
    pipelineLayout = win->device.logical.createPipelineLayout(vk::PipelineLayoutCreateInfo(
        {}, layouts.size(), layouts.data(), 0, nullptr
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

ObjRender::~ObjRender() {
    
    // Free/Destroy ce qui est créé dans init();
    
    win->device.logical.destroy(graphicsPipeline);
    
    win->device.logical.destroy(pipelineLayout);
    
    win->device.logical.destroy(descriptorSetLayout);
    
    win->device.logical.destroy(descriptorPool);
    
}
