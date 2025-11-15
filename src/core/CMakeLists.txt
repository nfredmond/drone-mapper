add_library(DroneMapperCore STATIC
    ${CMAKE_SOURCE_DIR}/include/core/ProjectManager.h
    ${CMAKE_SOURCE_DIR}/include/core/DatabaseManager.h
    ${CMAKE_SOURCE_DIR}/include/core/Settings.h
    ${CMAKE_SOURCE_DIR}/include/core/Logger.h
    ProjectManager.cpp
    DatabaseManager.cpp
    Settings.cpp
    Logger.cpp
)

target_link_libraries(DroneMapperCore
    Qt6::Core
    Qt6::Sql
    DroneMapperModels
    SQLite::SQLite3
)

target_include_directories(DroneMapperCore PUBLIC
    ${CMAKE_SOURCE_DIR}/include/core
    ${CMAKE_SOURCE_DIR}/include/models
)
