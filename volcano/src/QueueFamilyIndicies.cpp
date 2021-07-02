#include "QueueFamilyIndices.h"

bool QueueFamilyIndicies::isComplete()
{
    return graphicsFamily.has_value() && presentFamily.has_value();
}
