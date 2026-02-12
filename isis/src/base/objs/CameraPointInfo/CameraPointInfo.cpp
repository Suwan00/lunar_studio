// // SPDX-License-Identifier: CC0-1.0
// #include "CameraPointInfo.h"

// #include <QDebug>
// #include <iomanip>

// #include "Brick.h"
// #include "Camera.h"
// #include "CameraFocalPlaneMap.h"
// #include "Cube.h"
// #include "CubeManager.h"
// #include "Distance.h"
// #include "IException.h"
// #include "iTime.h"
// #include "Longitude.h"
// #include "PvlGroup.h"
// #include "SpecialPixel.h"
// #include "TProjection.h"
// #include "SpiceRotation.h"

// using namespace Isis;
// using namespace std;

// namespace Isis {


//   CameraPointInfo::CameraPointInfo() {
//     m_usedCubes = NULL;
//     m_usedCubes = new CubeManager();
//     m_usedCubes->SetNumOpenCubes(50);
//     m_currentCube = NULL;
//     m_camera = NULL;
//     m_csvOutput = false;
//   }

//   void CameraPointInfo::SetCSVOutput(bool csvOutput) {
//      m_csvOutput = csvOutput;
//    }

//   CameraPointInfo::~CameraPointInfo() {
//     if (m_usedCubes) {
//       delete m_usedCubes;
//       m_usedCubes = NULL;
//     }
//   }

//   void CameraPointInfo::SetCube(const QString &cubeFileName) {
//     m_currentCube = m_usedCubes->OpenCube(cubeFileName);
//     m_camera = m_currentCube->camera();
//   }

//   PvlGroup *CameraPointInfo::SetImage(const double sample, const double line,
//                                       const bool allowOutside, const bool allowErrors) {
//     if (CheckCube()) {
//       bool passed = m_camera->SetImage(sample, line);
//       return GetPointInfo(passed, allowOutside, allowErrors);
//     }
//     return NULL;
//   }

//   PvlGroup *CameraPointInfo::SetCenter(const bool allowOutside, const bool allowErrors) {
//     if (CheckCube()) {
//       bool passed = m_camera->SetImage(m_currentCube->sampleCount() / 2.0,
//                                        m_currentCube->lineCount() / 2.0);
//       return GetPointInfo(passed, allowOutside, allowErrors);
//     }
//     return NULL;
//   }

//   PvlGroup *CameraPointInfo::SetSample(const double sample,
//                                        const bool allowOutside,
//                                        const bool allowErrors) {
//     if (CheckCube()) {
//       bool passed = m_camera->SetImage(sample, m_currentCube->lineCount() / 2.0);
//       return GetPointInfo(passed, allowOutside, allowErrors);
//     }
//     return NULL;
//   }

//   PvlGroup *CameraPointInfo::SetLine(const double line,
//                                      const bool allowOutside,
//                                      const bool allowErrors) {
//     if (CheckCube()) {
//       bool passed = m_camera->SetImage(m_currentCube->sampleCount() / 2.0, line);
//       return GetPointInfo(passed, allowOutside, allowErrors);
//     }
//     return NULL;
//   }

//   PvlGroup *CameraPointInfo::SetGround(const double latitude, const double longitude,
//                                        const bool allowOutside, const bool allowErrors) {
//     if (CheckCube()) {
//       bool passed = m_camera->SetUniversalGround(latitude, longitude);
//       return GetPointInfo(passed, allowOutside, allowErrors);
//     }
//     return NULL;
//   }

//   bool CameraPointInfo::CheckCube() {
//     if (m_currentCube == NULL) {
//       string msg = "Please set a cube before setting parameters";
//       throw IException(IException::Programmer, msg, _FILEINFO_);
//       return false;
//     }
//     return true;
//   }

//   PvlGroup *CameraPointInfo::GetPointInfo(bool passed, bool allowOutside, bool allowErrors) {
//     PvlGroup *gp = new PvlGroup("GroundPoint");

//     // Declare keywords based on output format
//     if (!m_csvOutput) { // PVL Format
//       gp->addKeyword(PvlKeyword("Filename"));
//       gp->addKeyword(PvlKeyword("Sample"));
//       gp->addKeyword(PvlKeyword("Line"));
//       gp->addKeyword(PvlKeyword("PixelValue"));
//       gp->addKeyword(PvlKeyword("RightAscension"));
//       gp->addKeyword(PvlKeyword("Declination"));
//       gp->addKeyword(PvlKeyword("PlanetocentricLatitude"));
//       gp->addKeyword(PvlKeyword("PlanetographicLatitude"));
//       gp->addKeyword(PvlKeyword("PositiveEast360Longitude"));
//       gp->addKeyword(PvlKeyword("PositiveEast180Longitude"));
//       gp->addKeyword(PvlKeyword("PositiveWest360Longitude"));
//       gp->addKeyword(PvlKeyword("PositiveWest180Longitude"));
//       gp->addKeyword(PvlKeyword("BodyFixedCoordinate")); // 3 values
//       gp->addKeyword(PvlKeyword("LocalRadius"));
//       gp->addKeyword(PvlKeyword("SampleResolution"));
//       gp->addKeyword(PvlKeyword("LineResolution"));
//       gp->addKeyword(PvlKeyword("ObliqueDetectorResolution"));
//       gp->addKeyword(PvlKeyword("ObliquePixelResolution"));
//       gp->addKeyword(PvlKeyword("ObliqueLineResolution"));
//       gp->addKeyword(PvlKeyword("ObliqueSampleResolution"));
//       gp->addKeyword(PvlKeyword("SpacecraftPosition")); // 3 values
//       gp->addKeyword(PvlKeyword("SpacecraftAzimuth"));
//       gp->addKeyword(PvlKeyword("SlantDistance"));
//       gp->addKeyword(PvlKeyword("TargetCenterDistance"));
//       gp->addKeyword(PvlKeyword("SubSpacecraftLatitude"));
//       gp->addKeyword(PvlKeyword("SubSpacecraftLongitude"));
//       gp->addKeyword(PvlKeyword("SpacecraftAltitude"));
//       gp->addKeyword(PvlKeyword("OffNadirAngle"));
//       gp->addKeyword(PvlKeyword("SubSpacecraftGroundAzimuth"));
//       gp->addKeyword(PvlKeyword("SunPosition")); // 3 values
//       gp->addKeyword(PvlKeyword("SubSolarAzimuth"));
//       gp->addKeyword(PvlKeyword("SolarDistance"));
//       gp->addKeyword(PvlKeyword("SubSolarLatitude"));
//       gp->addKeyword(PvlKeyword("SubSolarLongitude"));
//       gp->addKeyword(PvlKeyword("SubSolarGroundAzimuth"));
//       gp->addKeyword(PvlKeyword("Phase"));
//       gp->addKeyword(PvlKeyword("Incidence"));
//       gp->addKeyword(PvlKeyword("Emission"));
//       gp->addKeyword(PvlKeyword("NorthAzimuth"));
//       gp->addKeyword(PvlKeyword("EphemerisTime"));
//       gp->addKeyword(PvlKeyword("UTC"));
//       gp->addKeyword(PvlKeyword("LocalSolarTime"));
//       gp->addKeyword(PvlKeyword("SolarLongitude"));
//       gp->addKeyword(PvlKeyword("LookDirectionBodyFixed")); // 3 values
//       gp->addKeyword(PvlKeyword("LookDirectionJ2000"));   // 3 values
//       gp->addKeyword(PvlKeyword("LookDirectionCamera"));  // 3 values
//       gp->addKeyword(PvlKeyword("C2M_RotationMatrix"));   // 9 values
//       // if (allowErrors) gp->addKeyword(PvlKeyword("Error"));
//     }
//     else { // CSV Format (User confirmed C2M_RotationMatrix as a single keyword is fine)
//         gp->addKeyword(PvlKeyword("Filename"));
//         gp->addKeyword(PvlKeyword("Sample"));
//         gp->addKeyword(PvlKeyword("Line"));
//         gp->addKeyword(PvlKeyword("PixelValue"));
//         gp->addKeyword(PvlKeyword("RightAscension"));
//         gp->addKeyword(PvlKeyword("Declination"));
//         gp->addKeyword(PvlKeyword("PlanetocentricLatitude"));
//         gp->addKeyword(PvlKeyword("PlanetographicLatitude"));
//         gp->addKeyword(PvlKeyword("PositiveEast360Longitude"));
//         gp->addKeyword(PvlKeyword("PositiveEast180Longitude"));
//         gp->addKeyword(PvlKeyword("PositiveWest360Longitude"));
//         gp->addKeyword(PvlKeyword("PositiveWest180Longitude"));
//         gp->addKeyword(PvlKeyword("BodyFixedCoordinate"));
//         gp->addKeyword(PvlKeyword("LocalRadius"));
//         gp->addKeyword(PvlKeyword("SampleResolution"));
//         gp->addKeyword(PvlKeyword("LineResolution"));
//         gp->addKeyword(PvlKeyword("SpacecraftPosition"));
//         gp->addKeyword(PvlKeyword("SpacecraftAzimuth"));
//         gp->addKeyword(PvlKeyword("SlantDistance"));
//         gp->addKeyword(PvlKeyword("TargetCenterDistance"));
//         gp->addKeyword(PvlKeyword("SubSpacecraftLatitude"));
//         gp->addKeyword(PvlKeyword("SubSpacecraftLongitude"));
//         gp->addKeyword(PvlKeyword("SpacecraftAltitude"));
//         gp->addKeyword(PvlKeyword("OffNadirAngle"));
//         gp->addKeyword(PvlKeyword("SubSpacecraftGroundAzimuth"));
//         gp->addKeyword(PvlKeyword("SunPosition"));
//         gp->addKeyword(PvlKeyword("SubSolarAzimuth"));
//         gp->addKeyword(PvlKeyword("SolarDistance"));
//         gp->addKeyword(PvlKeyword("SubSolarLatitude"));
//         gp->addKeyword(PvlKeyword("SubSolarLongitude"));
//         gp->addKeyword(PvlKeyword("SubSolarGroundAzimuth"));
//         gp->addKeyword(PvlKeyword("Phase"));
//         gp->addKeyword(PvlKeyword("Incidence"));
//         gp->addKeyword(PvlKeyword("Emission"));
//         gp->addKeyword(PvlKeyword("NorthAzimuth"));
//         gp->addKeyword(PvlKeyword("EphemerisTime"));
//         gp->addKeyword(PvlKeyword("UTC"));
//         gp->addKeyword(PvlKeyword("LocalSolarTime"));
//         gp->addKeyword(PvlKeyword("SolarLongitude"));
//         gp->addKeyword(PvlKeyword("LookDirectionBodyFixed"));
//         gp->addKeyword(PvlKeyword("LookDirectionJ2000"));
//         gp->addKeyword(PvlKeyword("LookDirectionCamera"));
//         gp->addKeyword(PvlKeyword("C2M_RotationMatrix"));
//         gp->addKeyword(PvlKeyword("ObliqueDetectorResolution"));
//         gp->addKeyword(PvlKeyword("ObliquePixelResolution"));
//         gp->addKeyword(PvlKeyword("ObliqueLineResolution"));
//         gp->addKeyword(PvlKeyword("ObliqueSampleResolution"));
//         // if (allowErrors) gp->addKeyword(PvlKeyword("Error"));
//     }

