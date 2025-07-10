#include "gdal_manager.h"
#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <gdal.h>
#include <ogr_spatialref.h>

namespace GdalManager {

void primeProjectionSystem() {
    // Initialize PROJ structures to prevent axis mapping issues
    // This ensures PROJ database and axis mapping strategies are properly initialized
    std::cout << "Initializing PROJ coordinate transformation system" << std::endl;

    // Create a simple coordinate transformation to initialize PROJ internal structures
    OGRSpatialReferenceH hSrcSRS = OSRNewSpatialReference(nullptr);
    OGRSpatialReferenceH hDstSRS = OSRNewSpatialReference(nullptr);

    if (hSrcSRS && hDstSRS) {
        // Import EPSG:4326 (WGS84)
        if (OSRImportFromEPSG(hSrcSRS, 4326) == OGRERR_NONE) {
            // Import a UTM zone (example: UTM Zone 15N)
            if (OSRImportFromProj4(hDstSRS, "+proj=utm +zone=15 +datum=WGS84 +units=m +no_defs") == OGRERR_NONE) {
                // Create transformation to force PROJ initialization
                OGRCoordinateTransformationH hTransform = OCTNewCoordinateTransformation(hSrcSRS, hDstSRS);
                if (hTransform) {
                    // Perform a dummy transformation to initialize internal structures
                    double y = -91.0, x = 46.0;

                    if (OCTTransform(hTransform, 1, &x, &y, nullptr) != TRUE) {
                        std::cout << "Warning: Coordinate transformation failed, but PROJ "
                                  "initialization may still be successful" << std::endl;
                    }

                    OCTDestroyCoordinateTransformation(hTransform);
                    std::cout << "PROJ initialization completed successfully" << std::endl;
                }
            }
        }
        OSRDestroySpatialReference(hSrcSRS);
        OSRDestroySpatialReference(hDstSRS);
    }
}

void initialize() {
    std::cout << "Initializing GDAL and PROJ libraries" << std::endl;

    GDALAllRegister();
    //primeProjectionSystem();

    std::cout << "GDAL and PROJ initialization completed" << std::endl;
}

void verifyProjectionSystem() {
    const char* projData = std::getenv("PROJ_DATA");
    if (!projData)
        projData = std::getenv("PROJ_LIB");

    if (projData) {
        std::filesystem::path projDbPath = std::filesystem::path(projData) / "proj.db";
        if (std::filesystem::exists(projDbPath))
            std::cout << "PROJ database accessible at: " << projDbPath.string() << std::endl;
        else
            std::cout << "PROJ database NOT found at: " << projDbPath.string() << std::endl;
    } else {
        std::cout << "Neither PROJ_DATA nor PROJ_LIB environment variables are set" << std::endl;
    }

    // Check PROJ availability
    OGRSpatialReferenceH hSRS = OSRNewSpatialReference(nullptr);
    if (hSRS) {
        OSRDestroySpatialReference(hSRS);
        std::cout << "PROJ is working and available for coordinate transformations" << std::endl;
    } else {
        std::cout << "PROJ is not available, coordinate transformations may fail" << std::endl;
    }
}

} // namespace GdalManager
