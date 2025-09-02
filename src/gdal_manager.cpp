#include "gdal_manager.h"
#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <gdal.h>
#include <ogr_spatialref.h>

namespace GdalManager {

void initialize() {
    std::cout << "Initializing GDAL and PROJ libraries" << std::endl;

    GDALAllRegister();

    CPLSetConfigOption("OGR_CT_FORCE_TRADITIONAL_GIS_ORDER", "YES");
    //CPLSetConfigOption("PROJ_NETWORK", "ON");
    //CPLSetConfigOption("CPL_DEBUG", "ON");
    //CPLSetConfigOption("CPL_LOG", "gdal.log");
    //CPLSetConfigOption("CPL_LOG_ERRORS", "ON");
    //CPLSetConfigOption("CPL_TIMESTAMP", "ON");

    // SET PROJ_DEBUG to "ON"
    //CPLSetConfigOption("PROJ_DEBUG", "ON");

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
