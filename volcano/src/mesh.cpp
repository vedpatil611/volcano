#include "volcanoPCH.h"
#include "mesh.h"

#include "iostream"
#include "volcano.h"

Mesh::Mesh(vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::Queue& transferQueue, vk::CommandPool& transferCommandPool, std::vector<Vertex>& vertices) 
    : physicalDevice(physicalDevice), device(device), vertexCount(vertices.size())
{
    createVertexBuffer(transferQueue, transferCommandPool, vertices);
}

Mesh::~Mesh()
{
    device.destroyBuffer(vertexBuffer);
    device.freeMemory(vertexBufferMemory);
}


void Mesh::createVertexBuffer(vk::Queue& transferQueue, vk::CommandPool& transferCommandPool, std::vector<Vertex>& vertices)
{
    // get size of buffer
    vk::DeviceSize bufferSize = sizeof(Vertex) * vertices.size();
 
    // Temporary buffer to "stage" vertex data before transerfering to gpu
    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;

    // Create stating buffer and allocate memory
    Volcano::createBuffer(physicalDevice, device, bufferSize, vk::BufferUsageFlagBits::eTransferSrc, 
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuffer, stagingBufferMemory);

    
    // Map memory to buffer
    void* data;                                                         // pointer in normal memory
    data = device.mapMemory(stagingBufferMemory, 0, bufferSize);        // Connnect vertex buffer memory and pointer
    memcpy(data, vertices.data(), bufferSize);                          // copy form vertices array to pointer
    device.unmapMemory(stagingBufferMemory);                            // Unmap buffer memory

    // Create buffer with transfer dst to mark as reciptant for of transfer data
    Volcano::createBuffer(physicalDevice, device, bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal, vertexBuffer, vertexBufferMemory);        // local bit means memory is on gpu and only accescible by it


    Volcano::copyBuffer(transferQueue, transferCommandPool, stagingBuffer, vertexBuffer, bufferSize);

    // Clean up staging buffer
    device.destroyBuffer(stagingBuffer);
    device.freeMemory(stagingBufferMemory);
}