//     bool noErrors = passed;
//     QString error = "";
//     if (!m_camera->HasSurfaceIntersection()) {
//       error = "Requested position does not project in camera model; no surface intersection";
//       noErrors = false;
//       if (!allowErrors) throw IException(IException::Unknown, error, _FILEINFO_);
//     }
//     if (!m_camera->InCube() && !allowOutside) {
//       error = "Requested position does not project in camera model; not inside cube";
//       noErrors = false;
//       if (!allowErrors) throw IException(IException::Unknown, error, _FILEINFO_);
//     }

//     if (!noErrors) {
//       // Set all keywords to NULL or multiple "NULL"s if errors occurred.
//       // This revised loop attempts to handle multi-value keywords better.
//       for (int i = 0; i < gp->keywords(); i++) {
//         PvlKeyword &currentKeyword = (*gp)[i];
//         QString name = currentKeyword.name();
//         currentKeyword.clear(); // Clear existing values and units first

//         if (name == "BodyFixedCoordinate" || name == "SpacecraftPosition" || name == "SunPosition" ||
//             name == "LookDirectionBodyFixed" || name == "LookDirectionJ2000" || name == "LookDirectionCamera") {
//           currentKeyword.addValue("NULL"); currentKeyword.addValue("NULL"); currentKeyword.addValue("NULL");
//         }
//         else if (name == "C2M_RotationMatrix") {
//           for (int k=0; k<9; ++k) currentKeyword.addValue("NULL");
//         }
//         else { // For single-value keywords
//           // Add a single "NULL" value. If it's meant to be empty, PvlKeyword handles that.
//           currentKeyword.addValue("NULL");
//         }
//       }

//       // Original logic: If not allowed to throw errors, try to populate some minimal info
//       if (!allowErrors) {
//         double spB_err[3], sB_sun_err[3]; // Renamed sB to avoid conflict

//         m_camera->instrumentPosition(spB_err);
//         PvlKeyword &spKeyword = gp->findKeyword("SpacecraftPosition");
//         spKeyword.clear(); // Values might have been set to NULL already, clear them
//         spKeyword.addValue(toString(spB_err[0]), "km");
//         spKeyword.addValue(toString(spB_err[1])); // km unit typically for the array, not each element
//         spKeyword.addValue(toString(spB_err[2]));


//         try {
//           m_camera->sunPosition(sB_sun_err);
//           PvlKeyword &sunKeyword = gp->findKeyword("SunPosition");
//           sunKeyword.clear();
//           sunKeyword.addValue(toString(sB_sun_err[0]), "km");
//           sunKeyword.addValue(toString(sB_sun_err[1]), "km");
//           sunKeyword.addValue(toString(sB_sun_err[2]), "km");
//         }
//         catch (IException &) {
//           // Values already set to NULL by the loop above
//         }

//         // Attempt to populate LookDirections even in error state if !allowErrors
//         try {
//           std::vector<double>lookB_err = m_camera->lookDirectionBodyFixed();
//           PvlKeyword &ldbfKeyword = gp->findKeyword("LookDirectionBodyFixed");
//           ldbfKeyword.clear();
//           ldbfKeyword.addValue(toString(lookB_err[0])); // Units like DEGREE are removed by campt.cpp for PVL
//           ldbfKeyword.addValue(toString(lookB_err[1]));
//           ldbfKeyword.addValue(toString(lookB_err[2]));
//         } catch (IException &) {}

//         try {
//           std::vector<double>lookJ_err = m_camera->lookDirectionJ2000();
//           PvlKeyword &ldjKeyword = gp->findKeyword("LookDirectionJ2000");
//           ldjKeyword.clear();
//           ldjKeyword.addValue(toString(lookJ_err[0]));
//           ldjKeyword.addValue(toString(lookJ_err[1]));
//           ldjKeyword.addValue(toString(lookJ_err[2]));
//         } catch (IException &) {}

//         try {
//           double lookC_err[3];
//           m_camera->LookDirection(lookC_err);
//           PvlKeyword &ldcKeyword = gp->findKeyword("LookDirectionCamera");
//           ldcKeyword.clear();
//           ldcKeyword.addValue(toString(lookC_err[0]));
//           ldcKeyword.addValue(toString(lookC_err[1]));
//           ldcKeyword.addValue(toString(lookC_err[2]));
//         } catch (IException &) {}
//       } // end if (!allowErrors) inside if (!noErrors)

//       // Set final error message and some basic always-available info
//       // Ensure Error keyword exists before setting (it's added if allowErrors is true)
//       if (gp->hasKeyword("Error")) {
//           gp->findKeyword("Error").setValue(error); // setValue is fine for single value
//       }
//       // For other keywords, ensure they are cleared before setting single value if not done in loop
//       gp->findKeyword("FileName").setValue(m_currentCube->fileName());
//       gp->findKeyword("Sample").setValue(toString(m_camera->Sample()));
//       gp->findKeyword("Line").setValue(toString(m_camera->Line()));
      
//       PvlKeyword &etKeyword = gp->findKeyword("EphemerisTime");
//       etKeyword.clear();
//       etKeyword.setValue(toString(m_camera->time().Et()), "seconds");
//       if (!m_csvOutput) etKeyword.addComment("Time");
      
//       QString utc_val_err = m_camera->time().UTC(); // Renamed variable
//       gp->findKeyword("UTC").setValue(utc_val_err);

//       // Original comments for PVL output
//       if (!m_csvOutput) {
//           if (gp->hasKeyword("SpacecraftPosition")) gp->findKeyword("SpacecraftPosition").addComment("Spacecraft Information");
//           if (gp->hasKeyword("SunPosition")) gp->findKeyword("SunPosition").addComment("Sun Information");
//           if (gp->hasKeyword("Phase")) gp->findKeyword("Phase").addComment("Illumination and Other");
//       }
//     } // end if (!noErrors)
//     else { // noErrors == true (Successful case)
//       Brick b(3, 3, 1, m_currentCube->pixelType());

//       int intSamp = (int)(m_camera->Sample() + 0.5);
//       int intLine = (int)(m_camera->Line() + 0.5);
//       b.SetBasePosition(intSamp, intLine, 1);
//       m_currentCube->read(b);

//       // Rename local variables to avoid shadowing or confusion
//       double pB_coords[3], spB_coords[3], sB_sun_coords[3];
//       QString utc_val_succ;
//       double ssplat_val, ssplon_val, ocentricLat_val, ographicLat_val, pe360Lon_val, pw360Lon_val;

//       // Clear all keywords in the group before populating for the success case.
//       // This ensures no old/NULL values linger if a keyword isn't explicitly set below.
//       // However, this might remove keywords if they are not re-added/re-found below.
//       // A safer approach is to clear each keyword individually before setting its value(s).
//       // For this example, I will clear each keyword before setting it.

//       PvlKeyword *keyPtr; // For convenience

//       keyPtr = &gp->findKeyword("FileName"); keyPtr->clear(); keyPtr->setValue(m_currentCube->fileName());
//       keyPtr = &gp->findKeyword("Sample"); keyPtr->clear(); keyPtr->setValue(toString(m_camera->Sample()));
//       keyPtr = &gp->findKeyword("Line"); keyPtr->clear(); keyPtr->setValue(toString(m_camera->Line()));
//       keyPtr = &gp->findKeyword("PixelValue"); keyPtr->clear(); keyPtr->setValue(PixelToString(b[0]));

//       try {
//         keyPtr = &gp->findKeyword("RightAscension"); keyPtr->clear();
//         keyPtr->setValue(toString(m_camera->RightAscension()), "DEGREE");
//       } catch (IException &) { gp->findKeyword("RightAscension").addValue("Null"); } // Keep addValue if clear was done by name before
//       try {
//         keyPtr = &gp->findKeyword("Declination"); keyPtr->clear();
//         keyPtr->setValue(toString(m_camera->Declination()), "DEGREE");
//       } catch (IException &) { gp->findKeyword("Declination").addValue("Null"); }

//       ocentricLat_val = m_camera->UniversalLatitude();
//       keyPtr = &gp->findKeyword("PlanetocentricLatitude"); keyPtr->clear();
//       keyPtr->setValue(toString(ocentricLat_val), "DEGREE");

//       Distance radii[3];
//       m_camera->radii(radii);
//       ographicLat_val = TProjection::ToPlanetographic(ocentricLat_val,
//                                               radii[0].kilometers(),
//                                               radii[2].kilometers());
//       keyPtr = &gp->findKeyword("PlanetographicLatitude"); keyPtr->clear();
//       keyPtr->setValue(toString(ographicLat_val), "DEGREE");

//       pe360Lon_val = m_camera->UniversalLongitude();
//       keyPtr = &gp->findKeyword("PositiveEast360Longitude"); keyPtr->clear();
//       keyPtr->setValue(toString(pe360Lon_val), "DEGREE");
//       keyPtr = &gp->findKeyword("PositiveEast180Longitude"); keyPtr->clear();
//       keyPtr->setValue(toString(TProjection::To180Domain(pe360Lon_val)), "DEGREE");
//       pw360Lon_val = TProjection::ToPositiveWest(pe360Lon_val, 360);
//       keyPtr = &gp->findKeyword("PositiveWest360Longitude"); keyPtr->clear();
//       keyPtr->setValue(toString(pw360Lon_val), "DEGREE");
//       keyPtr = &gp->findKeyword("PositiveWest180Longitude"); keyPtr->clear();
//       keyPtr->setValue(toString( TProjection::To180Domain(pw360Lon_val)), "DEGREE");

