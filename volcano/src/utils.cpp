#include "utils.h"

#include <fstream>
#include <stdexcept>

namespace utils
{
    char* readFile(const char* filename)
    {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if(!file.is_open())
            throw std::runtime_error("Failed to open file");

        size_t fileSize = file.tellg();
        char* fileData = new char[fileSize];

        // seek to start of file
        file.seekg(0);
        file.read(fileData, fileSize);
        file.close();

        return fileData;
    }
}
