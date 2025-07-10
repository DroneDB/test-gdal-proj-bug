#ifndef THUMBS_H
#define THUMBS_H

#include <string>
#include <filesystem>

namespace fs = std::filesystem;

namespace ddb {
    void generateImageThumb(const fs::path &imagePath, int thumbSize, const fs::path &outImagePath, uint8_t **outBuffer = nullptr, int *outBufferSize = nullptr);

}

#endif