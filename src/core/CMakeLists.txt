add_library(DroneMapperCore STATIC
    ${CMAKE_SOURCE_DIR}/include/core/ProjectManager.h
    ${CMAKE_SOURCE_DIR}/include/core/DatabaseManager.h
    ${CMAKE_SOURCE_DIR}/include/core/Settings.h
    ${CMAKE_SOURCE_DIR}/include/core/Logger.h
    ${CMAKE_SOURCE_DIR}/include/core/WeatherService.h
    ${CMAKE_SOURCE_DIR}/include/core/BatteryManager.h
    ${CMAKE_SOURCE_DIR}/include/core/SunCalculator.h
    ${CMAKE_SOURCE_DIR}/include/core/FlightPathOptimizer.h
    ${CMAKE_SOURCE_DIR}/include/core/AltitudeSafetyChecker.h
    ${CMAKE_SOURCE_DIR}/include/core/MissionStatistics.h
    ProjectManager.cpp
    DatabaseManager.cpp
    Settings.cpp
    Logger.cpp
    WeatherService.cpp
    BatteryManager.cpp
    SunCalculator.cpp
    FlightPathOptimizer.cpp
    AltitudeSafetyChecker.cpp
    MissionStatistics.cpp
)

target_link_libraries(DroneMapperCore
    Qt6::Core
    Qt6::Sql
    Qt6::Network
    Qt6::Gui
    DroneMapperModels
    SQLite::SQLite3
)

target_include_directories(DroneMapperCore PUBLIC
    ${CMAKE_SOURCE_DIR}/include/core
    ${CMAKE_SOURCE_DIR}/include/models
)
