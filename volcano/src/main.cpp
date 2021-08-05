#include "volcanoPCH.h"
#include "volcano.h"
#include "window.h"
#include <iostream>

int main()
{
    Window window;
    Volcano::init(&window);

    while(!window.shouldClose())
    {
        window.pollEvents();
        Volcano::draw();
    }

    Volcano::destroy();
}
