add_library(DroneMapperUI STATIC
    ${CMAKE_SOURCE_DIR}/include/ui/MainWindow.h
    MainWindow.cpp
    MapWidget.cpp
    FlightPlanningWidget.cpp
    ProjectExplorer.cpp
    MissionParametersDialog.cpp
    DrawingToolsWidget.cpp
)

target_link_libraries(DroneMapperUI
    Qt6::Widgets
    Qt6::Gui
    Qt6::WebEngineWidgets
    DroneMapperCore
    DroneMapperModels
    DroneMapperGeospatial
)

target_include_directories(DroneMapperUI PUBLIC
    ${CMAKE_SOURCE_DIR}/include/ui
    ${CMAKE_SOURCE_DIR}/include/core
    ${CMAKE_SOURCE_DIR}/include/models
    ${CMAKE_SOURCE_DIR}/include/geospatial
)
