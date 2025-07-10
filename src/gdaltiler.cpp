/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "gdaltiler.h"
#include "exceptions.h"
#include <memory>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

namespace ddb
{

    GDALTiler::GDALTiler(const std::string& inputPath, const std::string& outputPath,
        int tileSize, bool tms)
        : Tiler(inputPath, outputPath, tileSize, tms), inputPath(inputPath)
    {
        pngDrv = GDALGetDriverByName("PNG");
        if (pngDrv == nullptr)
            throw GDALException("Cannot create PNG driver");

        memDrv = GDALGetDriverByName("MEM");
        if (memDrv == nullptr)
            throw GDALException("Cannot create MEM driver");

        std::string openPath = inputPath;

        inputDataset = GDALOpen(inputPath.c_str(), GA_ReadOnly);
        if (inputDataset == nullptr)
            throw GDALException("Cannot open " + inputPath);

        nBands = GDALGetRasterCount(inputDataset);
        if (nBands == 0)
            throw GDALException("No raster bands found in " + inputPath);

        // Extract input SRS
        const OGRSpatialReferenceH inputSrs = OSRNewSpatialReference(nullptr);
        std::string inputSrsWkt;
        if (GDALGetProjectionRef(inputDataset) != nullptr)
        {
            inputSrsWkt = GDALGetProjectionRef(inputDataset);
        }
        else if (GDALGetGCPCount(inputDataset) > 0)
        {
            inputSrsWkt = GDALGetGCPProjection(inputDataset);
        }
        else
        {
            throw GDALException("No projection found in " + openPath);
        }

        char* wktp = const_cast<char*>(inputSrsWkt.c_str());
        if (OSRImportFromWkt(inputSrs, &wktp) != OGRERR_NONE)
            throw GDALException("Cannot read spatial reference system for " + openPath + ". Is PROJ available?");

        OSRSetAxisMappingStrategy(inputSrs, OSRAxisMappingStrategy::OAMS_TRADITIONAL_GIS_ORDER);

        // Setup output SRS
        const OGRSpatialReferenceH outputSrs = OSRNewSpatialReference(nullptr);
        OSRImportFromEPSG(outputSrs, 3857); // TODO: support for geodetic?

        // OSRSetAxisMappingStrategy(outputSrs, OSRAxisMappingStrategy::OAMS_TRADITIONAL_GIS_ORDER);

        if (!hasGeoreference(inputDataset))
            throw GDALException(openPath + " is not georeferenced.");

        // Check if we need to reproject
        if (!sameProjection(inputSrs, outputSrs))
        {
            origDataset = inputDataset;
            inputDataset = createWarpedVRT(inputDataset, outputSrs);
        }

        OSRDestroySpatialReference(inputSrs);
        OSRDestroySpatialReference(outputSrs);

        // TODO: nodata?
        // if (inNodata.size() > 0){
        //        update_no_data_values
        //}

        // warped_input_dataset = inputDataset
        nBands = dataBandsCount(inputDataset);

        double outGt[6];
        if (GDALGetGeoTransform(inputDataset, outGt) != CE_None)
            throw GDALException("Cannot fetch geotransform outGt");

        // Validate geotransform values
        if (std::abs(outGt[1]) < std::numeric_limits<double>::epsilon() ||
            std::abs(outGt[5]) < std::numeric_limits<double>::epsilon())
        {
            throw GDALException("Invalid geotransform: pixel size is zero");
        }

        oMinX = outGt[0];
        oMaxX = outGt[0] + GDALGetRasterXSize(inputDataset) * outGt[1];
        oMaxY = outGt[3];
        oMinY = outGt[3] - GDALGetRasterYSize(inputDataset) * outGt[1];

        std::cout << "Bounds (output SRS): " << oMinX << "," << oMinY << "," << oMaxX
            << "," << oMaxY << std::endl;

        // Max/min zoom level
        tMaxZ = mercator.zoomForPixelSize(outGt[1]);
        tMinZ = mercator.zoomForPixelSize(outGt[1] *
            std::max(GDALGetRasterXSize(inputDataset),
                GDALGetRasterYSize(inputDataset)) /
            tileSize);

        std::cout << "MinZ: " << tMinZ << std::endl;
        std::cout << "MaxZ: " << tMaxZ << std::endl;
        std::cout << "Num bands: " << nBands << std::endl;
    }

    bool GDALTiler::hasGeoreference(const GDALDatasetH& dataset)
    {
        double geo[6] = { 0.0, 1.0, 0.0, 0.0, 0.0, 1.0 };
        if (GDALGetGeoTransform(dataset, geo) != CE_None)
            throw GDALException("Cannot fetch geotransform in hasGeoreference");

        return (geo[0] != 0.0 || geo[1] != 1.0 || geo[2] != 0.0 || geo[3] != 0.0 ||
            geo[4] != 0.0 || geo[5] != 1.0) ||
            GDALGetGCPCount(dataset) != 0;
    }

