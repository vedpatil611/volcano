#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>
#include "vertex.h"

class Mesh
{
private:
    int vertexCount;
    vk::Buffer vertexBuffer;
    vk::PhysicalDevice& physicalDevice;
    vk::Device& device;
    vk::DeviceMemory vertexBufferMemory;
public:
    Mesh(vk::PhysicalDevice& physicalDevice, vk::Device& device, std::vector<Vertex>& vertices);
    ~Mesh();

    inline int getVertexCount() const { return vertexCount; };
    inline vk::Buffer getVertexBuffer() const { return vertexBuffer; };
private:
    void createVertexBuffer(std::vector<Vertex>& vertices);
    uint32_t findMemoryTypeIndex(uint32_t allowedTypes, vk::MemoryPropertyFlags properties);
};
