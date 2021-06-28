#include "volcano.h"
#include "window.h"

int main()
{
    Window window;
    Volcano::init(&window);

    while(!window.shouldClose())
    {
        window.pollEvents();
    }

    Volcano::destroy();
}
