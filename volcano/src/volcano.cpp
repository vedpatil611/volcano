#include "volcanoPCH.h"
#include "volcano.h"

#include <array>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include "SwapChainImage.h"
#include "SwapChainSupportDetails.h"
#include "QueueFamilyIndices.h"
#include "utils.h"
#include "vertex.h"
#include "window.h"

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
        appInfo.apiVersion = VK_API_VERSION_1_2;
        
        vk::InstanceCreateInfo instanceInfo;
        instanceInfo.pApplicationInfo = &appInfo;
    
        auto extensions = getRequiredExtensions();

        instanceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        instanceInfo.ppEnabledExtensionNames = extensions.data();

#ifdef DEBUG
        instanceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
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
            UNUSED(e);
            throw std::runtime_error("Failed to create instance");
        }
    }

#ifdef DEBUG
    {   
        // Setup debug messenger
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

    Volcano::createSurface();
    Volcano::pickPhysicalDevice();
    Volcano::createLogicalDevice();
    Volcano::createSwapChain();
    Volcano::createDepthBufferImage();
    Volcano::createRenderPass();
    Volcano::createDescriptorSetLayout();
    Volcano::createGraphicsPipeline();
    Volcano::createFramebuffers();
    Volcano::createCommandPool();
 
    int tex = Volcano::createTextureImage("brick.png");

    mvp.proj = glm::perspective(glm::radians(45.0f), (float) Volcano::swapChainExtent.width / (float) Volcano::swapChainExtent.height, 0.1f, 100.0f);
    mvp.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 50.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    mvp.proj[1][1] *= -1;

    std::vector<Vertex> meshVertex = {
        {{  1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }},   // 0
        {{  1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f }},   // 1
        {{ -1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f }},   // 2
        {{ -1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 0.0f, 1.0f }}    // 3
    };
    
    std::vector<uint32_t> meshIndices = {
        0, 1, 2, 
        2, 3, 0
    };

    meshList.emplace_back(std::make_shared<Mesh>(Volcano::device.get(), meshVertex, meshIndices));
    meshList.emplace_back(std::make_shared<Mesh>(Volcano::device.get(), meshVertex, meshIndices));
   
   /* glm::mat4 meshModel = meshList[0]->getModel().model;
    meshModel = glm::rotate(meshModel, glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    meshList[0]->setModel(meshModel);*/

    Volcano::createCommandBuffer();
    //Volcano::allocateDynamicBufferTransferSpace();
    Volcano::createUniformBuffer();
    Volcano::createDescriptorPool();
    Volcano::createDescriptorSets();
    Volcano::createSynchronization();
}

void Volcano::destroy()
{
    Volcano::device->waitIdle();

    //_aligned_free(modelTransferSpace);

    for(int i = 0; i < MAX_FRAME_DRAWS; ++i)
    {
        Volcano::device->destroySemaphore(Volcano::renderFinished[i]);
        Volcano::device->destroySemaphore(Volcano::imageAvailable[i]);
        Volcano::device->destroyFence(Volcano::drawFences[i]);
    }
    
    for (size_t i = 0; i < Volcano::textureImages.size(); ++i)
    {
        Volcano::device->destroyImageView(Volcano::textureImageView[i]);
        Volcano::device->destroyImage(Volcano::textureImages[i]);
        Volcano::device->free(Volcano::textureImageMemory[i]);
    }

    Volcano::device->destroyImageView(depthBufferImageView);
    Volcano::device->destroyImage(depthBufferImage);
    Volcano::device->freeMemory(depthBufferMemory);

    //Volcano::device->freeDescriptorSets(Volcano::descriptorPool, Volcano::descriptorSets);
    Volcano::device->destroyDescriptorPool(Volcano::descriptorPool);
    Volcano::device->destroyDescriptorSetLayout(Volcano::descriptorSetLayout);
    for(size_t i = 0; i < swapChainImages.size(); ++i)
    {
        Volcano::device->destroyBuffer(vpUniformBuffer[i]);
        Volcano::device->freeMemory(vpUniformBufferMemory[i]);
        
        //Volcano::device->destroyBuffer(modelUniformBuffer[i]);
        //Volcano::device->freeMemory(modelUniformBufferMemory[i]);
    }
    Volcano::cleanupSwapChain();
    Volcano::device->destroyCommandPool(Volcano::graphicsCommandPool);
    Volcano::instance->destroySurfaceKHR(Volcano::surface);

#ifdef DEBUG
    destroyDebugUtilMessengerEXT(instance, callback, nullptr);
#endif
}

