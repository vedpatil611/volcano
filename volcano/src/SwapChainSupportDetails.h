#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

struct SwapChainSupportDetails
{
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;

    static SwapChainSupportDetails queryDetails(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface);
};
