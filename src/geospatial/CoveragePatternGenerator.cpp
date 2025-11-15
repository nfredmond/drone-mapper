#include "CoveragePatternGenerator.h"
#include "GeoUtils.h"
#include <cmath>
#include <QtGui/QTransform>

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

    // Calculate line points
    QList<QPointF> linePoints = generateParallelLinePoints(polygon, direction, spacing);

    // Convert to waypoints with coordinates
    Models::GeospatialCoordinate origin = GeoUtils::calculateCentroid(polygon);

    for (const QPointF& point : linePoints) {
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

    // Rotate polygon to align with direction
    QPolygonF rotated = rotatePolygon(polygon, -direction);

    // Find bounding box
    QRectF bounds = rotated.boundingRect();

    // Generate parallel lines
    int numLines = static_cast<int>(bounds.height() / spacing) + 1;
    bool goingRight = true;

    for (int i = 0; i <= numLines; ++i) {
        double y = bounds.top() + i * spacing;

        // Find intersection points with polygon at this y
        QList<double> intersections;
        int n = rotated.count();

        for (int j = 0; j < n; ++j) {
            QPointF p1 = rotated[j];
            QPointF p2 = rotated[(j + 1) % n];

            if ((p1.y() <= y && p2.y() > y) || (p2.y() <= y && p1.y() > y)) {
                double x = p1.x() + (y - p1.y()) * (p2.x() - p1.x()) / (p2.y() - p1.y());
                intersections.append(x);
            }
        }

        // Sort intersections
        std::sort(intersections.begin(), intersections.end());

        // Add waypoints for this line
        if (intersections.count() >= 2) {
            if (goingRight) {
                for (int k = 0; k < intersections.count(); k += 2) {
                    if (k + 1 < intersections.count()) {
                        points.append(QPointF(intersections[k], y));
                        points.append(QPointF(intersections[k + 1], y));
                    }
                }
            } else {
                for (int k = intersections.count() - 1; k >= 0; k -= 2) {
                    if (k - 1 >= 0) {
                        points.append(QPointF(intersections[k], y));
                        points.append(QPointF(intersections[k - 1], y));
                    }
                }
            }
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

    // Generate two sets of parallel lines at 90 degrees
    auto lines1 = generateParallelLines(polygon, altitude, 0.0, spacing, 75.0);
    auto lines2 = generateParallelLines(polygon, altitude, 90.0, spacing, 75.0);

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
    // Calculate image footprint width
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
    QPointF center = polygon.boundingRect().center();
    transform.translate(center.x(), center.y());
    transform.rotate(angleDegrees);
    transform.translate(-center.x(), -center.y());

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
