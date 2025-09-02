#include "system_info.h"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <clocale>
#include <gdal.h>
#include <proj.h>
#include <pdal/pdal_features.hpp>

namespace SystemInfo {

std::string getBuildInfo() {
    std::ostringstream ss;

#ifdef DEBUG
    ss << "Debug";
#else
    ss << "Release";
#endif

    ss << " build";

#ifdef __GNUC__
    ss << " (GCC " << __GNUC__ << "." << __GNUC_MINOR__ << ")";
#elif defined(_MSC_VER)
    ss << " (MSVC " << _MSC_VER << ")";
#endif

    ss << " compiled " << __DATE__ << " " << __TIME__;

    return ss.str();
}

void printVersions() {
    std::cout << "Build info: " << getBuildInfo() << std::endl;

    std::cout << "GDAL: " << GDALVersionInfo("RELEASE_NAME") << std::endl;

    auto pj_info = proj_info();
    std::cout << "PROJ: " << std::string(pj_info.release) + " (search path: " + pj_info.searchpath + ")" << std::endl;

    std::cout << "PROJ_LIB = " << (getenv("PROJ_LIB") ? getenv("PROJ_LIB") : "(not set)") << std::endl;
    std::cout << "GDAL_DATA = " << (getenv("GDAL_DATA") ? getenv("GDAL_DATA") : "(not set)") << std::endl;
    std::cout << "PROJ_DATA = " << (getenv("PROJ_DATA") ? getenv("PROJ_DATA") : "(not set)") << std::endl;

    std::cout << "PDAL: " << pdal::pdalVersion << std::endl;

    std::cout << "Current locale (LC_ALL): " << std::setlocale(LC_ALL, nullptr) << std::endl;
    std::cout << "Current locale (LC_CTYPE): " << std::setlocale(LC_CTYPE, nullptr) << std::endl;
    std::cout << "LC_ALL env var: " << (std::getenv("LC_ALL") ? std::getenv("LC_ALL") : "(not set)") << std::endl;
}

} // namespace SystemInfo
