# UI Integration Implementation Checklist

This checklist guides the integration of 4 advanced features into DroneMapper's MainWindow UI.

---

## Phase 1: Wind Overlay Widget Integration

### 1.1 Header Modifications (MainWindow.h)
- [ ] Add include: `#include "WindOverlayWidget.h"`
- [ ] Add member variable: `WindOverlayWidget *m_windOverlay;`
- [ ] Add dock widget: `QDockWidget *m_windOverlayDock;`
- [ ] Add action: `QAction *m_showWindOverlayAction;`
- [ ] Add slot declaration: `void onToggleWindOverlay();`

### 1.2 Implementation (MainWindow.cpp)

#### In constructor:
- [ ] Create WindOverlayWidget instance: `m_windOverlay = new WindOverlayWidget(this);`
- [ ] Connect signals if needed

#### In createActions():
- [ ] Create action: `m_showWindOverlayAction = new QAction(tr("&Wind Overlay"), this);`
- [ ] Set shortcut (e.g., `Ctrl+W`)
- [ ] Connect to slot: `connect(..., &MainWindow::onToggleWindOverlay);`

#### In createMenus():
- [ ] Add to Tools menu: `QMenu *weatherMenu = m_toolsMenu->addMenu(tr("&Weather"));`
- [ ] Add action to menu: `weatherMenu->addAction(m_showWindOverlayAction);`

#### In createDockWidgets():
- [ ] Create dock widget: `m_windOverlayDock = new QDockWidget(tr("Wind Overlay Settings"), this);`
- [ ] Set widget: `m_windOverlayDock->setWidget(m_windOverlay);`
- [ ] Add to window: `addDockWidget(Qt::RightDockWidgetArea, m_windOverlayDock);`
- [ ] Add toggle to View menu: `m_viewMenu->addAction(m_windOverlayDock->toggleViewAction());`

#### New slot implementation:
```cpp
void MainWindow::onToggleWindOverlay()
{
    if (m_windOverlayDock->isVisible()) {
        m_windOverlayDock->hide();
    } else {
        m_windOverlayDock->show();
    }
}
```

### 1.3 Testing
- [ ] Compile without errors
- [ ] Wind Overlay menu item appears
- [ ] Dock widget toggling works
- [ ] Keyboard shortcut works (Ctrl+W)
- [ ] Widget displays correctly
- [ ] No memory leaks (check with valgrind)
- [ ] Can be toggled on/off repeatedly

---

## Phase 2: Terrain Elevation Viewer Integration

### 2.1 Header Modifications (MainWindow.h)
- [ ] Add include: `#include "TerrainElevationViewer.h"`
- [ ] Add member variable: `TerrainElevationViewer *m_terrainViewer;`
- [ ] Add dock widget: `QDockWidget *m_terrainViewerDock;`
- [ ] Add action: `QAction *m_showTerrainViewerAction;`
- [ ] Add slot: `void onLoadTerrainDEM();`
- [ ] Add slot: `void onToggleTerrainViewer();`

### 2.2 Implementation (MainWindow.cpp)

#### In constructor:
- [ ] Create TerrainElevationViewer instance: `m_terrainViewer = new TerrainElevationViewer(this);`

#### In createActions():
- [ ] Create load action: `m_loadTerrainAction = new QAction(tr("&Load Terrain..."), this);`
- [ ] Create show action: `m_showTerrainViewerAction = new QAction(tr("&Terrain Viewer"), this);`
- [ ] Set keyboard shortcuts
- [ ] Connect to slots

#### In createMenus():
- [ ] Add to Tools > Visualization: `visualizationMenu->addAction(m_loadTerrainAction);`
- [ ] Add show action: `visualizationMenu->addAction(m_showTerrainViewerAction);`

#### In createDockWidgets():
- [ ] Create dock widget for viewer
- [ ] Create dock widget for settings (optional)
- [ ] Add to window