//       m_camera->Coordinate(pB_coords);
//       PvlKeyword &bfcKeyword = gp->findKeyword("BodyFixedCoordinate"); bfcKeyword.clear();
//       bfcKeyword.addValue(toString(pB_coords[0]), "km");
//       bfcKeyword.addValue(toString(pB_coords[1]), "km"); // No unit on subsequent
//       bfcKeyword.addValue(toString(pB_coords[2]), "km");

//       keyPtr = &gp->findKeyword("LocalRadius"); keyPtr->clear();
//       keyPtr->setValue(toString(m_camera->LocalRadius().meters()), "meters");
//       keyPtr = &gp->findKeyword("SampleResolution"); keyPtr->clear();
//       keyPtr->setValue(toString(m_camera->SampleResolution()), "meters/pixel");
//       keyPtr = &gp->findKeyword("LineResolution"); keyPtr->clear();
//       keyPtr->setValue(toString(m_camera->LineResolution()), "meters/pixel");

//       if (gp->hasKeyword("ObliqueDetectorResolution")) {
//           keyPtr = &gp->findKeyword("ObliqueDetectorResolution"); keyPtr->clear();
//           keyPtr->setValue(toString(m_camera->ObliqueDetectorResolution()),"meters");
//       }
//       if (gp->hasKeyword("ObliquePixelResolution")) {
//           keyPtr = &gp->findKeyword("ObliquePixelResolution"); keyPtr->clear();
//           keyPtr->setValue(toString(m_camera->ObliquePixelResolution()), "meters/pix");
//       }
//       if (gp->hasKeyword("ObliqueLineResolution")) {
//           keyPtr = &gp->findKeyword("ObliqueLineResolution"); keyPtr->clear();
//           keyPtr->setValue(toString(m_camera->ObliqueLineResolution()),"meters");
//       }
//       if (gp->hasKeyword("ObliqueSampleResolution")) {
//           keyPtr = &gp->findKeyword("ObliqueSampleResolution"); keyPtr->clear();
//           keyPtr->setValue(toString(m_camera->ObliqueSampleResolution()),"meters");
//       }

//       m_camera->instrumentPosition(spB_coords);
//       PvlKeyword &sposKeyword = gp->findKeyword("SpacecraftPosition"); sposKeyword.clear();
//       sposKeyword.addValue(toString(spB_coords[0]), "km");
//       sposKeyword.addValue(toString(spB_coords[1]), "km");
//       sposKeyword.addValue(toString(spB_coords[2]), "km");
//       if (!m_csvOutput) sposKeyword.addComment("Spacecraft Information");


//       double spacecraftAzi_val_succ = m_camera->SpacecraftAzimuth(); // Renamed
//       PvlKeyword &scAziKeyword = gp->findKeyword("SpacecraftAzimuth"); scAziKeyword.clear();
//       if (Isis::IsValidPixel(spacecraftAzi_val_succ)) {
//         scAziKeyword.setValue(toString(spacecraftAzi_val_succ), "DEGREE");
//       } else {
//         scAziKeyword.addValue("NULL");
//       }

//       keyPtr = &gp->findKeyword("SlantDistance"); keyPtr->clear();
//       keyPtr->setValue(toString(m_camera->SlantDistance()), "km");
//       keyPtr = &gp->findKeyword("TargetCenterDistance"); keyPtr->clear();
//       keyPtr->setValue(toString(m_camera->targetCenterDistance()), "km");

//       m_camera->subSpacecraftPoint(ssplat_val, ssplon_val);
//       keyPtr = &gp->findKeyword("SubSpacecraftLatitude"); keyPtr->clear();
//       keyPtr->setValue(toString(ssplat_val), "DEGREE");
//       keyPtr = &gp->findKeyword("SubSpacecraftLongitude"); keyPtr->clear();
//       keyPtr->setValue(toString(ssplon_val), "DEGREE");
//       keyPtr = &gp->findKeyword("SpacecraftAltitude"); keyPtr->clear();
//       keyPtr->setValue(toString(m_camera->SpacecraftAltitude()), "km");
//       keyPtr = &gp->findKeyword("OffNadirAngle"); keyPtr->clear();
//       keyPtr->setValue(toString(m_camera->OffNadirAngle()), "DEGREE");

//       double subspcgrdaz_val_succ = m_camera->GroundAzimuth(m_camera->UniversalLatitude(), // Renamed
//                                               m_camera->UniversalLongitude(),
//                                               ssplat_val, ssplon_val);
//       keyPtr = &gp->findKeyword("SubSpacecraftGroundAzimuth"); keyPtr->clear();
//       keyPtr->setValue(toString(subspcgrdaz_val_succ), "DEGREE");

//       try {
//         m_camera->sunPosition(sB_sun_coords);
//         PvlKeyword &sunPosKeyword = gp->findKeyword("SunPosition"); sunPosKeyword.clear();
//         sunPosKeyword.addValue(toString(sB_sun_coords[0]), "km");
//         sunPosKeyword.addValue(toString(sB_sun_coords[1]));
//         sunPosKeyword.addValue(toString(sB_sun_coords[2]));
//         if (!m_csvOutput) sunPosKeyword.addComment("Sun Information");
//       }
//       catch (IException &) {
//         PvlKeyword &sunPosKeyword = gp->findKeyword("SunPosition"); sunPosKeyword.clear();
//         sunPosKeyword.addValue("Null"); sunPosKeyword.addValue("Null"); sunPosKeyword.addValue("Null");
//         if (!m_csvOutput) sunPosKeyword.addComment("Sun Information");
//       }

//       try {
//         double sunAzi_val_succ = m_camera->SunAzimuth(); // Renamed
//         PvlKeyword &subSolarAziKeyword = gp->findKeyword("SubSolarAzimuth"); subSolarAziKeyword.clear();
//         if (Isis::IsValidPixel(sunAzi_val_succ)) {
//           subSolarAziKeyword.setValue(toString(sunAzi_val_succ), "DEGREE");
//         } else { subSolarAziKeyword.addValue("NULL"); }
//       } catch(IException &) { PvlKeyword &key = gp->findKeyword("SubSolarAzimuth"); key.clear(); key.addValue("NULL");}

//       try {
//         keyPtr = &gp->findKeyword("SolarDistance"); keyPtr->clear();
//         keyPtr->setValue(toString(m_camera->SolarDistance()), "AU");
//       } catch(IException &) { PvlKeyword &key = gp->findKeyword("SolarDistance"); key.clear(); key.addValue("NULL");}

//       try {
//         double sslat_sun_val_succ, sslon_sun_val_succ; // Renamed
//         m_camera->subSolarPoint(sslat_sun_val_succ, sslon_sun_val_succ);
//         keyPtr = &gp->findKeyword("SubSolarLatitude"); keyPtr->clear();
//         keyPtr->setValue(toString(sslat_sun_val_succ), "DEGREE");
//         keyPtr = &gp->findKeyword("SubSolarLongitude"); keyPtr->clear();
//         keyPtr->setValue(toString(sslon_sun_val_succ), "DEGREE");
//         try {
//           double subsolgrdaz_val_succ = m_camera->GroundAzimuth(m_camera->UniversalLatitude(), // Renamed
//                                                          m_camera->UniversalLongitude(),
//                                                          sslat_sun_val_succ, sslon_sun_val_succ);
//           keyPtr = &gp->findKeyword("SubSolarGroundAzimuth"); keyPtr->clear();
//           keyPtr->setValue(toString(subsolgrdaz_val_succ), "DEGREE");
//         } catch(IException &) { PvlKeyword &key = gp->findKeyword("SubSolarGroundAzimuth"); key.clear(); key.addValue("NULL");}
//       }
//       catch(IException &) {
//       PvlKeyword &key1 = gp->findKeyword("SubSolarLatitude"); key1.clear(); key1.addValue("NULL");
//       PvlKeyword &key2 = gp->findKeyword("SubSolarLongitude"); key2.clear(); key2.addValue("NULL");
//       PvlKeyword &key3 = gp->findKeyword("SubSolarGroundAzimuth"); key3.clear(); key3.addValue("NULL");
//       }

//       keyPtr = &gp->findKeyword("Phase"); keyPtr->clear();
//       keyPtr->setValue(toString(m_camera->PhaseAngle()), "DEGREE");
//       if (!m_csvOutput) keyPtr->addComment("Illumination and Other");

//       keyPtr = &gp->findKeyword("Incidence"); keyPtr->clear();
//       keyPtr->setValue(toString(m_camera->IncidenceAngle()), "DEGREE");
//       keyPtr = &gp->findKeyword("Emission"); keyPtr->clear();
//       keyPtr->setValue(toString(m_camera->EmissionAngle()), "DEGREE");

//       double northAzi_val_succ = m_camera->NorthAzimuth(); // Renamed
//       PvlKeyword &nAziKeyword = gp->findKeyword("NorthAzimuth"); nAziKeyword.clear();
//       if (Isis::IsValidPixel(northAzi_val_succ)) {
//         nAziKeyword.setValue(toString(northAzi_val_succ), "DEGREE");
//       } else { nAziKeyword.addValue("NULL"); }

//       PvlKeyword &etKeyword_s = gp->findKeyword("EphemerisTime"); etKeyword_s.clear(); // Renamed
//       etKeyword_s.setValue(toString(m_camera->time().Et()), "seconds");
//       if (!m_csvOutput) etKeyword_s.addComment("Time");

//       utc_val_succ = m_camera->time().UTC();
//       keyPtr = &gp->findKeyword("UTC"); keyPtr->clear();
//       keyPtr->setValue(utc_val_succ);

//       try {
//         keyPtr = &gp->findKeyword("LocalSolarTime"); keyPtr->clear();
//         keyPtr->setValue(toString(m_camera->LocalSolarTime()), "hour");
//       } catch (IException &) { PvlKeyword &key = gp->findKeyword("LocalSolarTime"); key.clear(); key.addValue("Null");}
//       try {
//         keyPtr = &gp->findKeyword("SolarLongitude"); keyPtr->clear();
//         keyPtr->setValue(toString(m_camera->solarLongitude().degrees()), "DEGREE");
//       } catch (IException &) { PvlKeyword &key = gp->findKeyword("SolarLongitude"); key.clear(); key.addValue("Null");}

//       // === C2W_RotationMatrix 계산 및 값 설정 (성공 시) ===
//       if (gp->hasKeyword("C2M_RotationMatrix")) {
//           try {
//               // Get the rotation matrices from Camera's inherited Spice methods
//               // instrumentRotation(): Camera to J2000
//               // bodyRotation(): J2000 to Body Fixed (MOON_ME)
//               std::vector<double> instrumentMatrix = m_camera->instrumentRotation()->Matrix();
//               std::vector<double> bodyMatrix = m_camera->bodyRotation()->Matrix();
              
