#include "volcano.h"

#include <GLFW/glfw3.h>

void Volcano::init()
{
    vk::ApplicationInfo appInfo;
    appInfo.pApplicationName = "volcano";
    appInfo.pEngineName = "Volcano";

    vk::InstanceCreateInfo instanceInfo;
    instanceInfo.pApplicationInfo = &appInfo;
    
    u_int32_t glfwExtensionCount;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    instanceInfo.enabledExtensionCount = glfwExtensionCount;
    instanceInfo.ppEnabledExtensionNames = glfwExtensions;
    instanceInfo.enabledLayerCount = 0;

    if(vk::createInstance(&instanceInfo, nullptr, &Volcano::instance) != vk::Result::eSuccess)
        throw std::runtime_error("Failed to create instance");
}

void Volcano::destroy()
{
    instance.destroy();
}
