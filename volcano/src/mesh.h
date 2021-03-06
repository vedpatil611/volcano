#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>
#include "vertex.h"

struct Model 
{
    glm::mat4 model;    
};

class Mesh
{
public:
    Mesh(vk::Device& device, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
    ~Mesh();

    void setModel(const glm::mat4& model);
    inline Model getModel() const { return model; }

    inline size_t getVertexCount() const { return vertexCount; };
    inline vk::Buffer getVertexBuffer() const { return vertexBuffer; };
    
    inline size_t getIndexCount() const { return indexCount; };
    inline vk::Buffer getIndexBuffer() const { return indexBuffer; };
private:
    Model model;

    size_t vertexCount;
    vk::Buffer vertexBuffer;
    vk::DeviceMemory vertexBufferMemory;

    size_t indexCount;
    vk::Buffer indexBuffer;
    vk::DeviceMemory indexBufferMemory;

    vk::Device& device;
private:
    void createVertexBuffer(std::vector<Vertex>& vertices);
    void createIndexBuffer(std::vector<uint32_t>& indices);
};
