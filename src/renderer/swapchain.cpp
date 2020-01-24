#include "swapchain.h"

#include <QVulkanFunctions>
#include <iostream>
#include <vulkan/vulkan.hpp>
#include <time.h>

#include "windu.h"
#include "helper.h"
#include "loader.inl"
#include "util/settings.h"

Swapchain::Swapchain(Windu *win) {
    this->win = win;
}

void Swapchain::init() {
    
    if(swapchain == VK_NULL_HANDLE) prepare(&win->sync);
    
    INST_LOAD(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
    INST_LOAD(vkGetPhysicalDeviceSurfaceFormatsKHR)
    INST_LOAD(vkGetPhysicalDeviceSurfacePresentModesKHR)
    
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
    
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(static_cast<VkPhysicalDevice> (win->device.physical), surface, &capabilities);
    
    uint32_t num;
    vkGetPhysicalDeviceSurfaceFormatsKHR(static_cast<VkPhysicalDevice> (win->device.physical), surface, &num, nullptr);
    formats.resize(num);
    vkGetPhysicalDeviceSurfaceFormatsKHR(static_cast<VkPhysicalDevice> (win->device.physical), surface, &num, formats.data());
    
    vkGetPhysicalDeviceSurfacePresentModesKHR(static_cast<VkPhysicalDevice> (win->device.physical), surface, &num, nullptr);
    presentModes.resize(num);
    vkGetPhysicalDeviceSurfacePresentModesKHR(static_cast<VkPhysicalDevice> (win->device.physical), surface, &num, presentModes.data());
    
    VkSurfaceFormatKHR surfaceformat = chooseSwapSurfaceFormat(formats, VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(presentModes, VK_PRESENT_MODE_FIFO_KHR);
    extent = chooseSwapExtent(capabilities);
    format = surfaceformat.format;
     
    NUM_FRAMES = std::max(capabilities.minImageCount, NUM_FRAMES);
    if (capabilities.maxImageCount > 0 && NUM_FRAMES > capabilities.maxImageCount) {
        NUM_FRAMES = capabilities.maxImageCount;
    }
    
    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = NUM_FRAMES;
    createInfo.imageFormat = surfaceformat.format;
    createInfo.imageColorSpace = surfaceformat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 1;
    createInfo.pQueueFamilyIndices = &win->device.g_i;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = swapchain;
    
    VkSwapchainKHR newSwapchain;
    
    DEV_LOAD(vkCreateSwapchainKHR)
    
    foAssert(vkCreateSwapchainKHR(static_cast<VkDevice> (win->device.logical), &createInfo, nullptr, &newSwapchain));
    
    swapchain = newSwapchain;
    
    if(createInfo.oldSwapchain != VK_NULL_HANDLE) {
        DEV_LOAD(vkDestroySwapchainKHR)
        vkDestroySwapchainKHR(static_cast<VkDevice> (win->device.logical), createInfo.oldSwapchain, nullptr);
        for (auto imageView : imageViews) {
            win->vkd->vkDestroyImageView(static_cast<VkDevice> (win->device.logical), imageView, nullptr);
        }
    }
    
    DEV_LOAD(vkGetSwapchainImagesKHR)
    
    vkGetSwapchainImagesKHR(static_cast<VkDevice> (win->device.logical), swapchain, &num, nullptr);
    images.resize(num);
    foAssert(vkGetSwapchainImagesKHR(static_cast<VkDevice> (win->device.logical), swapchain, &num, images.data()));
    
    imageViews.resize(num);
    for(uint32_t i = 0; i < num; i++) {
        
        VkImageViewCreateInfo vInfo = {};
        vInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vInfo.image = images[i];
        vInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vInfo.format = format;
        
        vInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        vInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        vInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        vInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        
        vInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        vInfo.subresourceRange.baseMipLevel = 0;
        vInfo.subresourceRange.levelCount = 1;
        vInfo.subresourceRange.baseArrayLayer = 0;
        vInfo.subresourceRange.layerCount = 1;
        
        foAssert(win->vkd->vkCreateImageView(static_cast<VkDevice> (win->device.logical), &vInfo, nullptr, &imageViews[i]));
        
    }
    
    DEV_LOAD(vkAcquireNextImageKHR)
    this->vkAcquireNextImageKHR = vkAcquireNextImageKHR;
    DEV_LOAD(vkQueuePresentKHR)
    this->vkQueuePresentKHR = vkQueuePresentKHR;
    
}

void Swapchain::reset() {
    
    win->vkd->vkDeviceWaitIdle(static_cast<VkDevice> (win->device.logical));
    
    for (auto imageView : imageViews) {
        win->vkd->vkDestroyImageView(static_cast<VkDevice> (win->device.logical), imageView, nullptr);
    }
    
    DEV_LOAD(vkDestroySwapchainKHR)
    vkDestroySwapchainKHR(static_cast<VkDevice> (win->device.logical), swapchain, nullptr);
    swapchain = VK_NULL_HANDLE;
    
}







/////////////
// RUNTIME //
/////////////

uint32_t Swapchain::acquire() {
    
    if(current == 1000) sync();
    
    VkResult result;
    do {
        result = vkAcquireNextImageKHR(static_cast<VkDevice> (win->device.logical), swapchain, 10000000000000L, signalCount > 0 ? static_cast<VkSemaphore>(signalSemaphores[0]) : VK_NULL_HANDLE, VK_NULL_HANDLE, &current);
        if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            if(Settings::is("logging/PrintResize")) qDebug() << "resize required";
            win->vkd->vkDeviceWaitIdle(static_cast<VkDevice> (win->device.logical));
            win->start();
        } else if(result != VK_SUCCESS) foAssert(result);
    } while(result != VK_SUCCESS);
    
    postsync();

    return current;
    
}

void Swapchain::present() {
    
    sync();
    
    VkPresentInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.swapchainCount = 1;
    info.pSwapchains = &swapchain;
    info.pImageIndices = &current;
    info.pResults = nullptr;
    info.waitSemaphoreCount = waitCount;
    std::vector<VkSemaphore> sems(waitSemaphores.begin(), waitSemaphores.end());
    info.pWaitSemaphores = sems.data();
    
    // This will display the image
    VkResult result = vkQueuePresentKHR(static_cast<VkQueue> (win->device.graphics), &info);
    if(result != VK_SUCCESS) {
        if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            if(Settings::is("logging/PrintResize")) qDebug() << "resize required";
            win->vkd->vkDeviceWaitIdle(static_cast<VkDevice> (win->device.logical));
            win->start();
        } else foAssert(result);
    }
    
}








