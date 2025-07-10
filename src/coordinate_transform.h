#pragma once

#include <vector>
#include <map>
#include <gdal.h>
#include <ogr_spatialref.h>

namespace CoordinateTransform {
    /**
     * Structure to hold coordinate data
     */
    struct Coordinate {
        double longitude;
        double latitude;
    };

    /**
     * Structure to hold geographic entry data with geometry
     */
    struct GeographicEntry {
        std::map<std::string, std::string> properties;
        std::vector<Coordinate> polygon_geometry;
        std::vector<Coordinate> point_geometry;
    };

    /**
     * Convert raster pixel coordinates to geographic coordinates
     * @param hTransform Coordinate transformation handle
     * @param geotransform GDAL geotransform array
     * @param pixelX Pixel X coordinate
     * @param pixelY Pixel Y coordinate
     * @return Geographic coordinate (longitude, latitude)
     */
    Coordinate convertRasterToGeographic(OGRCoordinateTransformationH hTransform,
                                       double* geotransform,
                                       double pixelX,
                                       double pixelY);

    /**
     * Verify coordinate values against expected results
     * @param entry Geographic entry containing geometries to verify
     */
    void verifyCoordinates(const GeographicEntry& entry);
}
