#include "volcanoPCH.h"
#include "volcano.h"

#include <array>
#include <GLFW/glfw3.h>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include "SwapChainImage.h"
#include "SwapChainSupportDetails.h"
#include "QueueFamilyIndices.h"
#include "utils.h"
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

    Volcano::createSurface();
    Volcano::pickPhysicalDevice();
    Volcano::createLogicalDevice();
    Volcano::createSwapChain();
    Volcano::createRenderPass();
    Volcano::createGraphicsPipeline();
    Volcano::createFramebuffers();
    Volcano::createCommandPool();
    Volcano::createCommandBuffer();
    Volcano::recordCommands();
    Volcano::createSynchronization();
}

void Volcano::destroy()
{
    Volcano::device->waitIdle();
    for(int i = 0; i < MAX_FRAME_DRAWS; ++i)
    {
        Volcano::device->destroySemaphore(Volcano::renderFinished[i]);
        Volcano::device->destroySemaphore(Volcano::imageAvailable[i]);
        Volcano::device->destroyFence(Volcano::drawFences[i]);
    }
    Volcano::device->destroyCommandPool(Volcano::graphicsCommandPool);
    for(auto& framebuffer: Volcano::swapChainFramebuffers)
        Volcano::device->destroyFramebuffer(framebuffer);
    Volcano::device->destroyPipeline(Volcano::graphicsPipeline);
    Volcano::device->destroyPipelineLayout(Volcano::pipelineLayout);
    Volcano::device->destroyRenderPass(Volcano::renderPass);
    for(auto& image: swapChainImages)
    {
        Volcano::device->destroyImageView(image.imageView);
    }
    Volcano::device->destroySwapchainKHR(Volcano::swapChain);
    Volcano::instance->destroySurfaceKHR(Volcano::surface);

#ifdef DEBUG
    destroyDebugUtilMessengerEXT(instance, callback, nullptr);
#endif
}

void Volcano::draw()
{
    // Wait for fence to signal
    Volcano::device->waitForFences(Volcano::drawFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
    // Manually reset closed fences
    Volcano::device->resetFences(Volcano::drawFences[currentFrame]);
    
    // 1. Get next available image to draw to
    uint32_t index;
    index = Volcano::device->acquireNextImageKHR(Volcano::swapChain, std::numeric_limits<uint64_t>::max(), Volcano::imageAvailable[Volcano::currentFrame], nullptr).value;

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

    if(Volcano::presentQueue.presentKHR(presentInfo) != vk::Result::eSuccess)
        throw std::runtime_error("Failed to create r");

    currentFrame = (currentFrame + 1) % MAX_FRAME_DRAWS;
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
        throw std::runtime_error("Failed to create swapchain");
    }
    
    // Save swapchain image format and extent
    Volcano::swapChainImageFormat = surfaceFormat.format;
    Volcano::swapChainExtent = extent;

    // Retrive list of swapchain images
    auto swapchainImages = Volcano::device->getSwapchainImagesKHR(Volcano::swapChain);
    for(vk::Image image: swapchainImages)
    {
        // Store image handle
        SwapChainImage swapChainImage = {};
        swapChainImage.image = image;

        // Create image view
        swapChainImage.imageView = Volcano::createImageView(image, Volcano::swapChainImageFormat, vk::ImageAspectFlagBits::eColor);

        // Add to swapchain image list
        swapChainImages.push_back(swapChainImage);
    }
}

vk::ImageView Volcano::createImageView(vk::Image& image, vk::Format& format, vk::ImageAspectFlags aspectFlag)
{
    vk::ImageViewCreateInfo viewCreateInfo = {};
    viewCreateInfo.image = image;                                   // Image to create view for
    viewCreateInfo.viewType = vk::ImageViewType::e2D;                // Type of image
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
    viewCreateInfo.subresourceRange.layerCount = 1;                  // No of array layers

    try 
    {
        return Volcano::device->createImageView(viewCreateInfo);
    }
    catch(vk::SystemError& e)
    {
        throw std::runtime_error("Failed to create image view");
    }
}

