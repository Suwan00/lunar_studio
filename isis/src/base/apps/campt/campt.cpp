// #include "campt.h"

// #include <string>
// #include <iomanip>
// #include <QSet>
// #include <QMap>
// #include <QStringList>
// #include <iostream> // For std::cout, std::endl
// #include <sstream>  // For std::stringstream

// #include "Brick.h"
// #include "Camera.h"
// #include "CameraPointInfo.h"
// #include "CSVReader.h"
// #include "Distance.h"
// #include "IException.h"
// #include "iTime.h"
// #include "Longitude.h"
// #include "Progress.h"
// #include "PvlGroup.h"
// #include "PvlKeyword.h"
// #include "SpecialPixel.h"
// #include "TProjection.h"
// #include "Application.h"

// #include "indicators.hpp" // Assuming this is correctly in your include path

// using namespace std;
// using namespace Isis;

// namespace Isis{

//   QList< QPair<double, double> > getPoints(const UserInterface &ui, bool usePointList, bool useOffset);
//   QList<PvlGroup *> getCameraPointInfo(const UserInterface &ui,
//                                       QList< QPair<double, double> > points,
//                                       CameraPointInfo &campt);
//   void writePoints(const UserInterface &ui, QList<PvlGroup*> camPoints, Pvl *log);
//   void printCategorizedPvl(const PvlGroup &group, std::ostream &outputStream);


//   void campt(UserInterface &ui, Pvl *log) {
//     Cube *cube = new Cube(ui.GetCubeName("FROM"), "r");
//     campt(cube, ui, log);
//   }

//   void campt(Cube *cube, UserInterface &ui, Pvl *log) {
//     CameraPointInfo campt;
//     QString fileFormat = ui.GetString("FORMAT");
//     if(fileFormat=="PVL")
//         campt.SetCSVOutput(false);
//     else
//         campt.SetCSVOutput(true);

//     QString inputCubePath = "";
//     try {
//       inputCubePath = ui.GetCubeName("FROM");
//     }
//     catch (IException &e) {
//       inputCubePath = cube->fileName();
//     }
//     campt.SetCube(inputCubePath);

//     if (ui.WasEntered("COORDLIST") && ui.GetBoolean("USEOFFSET")) {
//       QString msg = "COORDLIST and USEOFFSET are mutually exclusive.";
//       throw IException(IException::User, msg, _FILEINFO_);
//     }

//     QList< QPair<double, double> > points = getPoints(
//       ui, ui.WasEntered("COORDLIST") && ui.GetBoolean("USECOORDLIST"),
//       ui.WasEntered("USEOFFSET") && ui.GetBoolean("USEOFFSET"));
//     QList<PvlGroup*> camPoints = getCameraPointInfo(ui, points, campt);
//     writePoints(ui, camPoints, log);
//   }


//   QList< QPair<double, double> > getPoints(const UserInterface &ui, bool usePointList, bool useOffset) {
//     double point1 = 0.0;
//     double point2 = 0.0;
//     QList< QPair<double, double> > points;
//     QString pointType = ui.GetString("TYPE");

//     if (usePointList) {
//       CSVReader reader;
//       reader.read(FileName(ui.GetFileName("COORDLIST")).expanded());
//       if (!reader.isTableValid(reader.getTable()) || reader.columns() != 2) {
//         QString msg = "Coordinate file formatted incorrectly.\n"
//                       "Each row must have two columns: a sample,line or a latitude,longitude pair.";
//         throw IException(IException::User, msg, _FILEINFO_);
//       }
//       for (int row = 0; row < reader.rows(); row++) {
//         point1 = toDouble(reader.getRow(row)[0]);
//         point2 = toDouble(reader.getRow(row)[1]);
//         points.append(QPair<double, double>(point1, point2));
//       }
//     } else if (useOffset) {
//       double offset_sample = ui.GetDouble("SAMPLE");
//       double offset_line = ui.GetDouble("LINE");
//       int num_samples = ui.GetInteger("NSAMPLES");
//       int num_lines = ui.GetInteger("NLINES");
//       for (int i = 0; i < num_samples; i++) {
//         for (int j = 0; j < num_lines; j++) {
//           point1 = static_cast<double>(i + offset_sample);
//           point2 = static_cast<double>(j + offset_line);
//           points.append(QPair<double, double>(point1, point2));
//         }
//       }
//     } else {
//       if (pointType == "IMAGE") {
//         if (ui.WasEntered("SAMPLE")) point1 = ui.GetDouble("SAMPLE");
//         if (ui.WasEntered("LINE")) point2 = ui.GetDouble("LINE");
//       }
//       else {
//         point1 = ui.GetDouble("LATITUDE");
//         point2 = ui.GetDouble("LONGITUDE");
//       }
//       points.append(QPair<double, double>(point1, point2));
//     }
//     return points;
//   }

