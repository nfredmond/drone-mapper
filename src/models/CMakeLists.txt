add_library(DroneMapperModels STATIC
    Project.cpp
    FlightPlan.cpp
    Waypoint.cpp
    MissionParameters.cpp
    GeospatialCoordinate.cpp
)

target_link_libraries(DroneMapperModels
    Qt6::Core
)

target_include_directories(DroneMapperModels PUBLIC
    ${CMAKE_SOURCE_DIR}/include/models
)
