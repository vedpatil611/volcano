#include <vulkan/vulkan.hpp>

class Volcano
{
    private:
        inline static vk::Instance instance;
    public:
        static void init();
        static void destroy();
};