#### New slots:
```cpp
void MainWindow::onLoadTerrainDEM()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Load Terrain DEM"),
        QString(),
        tr("GeoTIFF (*.tif);;SRTM (*.hgt);;All Files (*)"));
    
    if (!fileName.isEmpty()) {
        if (m_terrainViewer->loadDEM(fileName)) {
            statusBar()->showMessage(tr("Terrain loaded successfully"), 5000);
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Failed to load terrain"));
        }
    }
}

void MainWindow::onToggleTerrainViewer()
{
    m_terrainViewerDock->setVisible(!m_terrainViewerDock->isVisible());
}
```

### 2.3 Testing
- [ ] Compile without errors
- [ ] Terrain Viewer menu items appear
- [ ] File dialog works
- [ ] Terrain file loading works
- [ ] 3D rendering displays
- [ ] Camera controls work (orbit, pan, zoom)
- [ ] Settings persist
- [ ] Export to image works
- [ ] No GPU errors
- [ ] Memory usage reasonable

---

## Phase 3: Point Cloud Viewer Integration

### 3.1 Header Modifications (MainWindow.h)
- [ ] Add include: `#include "PointCloudViewer.h"`
- [ ] Add member variable: `PointCloudViewer *m_pointCloudViewer;`
- [ ] Add dock widget: `QDockWidget *m_pointCloudDock;`
- [ ] Add action: `QAction *m_showPointCloudAction;`
- [ ] Add slots: `void onLoadPointCloud();` and `void onTogglePointCloud();`

### 3.2 Implementation (MainWindow.cpp)

#### In constructor:
- [ ] Create PointCloudViewer instance

#### In createActions():
- [ ] Create load action
- [ ] Create show action
- [ ] Set shortcuts
- [ ] Connect slots

#### In createMenus():
- [ ] Add to Tools > Visualization menu

#### In createDockWidgets():
- [ ] Create and add point cloud dock widget

#### New slots:
```cpp
void MainWindow::onLoadPointCloud()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Load Point Cloud"),
        QString(),
        tr("PLY (*.ply);;LAS (*.las);;LAZ (*.laz);;XYZ (*.xyz);;All Files (*)"));
    
    if (!fileName.isEmpty()) {
        if (m_pointCloudViewer->loadPointCloud(fileName)) {
            statusBar()->showMessage(tr("Point cloud loaded: %1 points")
                .arg(m_pointCloudViewer->pointCloud().size()), 5000);
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Failed to load point cloud"));
        }
    }
}

void MainWindow::onTogglePointCloud()
{
    m_pointCloudDock->setVisible(!m_pointCloudDock->isVisible());
}
```

### 3.3 Testing
- [ ] Compile without errors
- [ ] Point Cloud menu items work
- [ ] File loading works for all formats
- [ ] Large point clouds render
- [ ] Camera controls responsive
- [ ] Color schemes selectable
- [ ] Filtering works
- [ ] Measurement tools accessible
- [ ] Export functionality works
- [ ] Performance acceptable (test with 1M+ points)

---

## Phase 4: COLMAP Integration

### 4.1 Header Modifications (MainWindow.h)
- [ ] Add include: `#include "COLMAPIntegration.h"`
- [ ] Add member: `Photogrammetry::COLMAPIntegration *m_colmapIntegration;`
- [ ] Add dock widget: `QDockWidget *m_colmapProcessorDock;`
- [ ] Add action: `QAction *m_processCOLMAPAction;`
- [ ] Add slots:
  - [ ] `void onProcessWithCOLMAP();`
  - [ ] `void onCOLMAPProgressUpdated(double, QString);`
  - [ ] `void onCOLMAPPipelineCompleted(COLMAPResults);`
  - [ ] `void onCOLMAPError(QString);`

### 4.2 Implementation (MainWindow.cpp)

#### In constructor:
- [ ] Create COLMAPIntegration instance
- [ ] Connect all signals to slots

