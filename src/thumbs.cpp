
#include "thumbs.h"

#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <filesystem>

#include "exceptions.h"
#include "gdal_inc.h"
#include "hash.h"

#include "tiler.h"

#include <random>
#include <cstring>

namespace ddb
{

    const char *charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUWXYZ0123456789";

    // https://stackoverflow.com/a/50556436
    std::string generateRandomString(int length)
    {
        std::random_device rd;
        std::mt19937 generator(rd());
        const int len = static_cast<int>(strlen(charset));
        std::uniform_int_distribution<int> distribution{0, len - 1};

        std::string str(length, '\0');
        for (auto &dis : str)
            dis = charset[distribution(generator)];

        return str;
    }

    void generateImageThumb(const fs::path &imagePath,
                            int thumbSize,
                            const fs::path &outImagePath,
                            uint8_t **outBuffer,
                            int *outBufferSize)
    {
        std::string openPath = imagePath.string();
        bool tryReopen = false;

        GDALDatasetH hSrcDataset = GDALOpen(openPath.c_str(), GA_ReadOnly);

        if (!hSrcDataset && tryReopen)
        {
            openPath = imagePath.string();
            hSrcDataset = GDALOpen(openPath.c_str(), GA_ReadOnly);
        }

        if (!hSrcDataset)
        {
            throw GDALException("Cannot open " + openPath + " for reading");
        }

        const int width = GDALGetRasterXSize(hSrcDataset);
        const int height = GDALGetRasterYSize(hSrcDataset);
        const int bandCount = GDALGetRasterCount(hSrcDataset);

        int targetWidth;
        int targetHeight;

        if (width > height)
        {
            targetWidth = thumbSize;
            targetHeight =
                static_cast<int>((static_cast<float>(thumbSize) / static_cast<float>(width)) *
                                 static_cast<float>(height));
        }
        else
        {
            targetHeight = thumbSize;
            targetWidth =
                static_cast<int>((static_cast<float>(thumbSize) / static_cast<float>(height)) *
                                 static_cast<float>(width));
        }

        char **targs = nullptr;
        targs = CSLAddString(targs, "-outsize");
        targs = CSLAddString(targs, std::to_string(targetWidth).c_str());
        targs = CSLAddString(targs, std::to_string(targetHeight).c_str());
        targs = CSLAddString(targs, "-ot");
        targs = CSLAddString(targs, "Byte");

        // Using average resampling method
        targs = CSLAddString(targs, "-r");
        targs = CSLAddString(targs, "average");

        // Auto-scale values to 0-255 range
        targs = CSLAddString(targs, "-scale");

        // Detect and preserve nodata from source
        int hasNoData;
        double srcNoData = GDALGetRasterNoDataValue(
            GDALGetRasterBand(hSrcDataset, 1),
            &hasNoData); // Gestione delle bande: WebP supporta solo 3 (RGB) o 4 (RGBA) bande
        if (hasNoData)
        {
            // Con nodata, usiamo 4 bande (RGBA) per la trasparenza
            if (bandCount >= 3)
            {
                targs = CSLAddString(targs, "-b");
                targs = CSLAddString(targs, "1");
                targs = CSLAddString(targs, "-b");
                targs = CSLAddString(targs, "2");
                targs = CSLAddString(targs, "-b");
                targs = CSLAddString(targs, "3");
            }

            // Imposta il valore nodata sul dataset di destinazione
            targs = CSLAddString(targs, "-a_nodata");
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(0) << srcNoData;
            targs = CSLAddString(targs, oss.str().c_str());

            // Crea canale alpha dai valori nodata per trasparenza
            targs = CSLAddString(targs, "-dstalpha");
        }
        else
        {
            // Senza nodata, usiamo solo 3 bande (RGB)
            if (bandCount > 3)
            {
                targs = CSLAddString(targs, "-b");
                targs = CSLAddString(targs, "1");
                targs = CSLAddString(targs, "-b");
                targs = CSLAddString(targs, "2");
                targs = CSLAddString(targs, "-b");
                targs = CSLAddString(targs, "3");
            }
        }

        // Using WEBP driver
        targs = CSLAddString(targs, "-of");
        targs = CSLAddString(targs, "WEBP");

        targs = CSLAddString(targs, "-co");
        targs = CSLAddString(targs, "QUALITY=95");
        targs = CSLAddString(targs, "-co");
        targs = CSLAddString(targs, "LOSSLESS=FALSE");

        // Remove SRS
        targs = CSLAddString(targs, "-a_srs");
        targs = CSLAddString(targs, "");
        /*
            // Max 3 bands
            if (GDALGetRasterCount(hSrcDataset) > 3) {
                targs = CSLAddString(targs, "-b");
                targs = CSLAddString(targs, "1");
                targs = CSLAddString(targs, "-b");
                targs = CSLAddString(targs, "2");
                targs = CSLAddString(targs, "-b");
                targs = CSLAddString(targs, "3");
            }*/

        CPLSetConfigOption("GDAL_PAM_ENABLED", "NO"); // avoid aux files
        // CPLSetConfigOption("GDAL_ALLOW_LARGE_LIBJPEG_MEM_ALLOC", "YES"); // Avoids ERROR 6: Reading
        // this image would require libjpeg to allocate at least 107811081 bytes

        GDALTranslateOptions *psOptions = GDALTranslateOptionsNew(targs, nullptr);
        CSLDestroy(targs);
        bool writeToMemory = outImagePath.empty() && outBuffer != nullptr;
        if (writeToMemory)
        {
            // Write to memory via vsimem (assume WebP driver)
            std::string vsiPath = "/vsimem/" + generateRandomString(32) + ".webp";
            GDALDatasetH hNewDataset = GDALTranslate(vsiPath.c_str(), hSrcDataset, psOptions, nullptr);
            GDALFlushCache(hNewDataset);
            GDALClose(hNewDataset);

            // Read memory to buffer
            vsi_l_offset bufSize;
            *outBuffer = VSIGetMemFileBuffer(vsiPath.c_str(), &bufSize, TRUE);
            if (bufSize > std::numeric_limits<int>::max())
                throw GDALException("Exceeded max buf size");
            *outBufferSize = bufSize;
        }
        else
        {
            // Write directly to file
            GDALDatasetH hNewDataset =
                GDALTranslate(outImagePath.string().c_str(), hSrcDataset, psOptions, nullptr);
            GDALFlushCache(hNewDataset);
            GDALClose(hNewDataset);
        }

        GDALTranslateOptionsFree(psOptions);
        // GDALClose(hSrcVrt);
        GDALClose(hSrcDataset);
    }

}