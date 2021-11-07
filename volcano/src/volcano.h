#pragma once

#include <optional>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <memory>
#include <stb_image/stb_image.h>
#include "mesh.h"
#include "SwapChainImage.h"

struct QueueFamilyIndicies;
struct SwapChainSupportDetails;
class Window;

#define MAX_FRAME_DRAWS 2
#define MAX_OBJECTS 2

class Volcano
{
    public:
        // Initalize vulkan instance
        static void init(Window* window);
        static void destroy();
        static void draw();
        static void updateModel(int modelId, const glm::mat4& newModel);
        
        static bool& getFramebufferResized() { return Volcano::framebufferResized; }

        static void createBuffer(vk::DeviceSize bufferSize, vk::BufferUsageFlags bufferUsageFlags, vk::MemoryPropertyFlags bufferProperties,
            vk::Buffer& buffer, vk::DeviceMemory& bufferMemory);
        static void copyBuffer(vk::Buffer& src, vk::Buffer& dst, vk::DeviceSize bufferSize);
    private:
        inline static bool framebufferResized = false;
        // Current frame to be drawn
        inline static int currentFrame = 0;
        inline static struct UBOViewProj {
            glm::mat4 proj;
            glm::mat4 view;

            UBOViewProj() : proj(1.0f), view(1.0f) {}
        } mvp;
        // UniqueInstance destruction is done automatically
        inline static vk::UniqueInstance instance;
        // GPU device
        inline static vk::PhysicalDevice physicalDevice;
        // Logical device
        inline static vk::UniqueDevice device;
        
        inline static vk::Queue graphicsQueue;
        inline static vk::Queue presentQueue;
        inline static vk::SurfaceKHR surface;
        
        // List of device extensions
        inline static const std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME         // Swap chain extensions (macro)
        };
        
        inline static vk::SwapchainKHR swapChain;

        inline static std::vector<SwapChainImage> swapChainImages;
        inline static std::vector<vk::Framebuffer> swapChainFramebuffers;
        inline static std::vector<vk::CommandBuffer> commandBuffers;
        
        inline static vk::Image depthBufferImage;
        inline static vk::DeviceMemory depthBufferMemory;
        inline static vk::ImageView depthBufferImageView;

        inline static vk::Format swapChainImageFormat;
        inline static vk::Extent2D swapChainExtent;

        inline static vk::Format depthFormat;

        inline static vk::Pipeline graphicsPipeline;
        inline static vk::PipelineLayout pipelineLayout;
        inline static vk::RenderPass renderPass;
        inline static vk::CommandPool graphicsCommandPool;

        // Singal after image is ready to be rendered
        inline static std::vector<vk::Semaphore> imageAvailable;
        // Signal after rendering is finised
        inline static std::vector<vk::Semaphore> renderFinished;
        // Fences for cpu-gpu sync
        inline static std::vector<vk::Fence> drawFences;
        
        inline static vk::DescriptorSetLayout descriptorSetLayout;
        inline static vk::PushConstantRange pushConstantRange;

        inline static vk::DescriptorPool descriptorPool;
        inline static std::vector<vk::DescriptorSet> descriptorSets;
        
        inline static std::vector<vk::Buffer> vpUniformBuffer;
        inline static std::vector<vk::DeviceMemory> vpUniformBufferMemory;

        inline static std::vector<vk::Image> textureImages;
        inline static std::vector<vk::DeviceMemory> textureImageMemory;
        
        // inline static std::vector<vk::Buffer> modelUniformBuffer;
        // inline static std::vector<vk::DeviceMemory> modelUniformBufferMemory;

        //inline static vk::DeviceSize minBufferOffset;
        //inline static size_t modelUniformAlignment;
        //inline static UBOModel* modelTransferSpace;

        inline static Window* window;
// Validation layer only exist for debug build
#ifdef DEBUG
        inline static std::vector<const char*> validationLayers = {
            "VK_LAYER_KHRONOS_validation"
        };
        inline static VkDebugUtilsMessengerEXT callback;
#endif
        // Scene object
        inline static std::vector<std::shared_ptr<Mesh>> meshList; 
    
    private:
        static void pickPhysicalDevice();
        static bool isDeviceSuitable(const vk::PhysicalDevice& device);
        static QueueFamilyIndicies findQueueFamily(const vk::PhysicalDevice& device);
        static std::vector<const char*> getRequiredExtensions();
        static void createLogicalDevice();
        static void createSurface();
        static bool checkDeviceExtensionsSupport(const vk::PhysicalDevice& physicalDevice);
        static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
        static vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentMode);
        static vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
        static void createSwapChain();
        static vk::ImageView createImageView(vk::Image& image, vk::Format& format, vk::ImageAspectFlags aspectFlag);
        static void createRenderPass();
        static void createDescriptorSetLayout();
        static void createPushConstantRange();
        static void createGraphicsPipeline();
        static void createDepthBufferImage();
        static void createFramebuffers();

        static vk::UniqueShaderModule createShaderModule(const std::vector<char>& code);
        static vk::Image createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling,
            vk::ImageUsageFlags useFlags, vk::MemoryPropertyFlags propFlags, vk::DeviceMemory& imageMemory);
        static vk::Format chooseSupportedFormat(const std::vector<vk::Format>& formats, const vk::ImageTiling& tiling, const vk::FormatFeatureFlags& featureFlags);

        static void createCommandPool();
        static void createCommandBuffer();
        static void recordCommands(uint32_t currentImage);
        static void createSynchronization();
        
        static uint32_t findMemoryTypeIndex(uint32_t allowedTypes, vk::MemoryPropertyFlags properties);
        static void recreateSwapChain();
        static void cleanupSwapChain();
        static void createUniformBuffer();
        static void createDescriptorPool();
        static void createDescriptorSets();
        static void updateUniformBuffers(uint32_t imageIndex);

        static void allocateDynamicBufferTransferSpace();
        
        static void copyImageBuffer(vk::Buffer& src, vk::Image& image, uint32_t width, uint32_t height);
        static vk::CommandBuffer beginCopyBuffer(vk::CommandPool& commandPool);
        static void endCopyBuffer(vk::CommandPool& commandPool, vk::Queue& queue, vk::CommandBuffer& commandBuffer);

        static stbi_uc* loadTextureFile(const char* filename, int& width, int& height, vk::DeviceSize& imageSize);
        static int createTexture(const char* filename);
#ifdef DEBUG
        static bool checkValidationLayerSupport();
        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
        static VkResult createDebugUtilsMessengerEXT(vk::UniqueInstance& instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pCallback);
        static void destroyDebugUtilMessengerEXT(vk::UniqueInstance& instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
#endif
};
