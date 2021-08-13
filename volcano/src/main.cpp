#include "volcanoPCH.h"
#include "volcano.h"
#include "window.h"
#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

int main()
{
    Window window;
    Volcano::init(&window);

    float angle = 0.0f;
    float deltaTime = 0.0f;
    float lastTime = 0.0f;

    while(!window.shouldClose())
    {
        window.pollEvents();

        float now = glfwGetTime();
        deltaTime = now - lastTime;
        lastTime = now;

        angle += 10.0f * deltaTime;
        if(angle > 360) angle -= 360;
        Volcano::updateModel(glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f)));
        Volcano::draw();
    }

    Volcano::destroy();
}