void Volcano::draw()
{
    // Wait for fence to signal
    vk::Result result = Volcano::device->waitForFences(Volcano::drawFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
    // Manually reset closed fences
    Volcano::device->resetFences(Volcano::drawFences[currentFrame]);
    
    // 1. Get next available image to draw to
    uint32_t index;
    try 
    {
        index = Volcano::device->acquireNextImageKHR(Volcano::swapChain, std::numeric_limits<uint64_t>::max(), Volcano::imageAvailable[Volcano::currentFrame], nullptr).value;
    }
    catch(vk::OutOfDateKHRError err)
    {
        Volcano::framebufferResized = true;
        Volcano::recreateSwapChain();
        return;
    }
    catch(vk::SystemError& err)
    {
        UNUSED(err);
        throw std::runtime_error("Failed to aquire swapchain image");
    }

    Volcano::recordCommands(index);
    Volcano::updateUniformBuffers(index);

    // 2. Submit command buffer to graphics queue
    // Queue submit info
    vk::SubmitInfo submitInfo = {};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &Volcano::imageAvailable[Volcano::currentFrame];          // Semaphore to wait for
    
    vk::PipelineStageFlags waitStages[] = {
        vk::PipelineStageFlagBits::eColorAttachmentOutput
    };
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &Volcano::commandBuffers[index];
    submitInfo.signalSemaphoreCount = 1;                                                    // Semaphore to signal after rendering is finished
    submitInfo.pSignalSemaphores = &Volcano::renderFinished[Volcano::currentFrame];

    try
    {
        Volcano::graphicsQueue.submit(submitInfo, Volcano::drawFences[currentFrame]);
    }
    catch(vk::SystemError& e)
    {
        UNUSED(e);
        throw std::runtime_error("Failed to submit command buffer to queue");
    }
    
    // Images is drawn to buffer till this stage

    // 3. Present image to screen
    vk::PresentInfoKHR presentInfo = {};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &Volcano::renderFinished[Volcano::currentFrame];          // Semaphore to wait for
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &Volcano::swapChain;
    presentInfo.pImageIndices = &index;

    vk::Result presentResult;
    try 
    {
        presentResult = Volcano::presentQueue.presentKHR(presentInfo);
    }
    catch(vk::OutOfDateKHRError& err)
    {
        UNUSED(err);
        presentResult = vk::Result::eErrorOutOfDateKHR;
    }
    catch(vk::SystemError& err)
    {
        throw std::runtime_error(err.what());
    }

    if(presentResult == vk::Result::eSuboptimalKHR || presentResult == vk::Result::eErrorOutOfDateKHR || framebufferResized)
    {
        Volcano::framebufferResized = false;
        Volcano::recreateSwapChain();
        return;
    }

    // if(Volcano::presentQueue.presentKHR(presentInfo) != vk::Result::eSuccess)
    //     throw std::runtime_error("Failed to create r");

    currentFrame = (currentFrame + 1) % MAX_FRAME_DRAWS;
}

void Volcano::updateModel(int modelId, const glm::mat4& newModel)
{
    if (modelId >= meshList.size()) return;

    meshList[modelId]->setModel(newModel);
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

    //vk::PhysicalDeviceProperties deviceProperties = physicalDevice.getProperties();
    //minBufferOffset = deviceProperties.limits.minUniformBufferOffsetAlignment;

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
        auto swapChainSupport = SwapChainSupportDetails::queryDetails(physicalDevice, Volcano::surface);
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

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };
    
    float queuePriority = 1.0f;

    for(uint32_t queueFamily : uniqueQueueFamilies)
    {
        auto queueCreateInfo = vk::DeviceQueueCreateInfo(
            vk::DeviceQueueCreateFlags(),
            queueFamily,
            1,
            &queuePriority
        );

        queueCreateInfos.push_back(queueCreateInfo);
    }

    auto deviceFeature = vk::PhysicalDeviceFeatures();
    auto createInfo = vk::DeviceCreateInfo(
        vk::DeviceCreateFlags(),
        static_cast<uint32_t>(queueCreateInfos.size()),
        queueCreateInfos.data()
    );
    createInfo.pEnabledFeatures = &deviceFeature;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

#ifdef DEBUG
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
#endif

    try
    {
        device = physicalDevice.createDeviceUnique(createInfo);
    }
    catch(vk::SystemError& e)
    {
        UNUSED(e);
        throw std::runtime_error("Failed to create logical device");
    }

    Volcano::graphicsQueue = device->getQueue(indices.graphicsFamily.value(), 0);
    Volcano::presentQueue = device->getQueue(indices.presentFamily.value(), 0);
}

void Volcano::createSurface()
{
    // Raw surface of basic c struct style is required because glfw requirement
    VkSurfaceKHR rawSurface;
    if(glfwCreateWindowSurface(*instance, window->getWindow(), nullptr, &rawSurface) != VK_SUCCESS)
        throw std::runtime_error("Failed to create window surface");
    
    Volcano::surface = rawSurface;
}

bool Volcano::checkDeviceExtensionsSupport(const vk::PhysicalDevice& physicalDevice)
{
    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : physicalDevice.enumerateDeviceExtensionProperties())
        requiredExtensions.erase(extension.extensionName);

    return requiredExtensions.empty();
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
    auto swapChainSupport = SwapChainSupportDetails::queryDetails(Volcano::physicalDevice, Volcano::surface);

    // Find optimal surface values for swapchain
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
        // create swapchain
        Volcano::swapChain = Volcano::device->createSwapchainKHR(createInfo);
    }
    catch (vk::SystemError& e)
    {
        UNUSED(e);
        throw std::runtime_error("Failed to create swapchain");
    }
    
    // Save swapchain image format and extent
    Volcano::swapChainImageFormat = surfaceFormat.format;
    Volcano::swapChainExtent = extent;

    // Retrive list of swapchain images
    auto swapchainImages = Volcano::device->getSwapchainImagesKHR(Volcano::swapChain);
    Volcano::swapChainImages.resize(swapchainImages.size());

    for(size_t i = 0; i < swapchainImages.size(); ++i)
    {
        // Store image handle
        SwapChainImage swapChainImage = {};
        Volcano::swapChainImages[i].image = swapchainImages[i];

        // Create image view
        Volcano::swapChainImages[i].imageView = Volcano::createImageView(swapchainImages[i], Volcano::swapChainImageFormat, vk::ImageAspectFlagBits::eColor);
    }
}

vk::ImageView Volcano::createImageView(const vk::Image& image, const vk::Format& format, const vk::ImageAspectFlags aspectFlag)
{
    vk::ImageViewCreateInfo viewCreateInfo = {};
    viewCreateInfo.image = image;                                   // Image to create view for
    viewCreateInfo.viewType = vk::ImageViewType::e2D;               // Type of image
    viewCreateInfo.format = format;                                 // Format of image data
    viewCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;  // Allows remaping of rgba components  
    viewCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;    
    viewCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;    
    viewCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;    
    
    // Subresources allow the view to view only part of image
    viewCreateInfo.subresourceRange.aspectMask = aspectFlag;
    viewCreateInfo.subresourceRange.baseMipLevel = 0;               // Start mipmap level
    viewCreateInfo.subresourceRange.levelCount = 1;                 // No of mipmap layers
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;             // Start array level to view from
    viewCreateInfo.subresourceRange.layerCount = 1;                 // No of array layers

    try 
    {
        return Volcano::device->createImageView(viewCreateInfo);
    }
    catch(vk::SystemError& e)
    {
        UNUSED(e);
        throw std::runtime_error("Failed to create image view");
    }
}

