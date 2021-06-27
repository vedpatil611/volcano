#include "volcano.h"
#include "window.h"

int main()
{
    Window window;
    Volcano::init();

    while(!window.shouldClose())
    {
        window.pollEvents();
    }

    Volcano::destroy();
}
