#include "platform_utils.h"
#include "../hash.h"
#include <iostream>
#include <cstdlib>
#include <clocale>

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace PlatformUtils {

void setEnvironmentVariable(const std::string& name, const std::string& value) {
#ifdef WIN32
    _putenv_s(name.c_str(), value.c_str());
#else
    std::string env_string = name + "=" + value;
    putenv(const_cast<char*>(env_string.c_str()));
#endif
}

std::filesystem::path getExecutableDirectory() {
#ifdef WIN32
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    return std::filesystem::path(path).parent_path();
#else
    return std::filesystem::current_path();
#endif
}

void setupProjEnvironment(const std::string& executableDir) {
    const auto projDataPath = executableDir;

    // Check for proj.db existence
    const auto projDbPath = std::filesystem::path(projDataPath) / "proj.db";
    if (!std::filesystem::exists(projDbPath)) {
        std::cout << "PROJ database not found at: " << projDbPath.string() << std::endl;
        std::cout << "Coordinate transformations may fail" << std::endl;
    } else {
        std::cout << "PROJ database found at: " << projDbPath.string() << std::endl;

        // Debug: Print proj.db hash for verification
        try {
            const std::string projDbHash = Hash::fileSHA256(projDbPath.string());
            std::cout << "proj.db hash: " << projDbHash << " (path: " << projDbPath.string() << ")" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "Error computing proj.db hash: " << e.what() << std::endl;
        }
    }

    // Set PROJ_DATA as the preferred modern variable
    if (!std::getenv("PROJ_DATA")) {
        setEnvironmentVariable("PROJ_DATA", projDataPath);
        std::cout << "Set PROJ_DATA: " << projDataPath << std::endl;
    }

    // Set PROJ_LIB only as legacy fallback if PROJ_DATA is not present
    if (!std::getenv("PROJ_LIB") && !std::getenv("PROJ_DATA")) {
        setEnvironmentVariable("PROJ_LIB", projDataPath);
        std::cout << "Set PROJ_LIB (fallback): " << projDataPath << std::endl;
    }
}

void setupLocale() {
    // Strategy: LC_ALL=C for stability, LC_CTYPE=UTF-8 for Unicode
    try {
        setEnvironmentVariable("LC_ALL", "C");
        std::setlocale(LC_ALL, "C");

#ifdef WIN32
        std::setlocale(LC_CTYPE, "en_US.UTF-8");
        std::cout << "Windows locale set: LC_ALL=C, LC_CTYPE=UTF-8" << std::endl;
#else
        // Try different common UTF-8 locales on Unix
        const char* utf8_locales[] = {"en_US.UTF-8", "C.UTF-8", "en_US.utf8", nullptr};

        bool utf8_set = false;
        for (const char** locale_name = utf8_locales; *locale_name; ++locale_name) {
            if (std::setlocale(LC_CTYPE, *locale_name) != nullptr) {
                std::cout << "Unix locale set: LC_ALL=C, LC_CTYPE=" << *locale_name << std::endl;
                utf8_set = true;
                break;
            }
        }

        if (!utf8_set) {
            std::cout << "Could not set UTF-8 locale for LC_CTYPE, using C" << std::endl;
        }
#endif
    } catch (const std::exception& e) {
        std::cout << "Locale setup failed: " << e.what() << std::endl;
        setEnvironmentVariable("LC_ALL", "C");
        std::setlocale(LC_ALL, "C");
        std::cout << "Using minimal C locale" << std::endl;
    }
}

} // namespace PlatformUtils
