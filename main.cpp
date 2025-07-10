#include <iostream>
#include <filesystem>
#include <sstream>
#include <cstdlib>
#include <gdal.h>
#include <ogr_spatialref.h>
#include "hash.h"
#include <proj.h>

#ifdef WIN32
#include <windows.h>
#include <cstdlib>
#endif

#ifndef WIN32
#include <unistd.h>
#include <cstdlib>
#endif

// Cross-platform environment variable setting
void setEnvironmentVariable(const std::string& name, const std::string& value) {
#ifdef WIN32
    _putenv_s(name.c_str(), value.c_str());
#else
    std::string env_string = name + "=" + value;
    putenv(const_cast<char*>(env_string.c_str()));
#endif
}

// Simple function to get executable folder path
std::filesystem::path getExeFolderPath() {
#ifdef WIN32
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    return std::filesystem::path(path).parent_path();
#else
    return std::filesystem::current_path();
#endif
}


static void primeGDAL() {
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
            if (OSRImportFromProj4(hDstSRS, "+proj=utm +zone=15 +datum=WGS84 +units=m +no_defs") ==
                OGRERR_NONE) {
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


void setupEnvironmentVariables(const std::string& exeFolder) {
    // Setup percorsi PROJ (uniformi per entrambe le piattaforme)
    // For vcpkg, we should use the installed location
    const auto projDataPath = std::filesystem::path(exeFolder).string();

    // Verifica esistenza di proj.db
    const auto projDbPath = std::filesystem::path(projDataPath) / "proj.db";
    if (!std::filesystem::exists(projDbPath)) {
        std::cout << "PROJ database not found at: " << projDbPath.string() << std::endl;
        std::cout << "Coordinate transformations may fail" << std::endl;
    } else {
        std::cout << "PROJ database found at: " << projDbPath.string() << std::endl;

        // Debug: Print proj.db hash
        try {

            const std::string projDbHash = Hash::fileSHA256(projDbPath.string());
            std::cout << "proj.db hash: " << projDbHash << " (path: " << projDbPath.string() << ")" << std::endl;

        } catch (const std::exception& e) {
            std::cout << "Error computing proj.db hash: " << e.what() << std::endl;
        }

    }

#ifdef WIN32
    // Windows: usa _putenv_s per sincronizzazione CRT

    // PROJ_DATA è la variabile preferita (moderna)
    if (!std::getenv("PROJ_DATA")) {
        setEnvironmentVariable("PROJ_DATA", projDataPath);
        std::cout << "Set PROJ_DATA: " << projDataPath << std::endl;
    }

    // PROJ_LIB solo come fallback legacy se PROJ_DATA non è presente
    if (!std::getenv("PROJ_LIB") && !std::getenv("PROJ_DATA")) {
        setEnvironmentVariable("PROJ_LIB", projDataPath);
        std::cout << "Set PROJ_LIB (fallback): " << projDataPath << std::endl;
    }

#else
    // Unix: usa setenv standard

    // PROJ_DATA è la variabile preferita (moderna)
    if (!std::getenv("PROJ_DATA")) {
        setEnvironmentVariable("PROJ_DATA", projDataPath);
        std::cout << "Set PROJ_DATA: " << projDataPath << std::endl;
    }

    // PROJ_LIB solo come fallback legacy se PROJ_DATA non è presente
    if (!std::getenv("PROJ_LIB") && !std::getenv("PROJ_DATA")) {
        setEnvironmentVariable("PROJ_LIB", projDataPath);
        std::cout << "Set PROJ_LIB (fallback): " << projDataPath << std::endl;
    }
#endif

}

void setupLocaleUnified()
{

    // Strategy: LC_ALL=C per stability, LC_CTYPE=UTF-8 for Unicode

#ifdef WIN32

    try
    {

        setEnvironmentVariable("LC_ALL", "C");
        std::setlocale(LC_ALL, "C");

        std::setlocale(LC_CTYPE, "en_US.UTF-8");

        std::cout << "Windows locale set: LC_ALL=C, LC_CTYPE=UTF-8" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cout << "Locale setup failed on Windows: " << e.what() << std::endl;

        std::setlocale(LC_ALL, "C");
        std::cout << "Using minimal C locale" << std::endl;
    }

#else

    try
    {

        setEnvironmentVariable("LC_ALL", "C");
        std::setlocale(LC_ALL, "C");

        // Can overwrite only LC_CTYPE for UTF-8 support
        // Try different common UTF-8 locales
        const char *utf8_locales[] = {"en_US.UTF-8", "C.UTF-8", "en_US.utf8", nullptr};

        bool utf8_set = false;
        for (const char **locale_name = utf8_locales; *locale_name; ++locale_name)
        {
            if (std::setlocale(LC_CTYPE, *locale_name) != nullptr)
            {
                std::cout << "Unix locale set: LC_ALL=C, LC_CTYPE=" << *locale_name << std::endl;
                utf8_set = true;
                break;
            }
        }

        if (!utf8_set)
        {
            std::cout << "Could not set UTF-8 locale for LC_CTYPE, using C" << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        std::cout << "Locale setup failed on Unix: " << e.what() << std::endl;
        setEnvironmentVariable("LC_ALL", "C");
        std::setlocale(LC_ALL, "C");
        std::cout << "Using minimal C locale" << std::endl;
    }
#endif
}


void initializeGDALandPROJ()
{

    // Initialize GDAL and PROJ
    std::cout << "Initializing GDAL and PROJ libraries" << std::endl;

    GDALAllRegister();
    primeGDAL();

    std::cout << "GDAL and PROJ initialization completed" << std::endl;

    const char *projData = std::getenv("PROJ_DATA");
    if (!projData)
        projData = std::getenv("PROJ_LIB");

    if (projData)
    {
        std::filesystem::path projDbPath = std::filesystem::path(projData) / "proj.db";
        if (std::filesystem::exists(projDbPath))
            std::cout << "PROJ database accessible at: " << projDbPath.string() << std::endl;
        else
            std::cout << "PROJ database NOT found at: " << projDbPath.string() << std::endl;
    }
    else
        std::cout << "Neither PROJ_DATA nor PROJ_LIB environment variables are set" << std::endl;

    // Check PROJ availability
    OGRSpatialReferenceH hSRS = OSRNewSpatialReference(nullptr);
    if (hSRS)
    {
        OSRDestroySpatialReference(hSRS);
        std::cout << "PROJ is working and available for coordinate transformations" << std::endl;
    }
    else
    {
        std::cout << "PROJ is not available, coordinate transformations may fail" << std::endl;
    }
}

std::string getBuildInfo() {
    std::ostringstream ss;

#ifdef DEBUG
    ss << "Debug";
#else
    ss << "Release";
#endif

    ss << " build";

#ifdef __GNUC__
    ss << " (GCC " << __GNUC__ << "." << __GNUC_MINOR__ << ")";
#elif defined(_MSC_VER)
    ss << " (MSVC " << _MSC_VER << ")";
#endif

    ss << " compiled " << __DATE__ << " " << __TIME__;

    return ss.str();
}


void printVersions() {

    std::cout << "Build info: " << getBuildInfo() << std::endl;

    std::cout << "GDAL: " << GDALVersionInfo("RELEASE_NAME") << std::endl;

    auto pj_info = proj_info();
    std::cout << "PROJ: " << std::string(pj_info.release) + " (search path: " + pj_info.searchpath + ")" << std::endl;

    std::cout << "PROJ_LIB = " << (getenv("PROJ_LIB") ? getenv("PROJ_LIB") : "(not set)") << std::endl;
    std::cout << "GDAL_DATA = " << (getenv("GDAL_DATA") ? getenv("GDAL_DATA") : "(not set)") << std::endl;
    std::cout << "PROJ_DATA = " << (getenv("PROJ_DATA") ? getenv("PROJ_DATA") : "(not set)") << std::endl;

    std::cout << "Current locale (LC_ALL): " << std::setlocale(LC_ALL, nullptr) << std::endl;
    std::cout << "Current locale (LC_CTYPE): " << std::setlocale(LC_CTYPE, nullptr) << std::endl;
    std::cout << "LC_ALL env var: " << (std::getenv("LC_ALL") ? std::getenv("LC_ALL") : "(not set)") << std::endl;

}

// Structure to hold coordinate data
struct Coordinate {
    double longitude;
    double latitude;
};

// Structure to hold entry data
struct Entry {
    std::map<std::string, std::string> properties;
    std::vector<Coordinate> polygon_geom;
    std::vector<Coordinate> point_geom;
};

// Function to convert raster coordinates to geographic coordinates
Coordinate getRasterCoordinate(OGRCoordinateTransformationH hTransform, double* geotransform, double pixelX, double pixelY) {
    // Convert pixel coordinates to geographic coordinates using geotransform
    double geoX = geotransform[0] + pixelX * geotransform[1] + pixelY * geotransform[2];
    double geoY = geotransform[3] + pixelX * geotransform[4] + pixelY * geotransform[5];

    // Transform to WGS84
    if (OCTTransform(hTransform, 1, &geoX, &geoY, nullptr) == TRUE) {
        // Note: OCTTransform con OAMS_AUTHORITY_COMPLIANT restituisce lat,lon invece di lon,lat
        // Quindi geoX è latitudine e geoY è longitudine
        return {geoY, geoX}; // longitude, latitude
    } else {
        std::cout << "Warning: Coordinate transformation failed for pixel (" << pixelX << ", " << pixelY << ")" << std::endl;
        return {0.0, 0.0};
    }
}

// Function to analyze GeoTIFF file
void analyzeGeoTIFF(const std::string& filepath) {
    std::cout << "\n=== Analyzing GeoTIFF file: " << filepath << " ===" << std::endl;

    Entry entry;

    std::cout << "Processing GeoRaster file: " << filepath << std::endl;

    GDALDatasetH hDataset;
    hDataset = GDALOpen(filepath.c_str(), GA_ReadOnly);
    if (!hDataset) {
        std::cout << "GDAL failed to open dataset: " << filepath << std::endl;
        return;
    }

    std::cout << "GDAL successfully opened dataset" << std::endl;

    int width = GDALGetRasterXSize(hDataset);
    int height = GDALGetRasterYSize(hDataset);

    std::cout << "Raster dimensions - Width: " << width << ", Height: " << height << std::endl;

    entry.properties["width"] = std::to_string(width);
    entry.properties["height"] = std::to_string(height);

    double geotransform[6];
    CPLErr geoTransformResult = GDALGetGeoTransform(hDataset, geotransform);
    std::cout << "GDALGetGeoTransform result: " << (geoTransformResult == CE_None ? "Success" : "Failed") << std::endl;

    if (geoTransformResult == CE_None) {
        std::cout << "Geotransform values: ["
                 << geotransform[0] << ", " << geotransform[1] << ", " << geotransform[2] << ", "
                 << geotransform[3] << ", " << geotransform[4] << ", " << geotransform[5] << "]" << std::endl;

        const char* projectionRef = GDALGetProjectionRef(hDataset);
        std::cout << "GDALGetProjectionRef result: " << (projectionRef != NULL ? "Found" : "NULL") << std::endl;

        if (projectionRef != NULL) {
            std::string wkt = projectionRef;
            std::cout << "WKT string length: " << wkt.length() << std::endl;
            std::cout << "WKT content: " << wkt << std::endl;

            if (!wkt.empty()) {
                std::cout << "Setting projection property" << std::endl;
                entry.properties["projection"] = wkt;

                // Get lat/lon extent of raster
                char *wktp = const_cast<char *>(wkt.c_str());
                OGRSpatialReferenceH hSrs = OSRNewSpatialReference(nullptr);
                OGRSpatialReferenceH hWgs84 = OSRNewSpatialReference(nullptr);
                std::cout << "Created spatial reference objects" << std::endl;

                OGRErr importResult = OSRImportFromWkt(hSrs, &wktp);
                std::cout << "OSRImportFromWkt result: " << (importResult == OGRERR_NONE ? "Success" : "Failed") << std::endl;

                OSRSetAxisMappingStrategy(hSrs, OSRAxisMappingStrategy::OAMS_TRADITIONAL_GIS_ORDER);
                std::cout << "Set source axis mapping strategy" << std::endl;

                if (importResult != OGRERR_NONE) {
                    std::cout << "Failed to import WKT, cleaning up spatial references" << std::endl;
                    OSRDestroySpatialReference(hWgs84);
                    OSRDestroySpatialReference(hSrs);
                    GDALClose(hDataset);
                    return;
                }

                if (OSRImportFromEPSG(hWgs84, 4326) != OGRERR_NONE) {
                    std::cout << "Failed to import WGS84 EPSG, cleaning up spatial references" << std::endl;
                    OSRDestroySpatialReference(hWgs84);
                    OSRDestroySpatialReference(hSrs);
                    GDALClose(hDataset);
                    return;
                }

                std::cout << "OSRImportFromEPSG result: Success" << std::endl;

                OSRSetAxisMappingStrategy(hWgs84, OSRAxisMappingStrategy::OAMS_AUTHORITY_COMPLIANT);
                std::cout << "Set dest axis mapping strategy" << std::endl;

                OGRCoordinateTransformationH hTransform = OCTNewCoordinateTransformation(hSrs, hWgs84);
                std::cout << "Created coordinate transformation: " << (hTransform != nullptr ? "Success" : "Failed") << std::endl;

                if (hTransform != nullptr) {
                    std::cout << "Computing corner coordinates" << std::endl;

                    auto ul = getRasterCoordinate(hTransform, geotransform, 0.0, 0.0);
                    std::cout << "Upper Left: " << ul.latitude << ", " << ul.longitude << std::endl;

                    auto ur = getRasterCoordinate(hTransform, geotransform, width, 0);
                    std::cout << "Upper Right: " << ur.latitude << ", " << ur.longitude << std::endl;

                    auto lr = getRasterCoordinate(hTransform, geotransform, width, height);
                    std::cout << "Lower Right: " << lr.latitude << ", " << lr.longitude << std::endl;

                    auto ll = getRasterCoordinate(hTransform, geotransform, 0.0, height);
                    std::cout << "Lower Left: " << ll.latitude << ", " << ll.longitude << std::endl;

                    std::cout << "Adding points to polygon geometry" << std::endl;
                    entry.polygon_geom.push_back({ul.longitude, ul.latitude});
                    entry.polygon_geom.push_back({ur.longitude, ur.latitude});
                    entry.polygon_geom.push_back({lr.longitude, lr.latitude});
                    entry.polygon_geom.push_back({ll.longitude, ll.latitude});
                    entry.polygon_geom.push_back({ul.longitude, ul.latitude});

                    auto center = getRasterCoordinate(hTransform, geotransform, width / 2.0, height / 2.0);
                    std::cout << "Center point: " << center.longitude << ", " << center.latitude << std::endl;
                    entry.point_geom.push_back({center.longitude, center.latitude});

                    // Verifica delle coordinate come nei test commentati
                    std::cout << "\n=== Coordinate Verification ===" << std::endl;

                    // Controlla che abbiamo 1 punto centrale
                    if (entry.point_geom.size() == 1) {
                        std::cout << "✓ Point geometry has correct size (1)" << std::endl;
                    } else {
                        std::cout << "✗ Point geometry size mismatch. Expected: 1, Got: " << entry.point_geom.size() << std::endl;
                    }

                    // Controlla che abbiamo 5 punti per il poligono
                    if (entry.polygon_geom.size() == 5) {
                        std::cout << "✓ Polygon geometry has correct size (5)" << std::endl;
                    } else {
                        std::cout << "✗ Polygon geometry size mismatch. Expected: 5, Got: " << entry.polygon_geom.size() << std::endl;
                    }

                    // Verifica coordinate del punto centrale
                    if (entry.point_geom.size() > 0) {
                        const auto& centerPt = entry.point_geom[0];
                        if (std::abs(centerPt.longitude - 175.403526) < 1e-5) {
                            std::cout << "✓ Center point longitude correct: " << centerPt.longitude << std::endl;
                        } else {
                            std::cout << "✗ Center point longitude mismatch. Expected: ~175.403526, Got: " << centerPt.longitude << std::endl;
                        }

                        if (std::abs(centerPt.latitude - (-41.066254)) < 1e-5) {
                            std::cout << "✓ Center point latitude correct: " << centerPt.latitude << std::endl;
                        } else {
                            std::cout << "✗ Center point latitude mismatch. Expected: ~-41.066254, Got: " << centerPt.latitude << std::endl;
                        }
                    }

                    // Verifica coordinate dei vertici del poligono
                    if (entry.polygon_geom.size() >= 5) {
                        const double expectedCoords[][2] = {
                            {175.4029416126, -41.06584339802},  // Point 0 (UL)
                            {175.4040791346, -41.06581965903},  // Point 1 (UR)
                            {175.4041099344, -41.06666483358},  // Point 2 (LR)
                            {175.4029723979, -41.06668857327},  // Point 3 (LL)
                            {175.4029416126, -41.06584339802}   // Point 4 (UL again)
                        };

                        for (int i = 0; i < 5; i++) {
                            const auto& pt = entry.polygon_geom[i];
                            if (std::abs(pt.longitude - expectedCoords[i][0]) < 1e-9) {
                                std::cout << "✓ Polygon point " << i << " longitude correct: " << pt.longitude << std::endl;
                            } else {
                                std::cout << "✗ Polygon point " << i << " longitude mismatch. Expected: "
                                         << expectedCoords[i][0] << ", Got: " << pt.longitude << std::endl;
                            }

                            if (std::abs(pt.latitude - expectedCoords[i][1]) < 1e-9) {
                                std::cout << "✓ Polygon point " << i << " latitude correct: " << pt.latitude << std::endl;
                            } else {
                                std::cout << "✗ Polygon point " << i << " latitude mismatch. Expected: "
                                         << expectedCoords[i][1] << ", Got: " << pt.latitude << std::endl;
                            }
                        }
                    }

                    OCTDestroyCoordinateTransformation(hTransform);
                    std::cout << "Destroyed coordinate transformation" << std::endl;
                } else {
                    std::cout << "Failed to create coordinate transformation" << std::endl;
                }

                OSRDestroySpatialReference(hWgs84);
                OSRDestroySpatialReference(hSrs);
                std::cout << "Cleaned up spatial reference objects" << std::endl;
            } else {
                std::cout << "Projection is empty" << std::endl;
            }
        } else {
            std::cout << "No projection reference found in dataset" << std::endl;
        }
    } else {
        std::cout << "No geotransform found in dataset" << std::endl;
    }

    int bandCount = GDALGetRasterCount(hDataset);
    std::cout << "Number of raster bands: " << bandCount << std::endl;

    for (int i = 0; i < bandCount; i++) {
        std::cout << "Processing band " << (i + 1) << " of " << bandCount << std::endl;

        GDALRasterBandH hBand = GDALGetRasterBand(hDataset, i + 1);
        if (hBand != nullptr) {
            GDALDataType dataType = GDALGetRasterDataType(hBand);
            const char* dataTypeName = GDALGetDataTypeName(dataType);
            std::cout << "Band " << (i + 1) << " data type: " << dataTypeName << std::endl;

            GDALColorInterp colorInterp = GDALGetRasterColorInterpretation(hBand);
            const char* colorInterpName = GDALGetColorInterpretationName(colorInterp);
            std::cout << "Band " << (i + 1) << " color interpretation: " << colorInterpName << std::endl;
        } else {
            std::cout << "Failed to get band " << (i + 1) << std::endl;
        }
    }

    std::cout << "Closing GDAL dataset" << std::endl;
    GDALClose(hDataset);
    std::cout << "GeoRaster processing completed successfully" << std::endl;
}

int main()
{

    const auto exeFolder = getExeFolderPath().string();

    setupEnvironmentVariables(exeFolder);
    setupLocaleUnified();
    initializeGDALandPROJ();

    printVersions();

    // Analizza il file wro.tif
    const std::string wroFilePath = (std::filesystem::path(exeFolder) / "wro.tif").string();

    if (std::filesystem::exists(wroFilePath)) {
        analyzeGeoTIFF(wroFilePath);
    } else {
        std::cout << "\nWarning: wro.tif not found at: " << wroFilePath << std::endl;
        std::cout << "Looking for wro.tif in current directory..." << std::endl;

        if (std::filesystem::exists("wro.tif")) {
            analyzeGeoTIFF("wro.tif");
        } else {
            std::cout << "wro.tif not found in current directory either." << std::endl;
        }
    }

    return 0;
}