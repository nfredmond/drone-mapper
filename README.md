# DroneMapper - Professional Drone Flight Planning & Photogrammetry

A comprehensive desktop application for drone flight planning and photogrammetry processing, built with Qt6/C++ and designed for professional surveyors, municipal planning departments, and mapping consultants.

## 🎯 Project Status: Foundation Complete

**Build Status:** ✅ Successfully compiling
**Version:** 1.0.0-alpha
**Platform:** Linux (cross-platform capable via Qt6)

### ✅ Implemented Features (Phase 1)

#### Core Architecture
- ✅ Qt6-based desktop application framework
- ✅ MVVM architecture with proper separation of concerns
- ✅ SQLite database for project management
- ✅ Settings system with persistence
- ✅ Comprehensive logging infrastructure
- ✅ Project/flight plan data models

#### Flight Planning Module
- ✅ **Complete KMZ/WPML generator for DJI drones** (Mini 3, Air 3, Mavic 3)
  - Standards-compliant WPML XML generation
  - KMZ packaging with visualization
  - Support for all waypoint mission features
  - Compatible with DJI Fly manual import
- ✅ **Coverage pattern generators**
  - Parallel line (lawnmower) patterns
  - Grid patterns
  - Circular patterns around POI
  - Automatic spacing calculation based on camera parameters
- ✅ **Geospatial utilities**
  - Haversine distance calculations
  - Bearing and destination point calculations
  - Coordinate transformations (WGS84, UTM, projected systems)
  - Area and centroid calculations

## 🚀 Quick Start

```bash
# Build the project
mkdir build && cd build
cmake ..
make -j4

# Run the application
./src/app/DroneMapper
```

For detailed build instructions and dependencies, see the full README below.

---