//   QList<PvlGroup*> getCameraPointInfo(const UserInterface &ui,
//                                       QList< QPair<double, double> > points,
//                                       CameraPointInfo &campt) {
//     QList<PvlGroup*> cameraPoints;
//     bool useCoordListOrOffset = (ui.WasEntered("COORDLIST") && ui.GetBoolean("USECOORDLIST")) ||
//                                 (ui.WasEntered("USEOFFSET") && ui.GetBoolean("USEOFFSET"));
//     bool allowOutside = ui.GetBoolean("ALLOWOUTSIDE");
//     bool allowError = ui.GetBoolean("ALLOWERROR");
//     QString type;
//     if (ui.WasEntered("COORDLIST") && ui.GetBoolean("USECOORDLIST")) {
//       type = ui.GetString("COORDTYPE");
//     }
//     else if (ui.WasEntered("USEOFFSET") && ui.GetBoolean("USEOFFSET")) {
//       type = "IMAGE";
//     }
//     else {
//       type = ui.GetString("TYPE");
//     }
//     PvlGroup *camPoint = NULL;

//     indicators::ProgressBar bar{
//         indicators::option::BarWidth{50},
//         indicators::option::Start{"["},
//         indicators::option::Fill{"="},
//         indicators::option::Lead{">"},
//         indicators::option::Remainder{" "},
//         indicators::option::End{"]"},
//         indicators::option::PrefixText{"Parsing camera points "},
//         indicators::option::ForegroundColor{indicators::Color::white},
//         indicators::option::ShowElapsedTime{true},
//         indicators::option::ShowRemainingTime{true}
//     };

//     for (int i = 0; i < points.size(); i++) {
//       QPair<double, double> pt = points[i];
//       if (type == "GROUND") {
//         camPoint = campt.SetGround(pt.first, pt.second, allowOutside, allowError);
//       }
//       else {
//         if (useCoordListOrOffset || (ui.WasEntered("SAMPLE") && ui.WasEntered("LINE"))) {
//           camPoint = campt.SetImage(pt.first, pt.second, allowOutside, allowError);
//         }
//         else {
//           if (ui.WasEntered("SAMPLE")) {
//             camPoint = campt.SetSample(pt.first, allowOutside, allowError);
//           }
//           else if (ui.WasEntered("LINE")) {
//             camPoint = campt.SetLine(pt.second, allowOutside, allowError);
//           }
//           else {
//             camPoint = campt.SetCenter(allowOutside, allowError);
//           }
//         }
//       }
//       cameraPoints.append(camPoint);

//       if (ui.WasEntered("TO") && points.size() > 1) {
//         float progressVal = static_cast<float>(i + 1) / points.size() * 100.0f;
//         bar.set_option(indicators::option::PostfixText{
//           std::to_string(i + 1) + "/" + std::to_string(points.size())
//         });
//         bar.set_progress(static_cast<size_t>(progressVal));
//       }
//       camPoint = NULL;
//     }
//     if (ui.WasEntered("TO") && points.size() > 1 && !bar.is_completed()) {
//         bar.set_option(indicators::option::PostfixText{
//           std::to_string(points.size()) + "/" + std::to_string(points.size())
//         });
//         bar.set_progress(100);
//     }
//     return cameraPoints;
//   }


//   void writePoints(const UserInterface &ui, QList<PvlGroup*> camPoints, Pvl *log) {
//     Progress prog;
//     prog.SetMaximumSteps(camPoints.size());
//     QString outFile;

//     if (ui.WasEntered("TO")) {
//       outFile = FileName(ui.GetFileName("TO")).expanded();
//     }
//     bool append = ui.GetBoolean("APPEND");
//     QString fileFormat = ui.GetString("FORMAT");

//     // Mandatory keywords that are always output
//     const QSet<QString> mandatoryKeywordNames = {
//         "Filename", "Sample", "Line", "PixelValue",
//         "PlanetocentricLatitude", "PositiveEast360Longitude",
//         "PositiveEast180Longitude", "PositiveWest360Longitude",
//         "PositiveWest180Longitude", "BodyFixedCoordinate",
//         "SpacecraftPosition", "LocalRadius", "EphemerisTime",
//         "LookDirectionBodyFixed", "LookDirectionJ2000",
//         "LookDirectionCamera", "SampleResolution", "SlantDistance",
//         "C2M_RotationMatrix"
//     };

//     indicators::ProgressBar bar{
//         indicators::option::BarWidth{50},
//         indicators::option::Start{"["},
//         indicators::option::Fill{"="},
//         indicators::option::Lead{">"},
//         indicators::option::Remainder{" "},
//         indicators::option::End{"]"},
//         indicators::option::PrefixText{"Writing camera points "},
//         indicators::option::ForegroundColor{indicators::Color::white},
//         indicators::option::ShowElapsedTime{true},
//         indicators::option::ShowRemainingTime{true}
//     };

//     for (int p = 0; p < camPoints.size(); p++) {
//       bool fileExists = FileName(outFile).fileExists();
//       prog.CheckStatus();
//       PvlGroup *originalPointGroup = camPoints[p];
      
//       PvlGroup filteredPointGroup;
//       if (originalPointGroup->name().isEmpty()) {
//           filteredPointGroup.setName("GroundPoint");
//       } else {
//           filteredPointGroup.setName(originalPointGroup->name());
//       }