void Volcano::createRenderPass()
{
    // ATTACHMENTS
    //Colour attachment of renderpass
    vk::AttachmentDescription colourAttachment = {};
    colourAttachment.format = Volcano::swapChainImageFormat;               // Use stored format
    colourAttachment.samples = vk::SampleCountFlagBits::e1;                // Single sample (no multisampling)
    colourAttachment.loadOp = vk::AttachmentLoadOp::eClear;                // Operation on first load -> clear (similar to glClear())
    colourAttachment.storeOp = vk::AttachmentStoreOp::eStore;              // Store data to draw
    colourAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;      // Don't care about stencil
    colourAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;

    // Framebuffer data can be stored as image. Image can have different layout
    colourAttachment.initialLayout = vk::ImageLayout::eUndefined;          // Undefine initial layout before render pass start
    // Initial layout to subpass and then subpass to final
    colourAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;        // layout after render pass

    //Depth attachments of render pass
    vk::AttachmentDescription depthAttachment = {};
    depthAttachment.format = Volcano::depthFormat;
    depthAttachment.samples = vk::SampleCountFlagBits::e1;
    depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;            // Value never used. So we don't care
    depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
    depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    // Attachment reference
    vk::AttachmentReference colourAttachmentRef = {};
    colourAttachmentRef.attachment = 0;
    colourAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;  // Optimal for colour output

    vk::AttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    // info about subpass that renderpass use
    vk::SubpassDescription subpass = {};
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;           // Pipeline type
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colourAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;                  // Count not required as there can be only one depth attachment

    // When layout transition occur using subpass dependencies
    std::array<vk::SubpassDependency, 2> subpassDependencies;

    // Coversion from image layout optimal to image layout present src khr
    // Transition after
    subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependencies[0].srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;                                                 // At what stage transtion should occur
    subpassDependencies[0].srcAccessMask = vk::AccessFlagBits::eMemoryRead;                                                         // Read before conversion
    // Transition before
    subpassDependencies[0].dstSubpass = 0;                                                                                          // Before 1st subpass
    subpassDependencies[0].dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;                                        // Transtion before 
    subpassDependencies[0].dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;    // Before read and write color

    // Transition after
    subpassDependencies[1].srcSubpass = 0;                                                                                          // After 1st subpass
    subpassDependencies[1].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;                                        // At what stage transtion should occur
    subpassDependencies[1].srcAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;    // Read before conversion
    // Transition before
    subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;                                                                        // Before 1st subpass
    subpassDependencies[1].dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;                                                 // Transtion before 
    subpassDependencies[1].dstAccessMask = vk::AccessFlagBits::eMemoryRead;                                                         // Before read and write color

    std::array<vk::AttachmentDescription, 2> renderpassAttachments = {
        colourAttachment,
        depthAttachment
    };

    // renderpass create info
    vk::RenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.attachmentCount = static_cast<uint32_t>(renderpassAttachments.size());
    renderPassInfo.pAttachments = renderpassAttachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
    renderPassInfo.pDependencies = subpassDependencies.data();

    try
    {
        Volcano::renderPass = Volcano::device->createRenderPass(renderPassInfo);
    }
    catch (vk::SystemError& e)
    {
        UNUSED(e);
        throw std::runtime_error("Failed to create render pass");
    }
}

void Volcano::createDescriptorSetLayout()
{
    // Model View Uniform binding description
    vk::DescriptorSetLayoutBinding vpLayoutBinding = {};
    vpLayoutBinding.binding = 0;                                            // binding point in shaders
    vpLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;    // descriptor type
    vpLayoutBinding.descriptorCount = 1;                                    // just 1 struct of mvp
    vpLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;          // mvp struct bound in vertex shader
    vpLayoutBinding.pImmutableSamplers = nullptr;
    
    /*
    //Model binding info
    vk::DescriptorSetLayoutBinding modelLayoutBinding = {};
    modelLayoutBinding.binding = 1;                                         // binding 1 defined in vert shader
    modelLayoutBinding.descriptorType = vk::DescriptorType::eUniformBufferDynamic;
    modelLayoutBinding.descriptorCount = 1;
    modelLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
    modelLayoutBinding.pImmutableSamplers = nullptr;
    */
    
    std::array<vk::DescriptorSetLayoutBinding, 1> bindings = {
        vpLayoutBinding,
        //modelLayoutBinding
    };

    // create descriptor set layout with given binding
    vk::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());     // just 1 binding for now
    layoutCreateInfo.pBindings = bindings.data();                               // pointer to array of bindings
    
    // create descriptor set layout
    try
    {
        Volcano::descriptorSetLayout = Volcano::device->createDescriptorSetLayout(layoutCreateInfo);
    }
    catch(vk::SystemError& err)
    {
        UNUSED(err);
        throw std::runtime_error("Failed to create descriptor set layout");
    }
}

void Volcano::createPushConstantRange()
{
    pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(Model);
}