void Volcano::createRenderPass()
{
    //Colour attachment of renderpass
    vk::AttachmentDescription colourAttachments = {};
    colourAttachments.format = Volcano::swapChainImageFormat;               // Use stored format
    colourAttachments.samples = vk::SampleCountFlagBits::e1;                // Single sample (no multisampling)
    colourAttachments.loadOp = vk::AttachmentLoadOp::eClear;                // Operation on first load -> clear (similar to glClear())
    colourAttachments.storeOp = vk::AttachmentStoreOp::eStore;              // Store data to draw
    colourAttachments.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;      // Don't care about stencil
    colourAttachments.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;

    // Framebuffer data can be stored as image. Image can have different layout
    colourAttachments.initialLayout = vk::ImageLayout::eUndefined;          // Undefine initial layout before render pass start
    // Initial layout to subpass and then subpass to final
    colourAttachments.finalLayout = vk::ImageLayout::ePresentSrcKHR;        // layout after render pass

    // Attachment reference
    vk::AttachmentReference colourAttachmentRef = {};
    colourAttachmentRef.attachment = 0;
    colourAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;  // Optimal for colour output

    // info about subpass that renderpass use
    vk::SubpassDescription subpass = {};
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;           // Pipeline type
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colourAttachmentRef;

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

    // renderpass create info
    vk::RenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colourAttachments;
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
        throw std::runtime_error("Failed to create render pass");
    }
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

    // Vertex inputs
    
    vk::PipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
    vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
    vertexInputCreateInfo.pVertexBindingDescriptions = nullptr;             // list of vertex bindings (data spacing/stride)
    vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
    vertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;           // list of vertex attribute (data format and where/location)

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
    //std::vector<vk::DynamicState> dynamicStateEnable = { 
        //vk::DynamicState::eViewport,                                        // Dynamic viewport can be resized with command buffer
        //vk::DynamicState::eScissor                                          // Resize with command buffer
    //};

    //vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};         // Set dynamic state data
    //dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnable.size());
    //dynamicStateCreateInfo.pDynamicStates = dynamicStateEnable.data();

    // Rasterizer
    // Convert primitive to fragment
    vk::PipelineRasterizationStateCreateInfo rasterizerInfo = {};
    rasterizerInfo.depthClampEnable = VK_FALSE;                                 // Clip things beyond near/far plane
    rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;                          // Wheather to create data or skip rasterizer
    rasterizerInfo.polygonMode = vk::PolygonMode::eFill;                        // Fill entire polygon
    rasterizerInfo.lineWidth = 1.0f;                                            // Line thickness (need gpu extension for line width other than 1)
    rasterizerInfo.cullMode = vk::CullModeFlagBits::eBack;                      // Do not draw useless back face
    rasterizerInfo.frontFace = vk::FrontFace::eClockwise;                       // Clockwise indices are front. Don't draw useless anti clockwise back face
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
    
    // Pipeline actual layout (layout of descriptor sets)
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    try 
    {
        Volcano::pipelineLayout = Volcano::device->createPipelineLayout(pipelineLayoutInfo);
    }
    catch(vk::SystemError& e)
    {
        throw std::runtime_error("Failed to create pipleline layout");
    }

    // Set depth stencil later

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
    pipelineInfo.pDepthStencilState = nullptr;
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
        throw std::runtime_error("Failed to create shader module");
    }
}

void Volcano::createFramebuffers()
{
    Volcano::swapChainFramebuffers.resize(Volcano::swapChainImages.size());         // Resize for same size

    for(size_t i = 0; i < swapChainFramebuffers.size(); ++i)
    {
        // Swapchain image view
        std::array<vk::ImageView, 1> attachments = {
            Volcano::swapChainImages[i].imageView
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
            throw std::runtime_error("Failed to create framebuffer");
        }
    }
}

void Volcano::createCommandPool()
{
    QueueFamilyIndicies queueFamilyIndices = Volcano::findQueueFamily(Volcano::physicalDevice);

    vk::CommandPoolCreateInfo poolInfo = {};
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();      // Queue family type that command buffer will use

    // Create graphocs queue family command pool
    try
    {
        Volcano::graphicsCommandPool = Volcano::device->createCommandPool(poolInfo);
    }
    catch(vk::SystemError& e)
    {
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

void Volcano::recordCommands()
{
    // Info about how to begin each command buffer
    
    vk::CommandBufferBeginInfo bufferBeginInfo = {};
    // No longer needed because of fences
    //bufferBeginInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;   // Buffer submit whem already submmited and awaiting execution

    // Info about how to begin render pass (only for graphics)
    vk::RenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.renderPass = Volcano::renderPass;
    renderPassBeginInfo.renderArea.offset = vk::Offset2D(0, 0);                 // Start point of render pass
    renderPassBeginInfo.renderArea.extent = Volcano::swapChainExtent;           // Size of region to run renderpass on
    
    vk::ClearValue clearValues[] = {
        vk::ClearColorValue(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f })
    };

    renderPassBeginInfo.pClearValues = clearValues;                             // List of clear values (list because might add depth later)
    renderPassBeginInfo.clearValueCount = 1;

    for(size_t i = 0; i < Volcano::commandBuffers.size(); ++i)
    {
        renderPassBeginInfo.framebuffer = Volcano::swapChainFramebuffers[i];

        try
        {
            Volcano::commandBuffers[i].begin(bufferBeginInfo);                  // Begin recording
        }
        catch(vk::SystemError& e)
        {
            throw std::runtime_error("Failed to begin recording command buffer");
        }

        {
            Volcano::commandBuffers[i].beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
            
            {
                // bind pipeline to be used in command buffer
                Volcano::commandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, Volcano::graphicsPipeline);
                
                // Execute pipline
                Volcano::commandBuffers[i].draw(3, 1, 0, 0);
            }

            Volcano::commandBuffers[i].endRenderPass();
        }
        
        try
        {
            Volcano::commandBuffers[i].end();                                   // End recording
        }
        catch(vk::SystemError& e)
        {
            throw std::runtime_error("Failed to end recording command buffer");
        }
    }
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
        throw std::runtime_error("Failed to create semaphore/fences");
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