//       for (int k = 0; k < originalPointGroup->keywords(); k++) {
//         const PvlKeyword &originalKeyword = (*originalPointGroup)[k];
//         QString keywordName = originalKeyword.name();

//         if (keywordName == "Error") {
//           continue;
//         }

//         bool shouldAdd = false;

//         // Check if it's a mandatory keyword
//         if (mandatoryKeywordNames.contains(keywordName)) {
//           shouldAdd = true;
// 		    } else {
//           // For optional keywords, check if the boolean parameter is set to true
//           if (ui.WasEntered(keywordName) && ui.GetBoolean(keywordName)) {
//             shouldAdd = true;
//           }
//         }

//         if (shouldAdd) {
//           PvlKeyword keywordToAdd = originalKeyword;
//           // Strip units for look direction vectors if requested
//           if (keywordName == "LookDirectionBodyFixed" && ui.WasEntered("LOOKDIRECTIONBODYFIXED") && ui.GetBoolean("LOOKDIRECTIONBODYFIXED")) {
//             if (!keywordToAdd.unit().isEmpty()) keywordToAdd.setUnits("");
//           }
//           if (keywordName == "LookDirectionJ2000"&& ui.WasEntered("LOOKDIRECTIONJ2000") && ui.GetBoolean("LOOKDIRECTIONJ2000")) {
//              if (!keywordToAdd.unit().isEmpty()) keywordToAdd.setUnits("");
//           }
//           if (keywordName == "LookDirectionCamera"&& ui.WasEntered("LOOKDIRECTIONCAMERA") && ui.GetBoolean("LOOKDIRECTIONCAMERA")) {
//              if (!keywordToAdd.unit().isEmpty()) keywordToAdd.setUnits("");
//           }
//           filteredPointGroup.addKeyword(keywordToAdd);
//         }
//       }
      
//       if (ui.WasEntered("TO")) {
//         if (fileFormat == "PVL") {
//           Pvl temp;
//           temp.setTerminator("");
//           temp.addGroup(filteredPointGroup);
//           if (append || p > 0) {
//              temp.append(outFile);
//           } else {
//              temp.write(outFile);
//           }
//         }
//         else {
//           ofstream os;
//           bool writeHeader = false;
//           if (p == 0) {
//              if (!append || (append && !fileExists) ) {
//                  writeHeader = true;
//              }
//           }

//           if (append) {
//             os.open(outFile.toLatin1().data(), ios::app);
//             if (p == 0 && !fileExists) writeHeader = true;
//           } else {
//             os.open(outFile.toLatin1().data(), ios::out);
//             writeHeader = true;
//           }
          
//           if (writeHeader) {
//             for (int i = 0; i < filteredPointGroup.keywords(); i++) {
//               if (filteredPointGroup[i].size() == 3) {
//                 os << filteredPointGroup[i].name().toStdString() << "X,"
//                    << filteredPointGroup[i].name().toStdString() << "Y,"
//                    << filteredPointGroup[i].name().toStdString() << "Z";
//               }
//               else if (filteredPointGroup[i].size() == 9) {
//                 os << filteredPointGroup[i].name().toStdString() << "X1,"
//                    << filteredPointGroup[i].name().toStdString() << "X2,"
//                    << filteredPointGroup[i].name().toStdString() << "X3,"
//                    << filteredPointGroup[i].name().toStdString() << "Y1,"
//                    << filteredPointGroup[i].name().toStdString() << "Y2,"
//                    << filteredPointGroup[i].name().toStdString() << "Y3,"
//                    << filteredPointGroup[i].name().toStdString() << "Z1,"
//                    << filteredPointGroup[i].name().toStdString() << "Z2,"
//                    << filteredPointGroup[i].name().toStdString() << "Z3";
//               }
//               else {
//                 os << filteredPointGroup[i].name().toStdString();
//               }
//               if (i < filteredPointGroup.keywords() - 1) {
//                 os << ",";
//               }
//             }
//             os << endl;
//           }

//           for (int i = 0; i < filteredPointGroup.keywords(); i++) {
//             const PvlKeyword &key = filteredPointGroup[i];
//             for(int valIdx = 0; valIdx < key.size(); ++valIdx) {
//                 os << key[valIdx].toStdString();
//                 if (valIdx < key.size() - 1) {
//                     os << ",";
//                 }
//             }
//             if (i < filteredPointGroup.keywords() - 1) {
//               os << ",";
//             }
//           }
//           os << endl;
//           os.close();
//         }
//       }
//       else {
//         if (fileFormat == "FLAT") {
//           string msg = "Flat file format requires an output file name. Please specify the TO parameter.";
//           throw IException(IException::User, msg, _FILEINFO_);
//         }
//         printCategorizedPvl(filteredPointGroup, std::cout);
//       }