    GDALDatasetH GDALTiler::createWarpedVRT(const GDALDatasetH& src,
        const OGRSpatialReferenceH& srs,
        GDALResampleAlg resampling)
    {
        char* dstWkt = nullptr;
        if (OSRExportToWkt(srs, &dstWkt) != OGRERR_NONE)
            throw GDALException("Cannot export dst WKT " + inputPath +
                ". Is PROJ available?");
        const char* srcWkt = GDALGetProjectionRef(src);

        GDALWarpOptions* opts = GDALCreateWarpOptions();

        // If the dataset does not have alpha, add it
        bool hasAlpha = FindAlphaBand(src) != nullptr;
        if (!hasAlpha)
        {
            opts->nDstAlphaBand = GDALGetRasterCount(src) + 1;
        }

        const GDALDatasetH warpedVrt = GDALAutoCreateWarpedVRT(
            src, srcWkt, dstWkt, resampling, 0.001, opts);

        CPLFree(dstWkt);

        if (warpedVrt == nullptr)
        {
            GDALDestroyWarpOptions(opts);
            throw GDALException("Cannot create warped VRT");
        }

        GDALDestroyWarpOptions(opts);

        return warpedVrt;
    }

    GDALRasterBandH GDALTiler::FindAlphaBand(const GDALDatasetH& dataset)
    {
        // If the dataset does not have alpha, add it
        const int numBands = GDALGetRasterCount(dataset);
        for (int n = 0; n < numBands; n++)
        {
            GDALRasterBandH b = GDALGetRasterBand(dataset, n + 1);
            if (GDALGetRasterColorInterpretation(b) == GCI_AlphaBand)
            {
                return b;
            }
        }
        return nullptr;
    }

    bool GDALTiler::sameProjection(const OGRSpatialReferenceH& a,
        const OGRSpatialReferenceH& b)
    {
        char* aProj = nullptr;
        char* bProj = nullptr;

        if (OSRExportToProj4(a, &aProj) != CE_None)
            throw GDALException("Cannot export proj4");
        if (OSRExportToProj4(b, &bProj) != CE_None)
        {
            CPLFree(aProj);
            throw GDALException("Cannot export proj4");
        }

        bool result = std::string(aProj) == std::string(bProj);

        CPLFree(aProj);
        CPLFree(bProj);

        return result;
    }

    int GDALTiler::dataBandsCount(const GDALDatasetH& dataset)
    {
        const GDALRasterBandH raster = GDALGetRasterBand(dataset, 1);
        const GDALRasterBandH alphaBand = GDALGetMaskBand(raster);
        const int bandsCount = GDALGetRasterCount(dataset);
        const GDALRasterBandH lastBand = GDALGetRasterBand(dataset, bandsCount);

        if (GDALGetMaskFlags(alphaBand) & GMF_ALPHA || bandsCount == 4 ||
            bandsCount == 2 || GDALGetRasterColorInterpretation(lastBand) == GCI_AlphaBand)
        {
            return bandsCount - 1;
        }

        return bandsCount;
    }

    GDALTiler::~GDALTiler()
    {
        if (inputDataset)
            GDALClose(inputDataset);
    }

    GQResult GDALTiler::geoQuery(GDALDatasetH ds, double ulx, double uly, double lrx, double lry, int querySize)
    {
        // Simplified geoQuery implementation
        GQResult result;

        // Get dataset dimensions
        int rasterXSize = GDALGetRasterXSize(ds);
        int rasterYSize = GDALGetRasterYSize(ds);

        // Get geotransform
        double geotransform[6];
        if (GDALGetGeoTransform(ds, geotransform) != CE_None)
            throw GDALException("Cannot get geotransform");

        // Simple pixel coordinate calculation
        double geoWidth = geotransform[1];
        double geoHeight = geotransform[5];

        // Calculate pixel coordinates
        int x1 = (int)((ulx - geotransform[0]) / geoWidth);
        int y1 = (int)((uly - geotransform[3]) / geoHeight);
        int x2 = (int)((lrx - geotransform[0]) / geoWidth);
        int y2 = (int)((lry - geotransform[3]) / geoHeight);

        // Clamp to raster bounds
        x1 = std::max(0, std::min(x1, rasterXSize));
        y1 = std::max(0, std::min(y1, rasterYSize));
        x2 = std::max(0, std::min(x2, rasterXSize));
        y2 = std::max(0, std::min(y2, rasterYSize));

        result.r.x = x1;
        result.r.y = y1;
        result.r.xsize = x2 - x1;
        result.r.ysize = y2 - y1;

        // Window coordinates (simplified)
        result.w.x = 0;
        result.w.y = 0;
        result.w.xsize = querySize ? querySize : result.r.xsize;
        result.w.ysize = querySize ? querySize : result.r.ysize;

        return result;
    }

