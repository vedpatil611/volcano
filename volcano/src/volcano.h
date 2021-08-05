#pragma once

#include <optional>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <memory>
#include "mesh.h"
#include "SwapChainImage.h"

struct QueueFamilyIndicies;
struct SwapChainSupportDetails;
class Window;

#define MAX_FRAME_DRAWS 2

class Volcano
{
    private:
        inline static int currentFrame = 0;
        // UniqueInstance destruction is done automatically
        inline static vk::UniqueInstance instance;
        // GPU device
        inline static vk::PhysicalDevice physicalDevice;
        // Logical device
        inline static vk::UniqueDevice device;
        // Graphics queue
        inline static vk::Queue graphicsQueue;
        // Presentation queue
        inline static vk::Queue presentQueue;
        // Surface
        inline static vk::SurfaceKHR surface;
        // List of device extensions
        inline static const std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME         // Swap chain extensions (macro)
        };
        // Swap chain
        inline static vk::SwapchainKHR swapChain;
        // List of swap chain images
        inline static std::vector<SwapChainImage> swapChainImages;
        // List of framebuffers
        inline static std::vector<vk::Framebuffer> swapChainFramebuffers;
        // List of command buffers
        inline static std::vector<vk::CommandBuffer> commandBuffers;
        // Swapchain image format
        inline static vk::Format swapChainImageFormat;
        // Swapchain image extent
        inline static vk::Extent2D swapChainExtent;
        // Graphics pipeline
        inline static vk::Pipeline graphicsPipeline;
        // Pipeline layout (uniform/descriptor set layout)
        inline static vk::PipelineLayout pipelineLayout;
        // Renderpass
        inline static vk::RenderPass renderPass;
        // Graphics command pool
        inline static vk::CommandPool graphicsCommandPool;
        // Singal after image is ready to be rendered
        inline static std::vector<vk::Semaphore> imageAvailable;
        // Signal after rendering is finised
        inline static std::vector<vk::Semaphore> renderFinished;
        // Fences for cpu-gpu sync
        inline static std::vector<vk::Fence> drawFences;
        // Pointer to window
        inline static Window* window;
// Validation layer only exist for debug build
#ifdef DEBUG
        inline static std::vector<const char*> validationLayers = {
            "VK_LAYER_KHRONOS_validation"
        };
        inline static VkDebugUtilsMessengerEXT callback;
#endif
        // Scene object
        inline static std::shared_ptr<Mesh> mesh; 
    public:
        // Initalize vulkan instance
        static void init(Window* window);
        // Destroy instance
        static void destroy();
        // Render to screen
        static void draw();
        // Create buffer based on given flag
        static void createBuffer(vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::DeviceSize bufferSize, 
            vk::BufferUsageFlags bufferUsageFlags, vk::MemoryPropertyFlags bufferProperties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory);
    private:
        // Query and select logical device
        static void pickPhysicalDevice();
        // Check if GPU device is suitable
        static bool isDeviceSuitable(const vk::PhysicalDevice& device);
        // Find queue family within gpu device
        static QueueFamilyIndicies findQueueFamily(const vk::PhysicalDevice& device);
        // Get glfw extensions
        static std::vector<const char*> getRequiredExtensions();
        // Initalize logical device
        static void createLogicalDevice();
        // Create surface for graphics rendering
        static void createSurface();
        // Check for extension support
        static bool checkDeviceExtensionsSupport(const vk::PhysicalDevice& physicalDevice);
        // Pick appropriate swap surface format
        static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
        // Pick appropriate swap presentaion mode
        static vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentMode);
        // Pick swap extent
        static vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
        // Create swapchain
        static void createSwapChain();
        // Create image view
        static vk::ImageView createImageView(vk::Image& image, vk::Format& format, vk::ImageAspectFlags aspectFlag);
        // Create render pass
        static void createRenderPass();
        // Create graphics pipeline layout (set up all imp stuff)
        static void createGraphicsPipeline();
        // Create shader modules
        static vk::UniqueShaderModule createShaderModule(const std::vector<char>& code);
        // Create list of framebuffers
        static void createFramebuffers();
        // Create command pool
        static void createCommandPool();
        // Create command buffer
        static void createCommandBuffer();
        // record commands to buffer
        static void recordCommands();
        // Create semaphores for synchronization
        static void createSynchronization();

        static uint32_t findMemoryTypeIndex(uint32_t allowedTypes, vk::MemoryPropertyFlags properties);
#ifdef DEBUG
        static bool checkValidationLayerSupport();
        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
        static VkResult createDebugUtilsMessengerEXT(vk::UniqueInstance& instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pCallback);
        static void destroyDebugUtilMessengerEXT(vk::UniqueInstance& instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
#endif
};
