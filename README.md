# GDAL/PROJ Test Application

This application demonstrates GDAL and PROJ coordinate transformation functionality. The project has been reorganized into modular components for better maintainability and code reusability.

## Project Structure

```
├── main.cpp                    # Main application entry point
├── hash.h / hash.cpp          # File hashing utilities (SHA256)
├── src/                       # Modular source components
│   ├── platform_utils.h/.cpp    # Cross-platform utilities (environment, paths)
│   ├── gdal_manager.h/.cpp       # GDAL/PROJ initialization and management
│   ├── coordinate_transform.h/.cpp # Coordinate transformation utilities
│   ├── geotiff_analyzer.h/.cpp    # GeoTIFF file analysis functionality
│   └── system_info.h/.cpp        # System and version information
├── wro.tif                    # Test GeoTIFF file
├── CMakeLists.txt            # Build configuration
└── vcpkg.json               # Package dependencies
```

## Components Description

### Platform Utils
- **Purpose**: Cross-platform environment variable management and path utilities
- **Key Functions**:
  - `setEnvironmentVariable()`: Cross-platform environment variable setting
  - `getExecutableDirectory()`: Get directory containing the executable
  - `setupProjEnvironment()`: Configure PROJ_DATA and PROJ_LIB paths
  - `setupLocale()`: Set up consistent locale settings

### GDAL Manager
- **Purpose**: Initialize and manage GDAL/PROJ libraries
- **Key Functions**:
  - `initialize()`: Initialize GDAL drivers and PROJ system
  - `primeProjectionSystem()`: Prime PROJ with dummy transformation
  - `verifyProjectionSystem()`: Verify PROJ configuration

### Coordinate Transform
- **Purpose**: Handle coordinate transformations and geometry operations
- **Key Structures**:
  - `Coordinate`: Longitude/latitude pair
  - `GeographicEntry`: Complete geographic data with properties and geometries
- **Key Functions**:
  - `convertRasterToGeographic()`: Convert pixel to geographic coordinates
  - `verifyCoordinates()`: Validate coordinate transformation results

### GeoTIFF Analyzer
- **Purpose**: Analyze GeoTIFF files and extract geographic information
- **Key Functions**:
  - `analyzeFile()`: Complete GeoTIFF analysis with coordinate extraction
  - `processBands()`: Extract raster band metadata

### System Info
- **Purpose**: Display system and library version information
- **Key Functions**:
  - `getBuildInfo()`: Get compiler and build information
  - `printVersions()`: Display GDAL, PROJ, and environment information

## Building

The project uses CMake and vcpkg for dependency management:

```bash
# Configure
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake

# Build
cmake --build build --config Release
```

## Dependencies

- **GDAL**: Geospatial Data Abstraction Library
- **PROJ**: Cartographic projections and coordinate transformations
- **Hash Library**: SHA256 file hashing
- **GeoTIFF**: GeoTIFF format support

## Usage

The application analyzes the provided `wro.tif` test file and:

1. Sets up cross-platform environment variables
2. Initializes GDAL and PROJ libraries
3. Displays system and library version information
4. Analyzes the GeoTIFF file to extract:
   - Raster dimensions and geotransform
   - Coordinate reference system (CRS) information
   - Geographic extent (polygon and center point)
   - Raster band metadata
5. Verifies coordinate transformation accuracy

## Code Organization Benefits

The reorganized structure provides:

- **Modularity**: Each component has a single responsibility
- **Reusability**: Functions can be used independently
- **Maintainability**: Clear separation of concerns
- **Testability**: Individual components can be unit tested
- **Documentation**: Well-defined interfaces with English comments

## CI/CD

The project includes a GitLab CI pipeline that:
- Automatically compiles the project
- Runs the test
- Returns success (green) if the test passes, failure (red) otherwise
