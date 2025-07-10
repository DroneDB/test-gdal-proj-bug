#pragma once

#include <string>

namespace SystemInfo {
    /**
     * Get build information including compiler and build configuration
     * @return String containing build information
     */
    std::string getBuildInfo();

    /**
     * Print version information for GDAL, PROJ and environment variables
     */
    void printVersions();
}
