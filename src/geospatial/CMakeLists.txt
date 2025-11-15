add_library(DroneMapperGeospatial STATIC
    CoordinateTransform.cpp
    KMZGenerator.cpp
    WPMLWriter.cpp
    CoveragePatternGenerator.cpp
    FlightPathCalculator.cpp
    GeoUtils.cpp
)

target_link_libraries(DroneMapperGeospatial
    Qt6::Core
    DroneMapperModels
    GDAL::GDAL
    ${PROJ_LIBRARIES}
)

target_include_directories(DroneMapperGeospatial PUBLIC
    ${CMAKE_SOURCE_DIR}/include/geospatial
)
