#include "renderer.h"

#include <iostream>
#define _USE_MATH_DEFINES
#include <cmath>

#include "windu.h"

#include "logic/systems/render_system.h"

#define PROFILE false
#define TIMESTAMP_COUNT 8

Renderer::Renderer(Windu *win) : objrender(win), waterrender(win), uirender(win) {

    this->win = win;
    
}

void Renderer::render(uint32_t i) {
    sync();
    
    win->device.logical.waitForFences({fences[i]}, true, 1000000000000000L);
    
    if(!isFirst) {
        float depth = readbackMapped[i];
        highlight_pos = win->camera.getMouseWorldPosition(depth);
        highlightX = floor(highlight_pos.z/3 + 0.5);
        highlightY = floor((highlight_pos.x/1.732051-highlightX)/2 + 0.5);
        
        if(PROFILE) {
            uint64_t timestamps[TIMESTAMP_COUNT];
            win->device.logical.getQueryPoolResults(queryPool, 0, TIMESTAMP_COUNT, sizeof(uint64_t) * TIMESTAMP_COUNT, &timestamps[0], sizeof(uint64_t), vk::QueryResultFlagBits::e64 | vk::QueryResultFlagBits::eWait);
            float x = win->device.properties.limits.timestampPeriod;
            
            qDebug()
            << " Setup   " << (timestamps[1] - timestamps[0]) * x / 1000000 << "\n"
            << "Footman " << (timestamps[2] - timestamps[1]) * x / 1000000 << "\n"
            << "Castle  " << (timestamps[3] - timestamps[2]) * x / 1000000 << "\n"
            << "Arrow   " << (timestamps[4] - timestamps[3]) * x / 1000000 << "\n"
            << "Terrain " << (timestamps[5] - timestamps[4]) * x / 1000000 << "\n"
            << "Water   " << (timestamps[6] - timestamps[5]) * x / 1000000 << "\n"
            << "UI      " << (timestamps[7] - timestamps[6]) * x / 1000000 << "\n"
            << "Total   " << (timestamps[7] - timestamps[0]) * x / 1000000;
        }
    }
    
    
    
    // MAIN UBO INFO
    
    float time = win->getTime()/1000.;
    
    VkDeviceSize alignment = win->device.properties.limits.minUniformBufferOffsetAlignment;
    Transform* ubo = (Transform*) ((char*) uniformMapped + int(std::ceil(sizeof(Transform) / float(alignment)) * alignment * i));
    
    ubo->time = time;
    ubo->viewproj = win->camera.getViewProj();
    ubo->viewpos = glm::vec4(win->camera.getCameraPos(), 1.0);
    
    uint32_t index = 0; // current index in instance buffer
    
    
    
    
    
    auto beginInfo = vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    
    auto clearValue = std::vector<vk::ClearValue> {vk::ClearValue(vk::ClearColorValue( std::array<float, 4> { 0.3f, 0.3f, 0.3f, 1.0f })), vk::ClearValue(vk::ClearDepthStencilValue(1.0f, 0))};
    auto renderPassInfo = vk::RenderPassBeginInfo(renderPass, framebuffers[i], vk::Rect2D({0, 0}, win->swap.extent), clearValue.size(), clearValue.data());
    
    auto viewport = vk::Viewport(0, 0, static_cast<float>(win->swap.extent.width), static_cast<float>(win->swap.extent.height), 0.0f, 1.0f);
    
    auto scissor = vk::Rect2D({0,0}, win->swap.extent);
    
    FoBuffer* vertexBuffer = win->resman.getBuffer(FO_RESOURCE_VERTEX_BUFFER);
    
    
    auto commandBuffer = commandBuffers[i];
    
    
    commandBuffer.begin(&beginInfo);
    
    if(PROFILE) commandBuffer.resetQueryPool(queryPool, 0, TIMESTAMP_COUNT);
    
    if(PROFILE) commandBuffer.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, queryPool, 0);
    
    if(wasRecreated) {
        win->objects.transitionImages(commandBuffer);
    }
    
    commandBuffer.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);
    
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, objrender.graphicsPipeline);
    
    commandBuffer.setViewport(0, 1, &viewport);
    
    commandBuffer.setScissor(0, 1, &scissor);
    
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, objrender.pipelineLayout, 0, {objrender.descriptorSets[i]}, {});
    
    
    // Skybox
    
    commandBuffer.bindVertexBuffers(0, {vertexBuffer->buffer}, {skybox.offset});
    
    commandBuffer.bindIndexBuffer(vertexBuffer->buffer, skybox.offset + skybox.vertices.size() * sizeof(Vertex), vk::IndexType::eUint32);
    
    
    for (const SubObject& subobj : skybox.sub) {
        if(subobj.mat->hasTexture)
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, objrender.pipelineLayout, 1, {subobj.mat->tex}, {});
        commandBuffer.drawIndexed(subobj.num_index, 1, subobj.first_index, 0, 0);
    }
    
    ubo->obj[index] = {
        glm::scale(glm::translate(glm::mat4(1.0), win->camera.getCameraPos() - glm::vec3(0.0,150.0,0.0)), glm::vec3(500,500,500)),
        0, 1., 0., 0.
    };
    
    index++;
    
    
    if(PROFILE) commandBuffer.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, queryPool, 1);
    
    
    
    // CULLING
    
    uint sizes[RenderComponent::meshCount];
    uint offsets[RenderComponent::meshCount];
    
    cull(index, ubo, sizes, offsets);
    
    
    { // add one water tile
        
        ubo->obj[offsets[RenderComponent::water] + sizes[RenderComponent::water]] = {
            glm::scale(glm::translate(glm::mat4(1.0), glm::vec3(0, 1.5-0.237646, 0)), glm::vec3(500, 1, 500)),
            0, 0.5, 0.8, 0.5
        };
        
        sizes[RenderComponent::water]++;
        
    }
    
    
    
    
    {
        // Footman
        
        commandBuffer.bindVertexBuffers(0, {vertexBuffer->buffer}, {footman.offset});
        
        commandBuffer.bindIndexBuffer(vertexBuffer->buffer, footman.offset + footman.vertices.size() * sizeof(Vertex), vk::IndexType::eUint32);
        
        for (const SubObject& subobj : footman.sub) {
            if(subobj.mat->hasTexture)
                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, objrender.pipelineLayout, 1, {subobj.mat->tex}, {});
            commandBuffer.drawIndexed(subobj.num_index, sizes[RenderComponent::unit], subobj.first_index, 0, offsets[RenderComponent::unit]);
        }
        
    }
    
    
    if(PROFILE) commandBuffer.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, queryPool, 2);
    
    
    {
        // Castle
        
        commandBuffer.bindVertexBuffers(0, {vertexBuffer->buffer}, {castle.offset});
        
        commandBuffer.bindIndexBuffer(vertexBuffer->buffer, castle.offset + castle.vertices.size() * sizeof(Vertex), vk::IndexType::eUint32);
        
        for (const SubObject& subobj : castle.sub) {
            if(subobj.mat->hasTexture)
                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, objrender.pipelineLayout, 1, {subobj.mat->tex}, {});
            commandBuffer.drawIndexed(subobj.num_index, sizes[RenderComponent::castle], subobj.first_index, 0, offsets[RenderComponent::castle]);
        }
        
    }
    
    
    if(PROFILE) commandBuffer.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, queryPool, 3);
    
    {
        // Arrow
        
        commandBuffer.bindVertexBuffers(0, {vertexBuffer->buffer}, {arrow.offset});
        
        commandBuffer.bindIndexBuffer(vertexBuffer->buffer, arrow.offset + arrow.vertices.size() * sizeof(Vertex), vk::IndexType::eUint32);
        
        for (const SubObject& subobj : arrow.sub) {
            if(subobj.mat->hasTexture)
                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, objrender.pipelineLayout, 1, {subobj.mat->tex}, {});
            commandBuffer.drawIndexed(subobj.num_index, sizes[RenderComponent::arrow], subobj.first_index, 0, offsets[RenderComponent::arrow]);
        }
        
    }
    
    if(PROFILE) commandBuffer.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, queryPool, 4);
    
    {
        // Terrain tiles
        
        commandBuffer.bindVertexBuffers(0, {vertexBuffer->buffer}, {hex.offset});
        
        commandBuffer.bindIndexBuffer(vertexBuffer->buffer, hex.offset + hex.vertices.size() * sizeof(Vertex), vk::IndexType::eUint32);
        
        
        for (const SubObject& subobj : hex.sub) {
            if(subobj.mat->hasTexture)
                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, objrender.pipelineLayout, 1, {subobj.mat->tex}, {});
            commandBuffer.drawIndexed(subobj.num_index, sizes[RenderComponent::hexagon], subobj.first_index, 0, offsets[RenderComponent::hexagon]);
        }
        
    }
    
    
    if(PROFILE) commandBuffer.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, queryPool, 5);
    
    
    {
        // Water tiles
        
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, waterrender.graphicsPipeline);
        
        const SubObject& subobj = hex.sub[0];
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, waterrender.pipelineLayout, 1, {waterrender.descriptorSet}, {});
        commandBuffer.drawIndexed(subobj.num_index, sizes[RenderComponent::water], subobj.first_index, 0, offsets[RenderComponent::water]);
        
    }
    
    if(PROFILE) commandBuffer.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, queryPool, 6);
    
    {
        // UI render
        
        uirender.render(commandBuffer, i);
        
    }
    
    
    if(PROFILE) commandBuffer.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, queryPool, 7);
    
    
    commandBuffer.endRenderPass();
    
    
    {
        // Copy depth
        
        if(!isFirst) {
            FoBuffer* readback = win->resman.getBuffer(FO_RESOURCE_READBACK_BUFFER);
            commandBuffer.copyImageToBuffer(depthImages[i], vk::ImageLayout::eTransferSrcOptimal, readback->buffer, {vk::BufferImageCopy(
                sizeof(float)*i, 1, 1, vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eDepth, 0, 0, 1), vk::Offset3D(win->camera.mouseX, win->camera.mouseY, 0), vk::Extent3D(1, 1, 1)
            )});
        }
    
    }
    
    
    commandBuffer.end();
    
    win->device.logical.resetFences({fences[i]});
    
    auto info = vk::SubmitInfo(waitCount, waitSemaphores.data(), waitStages.data(), 1, &commandBuffer, signalCount, signalSemaphores.data());
    
    win->device.graphics.submit({info}, fences[i]);
    
    
    postsync();
    
    isFirst = false;
    wasRecreated = false;
}

