#pragma once

#include <GLFW/glfw3.h>

class Window
{
private:
    GLFWwindow* m_Window;
    int m_Width, m_Height;
public:
    Window(const char* name = "Volcano", int width = 800, int height = 800);
    ~Window();

    void pollEvents();
    bool shouldClose() const;
    void swapBuffer();

    inline GLFWwindow* getWindow() const { return m_Window; }
    inline int getWidth() const { return m_Width; }
    inline int getHeight() const { return m_Height; }

    inline static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
};
