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

        glm::mat4 model1(1.0f);
        glm::mat4 model2(1.0f);

        model1 = glm::translate(model1, glm::vec3(-2.0f, -5.0f, 0.0f));
        //model1 = glm::scale();
        model1 = glm::rotate(model1, glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f));

        model2 = glm::translate(model2, glm::vec3(5.0f, 5.0f, 0.0f));
        model2 = glm::rotate(model2, glm::radians(-angle * 10), glm::vec3(0.0f, 0.0f, 1.0f));
        
        Volcano::updateModel(0, model1);
        Volcano::updateModel(1, model2);

        //Volcano::updateModel(glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f)));
        Volcano::draw();
    }

    Volcano::destroy();
}