void Volcano::createGraphicsPipeline()
{
    // read compiled code
    auto vsCode = utils::readFile("shaders/vert.spv");
    auto fsCode = utils::readFile("shaders/frag.spv");

    // create shader modules for pipeline
    auto vertShaderModule = Volcano::createShaderModule(vsCode);
    auto fragShaderModule = Volcano::createShaderModule(fsCode);

    // Pipeline info for 2 types of shaders
    vk::PipelineShaderStageCreateInfo shaderStages[] = {
        {
            vk::PipelineShaderStageCreateFlags(),
            vk::ShaderStageFlagBits::eVertex,       // Vertex shader
            *vertShaderModule,                      // reference to module
            "main"                                  // Start at main function
        },
        {
            vk::PipelineShaderStageCreateFlags(),
            vk::ShaderStageFlagBits::eFragment,     // Fragment shader
            *fragShaderModule,
            "main"
        }
    };

    // Binding description data for single vertex as whole
    vk::VertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;                                         // can bind multiple stream of data
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = vk::VertexInputRate::eVertex;            // How to move between data after eavh vertex

    // How data for an attribute is defined within a vertex
    std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions;

    // position
    attributeDescriptions[0].binding = 0;                                   // Which binding the data is at
    attributeDescriptions[0].location = 0;                                  // Location in shader
    attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;         // Format the data will take
    attributeDescriptions[0].offset = offsetof(Vertex, pos);                // offset of data in struct

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = vk::Format::eR32G32B32A32Sfloat;
    attributeDescriptions[1].offset = offsetof(Vertex, col);

    // Vertex inputs
    vk::PipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
    vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescription;                 // list of vertex bindings (data spacing/stride)
    vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();      // list of vertex attribute (data format and where/location)

    // Input assembly
    vk::PipelineInputAssemblyStateCreateInfo assemblyCreateInfo = {};
    assemblyCreateInfo.topology = vk::PrimitiveTopology::eTriangleList;     // Primitive type to assembly vertex data to
    assemblyCreateInfo.primitiveRestartEnable = VK_FALSE;                   // Allow overriding of strip topology

    // Viewport and scissor
    vk::Viewport viewport = {};                                             // Equivalent to glViewport()
    viewport.x = 0.0f;                                                      // x start
    viewport.y = 0.0f;                                                      // y start
    viewport.width = static_cast<float>(Volcano::swapChainExtent.width);    // width
    viewport.height = static_cast<float>(Volcano::swapChainExtent.height);  // height
    viewport.minDepth = 0.0f;                                               // framebuffer depth
    viewport.maxDepth = 1.0f;

    vk::Rect2D scissor = {};
    scissor.offset = vk::Offset2D(0, 0);
    scissor.extent = Volcano::swapChainExtent;
    
    // save viewport and scissor data to viewport state
    vk::PipelineViewportStateCreateInfo viewportStateInfo = {};
    viewportStateInfo.viewportCount = 1;
    viewportStateInfo.pViewports = &viewport;
    viewportStateInfo.scissorCount = 1;
    viewportStateInfo.pScissors = &scissor;

    // Dynamic state
    std::vector<vk::DynamicState> dynamicStateEnable = { 
        vk::DynamicState::eViewport,                                        // Dynamic viewport can be resized with command buffer
        vk::DynamicState::eScissor                                          // Resize with command buffer
    };

    vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};         // Set dynamic state data
    dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnable.size());
    dynamicStateCreateInfo.pDynamicStates = dynamicStateEnable.data();

    // Rasterizer
    // Convert primitive to fragment
    vk::PipelineRasterizationStateCreateInfo rasterizerInfo = {};
    rasterizerInfo.depthClampEnable = VK_FALSE;                                 // Clip things beyond near/far plane
    rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;                          // Wheather to create data or skip rasterizer
    rasterizerInfo.polygonMode = vk::PolygonMode::eFill;                        // Fill entire polygon
    rasterizerInfo.lineWidth = 1.0f;                                            // Line thickness (need gpu extension for line width other than 1)
    rasterizerInfo.cullMode = vk::CullModeFlagBits::eBack;                      // Do not draw useless back face
    rasterizerInfo.frontFace = vk::FrontFace::eCounterClockwise;                // Clockwise indices are front. Don't draw useless anti clockwise back face
    rasterizerInfo.depthBiasEnable = VK_FALSE;                                  // true to overcome shadow acne

    // Multisampling
    vk::PipelineMultisampleStateCreateInfo multisampleInfo = {};
    multisampleInfo.sampleShadingEnable = VK_FALSE;                             // Disable multisampling
    multisampleInfo.rasterizationSamples = vk::SampleCountFlagBits::e1;         // Single sample (other value for amount of sample)

    // How to handle color blending
    vk::PipelineColorBlendAttachmentState colorBlendAttachment;
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | 
                    vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = VK_TRUE;                                 // enable blending
    // blending equation : (srcColorBlendFactor * newColour) colorBlendOp(dstBlendFactor * oldcolour)
    // (srcAlpha * newColour) + (1-srcAlpha * oldColour)
    
    // Color blending
    colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
    colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
    colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;

    // Alpha blending
    // (1 * newAlpha) + (0 * oldAlpha)
    colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;           // Do not blend alpha values
    colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
    colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;
    
    // Blending
    vk::PipelineColorBlendStateCreateInfo colorBlendCreateInfo = {};
    colorBlendCreateInfo.logicOpEnable = VK_FALSE;                              // Mathamatical > logical
    colorBlendCreateInfo.attachmentCount = 1;
    colorBlendCreateInfo.pAttachments = &colorBlendAttachment;
    
    // Setting push constant range
    Volcano::createPushConstantRange();

    // Pipeline actual layout (layout of descriptor sets)
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &Volcano::descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    try 
    {
        Volcano::pipelineLayout = Volcano::device->createPipelineLayout(pipelineLayoutInfo);
    }
    catch(vk::SystemError& e)
    {
        UNUSED(e);
        throw std::runtime_error("Failed to create pipleline layout");
    }

    // Depth stencil testing
    vk::PipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
    depthStencilCreateInfo.depthTestEnable = VK_TRUE;                   // enable depth checking for fragment write
    depthStencilCreateInfo.depthWriteEnable = VK_TRUE;                  // enable depth writing
    depthStencilCreateInfo.depthCompareOp = vk::CompareOp::eLess;       // If new value is less then override value
    depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;            // Depth bound test -> does depth exist between given min and max bounds
    depthStencilCreateInfo.stencilTestEnable = VK_FALSE;

    // Graphics pipeline creation
    vk::GraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.stageCount = 2;                        // 2 stage vertex and fragment shader
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputCreateInfo;
    pipelineInfo.pInputAssemblyState = &assemblyCreateInfo;
    pipelineInfo.pViewportState = &viewportStateInfo;
    pipelineInfo.pDynamicState = nullptr;
    pipelineInfo.pRasterizationState = &rasterizerInfo;
    pipelineInfo.pMultisampleState = &multisampleInfo;
    pipelineInfo.pColorBlendState = &colorBlendCreateInfo;
    pipelineInfo.pDepthStencilState = &depthStencilCreateInfo;
    pipelineInfo.layout = Volcano::pipelineLayout;
    pipelineInfo.renderPass = Volcano::renderPass;
    pipelineInfo.subpass = 0;

    // pipeline derivaties
    pipelineInfo.basePipelineHandle = nullptr;          // Existing pipeline to derive from
    pipelineInfo.basePipelineIndex = -1;                // index of existing pipeline

    try
    {
        Volcano::graphicsPipeline = Volcano::device->createGraphicsPipeline(nullptr, pipelineInfo).value;
    }
    catch (const std::exception&)
    {
        throw std::runtime_error("Failed to create graphics pipeline");
    }

    // Unique shader modules automatically destroys after pipeline creation
}

void Volcano::createDepthBufferImage()
{
    Volcano::depthFormat = Volcano::chooseSupportedFormat(
        { vk::Format::eD32SfloatS8Uint, vk::Format::eD32Sfloat, vk::Format::eD24UnormS8Uint },
        vk::ImageTiling::eOptimal, 
        vk::FormatFeatureFlagBits::eDepthStencilAttachment
    );

    Volcano::depthBufferImage = createImage(swapChainExtent.width, swapChainExtent.height, depthFormat, vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, depthBufferMemory);
    
    depthBufferImageView = createImageView(depthBufferImage, depthFormat, vk::ImageAspectFlagBits::eDepth);
}

vk::UniqueShaderModule Volcano::createShaderModule(const std::vector<char>& code)
{
    try 
    {
        vk::ShaderModuleCreateInfo shaderCreateInfo = vk::ShaderModuleCreateInfo(
            vk::ShaderModuleCreateFlags(),
            code.size(),
            reinterpret_cast<const uint32_t*>(code.data())
        );
        return Volcano::device->createShaderModuleUnique(shaderCreateInfo);
    }
    catch(vk::SystemError& err)
    {
        UNUSED(err);
        throw std::runtime_error("Failed to create shader module");
    }
}