//               // Calculate the camera-to-body rotation matrix
//               // C2W = bodyRotation * instrumentRotation^T
//               // First, transpose the instrument matrix to get J2000-to-Camera
//               std::vector<double> instrumentTranspose(9);
//               for (int i = 0; i < 3; i++) {
//                   for (int j = 0; j < 3; j++) {
//                       instrumentTranspose[i*3 + j] = instrumentMatrix[j*3 + i];
//                   }
//               }
              
//               // Then multiply: C2W = bodyMatrix * instrumentTranspose
//               std::vector<double> matrixElements(9);
//               for (int i = 0; i < 3; i++) {
//                   for (int j = 0; j < 3; j++) {
//                       matrixElements[i*3 + j] = 0.0;
//                       for (int k = 0; k < 3; k++) {
//                           matrixElements[i*3 + j] += bodyMatrix[i*3 + k] * instrumentTranspose[k*3 + j];
//                       }
//                   }
//               }

//               PvlKeyword &c2wMatrixKeyword = gp->findKeyword("C2M_RotationMatrix");
//               c2wMatrixKeyword.clear();
//               if (matrixElements.size() == 9) {
//                   for (size_t i = 0; i < matrixElements.size(); ++i) {
//                       c2wMatrixKeyword.addValue(Isis::toString(matrixElements[i]));
//                   }
//               } else {
//                   for (int i = 0; i < 9; ++i) c2wMatrixKeyword.addValue("Error_Matrix_Size");
//               }
//           }
//           catch (Isis::IException &e) {
//               PvlKeyword &c2wMatrixKeyword = gp->findKeyword("C2M_RotationMatrix");
//               c2wMatrixKeyword.clear();
//               // 단일 값으로 에러 메시지 설정 또는 9개의 NULL 값
//               c2wMatrixKeyword.addValue("Error_Calculating_Matrix");
//               // for (int i = 0; i < 9; ++i) { c2wMatrixKeyword.addValue("NULL"); } // 또는 이렇게
//           }
//       } // end if hasKeyword C2M_RotationMatrix

//       // === 기존 LookDirection 벡터들 값 설정 (주석 없이) ===
//       // 단위("DEGREE")는 원본 코드에 있었으나, campt.cpp에서 setUnits("")로 제거됩니다.
//       // 사용자 요청에 따라 "LookDirectionBodyFixed"에 대한 그룹 주석은 이전에 제거되었습니다.
//       std::vector<double>lookB_s = m_camera->lookDirectionBodyFixed(); // Renamed
//       PvlKeyword &ldbfKeyword_s = gp->findKeyword("LookDirectionBodyFixed");
//       ldbfKeyword_s.clear();
//       ldbfKeyword_s.addValue(toString(lookB_s[0]), "DEGREE");
//       ldbfKeyword_s.addValue(toString(lookB_s[1]), "DEGREE");
//       ldbfKeyword_s.addValue(toString(lookB_s[2]), "DEGREE");

//       try {
//         std::vector<double>lookJ_s = m_camera->lookDirectionJ2000(); // Renamed
//         PvlKeyword &ldjKeyword_s = gp->findKeyword("LookDirectionJ2000");
//         ldjKeyword_s.clear();
//         ldjKeyword_s.addValue(toString(lookJ_s[0]), "DEGREE");
//         ldjKeyword_s.addValue(toString(lookJ_s[1]), "DEGREE");
//         ldjKeyword_s.addValue(toString(lookJ_s[2]), "DEGREE");
//       }
//       catch (IException &) {
//         PvlKeyword &ldjKeyword_s = gp->findKeyword("LookDirectionJ2000");
//         ldjKeyword_s.clear();
//         ldjKeyword_s.addValue("Null"); ldjKeyword_s.addValue("Null"); ldjKeyword_s.addValue("Null");
//       }

//       try {
//         double lookC_s[3]; // Renamed
//         m_camera->LookDirection(lookC_s);
//         PvlKeyword &ldcKeyword_s = gp->findKeyword("LookDirectionCamera");
//         ldcKeyword_s.clear();
//         ldcKeyword_s.addValue(toString(lookC_s[0]), "DEGREE");
//         ldcKeyword_s.addValue(toString(lookC_s[1]), "DEGREE");
//         ldcKeyword_s.addValue(toString(lookC_s[2]), "DEGREE");
//       }
//       catch (IException &) {
//         PvlKeyword &ldcKeyword_s = gp->findKeyword("LookDirectionCamera");
//         ldcKeyword_s.clear();
//         ldcKeyword_s.addValue("Null"); ldcKeyword_s.addValue("Null"); ldcKeyword_s.addValue("Null");
//       }

//       if (gp->hasKeyword("Error") && allowErrors) { // Error 키워드가 있는 경우에만 접근
//           gp->findKeyword("Error").setValue("NULL"); // 성공했으므로 Error는 NULL
//       }
//     } // end else (noErrors == true)
//     return gp;
//   }


// Camera *CameraPointInfo::camera() {
//     return m_camera;
//   }

// Cube *CameraPointInfo::cube() {
//     return m_currentCube;
//   }
// } // end namespace Isis

#include "CameraPointInfo.h"

#include <QDebug>
#include <iomanip>

#include "Brick.h"
#include "Camera.h"
#include "CameraFocalPlaneMap.h"
#include "Cube.h"
#include "CubeManager.h"
#include "Distance.h"
#include "IException.h"
#include "iTime.h"
#include "Longitude.h"
#include "PvlGroup.h"
#include "SpecialPixel.h"
#include "TProjection.h"
#include "SpiceRotation.h"
#include "UserInterface.h"

using namespace Isis;
using namespace std;

namespace Isis {

  CameraPointInfo::CameraPointInfo() {
    m_usedCubes = NULL;
    m_usedCubes = new CubeManager();
    m_usedCubes->SetNumOpenCubes(50);
    m_currentCube = NULL;
    m_camera = NULL;
    m_csvOutput = false;
  }

  void CameraPointInfo::SetCSVOutput(bool csvOutput) {
     m_csvOutput = csvOutput;
  }

  CameraPointInfo::~CameraPointInfo() {
    if (m_usedCubes) {
      delete m_usedCubes;
      m_usedCubes = NULL;
    }
  }

  void CameraPointInfo::SetCube(const QString &cubeFileName) {
    m_currentCube = m_usedCubes->OpenCube(cubeFileName);
    m_camera = m_currentCube->camera();
  }

  // ============= 기존 메서드들 (UI 파라미터 없는 버전 - ringspt 등과 호환) =============
  PvlGroup *CameraPointInfo::SetImage(const double sample, const double line,
                                      const bool allowOutside, const bool allowErrors) {
    if (CheckCube()) {
      bool passed = m_camera->SetImage(sample, line);
      return GetPointInfo(passed, allowOutside, allowErrors);
    }
    return NULL;
  }

  PvlGroup *CameraPointInfo::SetCenter(const bool allowOutside, const bool allowErrors) {
    if (CheckCube()) {
      bool passed = m_camera->SetImage(m_currentCube->sampleCount() / 2.0,
                                       m_currentCube->lineCount() / 2.0);
      return GetPointInfo(passed, allowOutside, allowErrors);
    }
    return NULL;
  }

  PvlGroup *CameraPointInfo::SetSample(const double sample,
                                       const bool allowOutside, const bool allowErrors) {
    if (CheckCube()) {
      bool passed = m_camera->SetImage(sample, m_currentCube->lineCount() / 2.0);
      return GetPointInfo(passed, allowOutside, allowErrors);
    }
    return NULL;
  }

  PvlGroup *CameraPointInfo::SetLine(const double line,
                                     const bool allowOutside, const bool allowErrors) {
    if (CheckCube()) {
      bool passed = m_camera->SetImage(m_currentCube->sampleCount() / 2.0, line);
      return GetPointInfo(passed, allowOutside, allowErrors);
    }
    return NULL;
  }

  PvlGroup *CameraPointInfo::SetGround(const double latitude, const double longitude,
                                       const bool allowOutside, const bool allowErrors) {
    if (CheckCube()) {
      bool passed = m_camera->SetUniversalGround(latitude, longitude);
      return GetPointInfo(passed, allowOutside, allowErrors);
    }
    return NULL;
  }

  // ============= 새로운 메서드들 (UI 파라미터 포함 버전 - campt용) =============
  PvlGroup *CameraPointInfo::SetImage(const UserInterface &ui, const double sample, const double line,
                                      const bool allowOutside, const bool allowErrors) {
    if (CheckCube()) {
      bool passed = m_camera->SetImage(sample, line);
      return GetPointInfoWithOptions(ui, passed, allowOutside, allowErrors);
    }
    return NULL;
  }

  PvlGroup *CameraPointInfo::SetCenter(const UserInterface &ui, const bool allowOutside, const bool allowErrors) {
    if (CheckCube()) {
      bool passed = m_camera->SetImage(m_currentCube->sampleCount() / 2.0,
                                       m_currentCube->lineCount() / 2.0);
      return GetPointInfoWithOptions(ui, passed, allowOutside, allowErrors);
    }
    return NULL;
  }

  PvlGroup *CameraPointInfo::SetSample(const UserInterface &ui, const double sample,
                                       const bool allowOutside, const bool allowErrors) {
    if (CheckCube()) {
      bool passed = m_camera->SetImage(sample, m_currentCube->lineCount() / 2.0);
      return GetPointInfoWithOptions(ui, passed, allowOutside, allowErrors);
    }
    return NULL;
  }

  PvlGroup *CameraPointInfo::SetLine(const UserInterface &ui, const double line,
                                     const bool allowOutside, const bool allowErrors) {
    if (CheckCube()) {
      bool passed = m_camera->SetImage(m_currentCube->sampleCount() / 2.0, line);
      return GetPointInfoWithOptions(ui, passed, allowOutside, allowErrors);
    }
    return NULL;
  }

  PvlGroup *CameraPointInfo::SetGround(const UserInterface &ui, const double latitude, const double longitude,
                                       const bool allowOutside, const bool allowErrors) {
    if (CheckCube()) {
      bool passed = m_camera->SetUniversalGround(latitude, longitude);
      return GetPointInfoWithOptions(ui, passed, allowOutside, allowErrors);
    }
    return NULL;
  }

  bool CameraPointInfo::CheckCube() {
    if (m_currentCube == NULL) {
      string msg = "Please set a cube before setting parameters";
      throw IException(IException::Programmer, msg, _FILEINFO_);
      return false;
    }
    return true;
  }

