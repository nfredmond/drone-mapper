add_library(DroneMapperUI STATIC
    ${CMAKE_SOURCE_DIR}/include/ui/MainWindow.h
    ${CMAKE_SOURCE_DIR}/include/ui/MapWidget.h
    ${CMAKE_SOURCE_DIR}/include/ui/MapBridge.h
    ${CMAKE_SOURCE_DIR}/include/ui/MissionParametersDialog.h
    ${CMAKE_SOURCE_DIR}/include/ui/WeatherWidget.h
    MainWindow.cpp
    MapWidget.cpp
    MapBridge.cpp
    FlightPlanningWidget.cpp
    ProjectExplorer.cpp
    MissionParametersDialog.cpp
    DrawingToolsWidget.cpp
    WeatherWidget.cpp
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
