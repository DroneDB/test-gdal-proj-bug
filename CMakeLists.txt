cmake_minimum_required(VERSION 3.21.1)
project(test LANGUAGES CXX C)

# Enable C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_MESSAGE_LOG_LEVEL VERBOSE)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# LTO/IPO in Release (if supported)
if(POLICY CMP0069)
  cmake_policy(SET CMP0069 NEW)
endif()
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_ok OUTPUT ipo_msg)
if(ipo_ok)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
endif()

message(STATUS "Binary dir: ${CMAKE_BINARY_DIR}")

# Build type: only for single-config generators
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  message(STATUS "Setting build type to 'Release' (single-config generator).")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(OUTPUT_DIR "${CMAKE_BINARY_DIR}")

# Define support files directory (these are typically in the main build directory)
set(SUPPORT_DIR "${CMAKE_BINARY_DIR}")
message(STATUS "Using support directory: ${SUPPORT_DIR}")

# Warning level cross-platform
if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

message(STATUS "Using vcpkg triplet: ${VCPKG_TARGET_TRIPLET}")

# Define source files
set(SOURCES
    main.cpp
    src/hash.cpp
    src/platform_utils.cpp
    src/gdal_manager.cpp
    src/coordinate_transform.cpp
    src/geotiff_analyzer.cpp
    src/system_info.cpp
    src/gdaltiler.cpp
    src/tiler.cpp
    src/thumbs.cpp
)

# Define header files for IDE organization
set(HEADERS
    src/hash.h
    src/platform_utils.h
    src/gdal_manager.h
    src/coordinate_transform.h
    src/geotiff_analyzer.h
    src/system_info.h
    src/gdal_inc.h
    src/gdaltiler.h
    src/tiler.h
    src/thumbs.h
)

add_executable(${PROJECT_NAME}cmd ${SOURCES} ${HEADERS})

# Use C++17 features (alternative to CMAKE_CXX_STANDARD)
target_compile_features(${PROJECT_NAME}cmd PRIVATE cxx_std_17)

# Windows definitions
if(WIN32)
  target_compile_definitions(${PROJECT_NAME}cmd PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
  # Add _WINSOCKAPI_ if really necessary:
  # target_compile_definitions(${PROJECT_NAME}cmd PRIVATE _WINSOCKAPI_)
endif()

# Folder structure in IDE
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCES} ${HEADERS})

# Find and link dependencies from vcpkg
find_package(Threads REQUIRED)
find_package(GDAL CONFIG REQUIRED)
find_package(PROJ CONFIG REQUIRED)
find_package(PDAL CONFIG REQUIRED)
find_package(GeoTIFF CONFIG REQUIRED)
find_package(unofficial-hash-library CONFIG REQUIRED)

# Robust selection of PDAL and GeoTIFF targets
set(PDAL_TARGET "")
if(TARGET PDAL::pdalcpp)
  set(PDAL_TARGET PDAL::pdalcpp)
elseif(TARGET pdalcpp)
  set(PDAL_TARGET pdalcpp)
else()
  message(FATAL_ERROR "Cannot find PDAL::pdalcpp / pdalcpp target")
endif()

# Link (always use PRIVATE/PUBLIC)
target_link_libraries(${PROJECT_NAME}cmd PRIVATE
    Threads::Threads
    GDAL::GDAL
    PROJ::proj
    pdalcpp
    ${GEOTIFF_LIBRARIES}
    unofficial::hash-library
)

target_include_directories(${PROJECT_NAME}cmd PRIVATE ${GEOTIFF_INCLUDE_DIR})

# Automatic copy of ALL required DLLs next to the executable (MSVC only).
# Avoids endless manual chasing after gdal/proj/curl/zlib/etc.
if(WIN32)
  add_custom_command(TARGET ${PROJECT_NAME}cmd POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E echo "Staging runtime DLLs for ${PROJECT_NAME}cmd..."
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}cmd>
            $<TARGET_FILE_DIR:${PROJECT_NAME}cmd>
    COMMAND_EXPAND_LISTS
  )
endif()

# ---- PROJ data and example files ----

option(DEV_COPY_PROJ_DATA "Copy PROJ data near executable for runs from build dir" ON)

# Find PROJ data folder (vcpkg: share/proj or share/proj4)
# Note: this search is best-effort for local development.
find_path(PROJ_DATA_DIR
  NAMES proj.db
  HINTS
    "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/share/proj"
    "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/share/proj4"
    "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/share/proj"
    "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/share/proj4"
  PATH_SUFFIXES share/proj share/proj4
)

if(DEV_COPY_PROJ_DATA AND PROJ_DATA_DIR)
  add_custom_command(TARGET ${PROJECT_NAME}cmd POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E echo "Copying PROJ data from ${PROJ_DATA_DIR}..."
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${PROJ_DATA_DIR}/proj.db"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}cmd>/proj.db"
  )
endif()

# Copy example file
add_custom_command(TARGET ${PROJECT_NAME}cmd POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different
          "${CMAKE_SOURCE_DIR}/wro.tif"
          "$<TARGET_FILE_DIR:${PROJECT_NAME}cmd>/wro.tif"
)

# Final message
message(STATUS "CMake setup complete")
