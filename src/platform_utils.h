#pragma once

#include <string>
#include <filesystem>

namespace PlatformUtils {
    /**
     * Set environment variable in a cross-platform way
     * @param name Environment variable name
     * @param value Environment variable value
     */
    void setEnvironmentVariable(const std::string& name, const std::string& value);

    /**
     * Get the directory containing the current executable
     * @return Path to executable directory
     */
    std::filesystem::path getExecutableDirectory();

    /**
     * Setup PROJ environment variables (PROJ_DATA, PROJ_LIB)
     * @param executableDir Directory containing the executable
     */
    void setupProjEnvironment(const std::string& executableDir);

    /**
     * Setup locale settings for consistent behavior across platforms
     */
    void setupLocale();
}
