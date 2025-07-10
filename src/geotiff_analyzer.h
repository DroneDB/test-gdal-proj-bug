#pragma once

#include "coordinate_transform.h"
#include <string>

namespace GeotiffAnalyzer {
    /**
     * Analyze a GeoTIFF file and extract geographic information
     * @param filepath Path to the GeoTIFF file
     * @return GeographicEntry containing extracted geometry and properties
     */
    CoordinateTransform::GeographicEntry analyzeFile(const std::string& filepath);

    /**
     * Process raster bands and extract metadata
     * @param hDataset GDAL dataset handle
     */
    void processBands(GDALDatasetH hDataset);
}