    template <typename T>
    void GDALTiler::rescale(uint8_t* buffer, uint8_t* dstBuffer, size_t bufsize, double bMin, double bMax)
    {
        T* ptr = reinterpret_cast<T*>(buffer);

        // Avoid divide by zero
        if (bMin == bMax)
            bMax += 0.1;

        std::cout << "Min: " << bMin << " | Max: " << bMax << std::endl;

        // Can still happen according to GDAL for very large values
        if (bMin == bMax)
            throw GDALException(
                "Cannot scale values due to source min/max being equal");

        double deltamm = bMax - bMin;

        for (size_t i = 0; i < bufsize; i++)
        {
            double v = std::max(bMin, std::min(bMax, static_cast<double>(ptr[i])));
            dstBuffer[i] = static_cast<uint8_t>(255.0 * (v - bMin) / deltamm);
        }
    }

    std::string GDALTiler::tile(int tz, int tx, int ty)
    {
        std::string tilePath = getTilePath(tz, tx, ty);

        // Create folders for the tile

        // Get the parent directory
        fs::path dirPath = fs::path(tilePath).parent_path();

        // Create directories if they don't exist
        std::error_code ec;
        if (!fs::exists(dirPath)) {
            if (fs::create_directories(dirPath, ec)) {
                std::cout << "Directories created: " << dirPath << "\n";
            }
            else if (ec) {
                std::cerr << "Error creating directories: " << ec.message() << "\n";
                throw GDALException("Cannot create directories for tile path: " + dirPath.string());
            }
        }
        else {
            std::cout << "Directory already exists: " << dirPath << "\n";
        }


        if (tms) {
            ty = tmsToXYZ(ty, tz);
        }

        BoundingBox<Projected2Di> tMinMax = getMinMaxCoordsForZ(tz);
        if (!tMinMax.contains(tx, ty))
            throw GDALException("Out of bounds");

        // Create in-memory dataset for the tile
        int cappedBands = std::min(3, nBands);
        GDALDatasetH dsTile = GDALCreate(memDrv, "", tileSize, tileSize, cappedBands + 1, GDT_Byte, nullptr);
        if (dsTile == nullptr)
            throw GDALException("Cannot create dsTile");

        // Get tile bounds in projected coordinates
        BoundingBox<Projected2D> b = mercator.tileBounds(tx, ty, tz);

        // Query the source dataset
        GQResult g = geoQuery(inputDataset, b.min.x, b.max.y, b.max.x, b.min.y, tileSize);

        std::cout << "GeoQuery: " << g.r.x << "," << g.r.y << "|" << g.r.xsize << "x"
            << g.r.ysize << "|" << g.w.x << "," << g.w.y << "|" << g.w.xsize << "x"
            << g.w.ysize << std::endl;

        // Only process if we have valid data
        if (g.r.xsize != 0 && g.r.ysize != 0 && g.w.xsize != 0 && g.w.ysize != 0)
        {
            const GDALDataType type =
                GDALGetRasterDataType(GDALGetRasterBand(inputDataset, 1));

            const size_t wSize = g.w.xsize * g.w.ysize;
            std::unique_ptr<uint8_t[]> buffer(
                new uint8_t[GDALGetDataTypeSizeBytes(type) * cappedBands * wSize]);

            if (GDALDatasetRasterIO(inputDataset, GF_Read, g.r.x, g.r.y, g.r.xsize,
                g.r.ysize, buffer.get(), g.w.xsize, g.w.ysize, type,
                cappedBands, nullptr, 0, 0, 0) != CE_None)
            {
                throw GDALException("Cannot read input dataset window");
            }

            // Rescale if needed
            // We currently don't rescale byte datasets
            // TODO: allow people to specify rescale values

            if (type != GDT_Byte && type != GDT_Unknown)
            {
                std::unique_ptr<uint8_t[]> scaledBuffer(new uint8_t[GDALGetDataTypeSizeBytes(GDT_Byte) * cappedBands * wSize]);
                size_t bufSize = wSize * cappedBands;

                double globalMin = std::numeric_limits<double>::max(),
                    globalMax = std::numeric_limits<double>::min();

                for (int i = 0; i < cappedBands; i++)
                {
                    double bMin, bMax;

                    GDALDatasetH ds = origDataset != nullptr ? origDataset : inputDataset; // Use the actual dataset, not the VRT
                    GDALDatasetH hBand = GDALGetRasterBand(ds, i + 1);

                    CPLErr statsRes = GDALGetRasterStatistics(hBand, TRUE, FALSE, &bMin, &bMax, nullptr, nullptr);
                    if (statsRes == CE_Warning)
                    {
                        double bMean, bStdDev;
                        if (GDALGetRasterStatistics(hBand, TRUE, TRUE, &bMin, &bMax, &bMean, &bStdDev) != CE_None)
                            throw GDALException("Cannot compute band statistics (forced)");
                        if (GDALSetRasterStatistics(hBand, bMin, bMax, bMean, bStdDev) != CE_None)
                            throw GDALException("Cannot cache band statistics");

                        std::cout << "Cached band " << i << " statistics (" << bMin << ", " << bMax << ")" << std::endl;
                    }
                    else if (statsRes == CE_Failure)
                    {
                        throw GDALException("Cannot compute band statistics");
                    }

                    globalMin = std::min(globalMin, bMin);
                    globalMax = std::max(globalMax, bMax);
                }

                switch (type)
                {
                case GDT_Byte:
                    rescale<uint8_t>(buffer.get(), scaledBuffer.get(), bufSize, globalMin, globalMax);
                    break;
                case GDT_UInt16:
                    rescale<uint16_t>(buffer.get(), scaledBuffer.get(), bufSize, globalMin, globalMax);
                    break;
                case GDT_Int16:
                    rescale<int16_t>(buffer.get(), scaledBuffer.get(), bufSize, globalMin, globalMax);
                    break;
                case GDT_UInt32:
                    rescale<uint32_t>(buffer.get(), scaledBuffer.get(), bufSize, globalMin, globalMax);
                    break;
                case GDT_Int32:
                    rescale<int32_t>(buffer.get(), scaledBuffer.get(), bufSize, globalMin, globalMax);
                    break;
                case GDT_Float32:
                    rescale<float>(buffer.get(), scaledBuffer.get(), bufSize, globalMin, globalMax);
                    break;
                case GDT_Float64:
                    rescale<double>(buffer.get(), scaledBuffer.get(), bufSize, globalMin, globalMax);
                    break;
                default:
                    break;
                }

                buffer = std::move(scaledBuffer);
            }

            const GDALRasterBandH raster = GDALGetRasterBand(inputDataset, 1);
            GDALRasterBandH alphaBand = FindAlphaBand(inputDataset);
            if (alphaBand == nullptr)
                alphaBand = GDALGetMaskBand(raster);

            std::unique_ptr<uint8_t[]> alphaBuffer(
                new uint8_t[GDALGetDataTypeSizeBytes(GDT_Byte) * wSize]);
            if (GDALRasterIO(alphaBand, GF_Read, g.r.x, g.r.y, g.r.xsize, g.r.ysize,
                alphaBuffer.get(), g.w.xsize, g.w.ysize, GDT_Byte, 0,
                0) != CE_None)
            {
                throw GDALException("Cannot read input dataset alpha window");
            }

            // Write data


            if (GDALDatasetRasterIO(dsTile, GF_Write, g.w.x, g.w.y, g.w.xsize,
                g.w.ysize, buffer.get(), g.w.xsize, g.w.ysize,
                GDT_Byte, cappedBands, nullptr, 0, 0,
                0) != CE_None)
            {
                throw GDALException("Cannot write tile data");
            }

            std::cout << "Wrote tile data" << std::endl;

            const GDALRasterBandH tileAlphaBand =
                GDALGetRasterBand(dsTile, cappedBands + 1);
            GDALSetRasterColorInterpretation(tileAlphaBand, GCI_AlphaBand);

            if (GDALRasterIO(tileAlphaBand, GF_Write, g.w.x, g.w.y, g.w.xsize,
                g.w.ysize, alphaBuffer.get(), g.w.xsize, g.w.ysize,
                GDT_Byte, 0, 0) != CE_None)
            {
                throw GDALException("Cannot write tile alpha data");
            }

            std::cout << "Wrote tile alpha" << std::endl;

        }
        else
        {
            throw GDALException("Geoquery out of bounds");
        }

        const GDALDatasetH outDs = GDALCreateCopy(pngDrv, tilePath.c_str(), dsTile, FALSE,
            nullptr, nullptr, nullptr);
        if (outDs == nullptr)
            throw GDALException("Cannot create output dataset " + tilePath);

        GDALFlushCache(outDs);
        GDALClose(outDs);
        GDALClose(dsTile);


        return tilePath;

    }

} // namespace ddb
