# GDAL+PROJ+PDAL Coordinate Transformation Bug Reproduction

This repository reproduces a critical coordinate transformation bug that occurs when PDAL is linked alongside GDAL and PROJ. The bug causes incorrect coordinate transformations for projected coordinate systems with axis order different from the traditional lon/lat order.

## Problem Description

When PDAL is included in a project that uses GDAL and PROJ for coordinate transformations, PROJ fails to generate the necessary `axisswap` operation in its transformation pipeline. This results in incorrect coordinates being returned for projected coordinate systems that use a non-standard axis order.

**Affected System (not limited to)**: NZGD2000 / New Zealand Transverse Mercator 2000 (EPSG:2193)
- Expected axis order: Northing, Easting (as defined in EPSG)
- Bug behavior: PROJ treats it as Easting, Northing without proper axis swapping

## Test Case

The test performs multiple operations on a GeoTIFF file (`wro.tif`) with EPSG:2193 projection:

1. **Coordinate Transformation**: Transforms corner coordinates to WGS84 (EPSG:4326)
2. **GDAL Tiling**: Generates map tiles at various zoom levels
3. **Thumbnail Generation**: Creates a WebP thumbnail of the raster

### Expected Behavior (without PDAL)
- Center coordinates: `175.404, -41.0663` (longitude, latitude)
- Successful tile generation with correct spatial positioning
- Valid thumbnail creation

### Buggy Behavior (with PDAL linked)
- Coordinates are calculated incorrectly due to missing axis transformation
- Tile generation fails or produces incorrectly positioned tiles
- Thumbnail generation may also be affected by the coordinate system issues

## Build Instructions

### Prerequisites
- Ubuntu 22.04+ (tested on Ubuntu 24.04, minimum 22.04 required)
- CMake 3.15+
- GCC/Clang compiler
- Git with submodule support
- Curl
- pkg-config

**Note**: Currently tested only on Linux (Ubuntu). Windows compatibility not yet verified.

- Install required packages:
```bash
sudo apt update
sudo apt install -y build-essential cmake git curl zip unzip tar pkg-config bison flex autoconf automake libtool python3 python3-pip python3-jinja2 python3-setuptools libsqlite3-dev
```

### Step 1: Clone Repository
```bash
git clone --recurse-submodules https://github.com/DroneDB/test-gdal-proj-bug.git
cd test-gdal-proj-bug
```

Or if already cloned:
```bash
git submodule update --init --recursive
```

### Step 2: Configure Build
```bash
export VCPKG_ROOT="$(pwd)/vcpkg"
./vcpkg/bootstrap-vcpkg.sh
mkdir -p build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE="../vcpkg/scripts/buildsystems/vcpkg.cmake" \
         -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_BUILD_TESTING=ON
```

### Step 3: Build
```bash
make -j$(nproc)
```

### Step 4: Run Test
```bash
./testcmd
```

## Bug Reproduction

### Working Configuration (Correct Output)
To see the correct behavior, comment out the following lines in `CMakeLists.txt`:
```cmake
find_package(PDAL CONFIG REQUIRED)
```
in the same file, remove `pdalcpp` from `target_link_libraries`

In `src/system_info.cpp` remove:

```cpp
#include <pdal/pdal_features.hpp>
```

and in function `printVersions` remove:

```cpp
std::cout << "PDAL: " << pdal::pdalVersion << std::endl;
```

Then rebuild and run. You should see coordinates like:
```
Center point: 175.404, -41.0663
Upper Left: 175.403, -41.0658
Upper Right: 175.404, -41.0658
Lower Right: 175.404, -41.0667
Lower Left: 175.403, -41.0667
```

### Buggy Configuration (Incorrect Output)
With PDAL linked (default configuration), the coordinate transformation produces incorrect results.

## Technical Details

### Environment Information
The test outputs comprehensive environment information including:
- GDAL version and data paths
- PROJ version and search paths
- Locale settings
- Build configuration

### Test Data
- **File**: `wro.tif`
- **Projection**: EPSG:2193 (NZGD2000 / New Zealand Transverse Mercator 2000)
- **Axis Order**: Northing, Easting (non-standard)
- **Dimensions**: 1913x1878 pixels

### Expected Correct Output
```
GDAL: 3.11.3
PROJ: Rel. 9.6.2, June 4th, 2025
Current locale (LC_CTYPE): C.UTF-8

=== Analyzing GeoTIFF file: wro.tif ===
Center point: 175.404, -41.0663
Upper Left: 175.403, -41.0658
Upper Right: 175.404, -41.0658
Lower Right: 175.404, -41.0667
Lower Left: 175.403, -41.0667

=== Coordinate Verification ===
✓ All coordinate checks passed

=== Testing GDALTiler Bug Reproduction ===
Testing 9 tiles for bug reproduction...
✓ Tile 14/16174/10245 found (812 bytes)
✓ Tile 18/258796/163923 found (41599 bytes)
✓ Tile 18/258797/163923 found (26845 bytes)
[...additional tiles...]
GDALTiler bug reproduction test completed
Thumbnail generated successfully: thumb.webp
```

## Dependencies (via vcpkg)

- **GDAL**: 3.11.3
- **PROJ**: 9.6.2
- **PDAL**: Latest (when bug is triggered)

## Bug details

This bug appears to be related to:
1. **Library initialization order** when PDAL is present
2. **PROJ pipeline generation** being affected by PDAL's presence
3. **Axis mapping strategy** not being correctly applied
4. **Downstream effects** on GDAL operations like tiling and thumbnail generation that depend on correct coordinate transformations

### Investigation Areas
- Check if PDAL modifies global PROJ settings during initialization
- Verify if PROJ's axis transformation pipeline generation is affected
- Review whether GDAL's spatial reference handling changes with PDAL present
- Investigate if coordinate system issues cascade to GDAL's raster processing operations (tiling, resampling, etc.)

## Contact

This issue affects coordinate accuracy in geospatial applications and cascades to other GDAL operations including tile generation and image processing. The comprehensive test case provides coordinate transformation verification, tiling functionality testing, and thumbnail generation to demonstrate the full scope of the problem.
