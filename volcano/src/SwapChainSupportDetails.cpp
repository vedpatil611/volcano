#include "volcanoPCH.h"
#include "SwapChainSupportDetails.h"
#include <iostream>

SwapChainSupportDetails SwapChainSupportDetails::queryDetails(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface)
{
    SwapChainSupportDetails details = {
        device.getSurfaceCapabilitiesKHR(surface),
        device.getSurfaceFormatsKHR(surface),
        device.getSurfacePresentModesKHR(surface)
    };

    return details;
}
