# DroneMapper Architecture

## Overview

DroneMapper follows a modular, layered architecture designed for maintainability, testability, and extensibility.

## Layer Architecture

```
┌─────────────────────────────────────┐
│         Presentation Layer          │
│  (Qt Widgets, WebEngine, Dialogs)   │
└─────────────────────────────────────┘
                 ↓
┌─────────────────────────────────────┐
│       Application Logic Layer       │
│   (ProjectManager, UI Controllers)  │
└─────────────────────────────────────┘
                 ↓
┌─────────────────────────────────────┐
│         Business Logic Layer        │
│  (FlightPlan, CoveragePatterns,     │
│   KMZ Generation, Processing)       │
└─────────────────────────────────────┘
                 ↓
┌─────────────────────────────────────┐
│          Data Access Layer          │
│    (DatabaseManager, Settings,      │
│     File I/O, External Libraries)   │
└─────────────────────────────────────┘
```

## Module Breakdown

### Models (`src/models`)
**Responsibility:** Data structures and domain entities

- `Project` - Top-level container for a mapping project
- `FlightPlan` - Mission definition with waypoints
- `Waypoint` - Individual GPS waypoint with actions
- `MissionParameters` - Flight configuration
- `GeospatialCoordinate` - GPS coordinate representation

**Dependencies:** Qt::Core only

### Core (`src/core`)
**Responsibility:** Application infrastructure

- `ProjectManager` - Project lifecycle and state management
- `DatabaseManager` - SQLite persistence layer
- `Settings` - Application configuration
- `Logger` - Logging system

**Dependencies:** Qt::Core, Qt::Sql, Models

### Geospatial (`src/geospatial`)
**Responsibility:** GIS operations and flight planning

**Key Classes:**
- `KMZGenerator` - Creates KMZ files for DJI drones
- `WPMLWriter` - Generates WPML XML per DJI spec
- `CoveragePatternGenerator` - Flight path algorithms
- `GeoUtils` - Geospatial calculations (distance, bearing, etc.)
- `CoordinateTransform` - PROJ-based coordinate conversions

**Dependencies:** Qt::Core, Qt::Gui, Models, GDAL, PROJ

### UI (`src/ui`)
**Responsibility:** User interface

- `MainWindow` - Application main window
- `MapWidget` - Interactive map (future: MapLibre GL)
- `FlightPlanningWidget` - Mission planning interface
- `ProjectExplorer` - Project tree view
- `MissionParametersDialog` - Configuration dialogs

**Dependencies:** Qt::Widgets, Qt::WebEngineWidgets, Core, Models, Geospatial

### Photogrammetry (`src/photogrammetry`)
**Responsibility:** 3D reconstruction and processing

*(Currently placeholders - future implementation)*

**Planned Classes:**
- `ProcessingPipeline` - Orchestrates processing workflow
- `ImageProcessor` - EXIF extraction, alignment
- `PointCloudGenerator` - Dense reconstruction
- `OrthomosaicGenerator` - Orthophoto creation
- `MeshGenerator` - 3D mesh creation

**Dependencies:** Qt::Core, Qt::Concurrent, Models, GDAL, (Metashape SDK / COLMAP)

## Design Patterns

### Singleton Pattern
Used for application-wide services:
- `ProjectManager::instance()`
- `DatabaseManager::instance()`
- `Settings::instance()`
- `Logger::instance()`

**Rationale:** Single source of truth for global state

### Factory Pattern
Used for creating complex objects:
- `CoveragePatternGenerator` creates waypoint patterns
- `KMZGenerator` creates KMZ files

**Rationale:** Encapsulates complex creation logic

### Observer Pattern (Qt Signals/Slots)
Used for event-driven communication:
- `ProjectManager` signals: `projectOpened()`, `projectSaved()`, `projectClosed()`
- UI components connect to these signals

**Rationale:** Loose coupling between modules

### Strategy Pattern
Used for interchangeable algorithms:
- `CoveragePatternGenerator::Pattern` enum
- Different photogrammetry backends (Metashape vs COLMAP)

**Rationale:** Algorithm flexibility

## Data Flow

### Flight Planning Workflow

```
User Input (UI)
    ↓
FlightPlanningWidget
    ↓
CoveragePatternGenerator.generateParallelLines()
    ↓
Creates List<Waypoint>
    ↓
FlightPlan.addWaypoints()
    ↓
KMZGenerator.generate()
    ↓
WPMLWriter.generate() → XML
    ↓
KMZ File (ZIP with WPML)
```

