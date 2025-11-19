#include "CoveragePatternGenerator.h"
#include "GeoUtils.h"
#include <cmath>
#include <QtGui/QTransform>
#include <algorithm>

namespace DroneMapper {
namespace Geospatial {

CoveragePatternGenerator::CoveragePatternGenerator()
{
}

QList<Models::Waypoint> CoveragePatternGenerator::generateParallelLines(
    const QPolygonF& polygon,
    double altitude,
    double direction,
    double spacing,
    double overlap)
{
    QList<Models::Waypoint> waypoints;

    if (polygon.count() < 3) {
        m_lastError = "Invalid polygon (less than 3 points)";
        return waypoints;
    }

    // 1. Calculate Centroid (Origin)
    Models::GeospatialCoordinate origin = GeoUtils::calculateCentroid(polygon);

    // 2. Project Polygon to Cartesian (Meters)
    QPolygonF cartesianPolygon;
    for (const QPointF& p : polygon) {
        // Create coord from point (y=lat, x=lon)
        Models::GeospatialCoordinate coord(p.y(), p.x(), 0);
        cartesianPolygon.append(GeoUtils::toCartesian(coord, origin));
    }

    // 3. Generate Lines in Cartesian Space
    QList<QPointF> cartesianPoints = generateParallelLinePoints(cartesianPolygon, direction, spacing);

    // 4. Project Points back to Geographic (Lat/Lon)
    for (const QPointF& point : cartesianPoints) {
        Models::GeospatialCoordinate coord = GeoUtils::fromCartesian(point, origin);
        coord.setAltitude(altitude);

        Models::Waypoint wp(coord);
        wp.setSpeed(8.0);
        wp.addAction(Models::Waypoint::Action::TakePhoto);
        waypoints.append(wp);
    }

    // Number the waypoints
    for (int i = 0; i < waypoints.count(); ++i) {
        waypoints[i].setWaypointNumber(i);
    }

    return waypoints;
}

QList<QPointF> CoveragePatternGenerator::generateParallelLinePoints(
    const QPolygonF& polygon,
    double direction,
    double spacing)
{
    QList<QPointF> points;

    // Rotate polygon to align with direction (make lines horizontal)
    QPolygonF rotated = rotatePolygon(polygon, -direction);

    // Find bounding box
    QRectF bounds = rotated.boundingRect();

    // Generate parallel lines
    // Ensure we cover the whole area
    int numLines = static_cast<int>(bounds.height() / spacing) + 2;
    double startY = bounds.top() + (spacing / 2.0); // Center lines
    
    bool goingRight = true;

    for (int i = 0; i < numLines; ++i) {
        double y = startY + i * spacing;
        
        // Optimization: Skip if outside bounds
        if (y > bounds.bottom()) break;

        // Find intersection points with polygon at this y
        QList<double> intersections;
        int n = rotated.count();

        for (int j = 0; j < n; ++j) {
            QPointF p1 = rotated[j];
            QPointF p2 = rotated[(j + 1) % n];

            // Check if line segment crosses y
            if ((p1.y() <= y && p2.y() > y) || (p2.y() <= y && p1.y() > y)) {
                // Calculate x intersection
                // Avoid division by zero
                if (std::abs(p2.y() - p1.y()) > 1e-9) {
                    double x = p1.x() + (y - p1.y()) * (p2.x() - p1.x()) / (p2.y() - p1.y());
                    intersections.append(x);
                }
            }
        }

        // Sort intersections to find segments inside polygon
        std::sort(intersections.begin(), intersections.end());

        // Add waypoints for this line
        // We expect pairs of intersections (enter, exit)
        if (intersections.count() >= 2) {
            // Basic lawnmower: just connect ends of segments
            // Only handle simple convex/concave, might break on complex multi-polygons if odd number of intersections
            // Robustness: take first and last? No, should take all segments.
            
            QList<QPointF> linePoints;
            for (int k = 0; k < intersections.count(); k += 2) {
                if (k + 1 < intersections.count()) {
                    linePoints.append(QPointF(intersections[k], y));
                    linePoints.append(QPointF(intersections[k + 1], y));
                }
            }

            if (!goingRight) {
                // Reverse points for this line
                std::reverse(linePoints.begin(), linePoints.end());
            }
            
            points.append(linePoints);
            goingRight = !goingRight;
        }
    }

    // Rotate points back
    QTransform transform;
    transform.rotate(direction);
    for (QPointF& point : points) {
        point = transform.map(point);
    }

    return points;
}

QList<Models::Waypoint> CoveragePatternGenerator::generateGrid(
    const QPolygonF& polygon,
    double altitude,
    double spacing)
{
    QList<Models::Waypoint> waypoints;

    // Generate two sets of parallel lines at 90 degrees offset
    // Pass 0.0 overlap for the second pass to keep spacing consistent
    // Actually reuse generateParallelLines is easiest
    
    auto lines1 = generateParallelLines(polygon, altitude, 0.0, spacing, 0.0);
    auto lines2 = generateParallelLines(polygon, altitude, 90.0, spacing, 0.0);

    waypoints.append(lines1);
    waypoints.append(lines2);

    // Renumber waypoints
    for (int i = 0; i < waypoints.count(); ++i) {
        waypoints[i].setWaypointNumber(i);
    }

    return waypoints;
}

QList<Models::Waypoint> CoveragePatternGenerator::generateCircular(
    const Models::GeospatialCoordinate& center,
    double radius,
    double altitude,
    int points)
{
    QList<Models::Waypoint> waypoints;

    for (int i = 0; i < points; ++i) {
        double angle = (360.0 / points) * i;
        Models::GeospatialCoordinate coord = GeoUtils::destinationPoint(center, radius, angle);
        coord.setAltitude(altitude);

        Models::Waypoint wp(coord);
        wp.setWaypointNumber(i);
        wp.setSpeed(5.0);  // Slower for circular patterns
        wp.setHeadingMode(Models::Waypoint::HeadingMode::PointOfInterest);
        // POI would need to be set in flight plan separately or here
        wp.addAction(Models::Waypoint::Action::TakePhoto);

        waypoints.append(wp);
    }

    return waypoints;
}

double CoveragePatternGenerator::calculateOptimalSpacing(
    double altitude,
    double sideOverlap,
    double sensorWidth,
    double focalLength)
{
    // Calculate image footprint width (Ground coverage)
    double footprintWidth = (sensorWidth * altitude) / focalLength;

    // Calculate spacing based on desired overlap
    double overlapFraction = sideOverlap / 100.0;
    double spacing = footprintWidth * (1.0 - overlapFraction);

    return spacing;
}

bool CoveragePatternGenerator::pointInPolygon(const QPointF& point, const QPolygonF& polygon)
{
    return polygon.containsPoint(point, Qt::OddEvenFill);
}

QPolygonF CoveragePatternGenerator::rotatePolygon(const QPolygonF& polygon, double angleDegrees)
{
    QTransform transform;
    // Rotate around origin (0,0) which is centroid in our projection
    transform.rotate(angleDegrees);
    return transform.map(polygon);
}

QPolygonF CoveragePatternGenerator::translatePolygon(const QPolygonF& polygon, const QPointF& offset)
{
    QPolygonF translated;
    for (const QPointF& point : polygon) {
        translated.append(point + offset);
    }
    return translated;
}

} // namespace Geospatial
} // namespace DroneMapper
