#pragma once

namespace GdalManager {
    /**
     * Initialize GDAL and PROJ libraries
     * This includes registering GDAL drivers and priming PROJ transformations
     */
    void initialize();

    /**
     * Prime GDAL/PROJ by performing a dummy coordinate transformation
     * This ensures PROJ database and axis mapping strategies are properly initialized
     */
    void primeProjectionSystem();

    /**
     * Verify that PROJ is properly configured and accessible
     */
    void verifyProjectionSystem();
}
