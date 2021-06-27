#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>

namespace volcano
{
    class Volcano
    {
        private:
            vk::Instance instance;
        public:
            void init();
    };
}