#### In createActions():
- [ ] Create action with text "Process Images with COLMAP..."
- [ ] Set shortcut (e.g., Ctrl+P)
- [ ] Connect to slot

#### In createMenus():
- [ ] Add to Tools > Photogrammetry menu (or Tools directly)

#### In createDockWidgets():
- [ ] Create dock widget for COLMAP processor
- [ ] Add progress bar widget
- [ ] Add log output text edit
- [ ] Add cancel/pause buttons

#### New slots:
```cpp
void MainWindow::onProcessWithCOLMAP()
{
    // Show configuration dialog
    COLMAPConfigDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    
    // Get config from dialog
    Photogrammetry::COLMAPConfig config = dialog.getConfig();
    
    // Validate
    QString error = config.validate();
    if (!error.isEmpty()) {
        QMessageBox::warning(this, tr("Invalid Configuration"), error);
        return;
    }
    
    // Start processing
    statusBar()->showMessage(tr("Starting COLMAP processing..."));
    m_colmapIntegration->runFullPipeline(config);
}

void MainWindow::onCOLMAPProgressUpdated(double progress, QString message)
{
    // Update progress bar
    // Update log
    statusBar()->showMessage(message);
}

void MainWindow::onCOLMAPPipelineCompleted(const COLMAPResults& results)
{
    QMessageBox::information(this, tr("Processing Complete"),
        tr("COLMAP pipeline completed successfully!\n\n"
           "Sparse points: %1\n"
           "Dense points: %2\n"
           "Mesh vertices: %3")
        .arg(results.numPoints3D)
        .arg(results.numDensePoints)
        .arg(results.numVertices));
    
    // Option to load results in Point Cloud Viewer
    if (QMessageBox::question(this, tr("View Results"),
        tr("View reconstruction in Point Cloud Viewer?")) == QMessageBox::Yes) {
        if (m_pointCloudViewer->loadPointCloud(results.fusedPointCloudPath)) {
            m_pointCloudDock->show();
        }
    }
}

void MainWindow::onCOLMAPError(QString error)
{
    QMessageBox::critical(this, tr("COLMAP Error"), error);
    statusBar()->showMessage(tr("COLMAP processing failed"));
}
```

### 4.3 New Dialog Implementation

Create new file: `include/ui/COLMAPConfigDialog.h` and `.cpp`
- [ ] Configuration dialog for COLMAP settings
- [ ] COLMAP executable path selector
- [ ] Image input directory selector
- [ ] Workspace output directory selector
- [ ] GPU checkbox
- [ ] Camera model dropdown
- [ ] Quality parameters sliders
- [ ] Validation before execution

### 4.4 Testing
- [ ] Compile without errors
- [ ] Check if COLMAP installed: `COLMAPIntegration::isCOLMAPInstalled()`
- [ ] Configuration dialog works
- [ ] Processing starts successfully
- [ ] Progress updates appear
- [ ] Cancel button works
- [ ] Completion dialog works
- [ ] Results can load in Point Cloud Viewer
- [ ] Log output captured and displayed
- [ ] Error handling works
- [ ] Long processing doesn't freeze UI

---

## Phase 5: Global Integration Tasks

### 5.1 Settings Persistence
- [ ] Save dock widget positions in `writeSettings()`
- [ ] Restore dock widget positions in `readSettings()`
- [ ] Save feature settings (visibility, preferences)
- [ ] Restore feature settings

### 5.2 Keyboard Shortcuts
- [ ] Wind Overlay: Ctrl+W
- [ ] Terrain Viewer: Ctrl+T
- [ ] Point Cloud: Ctrl+L
- [ ] COLMAP: Ctrl+P
- [ ] Add shortcuts documentation to Help menu

### 5.3 Status Bar Integration
- [ ] Wind data updates in status bar
- [ ] Terrain DEM info in status bar
- [ ] Point cloud statistics in status bar
- [ ] COLMAP stage progress in status bar

