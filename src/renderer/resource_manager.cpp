#include "resource_manager.h"

#include "windu.h"

#include <cmath>
#include <vector>
#include <set>

#include "helper.h"

void ResourceManager::allocateResource(FoResourceName name, vk::DeviceSize size) {
    
    if(name < FO_RESOURCE_BUFFER_COUNT) { // IT'S A BUFFER
        FoBuffer buffer = {};
        buffer.size = size;
        buffers[name] = buffer;
    } else { // IT'S AN IMAGE
        FoImage image = {};
        image.size = size;
        images[name - FO_RESOURCE_BUFFER_COUNT] = image;
    }
    
}

FoBuffer* ResourceManager::getBuffer(FoResourceName name) {
    return &buffers[name];
}

FoImage* ResourceManager::getImage(FoResourceName name) {
    return &images[name - FO_RESOURCE_BUFFER_COUNT];
}

void ResourceManager::initBuffer(FoResourceName name, std::set<uint32_t> si, vk::BufferUsageFlags bufferUsageFlags, vk::MemoryPropertyFlags memoryPropertyFlags) {
    
    FoBuffer* buffer = getBuffer(name);
    
    std::vector<uint32_t> qi(si.begin(), si.end());
    buffer->buffer = win->device.logical.createBuffer({{}, buffer->size, bufferUsageFlags, qi.size() > 1 ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive, static_cast<uint32_t>(qi.size()), qi.data() });
    
    vk::MemoryRequirements memreq = win->device.logical.getBufferMemoryRequirements(buffer->buffer);
    
    vk::MemoryAllocateInfo info((memreq.size/memreq.alignment + 1) * memreq.alignment * 4, win->device.getMemoryType(memreq.memoryTypeBits, memoryPropertyFlags));
    
    buffer->memory = win->device.logical.allocateMemory(info);
    
    win->device.logical.bindBufferMemory(buffer->buffer, buffer->memory, 0);
    
}

void ResourceManager::init() {
    
    {
        initBuffer(FO_RESOURCE_STAGING_BUFFER,
                   {win->device.t_i},
                   vk::BufferUsageFlagBits::eTransferSrc,
                   vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    }

    {
        initBuffer(FO_RESOURCE_VERTEX_BUFFER,
                   {win->device.t_i},
                   vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
                   vk::MemoryPropertyFlagBits::eDeviceLocal);
    }

    {
        initBuffer(FO_RESOURCE_UNIFORM_BUFFER,
                   {win->device.g_i},
                   vk::BufferUsageFlagBits::eUniformBuffer,
                   vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    }
    
    {
        initBuffer(FO_RESOURCE_READBACK_BUFFER,
                   {win->device.t_i},
                   vk::BufferUsageFlagBits::eTransferDst,
                   vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    }
    
}

void ResourceManager::createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image& image, vk::DeviceMemory& imageMemory) {
    
    auto imageInfo = vk::ImageCreateInfo({}, vk::ImageType::e2D, format, vk::Extent3D(width, height, 1), 1, 1, vk::SampleCountFlagBits::e1, tiling, usage, vk::SharingMode::eExclusive, 1, &win->device.g_i, vk::ImageLayout::eUndefined);

    image = win->device.logical.createImage(imageInfo);
    
    vk::MemoryRequirements memRequirements = win->device.logical.getImageMemoryRequirements(image);
    
    vk::MemoryAllocateInfo allocInfo = {};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = win->device.getMemoryType(memRequirements.memoryTypeBits, properties);

    imageMemory = win->device.logical.allocateMemory(allocInfo);

    win->device.logical.bindImageMemory(image, imageMemory, 0);
}


ResourceManager::ResourceManager(Windu* win) {
    this->win = win;
}

ResourceManager::~ResourceManager() {
    
    for(int i = 0; i<FO_RESOURCE_BUFFER_COUNT; i++) {
        win->device.logical.destroy(buffers[i].buffer);
        win->device.logical.free(buffers[i].memory);
    }
    
}