vk::Image Volcano::createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags useFlags, vk::MemoryPropertyFlags propFlags, vk::DeviceMemory& imageMemory)
{
    // Create image
    vk::ImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.imageType = vk::ImageType::e2D;
    imageCreateInfo.extent.width = width;
    imageCreateInfo.extent.height = height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;                    // No of levels in image array
    imageCreateInfo.format = format;
    imageCreateInfo.tiling = tiling;
    imageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
    imageCreateInfo.usage = useFlags;
    imageCreateInfo.samples = vk::SampleCountFlagBits::e1;
    imageCreateInfo.sharingMode = vk::SharingMode::eExclusive;      // whetter image can be shared between queues
    
    vk::Image image;

    try 
    {
        image = Volcano::device->createImage(imageCreateInfo);
    }
    catch (vk::SystemError& e)
    {
        throw std::runtime_error(e.what());
    }

    // Create Memory for image
    vk::MemoryRequirements memRequirments = Volcano::device->getImageMemoryRequirements(image);

    vk::MemoryAllocateInfo memoryAllocInfo = {};
    memoryAllocInfo.allocationSize = memRequirments.size;
    memoryAllocInfo.memoryTypeIndex = findMemoryTypeIndex(memRequirments.memoryTypeBits, propFlags);

    try
    {
        imageMemory = Volcano::device->allocateMemory(memoryAllocInfo);
    }
    catch (const vk::SystemError& e)
    {
        throw std::runtime_error("Failed to allocate memory: " + std::string(e.what()));
    }

    Volcano::device->bindImageMemory(image, imageMemory, 0);

    return image;
}

vk::Format Volcano::chooseSupportedFormat(const std::vector<vk::Format>& formats, const vk::ImageTiling& tiling, const vk::FormatFeatureFlags& featureFlags)
{
    for (const vk::Format& format: formats) 
    {
        vk::FormatProperties properties = Volcano::physicalDevice.getFormatProperties(format);

        if (tiling == vk::ImageTiling::eLinear && (properties.linearTilingFeatures & featureFlags) == featureFlags)
        {
            return format;
        }
        else if (tiling == vk::ImageTiling::eOptimal && (properties.optimalTilingFeatures & featureFlags) == featureFlags)
        {
            return format;
        }
    }

    throw std::runtime_error("Failed to find matching format");
}

void Volcano::createFramebuffers()
{
    Volcano::swapChainFramebuffers.resize(Volcano::swapChainImages.size());         // Resize for same size

    for(size_t i = 0; i < swapChainFramebuffers.size(); ++i)
    {
        // Swapchain image view
        std::array<vk::ImageView, 2> attachments = {
            Volcano::swapChainImages[i].imageView,
            Volcano::depthBufferImageView
        };

        // Framebuffer info
        vk::FramebufferCreateInfo framebufferCreateInfo = {};
        framebufferCreateInfo.renderPass = Volcano::renderPass;
        framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferCreateInfo.pAttachments = attachments.data();
        framebufferCreateInfo.width = Volcano::swapChainExtent.width;
        framebufferCreateInfo.height = Volcano::swapChainExtent.height;
        framebufferCreateInfo.layers = 1;

        try
        {
            Volcano::swapChainFramebuffers[i] = Volcano::device->createFramebuffer(framebufferCreateInfo);
        }
        catch(vk::SystemError& e)
        {
            UNUSED(e);
            throw std::runtime_error("Failed to create framebuffer");
        }
    }
}

void Volcano::createCommandPool()
{
    QueueFamilyIndicies queueFamilyIndices = Volcano::findQueueFamily(Volcano::physicalDevice);

    vk::CommandPoolCreateInfo poolInfo = {};
    poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;        // Allow reseting command buffer to update push constant
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();      // Queue family type that command buffer will use

    // Create graphocs queue family command pool
    try
    {
        Volcano::graphicsCommandPool = Volcano::device->createCommandPool(poolInfo);
    }
    catch(vk::SystemError& e)
    {
        UNUSED(e);
        throw std::runtime_error("Failed to create a command pool");
    }
}

void Volcano::createCommandBuffer()
{
    Volcano::commandBuffers.resize(Volcano::swapChainFramebuffers.size());
    
    vk::CommandBufferAllocateInfo cbAllocInfo = {};
    cbAllocInfo.commandPool = Volcano::graphicsCommandPool;
    cbAllocInfo.level = vk::CommandBufferLevel::ePrimary;                       // Buffer directly submit to queue. Can't be called by other buffers
    cbAllocInfo.commandBufferCount = static_cast<uint32_t>(Volcano::commandBuffers.size());

    // allocate command buffer
    vk::Result result = Volcano::device->allocateCommandBuffers(&cbAllocInfo, Volcano::commandBuffers.data());
    if(result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to allocate command buffers");
}

void Volcano::recordCommands(uint32_t currentImage)
{
    // Info about how to begin each command buffer
    
    vk::CommandBufferBeginInfo bufferBeginInfo = {};
    // No longer needed because of fences
    //bufferBeginInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;     // Buffer submit whem already submmited and awaiting execution

    // Info about how to begin render pass (only for graphics)
    vk::RenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.renderPass = Volcano::renderPass;
    renderPassBeginInfo.renderArea.offset = vk::Offset2D(0, 0);                 // Start point of render pass
    renderPassBeginInfo.renderArea.extent = Volcano::swapChainExtent;           // Size of region to run renderpass on
    
    std::array<vk::ClearValue, 2> clearValues = {
        vk::ClearColorValue(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f }),
        vk::ClearDepthStencilValue(1.0f)
    };

    renderPassBeginInfo.pClearValues = clearValues.data();                             // List of clear values (list because might add depth later)
    renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());

    //for(size_t i = 0; i < Volcano::commandBuffers.size(); ++i)
    //{
        renderPassBeginInfo.framebuffer = Volcano::swapChainFramebuffers[currentImage];

        try
        {
            Volcano::commandBuffers[currentImage].begin(bufferBeginInfo);                  // Begin recording
        }
        catch(const vk::SystemError& e)
        {
            UNUSED(e);
            throw std::runtime_error("Failed to begin recording command buffer");
        }

        {
            Volcano::commandBuffers[currentImage].beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
            
            {
                // bind pipeline to be used in command buffer
                Volcano::commandBuffers[currentImage].bindPipeline(vk::PipelineBindPoint::eGraphics, Volcano::graphicsPipeline);
               
                for(size_t j = 0; j < meshList.size(); ++j)
                {
                    // Bind vertex buffer
                    vk::Buffer vertexBuffer[] = { meshList[j]->getVertexBuffer() };             // list of buffer to bind
                    vk::DeviceSize offsets[] = { 0 };                                           // list of offsets
                    Volcano::commandBuffers[currentImage].bindVertexBuffers(0, 1, vertexBuffer, offsets);  // bind buffer before drawing
                
                    // Bind index buffer
                    Volcano::commandBuffers[currentImage].bindIndexBuffer(meshList[j]->getIndexBuffer(), 0, vk::IndexType::eUint32);
                
                    // Dynamic offset amount
                    // std::array<uint32_t, 1> dynamicOffsets = {
                    //    static_cast<uint32_t>(modelUniformAlignment) * j
                    // };
                    //uint32_t dynamicOffset = static_cast<uint32_t>(modelUniformAlignment) * j;

                    Volcano::commandBuffers[currentImage].pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0 /* Offset of data*/, sizeof(Model), &meshList[j]->getModel());

                    // bind descriptor sets
                    Volcano::commandBuffers[currentImage].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, Volcano::pipelineLayout, 0,
                        Volcano::descriptorSets[currentImage], nullptr);

                    // Execute pipline
                    Volcano::commandBuffers[currentImage].drawIndexed(static_cast<uint32_t>(meshList[j]->getIndexCount()), 1, 0, 0, 0);
                }
            }
            Volcano::commandBuffers[currentImage].endRenderPass();
        }
        
        try
        {
            Volcano::commandBuffers[currentImage].end();                                   // End recording
        }
        catch(vk::SystemError& e)
        {
            UNUSED(e);
            throw std::runtime_error("Failed to end recording command buffer");
        }
    //}
}

