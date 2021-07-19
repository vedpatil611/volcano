#pragma once

#include <optional>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "SwapChainImage.h"

struct QueueFamilyIndicies;
struct SwapChainSupportDetails;
class Window;

class Volcano
{
    private:
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
        // Swapchain image format
        inline static vk::Format swapChainImageFormat;
        // Swapchain image extent
        inline static vk::Extent2D swapChainExtent;
        // Pointer to window
        inline static Window* window;
// Validation layer only exist for debug build
#ifdef DEBUG
        inline static std::vector<const char*> validationLayers = {
            "VK_LAYER_KHRONOS_validation"
        };
        inline static VkDebugUtilsMessengerEXT callback;
#endif
    public:
        // Initalize vulkan instance
        static void init(Window* window);
        // Destroy instance
        static void destroy();
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

        static void createGraphicsPipeline();
        // Create shader modules
        static vk::UniqueShaderModule createShaderModule(const std::vector<char>& code);
#ifdef DEBUG
        static bool checkValidationLayerSupport();
        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
        static VkResult createDebugUtilsMessengerEXT(vk::UniqueInstance& instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pCallback);
        static void destroyDebugUtilMessengerEXT(vk::UniqueInstance& instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
#endif
};