  // GetPointInfo - 기존 메서드용 (모든 키워드 포함)
  PvlGroup *CameraPointInfo::GetPointInfo(bool passed, bool allowOutside, bool allowErrors) {
    PvlGroup *gp = new PvlGroup("GroundPoint");

    // 모든 키워드 추가 (기존 방식대로)
    gp->addKeyword(PvlKeyword("Filename"));
    gp->addKeyword(PvlKeyword("Sample"));
    gp->addKeyword(PvlKeyword("Line"));
    gp->addKeyword(PvlKeyword("PixelValue"));
    gp->addKeyword(PvlKeyword("RightAscension"));
    gp->addKeyword(PvlKeyword("Declination"));
    gp->addKeyword(PvlKeyword("PlanetocentricLatitude"));
    gp->addKeyword(PvlKeyword("PlanetographicLatitude"));
    gp->addKeyword(PvlKeyword("PositiveEast360Longitude"));
    gp->addKeyword(PvlKeyword("PositiveEast180Longitude"));
    gp->addKeyword(PvlKeyword("PositiveWest360Longitude"));
    gp->addKeyword(PvlKeyword("PositiveWest180Longitude"));
    gp->addKeyword(PvlKeyword("BodyFixedCoordinate"));
    gp->addKeyword(PvlKeyword("LocalRadius"));
    gp->addKeyword(PvlKeyword("SampleResolution"));
    gp->addKeyword(PvlKeyword("LineResolution"));
    gp->addKeyword(PvlKeyword("ObliqueDetectorResolution"));
    gp->addKeyword(PvlKeyword("ObliquePixelResolution"));
    gp->addKeyword(PvlKeyword("ObliqueLineResolution"));
    gp->addKeyword(PvlKeyword("ObliqueSampleResolution"));
    gp->addKeyword(PvlKeyword("SpacecraftPosition"));
    gp->addKeyword(PvlKeyword("SpacecraftAzimuth"));
    gp->addKeyword(PvlKeyword("SlantDistance"));
    gp->addKeyword(PvlKeyword("TargetCenterDistance"));
    gp->addKeyword(PvlKeyword("SubSpacecraftLatitude"));
    gp->addKeyword(PvlKeyword("SubSpacecraftLongitude"));
    gp->addKeyword(PvlKeyword("SpacecraftAltitude"));
    gp->addKeyword(PvlKeyword("OffNadirAngle"));
    gp->addKeyword(PvlKeyword("SubSpacecraftGroundAzimuth"));
    gp->addKeyword(PvlKeyword("SunPosition"));
    gp->addKeyword(PvlKeyword("SubSolarAzimuth"));
    gp->addKeyword(PvlKeyword("SolarDistance"));
    gp->addKeyword(PvlKeyword("SubSolarLatitude"));
    gp->addKeyword(PvlKeyword("SubSolarLongitude"));
    gp->addKeyword(PvlKeyword("SubSolarGroundAzimuth"));
    gp->addKeyword(PvlKeyword("Phase"));
    gp->addKeyword(PvlKeyword("Incidence"));
    gp->addKeyword(PvlKeyword("Emission"));
    gp->addKeyword(PvlKeyword("NorthAzimuth"));
    gp->addKeyword(PvlKeyword("EphemerisTime"));
    gp->addKeyword(PvlKeyword("UTC"));
    gp->addKeyword(PvlKeyword("LocalSolarTime"));
    gp->addKeyword(PvlKeyword("SolarLongitude"));
    gp->addKeyword(PvlKeyword("LookDirectionBodyFixed"));
    gp->addKeyword(PvlKeyword("LookDirectionJ2000"));
    gp->addKeyword(PvlKeyword("LookDirectionCamera"));
    gp->addKeyword(PvlKeyword("C2M_RotationMatrix"));

    bool noErrors = passed;
    QString error = "";
    if (!m_camera->HasSurfaceIntersection()) {
      error = "Requested position does not project in camera model; no surface intersection";
      noErrors = false;
      if (!allowErrors) throw IException(IException::Unknown, error, _FILEINFO_);
    }
    if (!m_camera->InCube() && !allowOutside) {
      error = "Requested position does not project in camera model; not inside cube";
      noErrors = false;
      if (!allowErrors) throw IException(IException::Unknown, error, _FILEINFO_);
    }

    if (!noErrors) {
      // 에러 발생 시 모든 키워드를 NULL로 설정
      for (int i = 0; i < gp->keywords(); i++) {
        PvlKeyword &currentKeyword = (*gp)[i];
        QString name = currentKeyword.name();
        currentKeyword.clear();

        if (name == "BodyFixedCoordinate" || name == "SpacecraftPosition" || name == "SunPosition" ||
            name == "LookDirectionBodyFixed" || name == "LookDirectionJ2000" || name == "LookDirectionCamera") {
          currentKeyword.addValue("NULL");
          currentKeyword.addValue("NULL");
          currentKeyword.addValue("NULL");
        }
        else if (name == "C2M_RotationMatrix") {
          for (int k=0; k<9; ++k) currentKeyword.addValue("NULL");
        }
        else {
          currentKeyword.addValue("NULL");
        }
      }

      // 기본 정보는 설정
      gp->findKeyword("Filename").setValue(m_currentCube->fileName());
      gp->findKeyword("Sample").setValue(toString(m_camera->Sample()));
      gp->findKeyword("Line").setValue(toString(m_camera->Line()));
      
      PvlKeyword &etKeyword = gp->findKeyword("EphemerisTime");
      etKeyword.clear();
      etKeyword.setValue(toString(m_camera->time().Et()), "seconds");
      
      gp->findKeyword("UTC").setValue(m_camera->time().UTC());
    }
    else {
      // 성공 시 - 모든 키워드 값 설정
      Brick b(3, 3, 1, m_currentCube->pixelType());
      int intSamp = (int)(m_camera->Sample() + 0.5);
      int intLine = (int)(m_camera->Line() + 0.5);
      b.SetBasePosition(intSamp, intLine, 1);
      m_currentCube->read(b);

      // 기본 정보
      gp->findKeyword("Filename").setValue(m_currentCube->fileName());
      gp->findKeyword("Sample").setValue(toString(m_camera->Sample()));
      gp->findKeyword("Line").setValue(toString(m_camera->Line()));
      gp->findKeyword("PixelValue").setValue(PixelToString(b[0]));
      
      // RightAscension & Declination
      try {
        gp->findKeyword("RightAscension").setValue(toString(m_camera->RightAscension()), "DEGREE");
      } catch (IException &) {
        gp->findKeyword("RightAscension").setValue("Null");
      }
      
      try {
        gp->findKeyword("Declination").setValue(toString(m_camera->Declination()), "DEGREE");
      } catch (IException &) {
        gp->findKeyword("Declination").setValue("Null");
      }
      
      // 위도/경도 정보
      double ocentricLat = m_camera->UniversalLatitude();
      gp->findKeyword("PlanetocentricLatitude").setValue(toString(ocentricLat), "DEGREE");
      
      Distance radii[3];
      m_camera->radii(radii);
      double ographicLat = TProjection::ToPlanetographic(ocentricLat,
                                                        radii[0].kilometers(),
                                                        radii[2].kilometers());
      gp->findKeyword("PlanetographicLatitude").setValue(toString(ographicLat), "DEGREE");
      
      double pe360Lon = m_camera->UniversalLongitude();
      gp->findKeyword("PositiveEast360Longitude").setValue(toString(pe360Lon), "DEGREE");
      gp->findKeyword("PositiveEast180Longitude").setValue(toString(TProjection::To180Domain(pe360Lon)), "DEGREE");
      
      double pw360Lon = TProjection::ToPositiveWest(pe360Lon, 360);
      gp->findKeyword("PositiveWest360Longitude").setValue(toString(pw360Lon), "DEGREE");
      gp->findKeyword("PositiveWest180Longitude").setValue(toString(TProjection::To180Domain(pw360Lon)), "DEGREE");
      
      // Body Fixed Coordinate
      double pB[3];
      m_camera->Coordinate(pB);
      PvlKeyword &bfcKeyword = gp->findKeyword("BodyFixedCoordinate");
      bfcKeyword.clear();
      bfcKeyword.addValue(toString(pB[0]), "km");
      bfcKeyword.addValue(toString(pB[1]), "km");
      bfcKeyword.addValue(toString(pB[2]), "km");
      
      gp->findKeyword("LocalRadius").setValue(toString(m_camera->LocalRadius().meters()), "meters");
      gp->findKeyword("SampleResolution").setValue(toString(m_camera->SampleResolution()), "meters/pixel");
      gp->findKeyword("LineResolution").setValue(toString(m_camera->LineResolution()), "meters/pixel");
      
      // Oblique resolutions
      gp->findKeyword("ObliqueDetectorResolution").setValue(toString(m_camera->ObliqueDetectorResolution()), "meters");
      gp->findKeyword("ObliquePixelResolution").setValue(toString(m_camera->ObliquePixelResolution()), "meters/pix");
      gp->findKeyword("ObliqueLineResolution").setValue(toString(m_camera->ObliqueLineResolution()), "meters");
      gp->findKeyword("ObliqueSampleResolution").setValue(toString(m_camera->ObliqueSampleResolution()), "meters");
      
      // Spacecraft info
      double spB[3];
      m_camera->instrumentPosition(spB);
      PvlKeyword &spKeyword = gp->findKeyword("SpacecraftPosition");
      spKeyword.clear();
      spKeyword.addValue(toString(spB[0]), "km");
      spKeyword.addValue(toString(spB[1]), "km");
      spKeyword.addValue(toString(spB[2]), "km");
      
      double spacecraftAzi = m_camera->SpacecraftAzimuth();
      if (IsValidPixel(spacecraftAzi)) {
        gp->findKeyword("SpacecraftAzimuth").setValue(toString(spacecraftAzi), "DEGREE");
      } else {
        gp->findKeyword("SpacecraftAzimuth").setValue("NULL");
      }
      
      gp->findKeyword("SlantDistance").setValue(toString(m_camera->SlantDistance()), "km");
      gp->findKeyword("TargetCenterDistance").setValue(toString(m_camera->targetCenterDistance()), "km");
      
      double ssplat, ssplon;
      m_camera->subSpacecraftPoint(ssplat, ssplon);
      gp->findKeyword("SubSpacecraftLatitude").setValue(toString(ssplat), "DEGREE");
      gp->findKeyword("SubSpacecraftLongitude").setValue(toString(ssplon), "DEGREE");
      gp->findKeyword("SpacecraftAltitude").setValue(toString(m_camera->SpacecraftAltitude()), "km");
      gp->findKeyword("OffNadirAngle").setValue(toString(m_camera->OffNadirAngle()), "DEGREE");
      
      double subspcgrdaz = m_camera->GroundAzimuth(m_camera->UniversalLatitude(),
                                                   m_camera->UniversalLongitude(),
                                                   ssplat, ssplon);
      gp->findKeyword("SubSpacecraftGroundAzimuth").setValue(toString(subspcgrdaz), "DEGREE");
      
      // Sun info
      try {
        double sB[3];
        m_camera->sunPosition(sB);
        PvlKeyword &sunKeyword = gp->findKeyword("SunPosition");
        sunKeyword.clear();
        sunKeyword.addValue(toString(sB[0]), "km");
        sunKeyword.addValue(toString(sB[1]), "km");
        sunKeyword.addValue(toString(sB[2]), "km");
      } catch (IException &) {
        PvlKeyword &sunKeyword = gp->findKeyword("SunPosition");
        sunKeyword.clear();
        sunKeyword.addValue("Null");
        sunKeyword.addValue("Null");
        sunKeyword.addValue("Null");
      }
      
      try {
        double sunAzi = m_camera->SunAzimuth();
        if (IsValidPixel(sunAzi)) {
          gp->findKeyword("SubSolarAzimuth").setValue(toString(sunAzi), "DEGREE");
        } else {
          gp->findKeyword("SubSolarAzimuth").setValue("NULL");
        }
      } catch(IException &) {
        gp->findKeyword("SubSolarAzimuth").setValue("NULL");
      }
      
      try {
        gp->findKeyword("SolarDistance").setValue(toString(m_camera->SolarDistance()), "AU");
      } catch(IException &) {
        gp->findKeyword("SolarDistance").setValue("NULL");
      }
      
      try {
        double sslat, sslon;
        m_camera->subSolarPoint(sslat, sslon);
        gp->findKeyword("SubSolarLatitude").setValue(toString(sslat), "DEGREE");
        gp->findKeyword("SubSolarLongitude").setValue(toString(sslon), "DEGREE");
        
        try {
          double subsolgrdaz = m_camera->GroundAzimuth(m_camera->UniversalLatitude(),
                                                       m_camera->UniversalLongitude(),
                                                       sslat, sslon);
          gp->findKeyword("SubSolarGroundAzimuth").setValue(toString(subsolgrdaz), "DEGREE");
        } catch(IException &) {
          gp->findKeyword("SubSolarGroundAzimuth").setValue("NULL");
        }
      } catch(IException &) {
        gp->findKeyword("SubSolarLatitude").setValue("NULL");
        gp->findKeyword("SubSolarLongitude").setValue("NULL");
        gp->findKeyword("SubSolarGroundAzimuth").setValue("NULL");
      }
      
      // Phase angles
      gp->findKeyword("Phase").setValue(toString(m_camera->PhaseAngle()), "DEGREE");
      gp->findKeyword("Incidence").setValue(toString(m_camera->IncidenceAngle()), "DEGREE");
      gp->findKeyword("Emission").setValue(toString(m_camera->EmissionAngle()), "DEGREE");
      
      double northAzi = m_camera->NorthAzimuth();
      if (IsValidPixel(northAzi)) {
        gp->findKeyword("NorthAzimuth").setValue(toString(northAzi), "DEGREE");
      } else {
        gp->findKeyword("NorthAzimuth").setValue("NULL");
      }
      
      // Time info
      gp->findKeyword("EphemerisTime").setValue(toString(m_camera->time().Et()), "seconds");
      gp->findKeyword("UTC").setValue(m_camera->time().UTC());
      
      try {
        gp->findKeyword("LocalSolarTime").setValue(toString(m_camera->LocalSolarTime()), "hour");
      } catch (IException &) {
        gp->findKeyword("LocalSolarTime").setValue("Null");
      }
      
      try {
        gp->findKeyword("SolarLongitude").setValue(toString(m_camera->solarLongitude().degrees()), "DEGREE");
      } catch (IException &) {
        gp->findKeyword("SolarLongitude").setValue("Null");
      }
      
      // Look directions
      std::vector<double> lookB = m_camera->lookDirectionBodyFixed();
      PvlKeyword &ldbfKeyword = gp->findKeyword("LookDirectionBodyFixed");
      ldbfKeyword.clear();
      ldbfKeyword.addValue(toString(lookB[0]), "DEGREE");
      ldbfKeyword.addValue(toString(lookB[1]), "DEGREE");
      ldbfKeyword.addValue(toString(lookB[2]), "DEGREE");
      
      try {
        std::vector<double> lookJ = m_camera->lookDirectionJ2000();
        PvlKeyword &ldjKeyword = gp->findKeyword("LookDirectionJ2000");
        ldjKeyword.clear();
        ldjKeyword.addValue(toString(lookJ[0]), "DEGREE");
        ldjKeyword.addValue(toString(lookJ[1]), "DEGREE");
        ldjKeyword.addValue(toString(lookJ[2]), "DEGREE");
      } catch (IException &) {
        PvlKeyword &ldjKeyword = gp->findKeyword("LookDirectionJ2000");
        ldjKeyword.clear();
        ldjKeyword.addValue("Null");
        ldjKeyword.addValue("Null");
        ldjKeyword.addValue("Null");
      }
      
      try {
        double lookC[3];
        m_camera->LookDirection(lookC);
        PvlKeyword &ldcKeyword = gp->findKeyword("LookDirectionCamera");
        ldcKeyword.clear();
        ldcKeyword.addValue(toString(lookC[0]), "DEGREE");
        ldcKeyword.addValue(toString(lookC[1]), "DEGREE");
        ldcKeyword.addValue(toString(lookC[2]), "DEGREE");
      } catch (IException &) {
        PvlKeyword &ldcKeyword = gp->findKeyword("LookDirectionCamera");
        ldcKeyword.clear();
        ldcKeyword.addValue("Null");
        ldcKeyword.addValue("Null");
        ldcKeyword.addValue("Null");
      }
      
      // C2M_RotationMatrix
      try {
        std::vector<double> instrumentMatrix = m_camera->instrumentRotation()->Matrix();
        std::vector<double> bodyMatrix = m_camera->bodyRotation()->Matrix();
        
        std::vector<double> instrumentTranspose(9);
        for (int i = 0; i < 3; i++) {
          for (int j = 0; j < 3; j++) {
            instrumentTranspose[i*3 + j] = instrumentMatrix[j*3 + i];
          }
        }
        
        std::vector<double> matrixElements(9);
        for (int i = 0; i < 3; i++) {
          for (int j = 0; j < 3; j++) {
            matrixElements[i*3 + j] = 0.0;
            for (int k = 0; k < 3; k++) {
              matrixElements[i*3 + j] += bodyMatrix[i*3 + k] * instrumentTranspose[k*3 + j];
            }
          }
        }
        
        PvlKeyword &c2mKeyword = gp->findKeyword("C2M_RotationMatrix");
        c2mKeyword.clear();
        for (size_t i = 0; i < 9; ++i) {
          c2mKeyword.addValue(toString(matrixElements[i]));
        }
      } catch (IException &) {
        PvlKeyword &c2mKeyword = gp->findKeyword("C2M_RotationMatrix");
        c2mKeyword.clear();
        for (int i = 0; i < 9; ++i) {
          c2mKeyword.addValue("NULL");
        }
      }
    }
    
    return gp;
  }

