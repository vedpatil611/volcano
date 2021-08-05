#include "volcanoPCH.h"
#include "mesh.h"

#include "volcano.h"

Mesh::Mesh(vk::PhysicalDevice& physicalDevice, vk::Device& device, std::vector<Vertex>& vertices) 
    : physicalDevice(physicalDevice), device(device), vertexCount(vertices.size())
{
    createVertexBuffer(vertices);
}

Mesh::~Mesh()
{
    device.destroyBuffer(vertexBuffer);
    device.freeMemory(vertexBufferMemory);
}


void Mesh::createVertexBuffer(std::vector<Vertex>& vertices)
{
    // get size of buffer
    vk::DeviceSize bufferSize = sizeof(Vertex) * vertices.size();
    
    // Create buffer and allocate memory
    Volcano::createBuffer(physicalDevice, device, bufferSize, vk::BufferUsageFlagBits::eVertexBuffer, 
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        vertexBuffer, vertexBufferMemory);

    // Map memory to buffer
    void* data;                                                         // pointer in normal memory
    data = device.mapMemory(vertexBufferMemory, 0, bufferSize);    // Connnect vertex buffer memory and pointer
    memcpy(data, vertices.data(), bufferSize);                     // copy form vertices array to pointer
    device.unmapMemory(vertexBufferMemory);                             // Unmap buffer memory
}



