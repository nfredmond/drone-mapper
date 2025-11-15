#ifndef KMZGENERATOR_H
#define KMZGENERATOR_H

#include "FlightPlan.h"
#include "WPMLWriter.h"
#include <QString>

namespace DroneMapper {
namespace Geospatial {

/**
 * @brief Generates KMZ files containing WPML for DJI Fly import
 *
 * KMZ format is a ZIP archive containing:
 * - waylines.wpml (mission definition)
 * - template.kml (optional visualization)
 * - res/ folder (optional resources)
 */
class KMZGenerator {
public:
    KMZGenerator();

    /**
     * @brief Generate KMZ file from flight plan
     * @param plan The flight plan to convert
     * @param outputPath Path where KMZ file will be saved
     * @param droneModel Target drone model
     * @return true if successful, false otherwise
     */
    bool generate(const Models::FlightPlan& plan,
                  const QString& outputPath,
                  WPMLWriter::DroneModel droneModel);

    /**
     * @brief Generate KMZ with visualization KML
     * @param plan The flight plan
     * @param outputPath Output file path
     * @param droneModel Target drone
     * @param includeVisualization Include KML for preview
     * @return true if successful
     */
    bool generateWithVisualization(const Models::FlightPlan& plan,
                                   const QString& outputPath,
                                   WPMLWriter::DroneModel droneModel,
                                   bool includeVisualization = true);

    QString lastError() const { return m_lastError; }

private:
    bool createKMZ(const QString& wpmlContent,
                   const QString& kmlContent,
                   const QString& outputPath);

    QString generateKML(const Models::FlightPlan& plan);
    QString generatePlacemark(const Models::Waypoint& waypoint, int index);

    WPMLWriter m_wpmlWriter;
    QString m_lastError;
};

} // namespace Geospatial
} // namespace DroneMapper

#endif // KMZGENERATOR_H
