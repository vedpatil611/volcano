#include "volcanoPCH.h"
#include "mesh.h"

#include "iostream"
#include "volcano.h"

Mesh::Mesh(vk::Device& device, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
    : device(device), vertexCount(vertices.size()), indexCount(indices.size())
{
    createVertexBuffer(vertices);
    createIndexBuffer(indices);

    model.model = glm::mat4(1.0f);
}

Mesh::~Mesh()
{
    device.destroyBuffer(vertexBuffer);
    device.freeMemory(vertexBufferMemory);
    device.destroyBuffer(indexBuffer);
    device.freeMemory(indexBufferMemory);
}

void Mesh::setModel(const glm::mat4 &model) 
{
    this->model.model = model;
}

void Mesh::createVertexBuffer(std::vector<Vertex>& vertices)
{
    // get size of buffer
    vk::DeviceSize bufferSize = sizeof(Vertex) * vertices.size();
 
    // Temporary buffer to "stage" vertex data before transerfering to gpu
    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;

    // Create stating buffer and allocate memory
    Volcano::createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, 
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuffer, stagingBufferMemory);

    // Map memory to buffer
    void* data;                                                         // pointer in normal memory
    data = device.mapMemory(stagingBufferMemory, 0, bufferSize);        // Connnect vertex buffer memory and pointer
    memcpy(data, vertices.data(), bufferSize);                          // copy form vertices array to pointer
    device.unmapMemory(stagingBufferMemory);                            // Unmap buffer memory

    // Create buffer with transfer dst to mark as reciptant for of transfer data
    Volcano::createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal, vertexBuffer, vertexBufferMemory);        // local bit means memory is on gpu and only accescible by it

    Volcano::copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    // Clean up staging buffer
    device.destroyBuffer(stagingBuffer);
    device.freeMemory(stagingBufferMemory);
}

void Mesh::createIndexBuffer(std::vector<uint32_t>& indices)
{
    vk::DeviceSize bufferSize = sizeof(uint32_t) * indices.size();

    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;

    Volcano::createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, 
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            stagingBuffer, stagingBufferMemory);

    void* data = device.mapMemory(stagingBufferMemory, 0, bufferSize);
    memcpy(data, indices.data(), bufferSize);
    device.unmapMemory(stagingBufferMemory);

    Volcano::createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal, indexBuffer, indexBufferMemory);

    Volcano::copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    device.destroyBuffer(stagingBuffer);
    device.freeMemory(stagingBufferMemory);
}
