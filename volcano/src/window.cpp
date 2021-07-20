#include "volcanoPCH.h"
#include "window.h"

#include <stdexcept>

Window::Window(const char* name, int width, int height)
    :m_Width(width), m_Height(height)
{
    if(!glfwInit())
        throw std::runtime_error("Failed to init glfw");

    // Set to no api for vulkan
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_Window = glfwCreateWindow(width, height, name, nullptr, nullptr);
    if(!m_Window)
    {
        glfwTerminate();
        throw std::runtime_error("Failed to create glfw window");
    }

    glfwMakeContextCurrent(m_Window);
}

Window::~Window()
{
    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

void Window::pollEvents()
{
    glfwPollEvents();
}

bool Window::shouldClose() const
{
    return glfwWindowShouldClose(m_Window);
}