void Renderer::cull(uint32_t& index, Transform* ubo, uint (&sizes)[RenderComponent::meshCount], uint (&offsets)[RenderComponent::meshCount]) {
    
    float tim = win->getTime();
    
    uint total = 0;
    uint current = 0;
    uint noffsets[RenderComponent::meshCount];
    
    for(uint i = 0; i < RenderComponent::meshCount; i++) {
        sizes[i] = 0;
        offsets[i] = index;
    }
    
    RenderSystem* rs = win->model;

    noffsets[0] = 0;
    for(uint i = 1; i < RenderComponent::meshCount; i++) {
        noffsets[i] = noffsets[i-1] + rs->numOfMesh[i-1];
    }
    
    glm::vec3 eye = win->camera.getCameraPos();
    
    float yheight = 1.5;
    
    glm::vec2 min(eye.x, eye.z);
    glm::vec2 max(eye.x, eye.z);
    for(glm::vec2 screenPos : {
        glm::vec2(0, 0),
        glm::vec2(0, win->height()),
        glm::vec2(win->width(), 0),
        glm::vec2(win->width(), win->height())
    }) {
        
        glm::vec3 pos = win->camera.getWorldPosition(glm::vec3(screenPos.x, screenPos.y, 0.9999999));
        
        
        float t = (yheight - eye.y) / (pos.y - eye.y);
        
        glm::vec3 floorPos = eye + (pos - eye) * t;
        
        min = glm::vec2(std::min(floorPos.x, min.x), std::min(floorPos.z, min.y));
        max = glm::vec2(std::max(floorPos.x, max.x), std::max(floorPos.z, max.y));
        
    }
    
    for (uint i = 0; i < rs->numObjects; i++) {
        
        if(i >= rs->numOfMesh[current] + noffsets[current]) {
            for(uint i = current+1; i < RenderComponent::meshCount; i++) {
                offsets[i] = offsets[current] + sizes[current];
            }
            total += sizes[current];
            current++;
            sizes[current] = 0;
        }
        
        auto& mat = rs->obj[i].model;
        
        bool culled = true;
        for(glm::vec4 localpos : {
            glm::vec4(-1.732051,0, 2,1),
            glm::vec4(-1.732051,0,-2,1),
            glm::vec4( 1.732051,0, 2,1),
            glm::vec4( 1.732051,0,-2,1)
        }) {
            glm::vec4 pos = mat[3] + localpos;
            if((pos.x >= min.x && pos.x <= max.x && pos.z >= min.y && pos.z <= max.y)) {
                culled = false;
                break;
            }
        }
        
        if(!culled) {
            ubo->obj[index] = rs->obj[i];
            index++;
            sizes[current]++;
            if(index >= 800) break;
        }
    }
    total += sizes[current];
    
    if(PROFILE) {
        qDebug() << total << " / " << rs->numObjects;
        qDebug() << "culltime" << win->getTime() - tim;
    }
}



