add_library(DroneMapperPhotogrammetry STATIC
    ${CMAKE_SOURCE_DIR}/include/photogrammetry/QualityEstimator.h
    ${CMAKE_SOURCE_DIR}/include/photogrammetry/GPUDetector.h
    ${CMAKE_SOURCE_DIR}/include/photogrammetry/ProcessingQueue.h
    ${CMAKE_SOURCE_DIR}/include/photogrammetry/COLMAPIntegration.h
    ProcessingPipeline.cpp
    ImageProcessor.cpp
    PointCloudGenerator.cpp
    OrthomosaicGenerator.cpp
    MeshGenerator.cpp
    ExportHandler.cpp
    QualityEstimator.cpp
    GPUDetector.cpp
    ProcessingQueue.cpp
    COLMAPIntegration.cpp
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