//       if (ui.WasEntered("TO") && camPoints.size() > 1) {
//         float progressVal = static_cast<float>(p + 1) / camPoints.size() * 100.0f;
//         bar.set_option(indicators::option::PostfixText{
//           std::to_string(p + 1) + "/" + std::to_string(camPoints.size())
//         });
//         bar.set_progress(static_cast<size_t>(progressVal));
//       }
//       delete originalPointGroup;
//       originalPointGroup = NULL;
//     }
//     prog.CheckStatus();
//      if (ui.WasEntered("TO") && camPoints.size() > 1 && !bar.is_completed()) {
//         bar.set_option(indicators::option::PostfixText{
//           std::to_string(camPoints.size()) + "/" + std::to_string(camPoints.size())
//         });
//         bar.set_progress(100);
//     }
//   }

//   void printCategorizedPvl(const PvlGroup &group, std::ostream &outputStream) {
//     outputStream << "Group = " << group.name().toStdString() << std::endl;

//     // Define categories with proper ordering
//     QStringList categoryOrder;
//     QMap<QString, QStringList> categories;
    
//     // General Point Information (no header)
//     categoryOrder << "";
//     categories[""] = QStringList()
//         << "Filename" << "Sample" << "Line" << "PixelValue"
//         << "RightAscension" << "Declination"
//         << "PlanetocentricLatitude" << "PlanetographicLatitude"
//         << "PositiveEast360Longitude" << "PositiveEast180Longitude"
//         << "PositiveWest360Longitude" << "PositiveWest180Longitude"
//         << "BodyFixedCoordinate" << "LocalRadius"
//         << "SampleResolution" << "LineResolution"
//         << "ObliqueDetectorResolution" << "ObliquePixelResolution"
//         << "ObliqueLineResolution" << "ObliqueSampleResolution";

//     // Spacecraft Information
//     categoryOrder << "  # Spacecraft Information";
//     categories["  # Spacecraft Information"] = QStringList()
//         << "SpacecraftPosition" << "SpacecraftAzimuth" << "SlantDistance"
//         << "TargetCenterDistance" << "SubSpacecraftLatitude" << "SubSpacecraftLongitude"
//         << "SpacecraftAltitude" << "OffNadirAngle" << "SubSpacecraftGroundAzimuth";

//     // Sun Information
//     categoryOrder << "  # Sun Information";
//     categories["  # Sun Information"] = QStringList()
//         << "SunPosition" << "SubSolarAzimuth" << "SolarDistance"
//         << "SubSolarLatitude" << "SubSolarLongitude" << "SubSolarGroundAzimuth";

//     // Illumination and Other
//     categoryOrder << "  # Illumination and Other";
//     categories["  # Illumination and Other"] = QStringList()
//         << "Phase" << "Incidence" << "Emission" << "NorthAzimuth";

//     // Time
//     categoryOrder << "  # Time";
//     categories["  # Time"] = QStringList()
//         << "EphemerisTime" << "UTC" << "LocalSolarTime" << "SolarLongitude"
//         << "LookDirectionBodyFixed" << "LookDirectionJ2000" << "LookDirectionCamera" << "C2M_RotationMatrix";
    
// //    // Look Directions
// //    categoryOrder << "  # Look Directions";
// //    categories["  # Look Directions"] = QStringList()
// //        << "LookDirectionBodyFixed" << "LookDirectionJ2000" << "LookDirectionCamera";
    
//     // Rotation Matrix (at the end)
// //    categoryOrder << "  # Rotation Matrix";
// //    categories["  # Rotation Matrix"] = QStringList()
// //        << "C2M_RotationMatrix";

//     QList<QString> printedKeywords;

//     // Print each category in order
//     for (const QString &categoryName : categoryOrder) {
//         const QStringList &keywordsInCategory = categories[categoryName];
//         bool categoryHasContent = false;
//         QString categoryBuffer;
        
//         // Check if any keywords in this category exist
//         for (const QString &kwName : keywordsInCategory) {
//             if (group.hasKeyword(kwName)) {
//                 categoryHasContent = true;
//                 break;
//             }
//         }
        
//         // If category has content, print header if it exists
//         if (categoryHasContent && !categoryName.isEmpty()) {
//             outputStream << std::endl << categoryName.toStdString() << std::endl;
//         }
        
//         // Print keywords in this category
//         for (const QString &kwName : keywordsInCategory) {
//             if (group.hasKeyword(kwName)) {
//                 const PvlKeyword &kw = group.findKeyword(kwName);
//                 outputStream << "  " << kw.name().toStdString() << std::setw(27 - kw.name().length()) << " = ";
                
//                 // Handle multi-value keywords
//                 if (kw.size() > 1 || kw.name().endsWith("Position") || kw.name().endsWith("Coordinate") ||
//                     kw.name().startsWith("LookDirection") || kw.name().contains("Rotation") ||
//                     kw.name() == "C2M_RotationMatrix") {
//                     outputStream << "(";
//                     for (int v = 0; v < kw.size(); ++v) {
//                         outputStream << kw[v].toStdString();
//                         if (kw.size() == 9 && (v + 1) % 3 == 0 && v < kw.size() - 1) {
//                             outputStream << "," << std::endl << std::setw(30) << "";
//                         } else if (v < kw.size() - 1) {
//                             outputStream << ", ";
//                         }
//                     }
//                     outputStream << ")";
//                 } else {
//                     outputStream << kw[0].toStdString();
//                 }
                