void Swapchain::getSurface() {
    surface = win->inst.surfaceForWindow(win);
    if(surface == VK_NULL_HANDLE) { 
        qCritical() << "Failed to retrieve surface" << endl;
        exit(7);
    }
}

VkSurfaceFormatKHR Swapchain::chooseSwapSurfaceFormat(std::vector<VkSurfaceFormatKHR> &formats, VkFormat wantedFormat, VkColorSpaceKHR wantedColorSpace) {
    if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
        return {wantedFormat, wantedColorSpace}; // Just give the format you want
    }

    for (const auto& availableFormat : formats) {
        if (availableFormat.format == wantedFormat && availableFormat.colorSpace == wantedColorSpace) { // Look for the wanted format
            return availableFormat;
        }
    }

    return formats[0];
}

VkPresentModeKHR Swapchain::chooseSwapPresentMode(std::vector<VkPresentModeKHR> &presentModes, VkPresentModeKHR wantedMode) {

    for (const auto& availablePresentMode : presentModes) {
        if (availablePresentMode == wantedMode) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Swapchain::chooseSwapExtent(VkSurfaceCapabilitiesKHR &capabilities) {

    VkExtent2D actualExtent = {(uint32_t) win->size.rwidth(), (uint32_t) win->size.rheight()};

    actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
    actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

    return actualExtent;

}

VkFormat Swapchain::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        win->vki->vkGetPhysicalDeviceFormatProperties(static_cast<VkPhysicalDevice> (win->device.physical), format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

Swapchain::~Swapchain() {
    
}
