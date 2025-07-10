#include <iostream>
#include <filesystem>
#include <vector>
#include "src/hash.h"
#include "src/platform_utils.h"
#include "src/gdal_manager.h"
#include "src/system_info.h"
#include "src/geotiff_analyzer.h"
#include "src/gdaltiler.h"
#include "src/thumbs.h"

/**
 * Test GDALTiler functionality with specific tile coordinates
 * This reproduces the specific bug scenario with precise tile coordinates
 */
void testGDALTiler(const std::string& wroFilePath, const std::string& executableDir) {
    std::cout << "\n=== Testing GDALTiler Bug Reproduction ===" << std::endl;

    try {

        // Create output directory for tiles
        std::filesystem::path tileDir = std::filesystem::path(executableDir) / "tiles";

        // Remove the "tiles" directory if it exists
        if (std::filesystem::exists(tileDir)) {
            std::filesystem::remove_all(tileDir);
            std::cout << "Removed existing tile directory: " << tileDir.string() << std::endl;
        }

        std::filesystem::create_directories(tileDir);

        // Use wro.tif as ortho input file
        std::filesystem::path ortho = std::filesystem::path(wroFilePath);

        std::cout << "Creating GDALTiler for: " << ortho.string() << std::endl;
        std::cout << "Output directory: " << tileDir.string() << std::endl;

        // Initialize GDALTiler with wro.tif as ortho
        ddb::GDALTiler t(ortho.string(), tileDir.string(), 256, true);

        // Test tiles from the provided table - specific coordinates for bug reproduction
        struct TileTest {
            int z, x, y;
            int tileSize;
        };

        std::vector<TileTest> testTiles = {
            {14, 16174, 10245, 256},
            {18, 258796, 163923, 256},
            {18, 258797, 163923, 256},
            {18, 258796, 163922, 256},
            {18, 258797, 163922, 256},
            {19, 517593, 327846, 256},
            {20, 1035186, 655693, 256},
            {20, 1035187, 655693, 256},
            {20, 1035186, 655694, 256}
        };

        std::cout << "Testing " << testTiles.size() << " tiles for bug reproduction..." << std::endl;

        for (const auto& tile : testTiles) {
            std::cout << "Generating tile " << tile.z << "/" << tile.x << "/" << tile.y << std::endl;

            try {
                auto tilePath = t.tile(tile.z, tile.x, tile.y);

                /*std::filesystem::path expectedTile = tileDir / std::to_string(tile.z) / std::to_string(tile.x) /
                                                   (std::to_string(tile.y) + ".png");*/

                if (std::filesystem::exists(tilePath)) {
                    std::cout << "✓ Tile " << tile.z << "/" << tile.x << "/" << tile.y << " found" << std::endl;
                    std::cout << "  File: " << tilePath << std::endl;
                    std::cout << "  Size: " << std::filesystem::file_size(tilePath) << " bytes" << std::endl;
                } else {
                    std::cout << "✗ Tile " << tile.z << "/" << tile.x << "/" << tile.y << " not found" << std::endl;
                    std::cout << "Expected at: " << tilePath << std::endl;
                }
            } catch (const std::exception& e) {
                std::cout << "✗ Error generating tile " << tile.z << "/" << tile.x << "/" << tile.y << ": " << e.what() << std::endl;
            }
        }

        std::cout << "GDALTiler bug reproduction test completed" << std::endl;

    } catch (const std::exception& e) {
        std::cout << "GDALTiler test failed: " << e.what() << std::endl;
    }
}



/**
 * Main application entry point
 * Demonstrates GDAL/PROJ coordinate transformation functionality and GDALTiler
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
        // Test GDALTiler functionality
        testGDALTiler(wroFilePath, executableDir);

        ddb::generateImageThumb(wroFilePath, 256, "thumb.webp", nullptr, nullptr);

        if (!fs::exists("thumb.webp")) {
            std::cout << "Thumbnail generation failed" << std::endl;
        } else {
            std::cout << "Thumbnail generated successfully: thumb.webp" << std::endl;
        }


    } else {
        std::cout << "wro.tif not found in current directory" << std::endl;
    }

    return 0;
}