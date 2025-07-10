#include <iostream>
#include <filesystem>
#include "hash.h"
#include "src/platform_utils.h"
#include "src/gdal_manager.h"
#include "src/system_info.h"
#include "src/geotiff_analyzer.h"

/**
 * Main application entry point
 * Demonstrates GDAL/PROJ coordinate transformation functionality
 */
int main() {
    // Get executable directory for finding support files
    const auto executableDir = PlatformUtils::getExecutableDirectory().string();

    // Setup environment and libraries
    PlatformUtils::setupProjEnvironment(executableDir);
    PlatformUtils::setupLocale();

    // Initialize GDAL and PROJ
    GdalManager::initialize();
    GdalManager::verifyProjectionSystem();

    // Print system information
    SystemInfo::printVersions();

    // Analyze the test GeoTIFF file
    const std::string wroFilePath = (std::filesystem::path(executableDir) / "wro.tif").string();

    if (std::filesystem::exists(wroFilePath)) {
        auto result = GeotiffAnalyzer::analyzeFile(wroFilePath);
    } else {
        std::cout << "\nWarning: wro.tif not found at: " << wroFilePath << std::endl;
        std::cout << "Looking for wro.tif in current directory..." << std::endl;

        if (std::filesystem::exists("wro.tif")) {
            auto result = GeotiffAnalyzer::analyzeFile("wro.tif");
        } else {
            std::cout << "wro.tif not found in current directory either." << std::endl;
        }
    }

    return 0;
}