//                 // Add units if present
//                 if (!kw.unit().isEmpty()) {
//                     outputStream << " <" << kw.unit().toStdString() << ">";
//                 }
//                 outputStream << std::endl;
//                 printedKeywords.append(kwName);
//             }
//         }
//     }

//     // Print any remaining keywords not covered by categories
//     std::string uncatBlockBuffer = "";
//     bool uncatKeywordsFound = false;
//     for (int k = 0; k < group.keywords(); ++k) {
//         const PvlKeyword &kw = group[k];
//         if (!printedKeywords.contains(kw.name())) {
//             uncatKeywordsFound = true;
//             std::stringstream kwStream;
//             kwStream << "  " << kw.name().toStdString() << std::setw(27 - kw.name().length()) << " = ";
//             if (kw.size() > 1) {
//                 kwStream << "(";
//                 for (int v = 0; v < kw.size(); ++v) {
//                     kwStream << kw[v].toStdString();
//                     if (kw.size() == 9 && (v + 1) % 3 == 0 && v < kw.size() - 1) {
//                         kwStream << "," << std::endl << std::setw(30) << "";
//                     } else if (v < kw.size() - 1) {
//                         kwStream << ", ";
//                     }
//                 }
//                 kwStream << ")";
//             } else {
//                 kwStream << kw[0].toStdString();
//             }
//             if (!kw.unit().isEmpty()) {
//                 kwStream << " <" << kw.unit().toStdString() << ">";
//             }
//             kwStream << std::endl;
//             uncatBlockBuffer += kwStream.str();
//         }
//     }
    
//     if(uncatKeywordsFound){
//         outputStream << std::endl << "  # Other Information" << std::endl;
//         outputStream << uncatBlockBuffer;
//     }

//     outputStream << "End_Group" << std::endl;
//   }

// }

#include "campt.h"

#include <string>
#include <iomanip>
#include <QSet>
#include <QMap>
#include <QStringList>
#include <iostream>
#include <sstream>

#include "Brick.h"
#include "Camera.h"
#include "CameraPointInfo.h"
#include "CSVReader.h"
#include "Distance.h"
#include "IException.h"
#include "iTime.h"
#include "Longitude.h"
#include "Progress.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "SpecialPixel.h"
#include "TProjection.h"
#include "Application.h"

#include "indicators.hpp"

using namespace std;
using namespace Isis;

namespace Isis{

  QList< QPair<double, double> > getPoints(const UserInterface &ui, bool usePointList, bool useOffset);
  QList<PvlGroup *> getCameraPointInfo(const UserInterface &ui,
                                      QList< QPair<double, double> > points,
                                      CameraPointInfo &campt);
  void writePoints(const UserInterface &ui, QList<PvlGroup*> camPoints, Pvl *log);
  void printCategorizedPvl(const PvlGroup &group, std::ostream &outputStream);

  void campt(UserInterface &ui, Pvl *log) {
    Cube *cube = new Cube(ui.GetCubeName("FROM"), "r");
    campt(cube, ui, log);
  }

  void campt(Cube *cube, UserInterface &ui, Pvl *log) {
    CameraPointInfo campt;
    QString fileFormat = ui.GetString("FORMAT");
    if(fileFormat=="PVL")
        campt.SetCSVOutput(false);
    else
        campt.SetCSVOutput(true);

    QString inputCubePath = "";
    try {
      inputCubePath = ui.GetCubeName("FROM");
    }
    catch (IException &e) {
      inputCubePath = cube->fileName();
    }
    campt.SetCube(inputCubePath);

    if (ui.WasEntered("COORDLIST") && ui.GetBoolean("USEOFFSET")) {
      QString msg = "COORDLIST and USEOFFSET are mutually exclusive.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    QList< QPair<double, double> > points = getPoints(
      ui, ui.WasEntered("COORDLIST") && ui.GetBoolean("USECOORDLIST"),
      ui.WasEntered("USEOFFSET") && ui.GetBoolean("USEOFFSET"));
    QList<PvlGroup*> camPoints = getCameraPointInfo(ui, points, campt);
    writePoints(ui, camPoints, log);
  }