### 5.4 Toolbar Enhancement
- [ ] Add Visualization toolbar
- [ ] Add Photogrammetry toolbar
- [ ] Add icon buttons for each feature
- [ ] Make toolbars toggleable from View menu

### 5.5 Context Menus
- [ ] Right-click in MapWidget: "Show Wind Overlay"
- [ ] Right-click in MapWidget: "Overlay Terrain"
- [ ] Right-click in viewers: Export, Settings, etc.

### 5.6 Help System
- [ ] Add Help > Feature Guides
- [ ] Add Help > Keyboard Shortcuts
- [ ] Add tooltips to all buttons
- [ ] Add status bar tips for actions

---

## Phase 6: Quality Assurance

### 6.1 Compilation
- [ ] Clean build completes
- [ ] No compiler warnings
- [ ] No linker errors
- [ ] All includes resolved

### 6.2 Runtime Testing
- [ ] Application starts
- [ ] MainWindow displays correctly
- [ ] All menus accessible
- [ ] All dock widgets toggle
- [ ] All keyboard shortcuts work

### 6.3 Feature Testing (Each Feature)
- [ ] Load/create instance
- [ ] Display correctly
- [ ] Settings changeable
- [ ] Export works
- [ ] Cleanup on close

### 6.4 Integration Testing
- [ ] Multiple features active simultaneously
- [ ] No feature interferes with others
- [ ] Data can transfer between features
- [ ] Settings persist across sessions

### 6.5 Performance Testing
- [ ] Wind overlay doesn't impact MapWidget performance
- [ ] Terrain viewer handles large DEMs (test 2GB+ files)
- [ ] Point cloud handles millions of points
- [ ] COLMAP processing doesn't freeze UI
- [ ] Memory doesn't leak over extended use

### 6.6 Memory Testing
- [ ] Run with valgrind: `valgrind --leak-check=full ./dronemapper`
- [ ] No memory leaks detected
- [ ] Memory usage stays reasonable
- [ ] Toggle features on/off repeatedly without leak

### 6.7 Edge Cases
- [ ] Invalid file formats handled gracefully
- [ ] Missing COLMAP handled gracefully
- [ ] GPU unavailable handled gracefully
- [ ] Large file operations show progress
- [ ] Cancel operations work cleanly

---

## Phase 7: Documentation

### 7.1 Code Documentation
- [ ] All new methods documented
- [ ] Parameter descriptions added
- [ ] Return value descriptions added
- [ ] Signal documentation complete

### 7.2 User Documentation
- [ ] Feature guides written
- [ ] Screenshots added
- [ ] Keyboard shortcuts listed
- [ ] File format support documented

### 7.3 Developer Documentation
- [ ] Integration architecture documented
- [ ] Signal/slot connections mapped
- [ ] File structure explained
- [ ] Build instructions updated

---

## Estimated Timeline

| Phase | Components | Estimated Time | Status |
|-------|-----------|-----------------|--------|
| 1 | Wind Overlay | 2-3 hours | [ ] Not Started |
| 2 | Terrain Viewer | 2-3 hours | [ ] Not Started |
| 3 | Point Cloud | 2-3 hours | [ ] Not Started |
| 4 | COLMAP | 4-5 hours | [ ] Not Started |
| 5 | Global Tasks | 3-4 hours | [ ] Not Started |
| 6 | QA & Testing | 4-5 hours | [ ] Not Started |
| 7 | Documentation | 2-3 hours | [ ] Not Started |
| | **TOTAL** | **19-26 hours** | |

---

## Sign-Off

- [ ] All phases completed
- [ ] All tests passing
- [ ] Code reviewed
- [ ] Documentation complete
- [ ] Ready for release

**Completed By**: _________________ **Date**: _________

---

## Notes & Issues

```
[Use this space to track issues, blockers, and decisions]


```

