#pragma once

#include <vulkan/vulkan.hpp>

struct SwapChainImage
{
    vk::Image image;
    vk::ImageView imageView;
};
