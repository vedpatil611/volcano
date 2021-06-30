#include "volcano.h"

#include <GLFW/glfw3.h>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include "window.h"

bool QueueFamilyIndicies::isComplete()
{
    return graphicsFamily.has_value() && presentFamily.has_value();
}

void Volcano::init(Window* window)
{
    Volcano::window = window;

    {   // Init vulkan instance
#ifdef DEBUG
        if(!checkValidationLayerSupport())
            throw std::runtime_error("Vaildation layer not found");
#endif
        vk::ApplicationInfo appInfo;
        appInfo.pApplicationName = "volcano";
        appInfo.pEngineName = "Volcano";

        vk::InstanceCreateInfo instanceInfo;
        instanceInfo.pApplicationInfo = &appInfo;
    
        auto extensions = getRequiredExtensions();

        instanceInfo.enabledExtensionCount = extensions.size();
        instanceInfo.ppEnabledExtensionNames = extensions.data();

#ifdef DEBUG
        instanceInfo.enabledLayerCount = validationLayers.size();
        instanceInfo.ppEnabledLayerNames = validationLayers.data();
#else
        instanceInfo.enabledLayerCount = 0;
#endif

        try
        {
            instance = vk::createInstanceUnique(instanceInfo, nullptr);
        } 
        catch(vk::SystemError& e)
        {
            throw std::runtime_error("Failed to create instance");
        }
    }

#ifdef DEBUG
    {   // Setup debug messenger
        auto createInfo = vk::DebugUtilsMessengerCreateInfoEXT(
                vk::DebugUtilsMessengerCreateFlagsEXT(),
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
                vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
                &Volcano::debugCallback, 
                nullptr
        );
        
        if (createDebugUtilsMessengerEXT(instance, reinterpret_cast<const VkDebugUtilsMessengerCreateInfoEXT*>(&createInfo), nullptr, &callback) != VK_SUCCESS)
            throw std::runtime_error("Failed to create debug callback");
    }
#endif

    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
}

void Volcano::destroy()
{
    instance->destroySurfaceKHR(surface);

#ifdef DEBUG
    destroyDebugUtilMessengerEXT(instance, callback, nullptr);
#endif
}

void Volcano::pickPhysicalDevice()
{
    auto devices = instance->enumeratePhysicalDevices();
    if(devices.size() == 0)
        throw std::runtime_error("Failed to find suitable GPU");

    for(const auto& device : devices)
    {
        if(isDeviceSuitable(device))
        {
            physicalDevice = device;
            break;
        }
    }

    if(!physicalDevice)
        throw std::runtime_error("Failed to find suitable GPU");
}

bool Volcano::isDeviceSuitable(const vk::PhysicalDevice& physicalDevice)
{
    auto indices = findQueueFamily(physicalDevice);

    bool extensionSupport = checkDeviceExtensionsSupport(physicalDevice);

    bool swapChainAdequate = false;
    if(extensionSupport)
    {
        auto swapChainSupport = querySwapChainSupport(physicalDevice);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return indices.isComplete() && extensionSupport && swapChainAdequate;
}


QueueFamilyIndicies Volcano::findQueueFamily(const vk::PhysicalDevice& physicalDevice)
{
    QueueFamilyIndicies indices;

    auto queueFamilies = physicalDevice.getQueueFamilyProperties();

    int i = 0;
    for(const auto& queueFamily: queueFamilies)
    {
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
            indices.graphicsFamily = i;

        if (queueFamily.queueCount > 0 && physicalDevice.getSurfaceSupportKHR(i, surface))
            indices.presentFamily = i;

        if (indices.isComplete()) break;

        ++i;
    }

    return indices;
}

std::vector<const char*> Volcano::getRequiredExtensions()
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

#ifdef DEBUG
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    return extensions;
}

