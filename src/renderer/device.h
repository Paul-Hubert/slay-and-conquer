#ifndef DEVICE_H
#define DEVICE_H

#include "include_vk.h"
#include <QVulkanInstance>

class Windu;

class Device {
public :
    Device(Windu *win);
    void init();
    ~Device();
    uint32_t getScore(vk::PhysicalDevice &device);
    uint32_t getMemoryType(uint32_t typeBits, vk::MemoryPropertyFlags properties);
    
    bool isSingleQueue();
    
    Windu *win;
    
    vk::PhysicalDevice physical;
    vk::Device logical;
    vk::Queue graphics, transfer;
    uint32_t g_i = 0, t_i = 0;
    
    vk::PhysicalDeviceFeatures requiredFeatures;
    std::vector<const char*> requiredExtensions;
    
    vk::PhysicalDeviceProperties properties;
    vk::PhysicalDeviceFeatures features;
    vk::PhysicalDeviceMemoryProperties memoryProperties;
    std::vector<vk::QueueFamilyProperties> queueFamilies;
    std::vector<vk::ExtensionProperties> extensions;
};

#endif
