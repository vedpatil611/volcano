#include <optional>
#include <vector>
#include <vulkan/vulkan.hpp>

struct QueueFamilyIndicies
{
    std::optional<uint32_t> graphicsFamily;

    bool isComplete();
};

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
        // Surface
        inline static vk::SurfaceKHR surface;
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
        static QueueFamilyIndicies findQueueFamily(const vk::PhysicalDevice& device);
        // Get glfw extensions
        static std::vector<const char*> getRequiredExtensions();
        // Initalize logical device
        static void createLogicalDevice();
        // Create surface for graphics rendering
        static void createSurface();
#ifdef DEBUG
        static bool checkValidationLayerSupport();
        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
        static VkResult createDebugUtilsMessengerEXT(vk::UniqueInstance& instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pCallback);
        static void destroyDebugUtilMessengerEXT(vk::UniqueInstance& instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
#endif
};