void Renderer::preinit() {
    
    prepare(&win->sync);
    
    objrender.preinit();
    
    waterrender.preinit();
    
    uirender.preinit();
    
    footman = win->objects.loadModel(QString(":/resources/model/footman/obj/footman.obj"), QString(":/resources/model/footman/obj/footman.mtl"));
    
    skybox = win->objects.loadModel(QString(":/resources/model/skybox/obj/skybox.obj"), QString(":/resources/model/skybox/obj/skybox.mtl"));
    
    hex = win->objects.loadModel(QString(":/resources/model/hex/obj/hex.obj"), QString(":/resources/model/hex/obj/hex.mtl"));
    
    castle = win->objects.loadModel(QString(":/resources/model/castle/obj/castle.obj"), QString(":/resources/model/castle/obj/castle.mtl"));
    
    arrow = win->objects.loadModel(QString(":/resources/model/arrow/obj/fleche.obj"), QString(":/resources/model/arrow/obj/fleche.mtl"));
    
    VkDeviceSize alignment = win->device.properties.limits.minUniformBufferOffsetAlignment;
    win->resman.allocateResource(FO_RESOURCE_UNIFORM_BUFFER, std::ceil(sizeof(Transform) / float(alignment)) * alignment * win->swap.NUM_FRAMES);
    
    win->resman.allocateResource(FO_RESOURCE_READBACK_BUFFER, sizeof(float) * win->swap.NUM_FRAMES);
    
}


