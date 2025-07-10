#include "geotiff_analyzer.h"
#include <iostream>
#include <filesystem>
#include <gdal.h>

namespace GeotiffAnalyzer {

void processBands(GDALDatasetH hDataset) {
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
}

CoordinateTransform::GeographicEntry analyzeFile(const std::string& filepath) {
    std::cout << "\n=== Analyzing GeoTIFF file: " << filepath << " ===" << std::endl;

    CoordinateTransform::GeographicEntry entry;

    std::cout << "Processing GeoRaster file: " << filepath << std::endl;

    GDALDatasetH hDataset = GDALOpen(filepath.c_str(), GA_ReadOnly);
    if (!hDataset) {
        std::cout << "GDAL failed to open dataset: " << filepath << std::endl;
        return entry;
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
                    return entry;
                }

                if (OSRImportFromEPSG(hWgs84, 4326) != OGRERR_NONE) {
                    std::cout << "Failed to import WGS84 EPSG, cleaning up spatial references" << std::endl;
                    OSRDestroySpatialReference(hWgs84);
                    OSRDestroySpatialReference(hSrs);
                    GDALClose(hDataset);
                    return entry;
                }

                std::cout << "OSRImportFromEPSG result: Success" << std::endl;

                OSRSetAxisMappingStrategy(hWgs84, OSRAxisMappingStrategy::OAMS_AUTHORITY_COMPLIANT);
                std::cout << "Set dest axis mapping strategy" << std::endl;

                OGRCoordinateTransformationH hTransform = OCTNewCoordinateTransformation(hSrs, hWgs84);
                std::cout << "Created coordinate transformation: " << (hTransform != nullptr ? "Success" : "Failed") << std::endl;

                if (hTransform != nullptr) {
                    std::cout << "Computing corner coordinates" << std::endl;

                    auto ul = CoordinateTransform::convertRasterToGeographic(hTransform, geotransform, 0.0, 0.0);
                    std::cout << "Upper Left: " << ul.latitude << ", " << ul.longitude << std::endl;

                    auto ur = CoordinateTransform::convertRasterToGeographic(hTransform, geotransform, width, 0);
                    std::cout << "Upper Right: " << ur.latitude << ", " << ur.longitude << std::endl;

                    auto lr = CoordinateTransform::convertRasterToGeographic(hTransform, geotransform, width, height);
                    std::cout << "Lower Right: " << lr.latitude << ", " << lr.longitude << std::endl;

                    auto ll = CoordinateTransform::convertRasterToGeographic(hTransform, geotransform, 0.0, height);
                    std::cout << "Lower Left: " << ll.latitude << ", " << ll.longitude << std::endl;

                    std::cout << "Adding points to polygon geometry" << std::endl;
                    entry.polygon_geometry.push_back({ul.longitude, ul.latitude});
                    entry.polygon_geometry.push_back({ur.longitude, ur.latitude});
                    entry.polygon_geometry.push_back({lr.longitude, lr.latitude});
                    entry.polygon_geometry.push_back({ll.longitude, ll.latitude});
                    entry.polygon_geometry.push_back({ul.longitude, ul.latitude});

                    auto center = CoordinateTransform::convertRasterToGeographic(hTransform, geotransform, width / 2.0, height / 2.0);
                    std::cout << "Center point: " << center.longitude << ", " << center.latitude << std::endl;
                    entry.point_geometry.push_back({center.longitude, center.latitude});

                    // Verify coordinates
                    CoordinateTransform::verifyCoordinates(entry);

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

    processBands(hDataset);

    std::cout << "Closing GDAL dataset" << std::endl;
    GDALClose(hDataset);
    std::cout << "GeoRaster processing completed successfully" << std::endl;

    return entry;
}

} // namespace GeotiffAnalyzer
