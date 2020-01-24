#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include "include_vk.h"
#include <QVulkanFunctions>
#include <set>

class Windu;

struct FoBuffer {
    vk::Buffer buffer;
    vk::DeviceMemory memory;
    vk::DeviceSize size, offset;
};

struct FoImage {
    vk::Image image;
    vk::Format format;
    vk::DeviceMemory memory;
    vk::DeviceSize size, offset;
};

typedef enum FoResourceName {
    FO_RESOURCE_STAGING_BUFFER = 0,
    FO_RESOURCE_VERTEX_BUFFER = 1,
    FO_RESOURCE_UNIFORM_BUFFER = 2,
    FO_RESOURCE_READBACK_BUFFER = 3
} FoResourceName;

#define FO_RESOURCE_BUFFER_COUNT 4
#define FO_RESOURCE_IMAGE_COUNT 0

class ResourceManager {
public :
    void init();
    ResourceManager(Windu *windu);
    ~ResourceManager();
    
    void allocateResource(FoResourceName name, vk::DeviceSize size);
    FoBuffer* getBuffer(FoResourceName name);
    FoImage* getImage(FoResourceName name);
    
    void initBuffer(FoResourceName name, std::set<uint32_t> si, vk::BufferUsageFlags bufferUsageFlags, vk::MemoryPropertyFlags memoryPropertyFlags);
    void createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image& image, vk::DeviceMemory& imageMemory);
    
private :
    
    Windu* win;
    
    FoBuffer buffers[FO_RESOURCE_BUFFER_COUNT];
    FoImage images[FO_RESOURCE_IMAGE_COUNT];
    
};

#endif