void Renderer::init() {
    
    // Création de ressources statiques, qui ne dépendent pas de la swapchain, qui ne sont donc pas recréés
    
    // RENDERPASS
    
    initRenderPass();
    
    initRest();
    
    objrender.init(renderPass);
    
    waterrender.init(renderPass);
    
    uirender.init(renderPass);
    
    setup();
    
}

void Renderer::initRenderPass() {
    
    VkFormat depthFormat = win->swap.findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    
    auto colorAttachment = std::vector<vk::AttachmentDescription> {
        vk::AttachmentDescription({}, vk::Format(win->swap.format), vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR),
        vk::AttachmentDescription({}, vk::Format(depthFormat), vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferSrcOptimal)
    };
    
    auto colorAttachmentRef = std::vector<vk::AttachmentReference> {
        vk::AttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal),
        vk::AttachmentReference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal)
    };
    
    auto subpass = std::vector<vk::SubpassDescription> {
        vk::SubpassDescription({}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, &colorAttachmentRef[0], nullptr, &colorAttachmentRef[1])
    };
    
    auto renderPassInfo = vk::RenderPassCreateInfo({}, colorAttachment.size(), colorAttachment.data(), subpass.size(), subpass.data(), 0, nullptr);
    
    renderPass = win->device.logical.createRenderPass(renderPassInfo);
    
}

