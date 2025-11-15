add_library(DroneMapperPhotogrammetry STATIC
    ProcessingPipeline.cpp
    ImageProcessor.cpp
    PointCloudGenerator.cpp
    OrthomosaicGenerator.cpp
    MeshGenerator.cpp
    ExportHandler.cpp
)

target_link_libraries(DroneMapperPhotogrammetry
    Qt6::Core
    Qt6::Concurrent
    DroneMapperModels
    GDAL::GDAL
)

target_include_directories(DroneMapperPhotogrammetry PUBLIC
    ${CMAKE_SOURCE_DIR}/include/photogrammetry
)
