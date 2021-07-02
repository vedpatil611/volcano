#include <cstdint>
#include <optional>

struct QueueFamilyIndicies
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete();
};
