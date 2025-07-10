#pragma once

#include <stdexcept>
#include <string>

namespace ddb {

// Simplified exception classes
class GDALException : public std::runtime_error {
public:
    GDALException(const std::string& message) : std::runtime_error(message) {}
};

} // namespace ddb
