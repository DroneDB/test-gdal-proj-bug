#include "coordinate_transform.h"
#include <iostream>
#include <cmath>
#include <map>

namespace CoordinateTransform {

Coordinate convertRasterToGeographic(OGRCoordinateTransformationH hTransform,
                                   double* geotransform,
                                   double pixelX,
                                   double pixelY) {
    // Convert pixel coordinates to geographic coordinates using geotransform
    double geoX = geotransform[0] + pixelX * geotransform[1] + pixelY * geotransform[2];
    double geoY = geotransform[3] + pixelX * geotransform[4] + pixelY * geotransform[5];

    // Transform to WGS84
    if (OCTTransform(hTransform, 1, &geoX, &geoY, nullptr) == TRUE) {
        // Note: OCTTransform with OAMS_AUTHORITY_COMPLIANT returns lat,lon instead of lon,lat
        // So geoX is latitude and geoY is longitude
        return {geoX, geoY}; // longitude, latitude
    } else {
        std::cout << "Warning: Coordinate transformation failed for pixel ("
                  << pixelX << ", " << pixelY << ")" << std::endl;
        return {0.0, 0.0};
    }
}

void verifyCoordinates(const GeographicEntry& entry) {
    std::cout << "\n=== Coordinate Verification ===" << std::endl;

    // Check that we have 1 center point
    if (entry.point_geometry.size() == 1) {
        std::cout << "✓ Point geometry has correct size (1)" << std::endl;
    } else {
        std::cout << "✗ Point geometry size mismatch. Expected: 1, Got: "
                  << entry.point_geometry.size() << std::endl;
    }

    // Check that we have 5 points for the polygon
    if (entry.polygon_geometry.size() == 5) {
        std::cout << "✓ Polygon geometry has correct size (5)" << std::endl;
    } else {
        std::cout << "✗ Polygon geometry size mismatch. Expected: 5, Got: "
                  << entry.polygon_geometry.size() << std::endl;
    }

    // Verify center point coordinates
    if (entry.point_geometry.size() > 0) {
        const auto& centerPt = entry.point_geometry[0];
        if (std::abs(centerPt.longitude - 175.403526) < 1e-5) {
            std::cout << "✓ Center point longitude correct: " << centerPt.longitude << std::endl;
        } else {
            std::cout << "✗ Center point longitude mismatch. Expected: ~175.403526, Got: "
                      << centerPt.longitude << std::endl;
        }

        if (std::abs(centerPt.latitude - (-41.066254)) < 1e-5) {
            std::cout << "✓ Center point latitude correct: " << centerPt.latitude << std::endl;
        } else {
            std::cout << "✗ Center point latitude mismatch. Expected: ~-41.066254, Got: "
                      << centerPt.latitude << std::endl;
        }
    }

    // Verify polygon vertex coordinates
    if (entry.polygon_geometry.size() >= 5) {
        const double expectedCoords[][2] = {
            {175.4029416126, -41.06584339802},  // Point 0 (UL)
            {175.4040791346, -41.06581965903},  // Point 1 (UR)
            {175.4041099344, -41.06666483358},  // Point 2 (LR)
            {175.4029723979, -41.06668857327},  // Point 3 (LL)
            {175.4029416126, -41.06584339802}   // Point 4 (UL again)
        };

        for (int i = 0; i < 5; i++) {
            const auto& pt = entry.polygon_geometry[i];
            if (std::abs(pt.longitude - expectedCoords[i][0]) < 1e-9) {
                std::cout << "✓ Polygon point " << i << " longitude correct: "
                          << pt.longitude << std::endl;
            } else {
                std::cout << "✗ Polygon point " << i << " longitude mismatch. Expected: "
                          << expectedCoords[i][0] << ", Got: " << pt.longitude << std::endl;
            }

            if (std::abs(pt.latitude - expectedCoords[i][1]) < 1e-9) {
                std::cout << "✓ Polygon point " << i << " latitude correct: "
                          << pt.latitude << std::endl;
            } else {
                std::cout << "✗ Polygon point " << i << " latitude mismatch. Expected: "
                          << expectedCoords[i][1] << ", Got: " << pt.latitude << std::endl;
            }
        }
    }
}

} // namespace CoordinateTransform