void Volcano::createSynchronization()
{
    Volcano::imageAvailable.resize(MAX_FRAME_DRAWS);
    Volcano::renderFinished.resize(MAX_FRAME_DRAWS);
    Volcano::drawFences.resize(MAX_FRAME_DRAWS);

    // Semaphore creation info
    vk::SemaphoreCreateInfo semaphoreCreateInfo = {};                           // Only deafault struct type is required

    // Fence create info
    vk::FenceCreateInfo fenceInfo = {};
    fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

    try 
    {
        for (int i = 0; i < MAX_FRAME_DRAWS; ++i)
        {
            Volcano::imageAvailable[i] = Volcano::device->createSemaphore(semaphoreCreateInfo);
            Volcano::renderFinished[i] = Volcano::device->createSemaphore(semaphoreCreateInfo);
            Volcano::drawFences[i] = Volcano::device->createFence(fenceInfo);
        }
    }
    catch(vk::SystemError& e)
    {
        UNUSED(e);
        throw std::runtime_error("Failed to create semaphore/fences");
    }
}

void Volcano::createBuffer(vk::DeviceSize bufferSize, vk::BufferUsageFlags bufferUsageFlags, 
                vk::MemoryPropertyFlags bufferProperties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory)
{
    vk::BufferCreateInfo bufferInfo = {};
    bufferInfo.size = bufferSize;
    bufferInfo.usage = bufferUsageFlags;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    try 
    {
        buffer = Volcano::device->createBuffer(bufferInfo);
    }
    catch(vk::SystemError& e)
    {
        UNUSED(e);
        throw std::runtime_error("Failed to create vertex buffer");
    }

    vk::MemoryRequirements memRequirements = {};
    memRequirements = Volcano::device->getBufferMemoryRequirements(buffer);

    vk::MemoryAllocateInfo memAllocInfo = {};
    memAllocInfo.allocationSize = memRequirements.size;

    // Host visible bit -> cpu can interact
    // Conherant bit -> allows placement of data straight into buffer after mapping 
    memAllocInfo.memoryTypeIndex = findMemoryTypeIndex(memRequirements.memoryTypeBits, bufferProperties);
    
    try 
    {
        // Allocate to device memory
        bufferMemory = Volcano::device->allocateMemory(memAllocInfo);
    }
    catch(vk::SystemError& e)
    {
        UNUSED(e);
        throw std::runtime_error("Failed to allocate memory");
    }

    // Allocate mem to vertex buffer
    Volcano::device->bindBufferMemory(buffer, bufferMemory, 0);
 
}

uint32_t Volcano::findMemoryTypeIndex(uint32_t allowedTypes, vk::MemoryPropertyFlags properties)
{
    vk::PhysicalDeviceMemoryProperties memProperties;
    memProperties = Volcano::physicalDevice.getMemoryProperties();

    for(uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
    {
        if((allowedTypes & (1 << i)) && memProperties.memoryTypes[i].propertyFlags & properties)
        {
            // Memory type is valid so return it
            return i;
        }
    }

    return 0;
}

void Volcano::copyBuffer(vk::Buffer& src, vk::Buffer& dst, vk::DeviceSize bufferSize)
{
    vk::CommandBuffer transferCommandBuffer = Volcano::beginCopyBuffer(Volcano::graphicsCommandPool);

    {
        // Region of data to copy from into
        vk::BufferCopy bufferCopyRegion = {};
        bufferCopyRegion.srcOffset = 0;
        bufferCopyRegion.dstOffset = 0;
        bufferCopyRegion.size = bufferSize;

        transferCommandBuffer.copyBuffer(src, dst, bufferCopyRegion);
    }
 
    Volcano::endCopyBuffer(Volcano::graphicsCommandPool, Volcano::graphicsQueue, transferCommandBuffer);
}

void Volcano::copyImageBuffer(vk::Buffer& src, vk::Image& image, uint32_t width, uint32_t height)
{
    vk::CommandBuffer transferCommandBuffer = Volcano::beginCopyBuffer(Volcano::graphicsCommandPool);

    {
        vk::BufferImageCopy imageRegion = {};
        imageRegion.bufferOffset = 0;
        imageRegion.bufferRowLength = 0;                // row lenght for data spacing
        imageRegion.bufferImageHeight = 0;              // row height for image data spacing
        imageRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        imageRegion.imageSubresource.mipLevel = 0;
        imageRegion.imageSubresource.baseArrayLayer = 0;
        imageRegion.imageSubresource.layerCount = 1;
        imageRegion.imageOffset = vk::Offset3D(0, 0, 0);
        imageRegion.imageExtent = vk::Extent3D(width, height, 1);

        transferCommandBuffer.copyBufferToImage(src, image, vk::ImageLayout::eTransferDstOptimal, imageRegion);
    }

    Volcano::endCopyBuffer(Volcano::graphicsCommandPool, Volcano::graphicsQueue, transferCommandBuffer);
}

vk::CommandBuffer Volcano::beginCopyBuffer(vk::CommandPool& commandPool)
{
    // Command Buffer to hold transfer commands
    vk::CommandBuffer transferCommandBuffer;

    // command buffer details
    vk::CommandBufferAllocateInfo allocInfo = {};
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = commandPool;       // graphics command pool is same as transfer command pool
    allocInfo.commandBufferCount = 1;

    // Allocate command buffer from pool
    transferCommandBuffer = Volcano::device->allocateCommandBuffers(allocInfo)[0];

    // Info to begin command buffer
    vk::CommandBufferBeginInfo beginInfo = {};
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    transferCommandBuffer.begin(beginInfo);

    return transferCommandBuffer;
}

void Volcano::endCopyBuffer(vk::CommandPool& commandPool, vk::Queue& queue, vk::CommandBuffer& transferCommandBuffer)
{
    transferCommandBuffer.end();

    // Queue submit info
    vk::SubmitInfo submitInfo = {};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &transferCommandBuffer;

    // Submit transfer command to transfer queue and wait till it finish
    // Graphics queue is same as transfer queue
    queue.submit(submitInfo);
    queue.waitIdle();

    // Free command buffer back to pool
    Volcano::device->freeCommandBuffers(commandPool, transferCommandBuffer);
}

void Volcano::recreateSwapChain() 
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(window->getWindow(), &width, &height);

    while(width == 0 || height == 0)
    {
        glfwGetFramebufferSize(window->getWindow(), &width, &height);
        glfwWaitEvents();
    }

    Volcano::device->waitIdle();
   
    Volcano::cleanupSwapChain();

    Volcano::createSwapChain();
    Volcano::createRenderPass();
    Volcano::createGraphicsPipeline();
    Volcano::createFramebuffers();
    //Volcano::createCommandPool();
    Volcano::createCommandBuffer();
    //Volcano::recordCommands();    
}