void Renderer::initRest() {
    
    auto poolInfo = vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, win->device.g_i);
    
    commandPool = win->device.logical.createCommandPool(poolInfo);
    
    auto allocInfo = vk::CommandBufferAllocateInfo(commandPool, vk::CommandBufferLevel::ePrimary, win->swap.NUM_FRAMES);
    
    commandBuffers = win->device.logical.allocateCommandBuffers(allocInfo);
    
    for(uint32_t i = 0; i < win->swap.NUM_FRAMES; i++) {
        fences.push_back(win->device.logical.createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled)));
    }
    
    if(PROFILE) queryPool = win->device.logical.createQueryPool(vk::QueryPoolCreateInfo({}, vk::QueryType::eTimestamp, TIMESTAMP_COUNT, {}));
    
    FoBuffer* readback = win->resman.getBuffer(FO_RESOURCE_READBACK_BUFFER);
    readbackMapped = static_cast<float*> (win->device.logical.mapMemory(readback->memory, readback->offset, readback->size, {}));
    
    FoBuffer* uniform = win->resman.getBuffer(FO_RESOURCE_UNIFORM_BUFFER);
    uniformMapped = static_cast<Transform*> (win->device.logical.mapMemory(uniform->memory, uniform->offset, uniform->size, {}));
    
}




void Renderer::setup() {
    
    VkFormat depthFormat = win->swap.findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );

    // Création et update ressources qui sont recréés à chaque récréation de la swapchain
    framebuffers.resize(win->swap.NUM_FRAMES);
    depthImages.resize(win->swap.NUM_FRAMES);
    depthMemories.resize(win->swap.NUM_FRAMES);
    depthImageViews.resize(win->swap.NUM_FRAMES);
    
    for (size_t i = 0; i < framebuffers.size(); i++) {
        
        win->resman.createImage(win->swap.extent.width, win->swap.extent.height, vk::Format(depthFormat), vk::ImageTiling::eOptimal,
                            vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eInputAttachment,
                            vk::MemoryPropertyFlagBits::eDeviceLocal, depthImages[i], depthMemories[i]);
        
        auto viewInfo = vk::ImageViewCreateInfo({}, depthImages[i], vk::ImageViewType::e2D, vk::Format(depthFormat));
        viewInfo.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1));
        
        depthImageViews[i] = win->device.logical.createImageView(viewInfo);
        
        auto attachment = std::vector<vk::ImageView> {static_cast<vk::ImageView> (win->swap.imageViews[i]), depthImageViews[i]};
        auto framebufferInfo = vk::FramebufferCreateInfo({}, renderPass, 2, attachment.data(), win->swap.extent.width, win->swap.extent.height, 1);
        
        framebuffers[i] = win->device.logical.createFramebuffer(framebufferInfo);
        
    }
    
    waterrender.setup();
}

void Renderer::transitionDepthImage(vk::CommandBuffer &commandBuffer) {
    
    std::vector<VkImageMemoryBarrier> barriers;
    barriers.resize(framebuffers.size());
    
    for (size_t i = 0; i < framebuffers.size(); i++) {
        
        VkImageMemoryBarrier &barrier = barriers[i];
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = static_cast<VkImageLayout> (vk::ImageLayout::eDepthStencilAttachmentOptimal);
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = static_cast<VkImage> (depthImages[i]);

        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        
    }

    vkCmdPipelineBarrier(
        static_cast<VkCommandBuffer> (commandBuffer),
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        0,
        0, nullptr,
        0, nullptr,
        barriers.size(), barriers.data()
    );
    
}




void Renderer::cleanup() {
    
    // Free/Destroy ce qui est créé dans setup
    for (size_t i = 0; i < framebuffers.size(); i++) {
        win->device.logical.destroy(framebuffers[i]);
        win->device.logical.destroy(depthImageViews[i]);
        win->device.logical.destroy(depthImages[i]);
        win->device.logical.free(depthMemories[i]);
    }
    
    waterrender.cleanup();
    
}

void Renderer::reset() {
    
    cleanup();
    setup();
    
}


Renderer::~Renderer() {
    
    cleanup();
    // Free/Destroy ce qui est créé dans init();
    
    if(PROFILE) win->device.logical.destroy(queryPool);
    
    FoBuffer* readback = win->resman.getBuffer(FO_RESOURCE_READBACK_BUFFER);
    win->device.logical.unmapMemory(readback->memory);
    
    FoBuffer* uniform = win->resman.getBuffer(FO_RESOURCE_UNIFORM_BUFFER);
    win->device.logical.unmapMemory(uniform->memory);
    
    for(uint32_t i = 0; i < win->swap.NUM_FRAMES; i++) {
        win->device.logical.destroy(fences[i]);
    }
    
    win->device.logical.destroy(commandPool);
    
    win->device.logical.destroy(renderPass);
}
