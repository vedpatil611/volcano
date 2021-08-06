#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>
#include "vertex.h"

class Mesh
{
private:
    int vertexCount;
    vk::Buffer vertexBuffer;
    vk::Device& device;
    vk::DeviceMemory vertexBufferMemory;
public:
    Mesh(vk::Device& device, std::vector<Vertex>& vertices);
    ~Mesh();

    inline int getVertexCount() const { return vertexCount; };
    inline vk::Buffer getVertexBuffer() const { return vertexBuffer; };
private:
    void createVertexBuffer(std::vector<Vertex>& vertices);
};
