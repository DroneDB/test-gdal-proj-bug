#pragma once

#include "geo.h"
#include <string>
#include <vector>

namespace ddb {

    class GlobalMercator
    {
        int tileSize;
        double originShift;
        double initialResolution;
        int maxZoomLevel;

    public:
        GlobalMercator(int tileSize);

        BoundingBox<Geographic2D> tileLatLonBounds(int tx, int ty, int zoom) const;

        // Bounds of the given tile in EPSG:3857 coordinates
        BoundingBox<Projected2D> tileBounds(int tx, int ty, int zoom) const;

        // Converts XY point from Spherical Mercator EPSG:3857 to lat/lon in WGS84
        // Datum
        Geographic2D metersToLatLon(double mx, double my) const;

        // Tile for given mercator coordinates
        Projected2Di metersToTile(double mx, double my, int zoom) const;

        // Converts pixel coordinates in given zoom level of pyramid to EPSG:3857"
        Projected2D pixelsToMeters(int px, int py, int zoom) const;

        // Converts EPSG:3857 to pyramid pixel coordinates in given zoom level
        Projected2D metersToPixels(double mx, double my, int zoom) const;

        // Tile covering region in given pixel coordinates
        Projected2Di pixelsToTile(double px, double py) const;

        // Resolution (meters/pixel) for given zoom level (measured at Equator)
        double resolution(int zoom) const;

        // Minimum zoom level that can fully contains a line of meterLength
        int zoomForLength(double meterLength) const;

        // Maximal scaledown zoom of the pyramid closest to the pixelSize.
        int zoomForPixelSize(double pixelSize) const;
    };

class  Tiler {
public:
    Tiler(const std::string& inputPath, const std::string& outputPath, int tileSize = 256, bool tms = false);

    std::string getTilePath(int z, int x, int y, bool createDirs = false) const;
    int tmsToXYZ(int ty, int tz) const;

    BoundingBox<Projected2Di> getMinMaxCoordsForZ(int z) const;

    int nBands;
    double oMinX, oMaxX, oMaxY, oMinY;
    int tMaxZ;
    int tMinZ;

protected:
    std::string inputPath;
    std::string outputPath;
    int tileSize;
    bool tms;
    GlobalMercator mercator;
};

}
