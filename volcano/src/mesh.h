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

    inline int getVertexCount() const { return vertexCount; };
    inline vk::Buffer getVertexBuffer() const { return vertexBuffer; };
    
    inline int getIndexCount() const { return indexCount; };
    inline vk::Buffer getIndexBuffer() const { return indexBuffer; };
private:
    Model model;

    int vertexCount;
    vk::Buffer vertexBuffer;
    vk::DeviceMemory vertexBufferMemory;

    int indexCount;
    vk::Buffer indexBuffer;
    vk::DeviceMemory indexBufferMemory;

    vk::Device& device;
private:
    void createVertexBuffer(std::vector<Vertex>& vertices);
    void createIndexBuffer(std::vector<uint32_t>& indices);
};
