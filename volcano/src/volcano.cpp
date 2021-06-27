#include "volcano.h"

void Volcano::init()
{
    vk::ApplicationInfo appInfo;
    appInfo.pApplicationName = "volcano";
    appInfo.pEngineName = "Volcano";

    vk::InstanceCreateInfo instanceInfo;
    instanceInfo.pApplicationInfo = &appInfo;

    if(vk::createInstance(&instanceInfo, nullptr, &Volcano::instance) != vk::Result::eSuccess)
        throw std::runtime_error("Failed to create instance");
}
