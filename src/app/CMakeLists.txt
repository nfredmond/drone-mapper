add_executable(DroneMapper
    main.cpp
    Application.cpp
)

target_link_libraries(DroneMapper
    Qt6::Widgets
    Qt6::Core
    DroneMapperCore
    DroneMapperModels
    DroneMapperGeospatial
    DroneMapperUI
    DroneMapperPhotogrammetry
)

target_include_directories(DroneMapper PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
