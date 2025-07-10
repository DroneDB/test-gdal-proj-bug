/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "tiler.h"
#include <cmath>
#include <string>
#include <filesystem>
#include "exceptions.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace ddb
{
    GlobalMercator::GlobalMercator(int tileSize) : tileSize(tileSize)
    {
        // 156543.03392804062 for tileSize 256 pixels
        initialResolution = 2 * M_PI * 6378137 / this->tileSize;
        // 20037508.342789244
        originShift = 2 * M_PI * 6378137 / 2.0;
    }

    BoundingBox<Projected2D> GlobalMercator::tileBounds(int tx, int ty, int zoom) const
    {
        Projected2D min = pixelsToMeters(tx * tileSize, ty * tileSize, zoom);
        Projected2D max = pixelsToMeters((tx + 1) * tileSize, (ty + 1) * tileSize, zoom);
        return BoundingBox<Projected2D>(min, max);
    }

    Projected2D GlobalMercator::pixelsToMeters(int px, int py, int zoom) const
    {
        double res = resolution(zoom);
        double mx = px * res - originShift;
        double my = py * res - originShift;
        return Projected2D(mx, my);
    }

    double GlobalMercator::resolution(int zoom) const
    {
        return initialResolution / (1 << zoom);
    }

    BoundingBox<Geographic2D> GlobalMercator::tileLatLonBounds(int tx, int ty, int zoom) const
    {
        BoundingBox<Projected2D> bounds = tileBounds(tx, ty, zoom);
        Geographic2D min = metersToLatLon(bounds.min.x, bounds.min.y);
        Geographic2D max = metersToLatLon(bounds.max.x, bounds.max.y);
        return BoundingBox<Geographic2D>(min, max);
    }

    Geographic2D GlobalMercator::metersToLatLon(double mx, double my) const
    {
        double lon = (mx / originShift) * 180.0;
        double lat = (my / originShift) * 180.0;
        lat = 180 / M_PI * (2 * atan(exp(lat * M_PI / 180.0)) - M_PI / 2.0);
        return Geographic2D(lat, lon);
    }

    Projected2Di GlobalMercator::metersToTile(double mx, double my, int zoom) const
    {
        Projected2D p = metersToPixels(mx, my, zoom);
        return Projected2Di(int(ceil(p.x / double(tileSize)) - 1),
                           int(ceil(p.y / double(tileSize)) - 1));
    }

    Projected2D GlobalMercator::metersToPixels(double mx, double my, int zoom) const
    {
        double res = resolution(zoom);
        double px = (mx + originShift) / res;
        double py = (my + originShift) / res;
        return Projected2D(px, py);
    }

    int GlobalMercator::zoomForLength(double meterLength) const
    {
        for (int zoom = 0; zoom < 32; zoom++)
        {
            if (resolution(zoom) <= meterLength)
                return zoom;
        }
        return 31;
    }

    int GlobalMercator::zoomForPixelSize(double pixelSize) const
    {
        for (int zoom = 0; zoom < 32; zoom++)
        {
            if (resolution(zoom) <= pixelSize)
                return zoom;
        }
        return 31;
    }

    Tiler::Tiler(const std::string &inputPath, const std::string &outputPath,
                 int tileSize, bool tms)
        : inputPath(inputPath), outputPath(outputPath), tileSize(tileSize),
          tms(tms), mercator(tileSize)
    {
        if (!std::filesystem::exists(inputPath))
            throw std::runtime_error(inputPath + " does not exist");
        if (tileSize <= 0 ||
            std::ceil(std::log2(tileSize) != std::floor(std::log2(tileSize))))
            throw GDALException("Tile size must be a power of 2 greater than 0");

        if (!outputPath.empty() && !std::filesystem::exists(outputPath))
        {
            // Try to create
            std::filesystem::create_directory(outputPath);
        }
    }

    std::string Tiler::getTilePath(int tz, int tx, int ty, bool createDirs) const
    {
        // Simple implementation for PNG tiles
        std::string path = outputPath + "/" + std::to_string(tz) + "/" +
                          std::to_string(tx) + "/" + std::to_string(ty) + ".png";
        return path;
    }

    int Tiler::tmsToXYZ(int ty, int tz) const
    {
        return (1 << tz) - 1 - ty;
    }

    BoundingBox<Projected2Di> Tiler::getMinMaxCoordsForZ(int tz) const
    {
        BoundingBox b(mercator.metersToTile(oMinX, oMinY, tz),
            mercator.metersToTile(oMaxX, oMaxY, tz));

        std::cout << "MinMaxCoordsForZ(" << tz << ") = (" << b.min.x << ", " << b.min.y << "), (" << b.max.x << ", " << b.max.y << ")" << std::endl;

        // Crop tiles extending world limits (+-180,+-90)
        b.min.x = std::max<int>(0, b.min.x);
        b.max.x = std::min<int>(static_cast<int>(std::pow(2, tz) - 1), b.max.x);

        // TODO: figure this out (TMS vs. XYZ)
        //    b.min.y = std::max<double>(0, b.min.y);
        //    b.max.y = std::min<double>(std::pow(2, tz - 1), b.max.y);

        return b;
    }

} // namespace ddb
