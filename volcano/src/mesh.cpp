#include "volcanoPCH.h"
#include "mesh.h"

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
    vk::BufferCreateInfo bufferInfo = {};
    bufferInfo.size = sizeof(Vertex) * vertices.size();
    bufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    try 
    {
        vertexBuffer = device.createBuffer(bufferInfo);
    }
    catch(vk::SystemError& e)
    {
        throw std::runtime_error("Failed to create vertex buffer");
    }

    vk::MemoryRequirements memRequirements = {};
    memRequirements = device.getBufferMemoryRequirements(vertexBuffer);

    vk::MemoryAllocateInfo memAllocInfo = {};
    memAllocInfo.allocationSize = memRequirements.size;

    // Host visible bit -> cpu can interact
    // Conherant bit -> allows placement of data straight into buffer after mapping 
    memAllocInfo.memoryTypeIndex = findMemoryTypeIndex(memRequirements.memoryTypeBits, 
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    
    try 
    {
        // Allocate to device memory
        vertexBufferMemory = device.allocateMemory(memAllocInfo);
    }
    catch(vk::SystemError& e)
    {
        throw std::runtime_error("Failed to allocate memory");
    }

    // Allocate mem to vertex buffer
    device.bindBufferMemory(vertexBuffer, vertexBufferMemory, 0);
    
    // Map memory to buffer
    void* data;                                                         // pointer in normal memory
    data = device.mapMemory(vertexBufferMemory, 0, bufferInfo.size);    // Connnect vertex buffer memory and pointer
    memcpy(data, vertices.data(), bufferInfo.size);                     // copy form vertices array to pointer
    device.unmapMemory(vertexBufferMemory);                             // Unmap buffer memory
}


uint32_t Mesh::findMemoryTypeIndex(uint32_t allowedTypes, vk::MemoryPropertyFlags properties)
{
    vk::PhysicalDeviceMemoryProperties memProperties;
    memProperties = physicalDevice.getMemoryProperties();

    for(uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
    {
        if((allowedTypes & (1 << i)) && memProperties.memoryTypes[i].propertyFlags & properties)
        {
            // Memory type is valid so return it
            return i;
        }
    }
}