### Project Persistence

```
ProjectManager.saveProject()
    ↓
DatabaseManager.saveProject()
    ↓
QSqlQuery INSERT/UPDATE
    ↓
SQLite Database
```

## Threading Model

**Main Thread:**
- UI operations
- Event handling
- Light-weight calculations

**Background Threads (Future):**
- Photogrammetry processing (`Qt::Concurrent`)
- Large file I/O
- Network operations

**Thread Safety:**
- `Logger` uses `QMutex` for thread-safe writing
- Database operations currently single-threaded
- Future: Processing pipeline will use worker threads

## Error Handling

**Strategy:**
- Return `bool` for success/failure
- Provide `lastError()` methods for error messages
- Use Qt's logging framework
- Critical errors: `QMessageBox` to user
- Debug info: `LOG_DEBUG()` macros

**Example:**
```cpp
if (!kmzGenerator.generate(plan, path, droneModel)) {
    LOG_ERROR(kmzGenerator.lastError());
    QMessageBox::critical(this, "Error", kmzGenerator.lastError());
    return;
}
```

## Configuration Management

**Settings Storage:**
- Platform: `QSettings` (INI format on Linux, Registry on Windows)
- Location: `~/.config/DroneMapper/DroneMapper.conf`

**Hierarchy:**
```
General/
    DefaultProjectPath
    RecentProjects
FlightPlanning/
    DefaultAltitude
    DefaultSpeed
    DefaultFrontOverlap
    DefaultSideOverlap
Processing/
    UseGPU
    Quality
UI/
    Theme
    MainWindowGeometry
    MainWindowState
```

## Database Schema

```sql
CREATE TABLE projects (
    id TEXT PRIMARY KEY,
    name TEXT NOT NULL,
    description TEXT,
    project_path TEXT NOT NULL,
    status INTEGER,
    created_date TEXT,
    modified_date TEXT,
    image_count INTEGER DEFAULT 0
);

CREATE TABLE flight_plans (
    id TEXT PRIMARY KEY,
    project_id TEXT,
    name TEXT NOT NULL,
    description TEXT,
    pattern_type INTEGER,
    created_date TEXT,
    modified_date TEXT,
    mission_data BLOB,
    FOREIGN KEY (project_id) REFERENCES projects(id) ON DELETE CASCADE
);

CREATE TABLE settings (
    key TEXT PRIMARY KEY,
    value TEXT
);
```

## Future Architecture Considerations

### Photogrammetry Pipeline Architecture

```
ImageImporter
    ↓
FeatureExtractor (GPU)
    ↓
FeatureMatcher (GPU)
    ↓
SparseReconstructor
    ↓
DenseReconstructor (GPU)
    ↓
MeshGenerator
    ↓
TextureMapper
    ↓
OrthomosaicGenerator
```

### Plugin Architecture (Future)

Allow third-party extensions:
- Custom coverage patterns
- Additional export formats
- Integration with external tools

**Interface:**
```cpp
class IFlightPatternPlugin {
public:
    virtual QString name() const = 0;
    virtual QList<Waypoint> generate(const QPolygonF& area, const Parameters& params) = 0;
};
```

## Performance Considerations

**Optimization Targets:**
1. **Flight Path Generation:** \u003c 100ms for typical survey areas
2. **KMZ File Creation:** \u003c 500ms
3. **Database Queries:** \u003c 50ms
4. **UI Responsiveness:** 60 FPS, \u003c 16ms per frame

**Strategies:**
- Spatial indexing for large polygon operations
- Lazy loading of project data
- Caching of computed patterns
- GPU acceleration for processing (future)

## Security Considerations

1. **SQL Injection:** Use parameterized queries (`QSqlQuery::addBindValue`)
2. **File Path Validation:** Sanitize user-provided paths
3. **Input Validation:** Validate all user inputs (coordinates, numbers)
4. **Code Signing:** Sign binaries for distribution

## Testing Strategy

**Unit Tests:**
- Models: Data integrity, validation
- Geospatial: Calculation accuracy
- KMZ Generation: WPML compliance

**Integration Tests:**
- Database operations
- File I/O
- End-to-end flight planning workflow

**UI Tests:**
- User workflows
- Error handling
- Settings persistence

---

*This architecture is designed to evolve. As the project grows, this document will be updated to reflect architectural changes and lessons learned.*
