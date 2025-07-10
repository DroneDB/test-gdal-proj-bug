#pragma once

#include <string>
#include "tiler.h"
#include "gdal_inc.h"

namespace ddb {

    struct GeoExtent
    {
        int x;
        int y;
        int xsize;
        int ysize;
    };

    struct GQResult
    {
        GeoExtent r;
        GeoExtent w;
    };

class GDALTiler : public Tiler {
public:
    GDALTiler(const std::string& inputPath, const std::string& outputPath, int tileSize = 256, bool tms = false);
    ~GDALTiler();

    std::string tile(int z, int x, int y);

private:
    std::string inputPath;

    GDALDriverH pngDrv;
    GDALDriverH memDrv;

    int rasterCount;

    GDALDatasetH inputDataset = nullptr;
    GDALDatasetH origDataset = nullptr;

    GQResult geoQuery(GDALDatasetH ds, double ulx, double uly, double lrx, double lry, int querySize = 0);
    int dataBandsCount(const GDALDatasetH& dataset);
    bool hasGeoreference(const GDALDatasetH& dataset);
    bool sameProjection(const OGRSpatialReferenceH& a,
        const OGRSpatialReferenceH& b);
    GDALDatasetH createWarpedVRT(
        const GDALDatasetH& src, const OGRSpatialReferenceH& srs,
        GDALResampleAlg resampling = GRA_NearestNeighbour);

    GDALRasterBandH FindAlphaBand(const GDALDatasetH& dataset);

    template <typename T>
    void rescale(uint8_t* buffer, uint8_t* dstBuffer, size_t bufsize, double bMin, double bMax);
};

}