  // GetPointInfoWithOptions - UI 버전용 (필요한 키워드만 선택적으로)
  PvlGroup *CameraPointInfo::GetPointInfoWithOptions(const UserInterface &ui, bool passed, 
                                                     bool allowOutside, bool allowErrors) {
    // 여기는 이미 작성된 코드 그대로 사용
    PvlGroup *gp = new PvlGroup("GroundPoint");

    // 필수 키워드들 (항상 추가)
    gp->addKeyword(PvlKeyword("Filename"));
    gp->addKeyword(PvlKeyword("Sample"));
    gp->addKeyword(PvlKeyword("Line"));
    gp->addKeyword(PvlKeyword("PixelValue"));
    gp->addKeyword(PvlKeyword("PlanetocentricLatitude"));
    gp->addKeyword(PvlKeyword("PositiveEast360Longitude"));
    gp->addKeyword(PvlKeyword("PositiveEast180Longitude"));
    gp->addKeyword(PvlKeyword("PositiveWest360Longitude"));
    gp->addKeyword(PvlKeyword("PositiveWest180Longitude"));
    gp->addKeyword(PvlKeyword("BodyFixedCoordinate"));
    gp->addKeyword(PvlKeyword("SpacecraftPosition"));
    gp->addKeyword(PvlKeyword("SubSolarGroundAzimuth"));
    gp->addKeyword(PvlKeyword("Phase"));
    gp->addKeyword(PvlKeyword("Incidence"));
    gp->addKeyword(PvlKeyword("LocalRadius"));
    gp->addKeyword(PvlKeyword("EphemerisTime"));
    gp->addKeyword(PvlKeyword("LookDirectionBodyFixed"));
    gp->addKeyword(PvlKeyword("LookDirectionJ2000"));
    gp->addKeyword(PvlKeyword("LookDirectionCamera"));
    gp->addKeyword(PvlKeyword("SampleResolution"));
    gp->addKeyword(PvlKeyword("SlantDistance"));
    gp->addKeyword(PvlKeyword("C2M_RotationMatrix"));

    // 선택적 키워드들 (UI 파라미터가 true일 때만 추가)
    if (ui.WasEntered("RIGHTASCENSION") && ui.GetBoolean("RIGHTASCENSION")) 
      gp->addKeyword(PvlKeyword("RightAscension"));
    if (ui.WasEntered("DECLINATION") && ui.GetBoolean("DECLINATION")) 
      gp->addKeyword(PvlKeyword("Declination"));
    if (ui.WasEntered("PLANETOGRAPHICLATITUDE") && ui.GetBoolean("PLANETOGRAPHICLATITUDE")) 
      gp->addKeyword(PvlKeyword("PlanetographicLatitude"));
    if (ui.WasEntered("LINERESOLUTION") && ui.GetBoolean("LINERESOLUTION")) 
      gp->addKeyword(PvlKeyword("LineResolution"));
    if (ui.WasEntered("OBLIQUEDETECTORRESOLUTION") && ui.GetBoolean("OBLIQUEDETECTORRESOLUTION")) 
      gp->addKeyword(PvlKeyword("ObliqueDetectorResolution"));
    if (ui.WasEntered("OBLIQUEPIXELRESOLUTION") && ui.GetBoolean("OBLIQUEPIXELRESOLUTION")) 
      gp->addKeyword(PvlKeyword("ObliquePixelResolution"));
    if (ui.WasEntered("OBLIQUELINERESOLUTION") && ui.GetBoolean("OBLIQUELINERESOLUTION")) 
      gp->addKeyword(PvlKeyword("ObliqueLineResolution"));
    if (ui.WasEntered("OBLIQUESAMPLERESOLUTION") && ui.GetBoolean("OBLIQUESAMPLERESOLUTION")) 
      gp->addKeyword(PvlKeyword("ObliqueSampleResolution"));
    if (ui.WasEntered("SPACECRAFTAZIMUTH") && ui.GetBoolean("SPACECRAFTAZIMUTH")) 
      gp->addKeyword(PvlKeyword("SpacecraftAzimuth"));
    if (ui.WasEntered("TARGETCENTERDISTANCE") && ui.GetBoolean("TARGETCENTERDISTANCE")) 
      gp->addKeyword(PvlKeyword("TargetCenterDistance"));
    if (ui.WasEntered("SUBSPACECRAFTLATITUDE") && ui.GetBoolean("SUBSPACECRAFTLATITUDE")) 
      gp->addKeyword(PvlKeyword("SubSpacecraftLatitude"));
    if (ui.WasEntered("SUBSPACECRAFTLONGITUDE") && ui.GetBoolean("SUBSPACECRAFTLONGITUDE")) 
      gp->addKeyword(PvlKeyword("SubSpacecraftLongitude"));
    if (ui.WasEntered("SPACECRAFTALTITUDE") && ui.GetBoolean("SPACECRAFTALTITUDE")) 
      gp->addKeyword(PvlKeyword("SpacecraftAltitude"));
    if (ui.WasEntered("OFFNADIRANGLE") && ui.GetBoolean("OFFNADIRANGLE")) 
      gp->addKeyword(PvlKeyword("OffNadirAngle"));
    if (ui.WasEntered("SUBSPACECRAFTGROUNDAZIMUTH") && ui.GetBoolean("SUBSPACECRAFTGROUNDAZIMUTH")) 
      gp->addKeyword(PvlKeyword("SubSpacecraftGroundAzimuth"));
    if (ui.WasEntered("SUNPOSITION") && ui.GetBoolean("SUNPOSITION")) 
      gp->addKeyword(PvlKeyword("SunPosition"));
    if (ui.WasEntered("SUBSOLARAZIMUTH") && ui.GetBoolean("SUBSOLARAZIMUTH")) 
      gp->addKeyword(PvlKeyword("SubSolarAzimuth"));
    if (ui.WasEntered("SOLARDISTANCE") && ui.GetBoolean("SOLARDISTANCE")) 
      gp->addKeyword(PvlKeyword("SolarDistance"));
    if (ui.WasEntered("SUBSOLARLATITUDE") && ui.GetBoolean("SUBSOLARLATITUDE")) 
      gp->addKeyword(PvlKeyword("SubSolarLatitude"));
    if (ui.WasEntered("SUBSOLARLONGITUDE") && ui.GetBoolean("SUBSOLARLONGITUDE")) 
      gp->addKeyword(PvlKeyword("SubSolarLongitude"));
    if (ui.WasEntered("EMISSION") && ui.GetBoolean("EMISSION")) 
      gp->addKeyword(PvlKeyword("Emission"));
    if (ui.WasEntered("NORTHAZIMUTH") && ui.GetBoolean("NORTHAZIMUTH")) 
      gp->addKeyword(PvlKeyword("NorthAzimuth"));
    if (ui.WasEntered("UTC") && ui.GetBoolean("UTC")) 
      gp->addKeyword(PvlKeyword("UTC"));
    if (ui.WasEntered("LOCALSOLARTIME") && ui.GetBoolean("LOCALSOLARTIME")) 
      gp->addKeyword(PvlKeyword("LocalSolarTime"));
    if (ui.WasEntered("SOLARLONGITUDE") && ui.GetBoolean("SOLARLONGITUDE")) 
      gp->addKeyword(PvlKeyword("SolarLongitude"));

    // 나머지는 현재 코드와 동일 (에러 처리, 값 설정 등)
    bool noErrors = passed;
    QString error = "";
    if (!m_camera->HasSurfaceIntersection()) {
      error = "Requested position does not project in camera model; no surface intersection";
      noErrors = false;
      if (!allowErrors) throw IException(IException::Unknown, error, _FILEINFO_);
    }
    if (!m_camera->InCube() && !allowOutside) {
      error = "Requested position does not project in camera model; not inside cube";
      noErrors = false;
      if (!allowErrors) throw IException(IException::Unknown, error, _FILEINFO_);
    }

    // 나머지 구현은 현재 코드와 동일...
    // (에러 처리 및 값 설정 부분은 이미 작성되어 있으므로 그대로 유지)
    
    if (!noErrors) {
      // 에러 처리 부분 (기존 코드 그대로)
      for (int i = 0; i < gp->keywords(); i++) {
        PvlKeyword &currentKeyword = (*gp)[i];
        QString name = currentKeyword.name();
        currentKeyword.clear();

        if (name == "BodyFixedCoordinate" || name == "SpacecraftPosition" || name == "SunPosition" ||
            name == "LookDirectionBodyFixed" || name == "LookDirectionJ2000" || name == "LookDirectionCamera") {
          currentKeyword.addValue("NULL");
          currentKeyword.addValue("NULL");
          currentKeyword.addValue("NULL");
        }
        else if (name == "C2M_RotationMatrix") {
          for (int k=0; k<9; ++k) currentKeyword.addValue("NULL");
        }
        else {
          currentKeyword.addValue("NULL");
        }
      }

      gp->findKeyword("Filename").setValue(m_currentCube->fileName());
      gp->findKeyword("Sample").setValue(toString(m_camera->Sample()));
      gp->findKeyword("Line").setValue(toString(m_camera->Line()));
      
      PvlKeyword &etKeyword = gp->findKeyword("EphemerisTime");
      etKeyword.clear();
      etKeyword.setValue(toString(m_camera->time().Et()), "seconds");
      
      if (gp->hasKeyword("UTC")) {
        gp->findKeyword("UTC").setValue(m_camera->time().UTC());
      }
    }
    else {
      // 성공 시 값 설정 (기존 코드 그대로)
      Brick b(3, 3, 1, m_currentCube->pixelType());
      int intSamp = (int)(m_camera->Sample() + 0.5);
      int intLine = (int)(m_camera->Line() + 0.5);
      b.SetBasePosition(intSamp, intLine, 1);
      m_currentCube->read(b);

      // 나머지 모든 값 설정 부분은 기존 코드 그대로...
      // (이미 작성되어 있으므로 생략)
      
      gp->findKeyword("Filename").setValue(m_currentCube->fileName());
      gp->findKeyword("Sample").setValue(toString(m_camera->Sample()));
      gp->findKeyword("Line").setValue(toString(m_camera->Line()));
      gp->findKeyword("PixelValue").setValue(PixelToString(b[0]));
      
      double ocentricLat = m_camera->UniversalLatitude();
      gp->findKeyword("PlanetocentricLatitude").setValue(toString(ocentricLat), "DEGREE");
      
      double pe360Lon = m_camera->UniversalLongitude();
      gp->findKeyword("PositiveEast360Longitude").setValue(toString(pe360Lon), "DEGREE");
      gp->findKeyword("PositiveEast180Longitude").setValue(toString(TProjection::To180Domain(pe360Lon)), "DEGREE");
      
      double pw360Lon = TProjection::ToPositiveWest(pe360Lon, 360);
      gp->findKeyword("PositiveWest360Longitude").setValue(toString(pw360Lon), "DEGREE");
      gp->findKeyword("PositiveWest180Longitude").setValue(toString(TProjection::To180Domain(pw360Lon)), "DEGREE");
      
      double pB[3];
      m_camera->Coordinate(pB);
      PvlKeyword &bfcKeyword = gp->findKeyword("BodyFixedCoordinate");
      bfcKeyword.clear();
      bfcKeyword.addValue(toString(pB[0]), "km");
      bfcKeyword.addValue(toString(pB[1]), "km");
      bfcKeyword.addValue(toString(pB[2]), "km");
      
      double spB[3];
      m_camera->instrumentPosition(spB);
      PvlKeyword &spKeyword = gp->findKeyword("SpacecraftPosition");
      spKeyword.clear();
      spKeyword.addValue(toString(spB[0]), "km");
      spKeyword.addValue(toString(spB[1]), "km");
      spKeyword.addValue(toString(spB[2]), "km");
      
      // SubSolarGroundAzimuth 계산
      try {
        double sslat, sslon;
        m_camera->subSolarPoint(sslat, sslon);
        double subsolgrdaz = m_camera->GroundAzimuth(m_camera->UniversalLatitude(),
                                                     m_camera->UniversalLongitude(),
                                                     sslat, sslon);
        gp->findKeyword("SubSolarGroundAzimuth").setValue(toString(subsolgrdaz), "DEGREE");
      } catch(IException &) {
        gp->findKeyword("SubSolarGroundAzimuth").setValue("NULL");
      }
      
      gp->findKeyword("Phase").setValue(toString(m_camera->PhaseAngle()), "DEGREE"); 
      gp->findKeyword("Incidence").setValue(toString(m_camera->IncidenceAngle()), "DEGREE");
      gp->findKeyword("LocalRadius").setValue(toString(m_camera->LocalRadius().meters()), "meters");
      gp->findKeyword("EphemerisTime").setValue(toString(m_camera->time().Et()), "seconds");
      
      // Look directions
      std::vector<double> lookB = m_camera->lookDirectionBodyFixed();
      PvlKeyword &ldbfKeyword = gp->findKeyword("LookDirectionBodyFixed");
      ldbfKeyword.clear();
      ldbfKeyword.addValue(toString(lookB[0]), "DEGREE");
      ldbfKeyword.addValue(toString(lookB[1]), "DEGREE");
      ldbfKeyword.addValue(toString(lookB[2]), "DEGREE");
      
      try {
        std::vector<double> lookJ = m_camera->lookDirectionJ2000();
        PvlKeyword &ldjKeyword = gp->findKeyword("LookDirectionJ2000");
        ldjKeyword.clear();
        ldjKeyword.addValue(toString(lookJ[0]), "DEGREE");
        ldjKeyword.addValue(toString(lookJ[1]), "DEGREE");
        ldjKeyword.addValue(toString(lookJ[2]), "DEGREE");
      } catch (IException &) {
        PvlKeyword &ldjKeyword = gp->findKeyword("LookDirectionJ2000");
        ldjKeyword.clear();
        ldjKeyword.addValue("Null");
        ldjKeyword.addValue("Null");
        ldjKeyword.addValue("Null");
      }
      
      try {
        double lookC[3];
        m_camera->LookDirection(lookC);
        PvlKeyword &ldcKeyword = gp->findKeyword("LookDirectionCamera");
        ldcKeyword.clear();
        ldcKeyword.addValue(toString(lookC[0]), "DEGREE");
        ldcKeyword.addValue(toString(lookC[1]), "DEGREE");
        ldcKeyword.addValue(toString(lookC[2]), "DEGREE");
      } catch (IException &) {
        PvlKeyword &ldcKeyword = gp->findKeyword("LookDirectionCamera");
        ldcKeyword.clear();
        ldcKeyword.addValue("Null");
        ldcKeyword.addValue("Null");
        ldcKeyword.addValue("Null");
      }
      
      gp->findKeyword("SampleResolution").setValue(toString(m_camera->SampleResolution()), "meters/pixel");
      gp->findKeyword("SlantDistance").setValue(toString(m_camera->SlantDistance()), "km");
      
      // C2M_RotationMatrix 계산
      try {
        std::vector<double> instrumentMatrix = m_camera->instrumentRotation()->Matrix();
        std::vector<double> bodyMatrix = m_camera->bodyRotation()->Matrix();
        
        std::vector<double> instrumentTranspose(9);
        for (int i = 0; i < 3; i++) {
          for (int j = 0; j < 3; j++) {
            instrumentTranspose[i*3 + j] = instrumentMatrix[j*3 + i];
          }
        }
        
        std::vector<double> matrixElements(9);
        for (int i = 0; i < 3; i++) {
          for (int j = 0; j < 3; j++) {
            matrixElements[i*3 + j] = 0.0;
            for (int k = 0; k < 3; k++) {
              matrixElements[i*3 + j] += bodyMatrix[i*3 + k] * instrumentTranspose[k*3 + j];
            }
          }
        }
        
        PvlKeyword &c2mKeyword = gp->findKeyword("C2M_RotationMatrix");
        c2mKeyword.clear();
        for (size_t i = 0; i < 9; ++i) {
          c2mKeyword.addValue(toString(matrixElements[i]));
        }
      } catch (IException &) {
        PvlKeyword &c2mKeyword = gp->findKeyword("C2M_RotationMatrix");
        c2mKeyword.clear();
        for (int i = 0; i < 9; ++i) {
          c2mKeyword.addValue("NULL");
        }
      }
      
      if (gp->hasKeyword("RightAscension")) {
        try {
          gp->findKeyword("RightAscension").setValue(toString(m_camera->RightAscension()), "DEGREE");
        } catch (IException &) {
          gp->findKeyword("RightAscension").setValue("Null");
        }
      }

      if (gp->hasKeyword("Declination")) {
        try {
          gp->findKeyword("Declination").setValue(toString(m_camera->Declination()), "DEGREE");
        } catch (IException &) {
          gp->findKeyword("Declination").setValue("Null");
        }
      }
      
      if (gp->hasKeyword("PlanetographicLatitude")) {
        Distance radii[3];
        m_camera->radii(radii);
        double ographicLat = TProjection::ToPlanetographic(ocentricLat,
                                                          radii[0].kilometers(),
                                                          radii[2].kilometers());
        gp->findKeyword("PlanetographicLatitude").setValue(toString(ographicLat), "DEGREE");
      }
      
      if (gp->hasKeyword("LineResolution")) {
        gp->findKeyword("LineResolution").setValue(toString(m_camera->LineResolution()), "meters/pixel");
      }
      
      if (gp->hasKeyword("ObliqueDetectorResolution")) {
        gp->findKeyword("ObliqueDetectorResolution").setValue(toString(m_camera->ObliqueDetectorResolution()), "meters");
      }
      
      if (gp->hasKeyword("ObliquePixelResolution")) {
        gp->findKeyword("ObliquePixelResolution").setValue(toString(m_camera->ObliquePixelResolution()), "meters/pix");
      }
      
      if (gp->hasKeyword("ObliqueLineResolution")) {
        gp->findKeyword("ObliqueLineResolution").setValue(toString(m_camera->ObliqueLineResolution()), "meters");
      }
      
      if (gp->hasKeyword("ObliqueSampleResolution")) {
        gp->findKeyword("ObliqueSampleResolution").setValue(toString(m_camera->ObliqueSampleResolution()), "meters");
      }
      
      if (gp->hasKeyword("SpacecraftAzimuth")) {
        double spacecraftAzi = m_camera->SpacecraftAzimuth();
        if (IsValidPixel(spacecraftAzi)) {
          gp->findKeyword("SpacecraftAzimuth").setValue(toString(spacecraftAzi), "DEGREE");
        } else {
          gp->findKeyword("SpacecraftAzimuth").setValue("NULL");
        }
      }
      
      if (gp->hasKeyword("TargetCenterDistance")) {
        gp->findKeyword("TargetCenterDistance").setValue(toString(m_camera->targetCenterDistance()), "km");
      }
      
      if (gp->hasKeyword("SubSpacecraftLatitude")) {
        double ssplat, ssplon;
        m_camera->subSpacecraftPoint(ssplat, ssplon);
        gp->findKeyword("SubSpacecraftLatitude").setValue(toString(ssplat), "DEGREE");
      }
      
      if (gp->hasKeyword("SubSpacecraftLongitude")) {
        double ssplat, ssplon;
        m_camera->subSpacecraftPoint(ssplat, ssplon);
        gp->findKeyword("SubSpacecraftLongitude").setValue(toString(ssplon), "DEGREE");
      }
      
      if (gp->hasKeyword("SpacecraftAltitude")) {
        gp->findKeyword("SpacecraftAltitude").setValue(toString(m_camera->SpacecraftAltitude()), "km");
      }
      
      if (gp->hasKeyword("OffNadirAngle")) {
        gp->findKeyword("OffNadirAngle").setValue(toString(m_camera->OffNadirAngle()), "DEGREE");
      }
      
      if (gp->hasKeyword("SubSpacecraftGroundAzimuth")) {
        double ssplat, ssplon;
        m_camera->subSpacecraftPoint(ssplat, ssplon);
        double subspcgrdaz = m_camera->GroundAzimuth(m_camera->UniversalLatitude(),
                                                     m_camera->UniversalLongitude(),
                                                     ssplat, ssplon);
        gp->findKeyword("SubSpacecraftGroundAzimuth").setValue(toString(subspcgrdaz), "DEGREE");
      }
      
      if (gp->hasKeyword("SunPosition")) {
        try {
          double sB[3];
          m_camera->sunPosition(sB);
          PvlKeyword &sunKeyword = gp->findKeyword("SunPosition");
          sunKeyword.clear();
          sunKeyword.addValue(toString(sB[0]), "km");
          sunKeyword.addValue(toString(sB[1]), "km");
          sunKeyword.addValue(toString(sB[2]), "km");
        } catch (IException &) {
          PvlKeyword &sunKeyword = gp->findKeyword("SunPosition");
          sunKeyword.clear();
          sunKeyword.addValue("Null");
          sunKeyword.addValue("Null");
          sunKeyword.addValue("Null");
        }
      }
      
      if (gp->hasKeyword("SubSolarAzimuth")) {
        try {
          double sunAzi = m_camera->SunAzimuth();
          if (IsValidPixel(sunAzi)) {
            gp->findKeyword("SubSolarAzimuth").setValue(toString(sunAzi), "DEGREE");
          } else {
            gp->findKeyword("SubSolarAzimuth").setValue("NULL");
          }
        } catch(IException &) {
          gp->findKeyword("SubSolarAzimuth").setValue("NULL");
        }
      }
      
      if (gp->hasKeyword("SolarDistance")) {
        try {
          gp->findKeyword("SolarDistance").setValue(toString(m_camera->SolarDistance()), "AU");
        } catch(IException &) {
          gp->findKeyword("SolarDistance").setValue("NULL");
        }
      }
      
      if (gp->hasKeyword("SubSolarLatitude")) {
        try {
          double sslat, sslon;
          m_camera->subSolarPoint(sslat, sslon);
          gp->findKeyword("SubSolarLatitude").setValue(toString(sslat), "DEGREE");
        } catch(IException &) {
          gp->findKeyword("SubSolarLatitude").setValue("NULL");
        }
      }
      
      if (gp->hasKeyword("SubSolarLongitude")) {
        try {
          double sslat, sslon;
          m_camera->subSolarPoint(sslat, sslon);
          gp->findKeyword("SubSolarLongitude").setValue(toString(sslon), "DEGREE");
        } catch(IException &) {
          gp->findKeyword("SubSolarLongitude").setValue("NULL");
        }
      }
      
      if (gp->hasKeyword("Phase")) {
        gp->findKeyword("Phase").setValue(toString(m_camera->PhaseAngle()), "DEGREE");
      }
      
      if (gp->hasKeyword("Emission")) {
        gp->findKeyword("Emission").setValue(toString(m_camera->EmissionAngle()), "DEGREE");
      }
      
      if (gp->hasKeyword("NorthAzimuth")) {
        double northAzi = m_camera->NorthAzimuth();
        if (IsValidPixel(northAzi)) {
          gp->findKeyword("NorthAzimuth").setValue(toString(northAzi), "DEGREE");
        } else {
          gp->findKeyword("NorthAzimuth").setValue("NULL");
        }
      }
      
      if (gp->hasKeyword("UTC")) {
        gp->findKeyword("UTC").setValue(m_camera->time().UTC());
      }
      
      if (gp->hasKeyword("LocalSolarTime")) {
        try {
          gp->findKeyword("LocalSolarTime").setValue(toString(m_camera->LocalSolarTime()), "hour");
        } catch (IException &) {
          gp->findKeyword("LocalSolarTime").setValue("Null");
        }
      }
      
      if (gp->hasKeyword("SolarLongitude")) {
        try {
          gp->findKeyword("SolarLongitude").setValue(toString(m_camera->solarLongitude().degrees()), "DEGREE");
        } catch (IException &) {
          gp->findKeyword("SolarLongitude").setValue("Null");
        }
      }
    }
    
    return gp;
  }

  Camera *CameraPointInfo::camera() {
    return m_camera;
  }

  Cube *CameraPointInfo::cube() {
    return m_currentCube;
  }
}