void Volcano::cleanupSwapChain()
{
    // Destroy all objects depended by swapchain first
    for(auto& framebuffer: Volcano::swapChainFramebuffers)
        Volcano::device->destroyFramebuffer(framebuffer);

    Volcano::device->freeCommandBuffers(Volcano::graphicsCommandPool, Volcano::commandBuffers);
    Volcano::device->destroyPipeline(Volcano::graphicsPipeline);
    Volcano::device->destroyPipelineLayout(Volcano::pipelineLayout);
    Volcano::device->destroyRenderPass(Volcano::renderPass);
    for(auto& imageView: Volcano::swapChainImages)
        Volcano::device->destroyImageView(imageView.imageView);

    Volcano::device->destroySwapchainKHR(Volcano::swapChain);
}

void Volcano::createUniformBuffer() 
{
    // View Projection buffer size
    vk::DeviceSize vpBufferSize = sizeof(UBOViewProj);
    
    // Model Buffer Size
    //vk::DeviceSize modelBufferSize = modelUniformAlignment * MAX_OBJECTS;

    // one uniform buffer for each image
    Volcano::vpUniformBuffer.resize(Volcano::swapChainImages.size());
    Volcano::vpUniformBufferMemory.resize(Volcano::swapChainImages.size());
    
    //Volcano::modelUniformBuffer.resize(Volcano::swapChainImages.size());
    //Volcano::modelUniformBufferMemory.resize(Volcano::swapChainImages.size());
    
    // Create uniform buffer
    for (size_t i = 0; i < Volcano::swapChainImages.size(); ++i)
    {
        createBuffer(vpBufferSize, vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, vpUniformBuffer[i], vpUniformBufferMemory[i]); 
        
        /*createBuffer(modelBufferSize, vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, modelUniformBuffer[i], modelUniformBufferMemory[i]);*/
    }
}

void Volcano::createDescriptorPool()
{
    vk::DescriptorPoolSize vpPoolSize = {};
    vpPoolSize.type = vk::DescriptorType::eUniformBuffer;
    vpPoolSize.descriptorCount = static_cast<uint32_t>(vpUniformBuffer.size());
    
    /*
    vk::DescriptorPoolSize modelPoolSize = {};
    modelPoolSize.type = vk::DescriptorType::eUniformBufferDynamic;
    modelPoolSize.descriptorCount = static_cast<uint32_t>(modelUniformBuffer.size());
    */

    std::array<vk::DescriptorPoolSize, 1> poolSizes = {
        vpPoolSize,
        //modelPoolSize
    };

    vk::DescriptorPoolCreateInfo poolCreateInfo = {};
    poolCreateInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());
    poolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolCreateInfo.pPoolSizes = poolSizes.data();

    try 
    {
        Volcano::descriptorPool = Volcano::device->createDescriptorPool(poolCreateInfo);
    }
    catch(vk::SystemError& e)
    {
        UNUSED(e);
        throw std::runtime_error("Failed to create descriptor pool");
    }
}

void Volcano::createDescriptorSets()
{
    std::vector<vk::DescriptorSetLayout> setLayouts(Volcano::swapChainImages.size(), Volcano::descriptorSetLayout);

    // alloc info
    vk::DescriptorSetAllocateInfo setAllocInfo = {};
    setAllocInfo.descriptorPool = Volcano::descriptorPool;
    setAllocInfo.descriptorSetCount = static_cast<uint32_t>(Volcano::swapChainImages.size());
    setAllocInfo.pSetLayouts = setLayouts.data();

    try
    {
        Volcano::descriptorSets = Volcano::device->allocateDescriptorSets(setAllocInfo);
    }
    catch(vk::SystemError& e)
    {
        UNUSED(e);
        throw std::runtime_error("Failed to allocate descriptor sets");
    }

    // update all descriptor set buffer binding
    for(size_t i = 0; i < Volcano::swapChainImages.size(); ++i)
    {
        // Buffer info and data offset info 
        vk::DescriptorBufferInfo vpBufferInfo = {};
        vpBufferInfo.buffer = Volcano::vpUniformBuffer[i];                   // Buffer to get data from
        vpBufferInfo.offset = 0;                                           // Start at
        vpBufferInfo.range = sizeof(UBOViewProj);

        // data about connection between binding and buffer
        vk::WriteDescriptorSet vpSetWrite = {};
        vpSetWrite.dstSet = Volcano::descriptorSets[i];                    // descriptor set to update
        vpSetWrite.dstBinding = 0;                                         // layout (binding = 0)
        vpSetWrite.dstArrayElement = 0;                                    // if uniform is array then index to update
        vpSetWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
        vpSetWrite.descriptorCount = 1;                                    // Amount to update
        vpSetWrite.pBufferInfo = &vpBufferInfo;

        /*
        vk::DescriptorBufferInfo modelBufferInfo = {};
        modelBufferInfo.buffer = modelUniformBuffer[i];
        modelBufferInfo.offset = 0;
        modelBufferInfo.range = modelUniformAlignment * MAX_OBJECTS;

        vk::WriteDescriptorSet modelWriteSet = {};
        modelWriteSet.dstSet = descriptorSets[i];
        modelWriteSet.dstBinding = 1;
        modelWriteSet.dstArrayElement = 0;
        modelWriteSet.descriptorType = vk::DescriptorType::eUniformBufferDynamic;
        modelWriteSet.descriptorCount = 1;
        modelWriteSet.pBufferInfo = &modelBufferInfo;
        */

        std::array<vk::WriteDescriptorSet, 1> setWrites = {
            vpSetWrite,
            //modelWriteSet
        };

        // update descripter set with new buffer binding info
        Volcano::device->updateDescriptorSets(setWrites, nullptr);
    }

    //for (auto& layout: setLayouts)
        //Volcano::device->destroyDescriptorSetLayout(layout);
}

