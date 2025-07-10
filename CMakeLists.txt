cmake_minimum_required(VERSION 3.16.3)
project(test LANGUAGES CXX C)

# Enable C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MESSAGE_LOG_LEVEL VERBOSE)

message(STATUS "CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE Release)
else()
    message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
endif()

set(OUTPUT_DIR "${CMAKE_BINARY_DIR}")

# Define support files directory (these are typically in the main build directory)
set(SUPPORT_DIR "${CMAKE_BINARY_DIR}")
message(STATUS "Using support directory: ${SUPPORT_DIR}")

# Use vcpkg manifest mode
if(DEFINED ENV{VCPKG_ROOT})
    file(TO_CMAKE_PATH "$ENV{VCPKG_ROOT}" VCPKG_ROOT)
    set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
    message(STATUS "Using vcpkg from VCPKG_ROOT: ${VCPKG_ROOT}")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed")
    message(STATUS "Using vcpkg manifest mode from existing installation")
else()
    message(STATUS "vcpkg is not detected via VCPKG_ROOT or manifest, trying system installation")
endif()

# Determine platform-specific triplet if not already set
if(NOT DEFINED VCPKG_TARGET_TRIPLET)
    if(WIN32)
        set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "vcpkg triplet")
    elseif(APPLE)
        set(VCPKG_TARGET_TRIPLET "x64-osx" CACHE STRING "vcpkg triplet")
    elseif(UNIX)
        set(VCPKG_TARGET_TRIPLET "x64-linux" CACHE STRING "vcpkg triplet")
    else()
        message(WARNING "Unknown platform, using x64-windows as default triplet")
        set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "vcpkg triplet")
    endif()
endif()
message(STATUS "Using vcpkg triplet: ${VCPKG_TARGET_TRIPLET}")

# On Windows, define _WINSOCKAPI_ to avoid including winsock.h in windows.h
if(WIN32)
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

# Consider position-independent code for all targets (especially for shared libs)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

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
)

add_executable(${PROJECT_NAME}cmd ${SOURCES} ${HEADERS})

# Find and link dependencies from vcpkg
find_package(Threads REQUIRED)
find_package(GDAL CONFIG REQUIRED)
find_package(PROJ CONFIG REQUIRED)
find_package(GeoTIFF CONFIG REQUIRED)
find_package(unofficial-hash-library CONFIG REQUIRED)


# Common libraries
set(COMMON_LIBS
    Threads::Threads
    GDAL::GDAL
    PROJ::proj
    unofficial::hash-library
)

# Link libraries to the executable
target_link_libraries(${PROJECT_NAME}cmd ${COMMON_LIBS})

# Set PROJ directory based on platform
if (WIN32)
    set(PROJ_DIR "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/share/proj")
else()
    set(PROJ_DIR "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/share/proj4")
    if(NOT EXISTS "${PROJ_DIR}/proj.db")
        set(PROJ_DIR "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/share/proj")
    endif()
endif()

# Copy the necessary files to the binary directory after build
add_custom_command(TARGET ${PROJECT_NAME}cmd POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Copying proj.db and wro.tif to the executable directory..."
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PROJ_DIR}/proj.db" "$<TARGET_FILE_DIR:${PROJECT_NAME}cmd>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/wro.tif" "$<TARGET_FILE_DIR:${PROJECT_NAME}cmd>"
)

# Final message
message(STATUS "CMake setup complete")