  QList< QPair<double, double> > getPoints(const UserInterface &ui, bool usePointList, bool useOffset) {
    double point1 = 0.0;
    double point2 = 0.0;
    QList< QPair<double, double> > points;
    QString pointType = ui.GetString("TYPE");

    if (usePointList) {
      CSVReader reader;
      reader.read(FileName(ui.GetFileName("COORDLIST")).expanded());
      if (!reader.isTableValid(reader.getTable()) || reader.columns() != 2) {
        QString msg = "Coordinate file formatted incorrectly.\n"
                      "Each row must have two columns: a sample,line or a latitude,longitude pair.";
        throw IException(IException::User, msg, _FILEINFO_);
      }
      for (int row = 0; row < reader.rows(); row++) {
        point1 = toDouble(reader.getRow(row)[0]);
        point2 = toDouble(reader.getRow(row)[1]);
        points.append(QPair<double, double>(point1, point2));
      }
    } else if (useOffset) {
      double offset_sample = ui.GetDouble("SAMPLE");
      double offset_line = ui.GetDouble("LINE");
      int num_samples = ui.GetInteger("NSAMPLES");
      int num_lines = ui.GetInteger("NLINES");
      for (int i = 0; i < num_samples; i++) {
        for (int j = 0; j < num_lines; j++) {
          point1 = static_cast<double>(i + offset_sample);
          point2 = static_cast<double>(j + offset_line);
          points.append(QPair<double, double>(point1, point2));
        }
      }
    } else {
      if (pointType == "IMAGE") {
        if (ui.WasEntered("SAMPLE")) point1 = ui.GetDouble("SAMPLE");
        if (ui.WasEntered("LINE")) point2 = ui.GetDouble("LINE");
      }
      else {
        point1 = ui.GetDouble("LATITUDE");
        point2 = ui.GetDouble("LONGITUDE");
      }
      points.append(QPair<double, double>(point1, point2));
    }
    return points;
  }

  QList<PvlGroup*> getCameraPointInfo(const UserInterface &ui,
                                      QList< QPair<double, double> > points,
                                      CameraPointInfo &campt) {
    QList<PvlGroup*> cameraPoints;
    bool useCoordListOrOffset = (ui.WasEntered("COORDLIST") && ui.GetBoolean("USECOORDLIST")) ||
                                (ui.WasEntered("USEOFFSET") && ui.GetBoolean("USEOFFSET"));
    bool allowOutside = ui.GetBoolean("ALLOWOUTSIDE");
    bool allowError = ui.GetBoolean("ALLOWERROR");
    QString type;
    if (ui.WasEntered("COORDLIST") && ui.GetBoolean("USECOORDLIST")) {
      type = ui.GetString("COORDTYPE");
    }
    else if (ui.WasEntered("USEOFFSET") && ui.GetBoolean("USEOFFSET")) {
      type = "IMAGE";
    }
    else {
      type = ui.GetString("TYPE");
    }
    PvlGroup *camPoint = NULL;

    indicators::ProgressBar bar{
        indicators::option::BarWidth{50},
        indicators::option::Start{"["},
        indicators::option::Fill{"="},
        indicators::option::Lead{">"},
        indicators::option::Remainder{" "},
        indicators::option::End{"]"},
        indicators::option::PrefixText{"Parsing camera points "},
        indicators::option::ForegroundColor{indicators::Color::white},
        indicators::option::ShowElapsedTime{true},
        indicators::option::ShowRemainingTime{true}
    };

    for (int i = 0; i < points.size(); i++) {
      QPair<double, double> pt = points[i];
      if (type == "GROUND") {
        camPoint = campt.SetGround(ui, pt.first, pt.second, allowOutside, allowError);
      }
      else {
        if (useCoordListOrOffset || (ui.WasEntered("SAMPLE") && ui.WasEntered("LINE"))) {
          camPoint = campt.SetImage(ui, pt.first, pt.second, allowOutside, allowError);
        }
        else {
          if (ui.WasEntered("SAMPLE")) {
            camPoint = campt.SetSample(ui, pt.first, allowOutside, allowError);
          }
          else if (ui.WasEntered("LINE")) {
            camPoint = campt.SetLine(ui, pt.second, allowOutside, allowError);
          }
          else {
            camPoint = campt.SetCenter(ui, allowOutside, allowError);
          }
        }
      }
      cameraPoints.append(camPoint);

      if (ui.WasEntered("TO") && points.size() > 1) {
        float progressVal = static_cast<float>(i + 1) / points.size() * 100.0f;
        bar.set_option(indicators::option::PostfixText{
          std::to_string(i + 1) + "/" + std::to_string(points.size())
        });
        bar.set_progress(static_cast<size_t>(progressVal));
      }
      camPoint = NULL;
    }
    if (ui.WasEntered("TO") && points.size() > 1 && !bar.is_completed()) {
        bar.set_option(indicators::option::PostfixText{
          std::to_string(points.size()) + "/" + std::to_string(points.size())
        });
        bar.set_progress(100);
    }
    return cameraPoints;
  }