void Volcano::updateUniformBuffers(uint32_t imageIndex)
{
    // copy vp data
    void* data = Volcano::device->mapMemory(Volcano::vpUniformBufferMemory[imageIndex], 0, sizeof(UBOViewProj));
    memcpy(data, &mvp, sizeof(UBOViewProj));
    Volcano::device->unmapMemory(Volcano::vpUniformBufferMemory[imageIndex]);

    /*
    * 
    * Code for dynamic descriptors
    * No longer needed
    * 
    // copy model data
    for (size_t i = 0; i < meshList.size(); ++i)
    {
        // Beginning of trasfer space + offset (ie total size that can be allocated) <-- store model data here
        Model* thisModel = (Model*)((uint64_t)modelTransferSpace + (i * modelUniformAlignment));
        *thisModel = meshList[i]->getModel();
    }

    data = Volcano::device->mapMemory(Volcano::modelUniformBufferMemory[imageIndex], 0, modelUniformAlignment * meshList.size());
    memcpy(data, modelTransferSpace, modelUniformAlignment * meshList.size());
    Volcano::device->unmapMemory(Volcano::modelUniformBufferMemory[imageIndex]);
    */
}

void Volcano::allocateDynamicBufferTransferSpace()
{
    /*
    // if minBufferOffset = 256 -> 256 - 1 = 255 -> all 7 bits are 1 and highest bit is 0 -> complemnet it
    // Pro tip: 2's complement of a number of power of is itself but -ve
    // if sizeof ubomdel is still greater than 0 this means data size is greater than 256 
    //  (1^0*)1 00000000 is valid. Last all bits should be zero
    modelUniformAlignment = (sizeof(Model) + minBufferOffset - 1) & ~(minBufferOffset - 1);                      // Calclutaion alignemnt of model data

    // Create space to hlod dynamic memory
    modelTransferSpace = (Model*) _aligned_malloc(modelUniformAlignment * MAX_OBJECTS, modelUniformAlignment);
    */
}

stbi_uc* Volcano::loadTextureFile(const char* filename, int& width, int& height, vk::DeviceSize& imageSize)
{
    // No of channels image uses
    int channels;

    std::string fileLoc = "Textures/" + std::string(std::move(filename));

    stbi_uc* image = stbi_load(fileLoc.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (!image)
        throw std::runtime_error("Failed to load texture: " + std::string(std::move(filename)));

    imageSize = static_cast<uint64_t>(width) * height * 4;

    return image;
}

int Volcano::createTextureImage(const char* filename)
{
    // loading image data
    int width, height;
    vk::DeviceSize imageSize;
    stbi_uc* imageData = loadTextureFile(filename, width, height, imageSize);

    // Create staging buffer to hold loaded data to copy to device
    vk::Buffer imageStagingBuffer;
    vk::DeviceMemory imageStagingMemory;

    Volcano::createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc, 
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible,
        imageStagingBuffer, imageStagingMemory
    );

    // copying data to staging buffer
    void* data = Volcano::device->mapMemory(imageStagingMemory, 0, imageSize);
    memcpy(data, imageData, static_cast<uint32_t>(imageSize));
    Volcano::device->unmapMemory(imageStagingMemory);

    // Free image data
    stbi_image_free(imageData);

    // Create image to hold texture
    vk::Image texImage;
    vk::DeviceMemory texImageMemory;

    texImage = Volcano::createImage(width, height, vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::MemoryPropertyFlagBits::eDeviceLocal, texImageMemory
    );

    // transition image state before copy
    Volcano::transitionImageLayout(Volcano::graphicsQueue, Volcano::graphicsCommandPool,
        texImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal
    );

    // copy image data
    Volcano::copyImageBuffer(imageStagingBuffer, texImage, width, height);
    
    // transtion image to shader readable stage
    Volcano::transitionImageLayout(Volcano::graphicsQueue, Volcano::graphicsCommandPool, texImage,
        vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

    // save texture data and memory
    Volcano::textureImages.emplace_back(texImage);
    Volcano::textureImageMemory.emplace_back(texImageMemory);

    // destory staging image and buffer
    Volcano::device->destroyBuffer(imageStagingBuffer);
    Volcano::device->freeMemory(imageStagingMemory);

    // Return index of texture
    return static_cast<int>(textureImages.size() - 1);
}

int Volcano::createTexture(const char* filename)
{
    int textureImageLoc = Volcano::createTextureImage(filename);

    vk::ImageView imageView = Volcano::createImageView(Volcano::textureImages[textureImageLoc], vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor);
    textureImageView.push_back(imageView);

    // Todo: Create descriptor set later
    return 0;
}

void Volcano::transitionImageLayout(vk::Queue queue, vk::CommandPool commandPool, vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
    vk::CommandBuffer commandBuffer = Volcano::beginCopyBuffer(commandPool);

    vk::ImageMemoryBarrier memoryBarrier = {};
    memoryBarrier.oldLayout = oldLayout;
    memoryBarrier.newLayout = newLayout;
    memoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    memoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    memoryBarrier.image = image;
    memoryBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    memoryBarrier.subresourceRange.baseMipLevel = 0;
    memoryBarrier.subresourceRange.levelCount = 1;
    memoryBarrier.subresourceRange.baseArrayLayer = 0;
    memoryBarrier.subresourceRange.layerCount = 1;

    vk::PipelineStageFlags srcFlag;
    vk::PipelineStageFlags dstFlag;

    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
    {
        memoryBarrier.srcAccessMask = (vk::AccessFlags)(0);
        memoryBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        srcFlag = vk::PipelineStageFlagBits::eTopOfPipe;
        dstFlag = vk::PipelineStageFlagBits::eTransfer;
    }
    else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        memoryBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        memoryBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        
        srcFlag = vk::PipelineStageFlagBits::eTransfer;
        dstFlag = vk::PipelineStageFlagBits::eFragmentShader;
    }
    
    commandBuffer.pipelineBarrier(
        srcFlag, dstFlag,                                           // match src and dst access mask
        (vk::DependencyFlags)0,                                     // dependency flag
        nullptr, nullptr,
        memoryBarrier
    );
    //{
    //    // Region of data to copy from into
    //    vk::BufferCopy bufferCopyRegion = {};
    //    bufferCopyRegion.srcOffset = 0;
    //    bufferCopyRegion.dstOffset = 0;
    //    bufferCopyRegion.size = bufferSize;

    //    commandBuffer.copyBuffer(src, dst, bufferCopyRegion);
    //}

    Volcano::endCopyBuffer(commandPool, queue, commandBuffer);
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