void Volcano::createLogicalDevice()
{
    auto indices = findQueueFamily(physicalDevice);

    float queuePriority = 1.0f;

    auto queueCreateInfo = vk::DeviceQueueCreateInfo(
        vk::DeviceQueueCreateFlags(),
        indices.graphicsFamily.value(),
        1,
        &queuePriority
    );

    auto deviceFeature = vk::PhysicalDeviceFeatures();
    auto createInfo = vk::DeviceCreateInfo(
        vk::DeviceCreateFlags(),
        1,
        &queueCreateInfo
    );
    createInfo.pEnabledFeatures = &deviceFeature;
    createInfo.enabledExtensionCount = deviceExtensions.size();
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

#ifdef DEBUG
    createInfo.enabledLayerCount = validationLayers.size();
    createInfo.ppEnabledLayerNames = validationLayers.data();
#endif

    try
    {
        device = physicalDevice.createDeviceUnique(createInfo);
    }
    catch(vk::SystemError& e)
    {
        throw std::runtime_error("Failed to create logical device");
    }

    graphicsQueue = device->getQueue(indices.graphicsFamily.value(), 0);
}

void Volcano::createSurface()
{
    VkSurfaceKHR rawSurface;
    if(glfwCreateWindowSurface(*instance, window->getWindow(), nullptr, &rawSurface) != VK_SUCCESS)
        throw std::runtime_error("Failed to create window surface");

    surface = rawSurface;
}

bool Volcano::checkDeviceExtensionsSupport(const vk::PhysicalDevice& physicalDevice)
{
    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : physicalDevice.enumerateDeviceExtensionProperties())
        requiredExtensions.erase(extension.extensionName);

    return requiredExtensions.empty();
}

SwapChainSupportDetails Volcano::querySwapChainSupport(const vk::PhysicalDevice& physicalDevice)
{
    SwapChainSupportDetails details = {
        physicalDevice.getSurfaceCapabilitiesKHR(surface),
        physicalDevice.getSurfaceFormatsKHR(surface),
        physicalDevice.getSurfacePresentModesKHR(surface)
    };

    return details;
}

vk::SurfaceFormatKHR Volcano::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
{
    if(availableFormats.size() == 1 && availableFormats[0].format == vk::Format::eUndefined)
        return { vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };

    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == vk::Format::eB8G8R8A8Unorm && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            return availableFormat;
    }

    return availableFormats[0];
}

vk::PresentModeKHR Volcano::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
{
    vk::PresentModeKHR bestMode = vk::PresentModeKHR::eFifo;

    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox)
            return availablePresentMode;
        else if (availablePresentMode == vk::PresentModeKHR::eImmediate)
            bestMode = availablePresentMode;
    }

    return bestMode;
}

vk::Extent2D Volcano::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }    
    else
    {
        vk::Extent2D actualExtent = { static_cast<uint32_t>(window->getWidth()), static_cast<uint32_t>(window->getHeight()) };
        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

void Volcano::createSwapChain()
{
    auto swapChainSupport = querySwapChainSupport(Volcano::physicalDevice);

    auto surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    auto presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    auto extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.minImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        imageCount = swapChainSupport.capabilities.maxImageCount;

    vk::SwapchainCreateInfoKHR createInfo(
        vk::SwapchainCreateFlagsKHR(),
        Volcano::surface,
        imageCount,
        surfaceFormat.format,
        surfaceFormat.colorSpace,
        extent,
        1,
        vk::ImageUsageFlagBits::eColorAttachment
    );

    auto indices = findQueueFamily(physicalDevice);
    uint32_t queueFamilyIndicies[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily)
    {
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndicies;
    }
    else
    {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    
    createInfo.oldSwapchain = vk::SwapchainKHR(nullptr);

    try
    {
        Volcano::swapChain = device->createSwapchainKHR(createInfo);
    }
    catch (vk::SystemError& e)
    {
        throw std::runtime_error("Failed to create swapchain");
    }
}

#ifdef DEBUG
bool Volcano::checkValidationLayerSupport()
{
    auto availableLayers = vk::enumerateInstanceLayerProperties();
    
    for(const char* layerName: validationLayers)
    {
        bool layerFound = false;
        for(const auto& layerProperties : availableLayers)
        {
            if(strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
        {
            return false;
        }
    }

    return true;
}

VkBool32 Volcano::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

VkResult Volcano::createDebugUtilsMessengerEXT(vk::UniqueInstance& instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pCallback)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance.get(), "vkCreateDebugUtilsMessengerEXT");

    if(func != nullptr)
        return func(instance.get(), pCreateInfo, pAllocator, pCallback);
    else
        return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void Volcano::destroyDebugUtilMessengerEXT(vk::UniqueInstance& instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance.get(), "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
        func(instance.get(), debugMessenger, pAllocator);
}
#endif
