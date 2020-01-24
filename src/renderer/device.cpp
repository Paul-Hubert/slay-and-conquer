#include "device.h"

#include <QVulkanFunctions>
#include <iostream>
#include <set>
#include <string>
#include <QMessageLogger>

#include "windu.h"
#include "helper.h"
#include "loader.inl"
#include "util/settings.h"

Device::Device(Windu *win) {
    this->win = win;
}

void Device::init() {
    
    QVulkanInstance *inst = &(win->inst);
    
    requiredFeatures = vk::PhysicalDeviceFeatures();
    // HERE : enable needed features (if present in 'features')
    
    requiredExtensions = {};
    requiredExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    // HERE : enable needed extensions (if present in 'extensions')
    
    
    auto instance = static_cast<vk::Instance>(inst->vkInstance());
    std::vector<vk::PhysicalDevice> p_devices = instance.enumeratePhysicalDevices(*win->vkip);
    
    // Rate each device and pick the first best in the list, if its score is > 0
    uint32_t index = 1000, max = 0;
    for(uint32_t i = 0; i<p_devices.size(); i++) {
        uint32_t score = getScore(p_devices[i]);
        if(score > max) { // Takes only a score higher than the last (implicitely higher than 0)
            max = score;
            index = i;
        }
    }
    
    if(index == 1000) {  // if no suitable device is found just take down the whole place
        qCritical() << "No suitable vulkan device found. Please check your driver and hardware." << endl;
        exit(1);
    }
    
    physical =  p_devices[index]; // Found physical device
    
    
    // Get device properties
    properties = physical.getProperties();
    features = physical.getFeatures();
    memoryProperties = physical.getMemoryProperties();
    
    queueFamilies = physical.getQueueFamilyProperties();
    extensions = physical.enumerateDeviceExtensionProperties();
    
    // Prepare queue choice data : GRAPHICS / COMPUTE / TRANSFER
    uint32_t g_j = 0, t_j = 0, countF = 0;
    
    std::vector<float> priorities(3); priorities[0] = 0.0f; priorities[1] = 0.0f; priorities[2] = 0.0f; 
    
    std::vector<vk::DeviceQueueCreateInfo> pqinfo(2); // Number of queues
    
    
    // Gets the first available queue family that supports graphics and presentation
    g_i = 1000;
    for(uint32_t i = 0; i < queueFamilies.size(); i++) {
        if(queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics && inst->supportsPresent(static_cast<VkPhysicalDevice> (physical), i, win)) {
            g_i = i;
            countF++;
            pqinfo[0] = {{}, i, 1, priorities.data()};
            break;
        }
    }
    
    if(g_i == 1000) {
        qCritical() << "Could not retrieve queue family" << endl;
        exit(3);
    }
    
    
    // Gets a transfer queue family different from graphics and compute family if possible, then different queue index if possible, else just the same queue.
    t_i = 1000;
    for(uint32_t i = 0; i < queueFamilies.size(); i++) {
        if(queueFamilies[i].queueFlags & vk::QueueFlagBits::eTransfer) {
            t_i = i; t_j = 0;
            if(t_i != g_i) {
                countF++;
                pqinfo[1] = {{}, i, 1, priorities.data()};
                break;
            }
            while((t_i == g_i && t_j == g_j) && queueFamilies[i].queueCount > t_j + 1) t_j++;
        }
    }
    
    if(t_i == 1000) {
        qCritical() << "Could not get transfer queue family" << endl;
        exit(5);
    }
    
    if(t_i == g_i && t_j != g_j) {pqinfo[0].queueCount++;}
    
    
    for(const auto &ext : extensions) {
        if(strcmp(VK_EXT_DEBUG_MARKER_EXTENSION_NAME, ext.extensionName) == 0) requiredExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
    }
    
    
    // Create Device
    
    logical = physical.createDevice(vk::DeviceCreateInfo({}, countF, pqinfo.data(), 0, nullptr, requiredExtensions.size(), requiredExtensions.data(), &requiredFeatures), nullptr, *win->vkip);
    
    win->vkdp = new vk::DispatchLoaderDynamic(instance, logical);
    
    graphics = logical.getQueue(g_i, g_j, *win->vkdp);
    
    
    if(t_i == g_i && t_j == g_j) {
        transfer = graphics;
    } else {
        transfer = logical.getQueue(t_i, t_j, *win->vkdp);
    }
    
}

    


uint32_t Device::getScore(vk::PhysicalDevice &device) {
    // Get device properties
    
    uint32_t score = 1;
    
    properties = device.getProperties();
    features = device.getFeatures();
    std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();
    std::vector<vk::ExtensionProperties> extensions = device.enumerateDeviceExtensionProperties();
    
    if(properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) score++; // is a dedicated graphics card
    if(queueFamilies.size() > 1) score ++; // has more than one queue family
    
    std::set<std::string> required(requiredExtensions.begin(), requiredExtensions.end());
    for(const auto &ext : extensions) {
        required.erase(ext.extensionName);
    }
    
    if(!required.empty()) score = 0;
    
    auto prop2 = device.getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceSubgroupProperties>();
    auto subgroup = prop2.get<vk::PhysicalDeviceSubgroupProperties>();
    
    if(Settings::is("logging/PrintDebugVulkan")) qDebug() << properties.deviceName << " : " << score;
    return score;
}

uint32_t Device::getMemoryType(uint32_t typeBits, vk::MemoryPropertyFlags properties) {
    for(uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
        if((typeBits & 1) == 1) {
            if((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        typeBits >>= 1;
    }
    return 1000;;
}

bool Device::isSingleQueue() {
    return g_i == t_i;
}


Device::~Device() {
    logical.destroy();
}