  void writePoints(const UserInterface &ui, QList<PvlGroup*> camPoints, Pvl *log) {
    Progress prog;
    prog.SetMaximumSteps(camPoints.size());
    QString outFile;

    if (ui.WasEntered("TO")) {
      outFile = FileName(ui.GetFileName("TO")).expanded();
    }
    bool append = ui.GetBoolean("APPEND");
    QString fileFormat = ui.GetString("FORMAT");

    indicators::ProgressBar bar{
        indicators::option::BarWidth{50},
        indicators::option::Start{"["},
        indicators::option::Fill{"="},
        indicators::option::Lead{">"},
        indicators::option::Remainder{" "},
        indicators::option::End{"]"},
        indicators::option::PrefixText{"Writing camera points "},
        indicators::option::ForegroundColor{indicators::Color::white},
        indicators::option::ShowElapsedTime{true},
        indicators::option::ShowRemainingTime{true}
    };

    for (int p = 0; p < camPoints.size(); p++) {
      bool fileExists = false;
      if (ui.WasEntered("TO")) {
        fileExists = FileName(outFile).fileExists();
      }
      
      prog.CheckStatus();
      PvlGroup *pointGroup = camPoints[p];
      
      // Look direction units 제거 (필요한 경우)
      if (pointGroup->hasKeyword("LookDirectionBodyFixed") && 
          ui.WasEntered("LOOKDIRECTIONBODYFIXED") && ui.GetBoolean("LOOKDIRECTIONBODYFIXED")) {
        pointGroup->findKeyword("LookDirectionBodyFixed").setUnits("");
      }
      if (pointGroup->hasKeyword("LookDirectionJ2000") && 
          ui.WasEntered("LOOKDIRECTIONJ2000") && ui.GetBoolean("LOOKDIRECTIONJ2000")) {
        pointGroup->findKeyword("LookDirectionJ2000").setUnits("");
      }
      if (pointGroup->hasKeyword("LookDirectionCamera") && 
          ui.WasEntered("LOOKDIRECTIONCAMERA") && ui.GetBoolean("LOOKDIRECTIONCAMERA")) {
        pointGroup->findKeyword("LookDirectionCamera").setUnits("");
      }
      
      if (ui.WasEntered("TO")) {
        if (fileFormat == "PVL") {
          Pvl temp;
          temp.setTerminator("");
          temp.addGroup(*pointGroup);
          if (append || p > 0) {
            temp.append(outFile);
          } else {
            temp.write(outFile);
          }
        }
        else {
          ofstream os;
          bool writeHeader = false;
          if (p == 0) {
            if (!append || (append && !fileExists)) {
              writeHeader = true;
            }
          }

          if (append) {
            os.open(outFile.toLatin1().data(), ios::app);
            if (p == 0 && !fileExists) writeHeader = true;
          } else {
            os.open(outFile.toLatin1().data(), ios::out);
            writeHeader = true;
          }
          
          if (writeHeader) {
            for (int i = 0; i < pointGroup->keywords(); i++) {
              if ((*pointGroup)[i].size() == 3) {
                os << (*pointGroup)[i].name().toStdString() << "X,"
                   << (*pointGroup)[i].name().toStdString() << "Y,"
                   << (*pointGroup)[i].name().toStdString() << "Z";
              }
              else if ((*pointGroup)[i].size() == 9) {
                os << (*pointGroup)[i].name().toStdString() << "X1,"
                   << (*pointGroup)[i].name().toStdString() << "X2,"
                   << (*pointGroup)[i].name().toStdString() << "X3,"
                   << (*pointGroup)[i].name().toStdString() << "Y1,"
                   << (*pointGroup)[i].name().toStdString() << "Y2,"
                   << (*pointGroup)[i].name().toStdString() << "Y3,"
                   << (*pointGroup)[i].name().toStdString() << "Z1,"
                   << (*pointGroup)[i].name().toStdString() << "Z2,"
                   << (*pointGroup)[i].name().toStdString() << "Z3";
              }
              else {
                os << (*pointGroup)[i].name().toStdString();
              }
              if (i < pointGroup->keywords() - 1) {
                os << ",";
              }
            }
            os << endl;
          }

          for (int i = 0; i < pointGroup->keywords(); i++) {
            const PvlKeyword &key = (*pointGroup)[i];
            for(int valIdx = 0; valIdx < key.size(); ++valIdx) {
              os << key[valIdx].toStdString();
              if (valIdx < key.size() - 1) {
                os << ",";
              }
            }
            if (i < pointGroup->keywords() - 1) {
              os << ",";
            }
          }
          os << endl;
          os.close();
        }
      }
      else {
        if (fileFormat == "FLAT") {
          string msg = "Flat file format requires an output file name. Please specify the TO parameter.";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        printCategorizedPvl(*pointGroup, std::cout);
      }

      if (ui.WasEntered("TO") && camPoints.size() > 1) {
        float progressVal = static_cast<float>(p + 1) / camPoints.size() * 100.0f;
        bar.set_option(indicators::option::PostfixText{
          std::to_string(p + 1) + "/" + std::to_string(camPoints.size())
        });
        bar.set_progress(static_cast<size_t>(progressVal));
      }
      delete pointGroup;
      pointGroup = NULL;
    }
    
    prog.CheckStatus();
    if (ui.WasEntered("TO") && camPoints.size() > 1 && !bar.is_completed()) {
      bar.set_option(indicators::option::PostfixText{
        std::to_string(camPoints.size()) + "/" + std::to_string(camPoints.size())
      });
      bar.set_progress(100);
    }
  }

  void printCategorizedPvl(const PvlGroup &group, std::ostream &outputStream) {
    outputStream << "Group = " << group.name().toStdString() << std::endl;

    QStringList categoryOrder;
    QMap<QString, QStringList> categories;
    
    categoryOrder << "";
    categories[""] = QStringList()
        << "Filename" << "Sample" << "Line" << "PixelValue"
        << "RightAscension" << "Declination"
        << "PlanetocentricLatitude" << "PlanetographicLatitude"
        << "PositiveEast360Longitude" << "PositiveEast180Longitude"
        << "PositiveWest360Longitude" << "PositiveWest180Longitude"
        << "BodyFixedCoordinate" << "LocalRadius"
        << "SampleResolution" << "LineResolution"
        << "ObliqueDetectorResolution" << "ObliquePixelResolution"
        << "ObliqueLineResolution" << "ObliqueSampleResolution";

    categoryOrder << "  # Spacecraft Information";
    categories["  # Spacecraft Information"] = QStringList()
        << "SpacecraftPosition" << "SpacecraftAzimuth" << "SlantDistance"
        << "TargetCenterDistance" << "SubSpacecraftLatitude" << "SubSpacecraftLongitude"
        << "SpacecraftAltitude" << "OffNadirAngle" << "SubSpacecraftGroundAzimuth";

    categoryOrder << "  # Sun Information";
    categories["  # Sun Information"] = QStringList()
        << "SunPosition" << "SubSolarAzimuth" << "SolarDistance"
        << "SubSolarLatitude" << "SubSolarLongitude" << "SubSolarGroundAzimuth";

    categoryOrder << "  # Illumination and Other";
    categories["  # Illumination and Other"] = QStringList()
        << "Phase" << "Incidence" << "Emission" << "NorthAzimuth";

    categoryOrder << "  # Time";
    categories["  # Time"] = QStringList()
        << "EphemerisTime" << "UTC" << "LocalSolarTime" << "SolarLongitude"
        << "LookDirectionBodyFixed" << "LookDirectionJ2000" << "LookDirectionCamera" << "C2M_RotationMatrix";

    QList<QString> printedKeywords;

    for (const QString &categoryName : categoryOrder) {
        const QStringList &keywordsInCategory = categories[categoryName];
        bool categoryHasContent = false;
        
        for (const QString &kwName : keywordsInCategory) {
            if (group.hasKeyword(kwName)) {
                categoryHasContent = true;
                break;
            }
        }
        
        if (categoryHasContent && !categoryName.isEmpty()) {
            outputStream << std::endl << categoryName.toStdString() << std::endl;
        }
        
        for (const QString &kwName : keywordsInCategory) {
            if (group.hasKeyword(kwName)) {
                const PvlKeyword &kw = group.findKeyword(kwName);
                outputStream << "  " << kw.name().toStdString() << std::setw(27 - kw.name().length()) << " = ";
                
                if (kw.size() > 1 || kw.name().endsWith("Position") || kw.name().endsWith("Coordinate") ||
                    kw.name().startsWith("LookDirection") || kw.name().contains("Rotation") ||
                    kw.name() == "C2M_RotationMatrix") {
                    outputStream << "(";
                    for (int v = 0; v < kw.size(); ++v) {
                        outputStream << kw[v].toStdString();
                        if (kw.size() == 9 && (v + 1) % 3 == 0 && v < kw.size() - 1) {
                            outputStream << "," << std::endl << std::setw(30) << "";
                        } else if (v < kw.size() - 1) {
                            outputStream << ", ";
                        }
                    }
                    outputStream << ")";
                } else {
                    outputStream << kw[0].toStdString();
                }
                
                if (!kw.unit().isEmpty()) {
                    outputStream << " <" << kw.unit().toStdString() << ">";
                }
                outputStream << std::endl;
                printedKeywords.append(kwName);
            }
        }
    }

    std::string uncatBlockBuffer = "";
    bool uncatKeywordsFound = false;
    for (int k = 0; k < group.keywords(); ++k) {
        const PvlKeyword &kw = group[k];
        if (!printedKeywords.contains(kw.name())) {
            uncatKeywordsFound = true;
            std::stringstream kwStream;
            kwStream << "  " << kw.name().toStdString() << std::setw(27 - kw.name().length()) << " = ";
            if (kw.size() > 1) {
                kwStream << "(";
                for (int v = 0; v < kw.size(); ++v) {
                    kwStream << kw[v].toStdString();
                    if (kw.size() == 9 && (v + 1) % 3 == 0 && v < kw.size() - 1) {
                        kwStream << "," << std::endl << std::setw(30) << "";
                    } else if (v < kw.size() - 1) {
                        kwStream << ", ";
                    }
                }
                kwStream << ")";
            } else {
                kwStream << kw[0].toStdString();
            }
            if (!kw.unit().isEmpty()) {
                kwStream << " <" << kw.unit().toStdString() << ">";
            }
            kwStream << std::endl;
            uncatBlockBuffer += kwStream.str();
        }
    }
    
    if(uncatKeywordsFound){
        outputStream << std::endl << "  # Other Information" << std::endl;
        outputStream << uncatBlockBuffer;
    }

    outputStream << "End_Group" << std::endl;
  }
}