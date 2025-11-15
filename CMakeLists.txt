cmake_minimum_required(VERSION 3.16)

project(DroneMapper VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find required packages
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Gui
    Network
    Sql
    WebEngineWidgets
    Concurrent
    PrintSupport
    OpenGL
    OpenGLWidgets
)

find_package(GDAL REQUIRED)
find_package(PROJ REQUIRED)
find_package(SQLite3 REQUIRED)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${GDAL_INCLUDE_DIRS}
    ${PROJ_INCLUDE_DIRS}
)

# Subdirectories
add_subdirectory(src/models)
add_subdirectory(src/core)
add_subdirectory(src/geospatial)
add_subdirectory(src/ui)
add_subdirectory(src/photogrammetry)
add_subdirectory(src/app)

# Tests (optional)
option(BUILD_TESTS "Build tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation rules
install(TARGETS DroneMapper
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Resources
install(DIRECTORY resources/ DESTINATION share/dronemapper)
