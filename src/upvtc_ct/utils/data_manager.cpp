#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include <limits.h>
#include <unistd.h>

#include <nlohmann/json.hpp>
#include <toml11/toml.hpp>

#include <upvtc_ct/ds/models.hpp>
#include <upvtc_ct/utils/data_manager.hpp>
#include <upvtc_ct/utils/errors.hpp>
#include <upvtc_ct/utils/hash_specializations.hpp>

namespace upvtc_ct::utils
{
  using json = nlohmann::json;
  namespace ds = upvtc_ct::ds;
  namespace utils = upvtc_ct::utils;

  unsigned int DataManager::getNewStudentGroupID()
  {
    unsigned int newID = this->currStudentGroupID;
    this->currStudentGroupID++;
    return newID;
  }

  bool Config::isEmpty() const
  {
    return this->configData.size() == 0;
  }

  void Config::addConfig(const std::string key, std::any value)
  {
    this->configData[key] = value;
  }

  ConfigError::ConfigError(const char* what_arg)
    : std::runtime_error(what_arg) {}

  DataManager::DataManager()
    : config(),
      currStudentGroupID(0)
  {
    this->parseConfigFile();
    const Config& config = this->getConfig();
    const unsigned int& semester = config.get<const unsigned int>("semester");

    this->parseRoomFeaturesJSON();
    this->parseTeachersJSON();
    this->parseDivisionsJSON();
    this->parseRoomsJSON();
    this->parseCoursesJSON();

    std::unordered_map<std::pair<std::string, unsigned int>,
                       ds::StudentGroup*,
                       PairHash> generatedStudentGroups;
    std::unordered_map<std::pair<std::string, unsigned int>,
                       std::unordered_set<ds::Course*>,
                       PairHash> studyPlans;
    this->parseStudyPlansJSON(semester, generatedStudentGroups, studyPlans);
    this->parseStudentGroupsJSON(generatedStudentGroups);
    this->parseRegularStudentGroupsGEsElectivesJSON(generatedStudentGroups);
    this->parseIrregularStudentGroupsJSON(studyPlans);
  }

  const Config& DataManager::getConfig() const
  {
    return this->config;
  }

  const std::unordered_set<std::unique_ptr<ds::Course>>&
  DataManager::getCourses() const
  {
    return this->courses;
  }

  const std::unordered_set<std::unique_ptr<ds::Class>>&
  DataManager::getClasses() const
  {
    return this->classes;
  }

  const std::unordered_set<std::unique_ptr<ds::Degree>>&
  DataManager::getDegrees() const
  {
    return this->degrees;
  }

  const std::unordered_set<std::unique_ptr<ds::Division>>&
  DataManager::getDivisions() const
  {
    return this->divisions;
  }

  const std::unordered_set<std::unique_ptr<ds::StudentGroup>>&
  DataManager::getStudentGroups() const
  {
    return this->studentGroups;
  }

  const std::unordered_set<std::unique_ptr<ds::Room>>&
  DataManager::getRooms() const
  {
    return this->rooms;
  }

  const std::unordered_set<std::unique_ptr<ds::RoomFeature>>&
  DataManager::getRoomFeatures() const
  {
    return this->roomFeatures;
  }

  const std::unordered_set<std::unique_ptr<ds::Teacher>>&
  DataManager::getTeachers() const
  {
    return this->teachers;
  }

  ds::Course* const DataManager::getCourseNameObject(
      const std::string courseName,
      const char* errorMsg) const
  {
    auto courseItem = this->courseNameToObject.find(courseName);
    if (courseItem == this->courseNameToObject.end()) {
      throw utils::InvalidContentsError(errorMsg);
    }

    return courseItem->second;
  }

  ds::Degree* const DataManager::getDegreeNameObject(
      const std::string degreeName) const
  {
    auto degreeItem = this->degreeNameToObject.find(degreeName);
    if (degreeItem == this->degreeNameToObject.end()) {
      std::stringstream errorMsgStream;
      errorMsgStream << "Degree, " << degreeName
                     << ", cannot be found. Degree object must not have been "
                     << "created yet.";
      const char* errorMsg = (errorMsgStream.str()).c_str();
      throw utils::InvalidContentsError(errorMsg);
    }

    return degreeItem->second;
  }

  ds::Division* const DataManager::getDivisionNameObject(
      const std::string divisionName) const
  {
    auto divisionItem = this->divisionNameToObject.find(divisionName);
    if (divisionItem == this->divisionNameToObject.end()) {
      std::stringstream errorMsgStream;
      errorMsgStream << "Division, " << divisionName
                     << ", cannot be found. Division object must not have "
                     << "been created yet.";
      const char* errorMsg = (errorMsgStream.str()).c_str();
      throw utils::InvalidContentsError(errorMsg);
    }

    return divisionItem->second;
  }

  ds::Room* const DataManager::getRoomNameObject(
      const std::string roomName) const
  {
    auto roomItem = this->roomNameToObject.find(roomName);
    if (roomItem == this->roomNameToObject.end()) {
      std::stringstream errorMsgStream;
      errorMsgStream << "Room, " << roomName << ", cannot be found. Room "
                     << "object must not have been created yet.";
      const char* errorMsg = (errorMsgStream.str()).c_str();
      throw utils::InvalidContentsError(errorMsg);
    }

    return roomItem->second;
  }

  ds::RoomFeature* const DataManager::getRoomFeatureObject(
        const std::string roomFeatureName,
        const char* errorMsg) const
  {
    auto roomFeatureItem = this->roomFeatureToObject.find(roomFeatureName);
    if (roomFeatureItem == this->roomFeatureToObject.end()) {
      throw utils::InvalidContentsError(errorMsg);
    }

    return roomFeatureItem->second;
  }

  ds::Teacher* const DataManager::getTeacherNameObject(
      const std::string teacherName) const
  {
    auto teacherItem = this->teacherNameToObject.find(teacherName);
    if (teacherItem == this->teacherNameToObject.end()) {
      std::stringstream errorMsgStream;
      errorMsgStream << "Teacher, " << teacherName << ", cannot be found. "
                     << "Teacher object must not have been created yet.";
      const char* errorMsg = (errorMsgStream.str()).c_str();
      throw utils::InvalidContentsError(errorMsg);
    }

    return teacherItem->second;
  }

  ds::Course* const DataManager::getCourseLab(ds::Course* const course) const
  {
    auto labCourseItem = this->courseToLabObject.find(course);
    if (labCourseItem == this->courseToLabObject.end()) {
      std::stringstream errorMsgStream;
      errorMsgStream << "Course, " << course->name << "does not have a lab "
                     << "unit.";

      throw utils::InvalidContentsError((errorMsgStream.str()).c_str());
    }

    return labCourseItem->second;
  }

  const std::unordered_map<size_t, std::unordered_set<ds::Class*>>&
  DataManager::getClassGroups() const
  {
    return this->classGroups;
  }

  const std::unordered_map<size_t, std::unordered_set<size_t>>&
  DataManager::getClassConflicts() const
  {
    return this->classConflicts;
  }

  void DataManager::addClass(std::unique_ptr<ds::Class>&& clsPtr)
  {
    // Add classes into a class group.
    auto classGroup = this->classGroups.find(clsPtr->classID);
    if (classGroup == this->classGroups.end()) {
      this->classGroups.insert({clsPtr->classID, {}});
    }

    this->classGroups[clsPtr->classID].insert(clsPtr.get());

    this->classes.insert(std::move(clsPtr));
  }

  void DataManager::addClassConflict(const size_t classGroup,
                                     const size_t conflictedGroup)
  {
    const auto item = this->classConflicts.find(classGroup);
    if (item == this->classConflicts.end()) {
      this->classConflicts.insert({classGroup, {}});
    }

    this->classConflicts[classGroup].insert(conflictedGroup);
  }

  const std::string DataManager::getBinFolderPath() const
  {
    // Note that this function is only guaranteed to work in Linux. Please refer
    // to the link
    //   https://stackoverflow.com/a/55579815/1116098
    // if you would like to add support for Windows.
    //
    // This function implementation is copied (with some minor changes) from
    // the function of the same nature from the sands-sim-2d project of Sean
    // Ballais, the original author of this project. You may access the
    // sands-sim-2d project via:
    //   https://github.com/seanballais/sands-sim-2d
    char path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
    std::string binPath = std::string(path, (count > 0) ? count : 0);

    return std::filesystem::path(binPath).parent_path();
  }

  const std::string DataManager::getDataFolderPath() const
  {
    return this->getBinFolderPath() + std::string("/data/");
  }

  void DataManager::parseConfigFile()
  {
    const std::string configFolder = this->getBinFolderPath()
                                     + std::string("/config/");
    const std::string configFilePath = configFolder
                                       + std::string("app.config");
    toml::value configData;
    try {
      configData = toml::parse(configFilePath);  
    } catch (std::runtime_error err) {
      throw utils::FileNotFoundError("The configuration file cannot be found.");
    }

    Config& config = this->config;
    config.addConfig("semester",
                     toml::find<const unsigned int>(configData, "semester"));
    config.addConfig("num_unique_days",
                     toml::find<const unsigned int>(configData,
                                                    "num_unique_days"));
    config.addConfig("days_with_double_timeslots",
                     toml::find<std::vector<unsigned int>>(
                       configData, "days_with_double_timeslots"));
    config.addConfig("num_timeslots",
                     toml::find<const unsigned int>(
                       configData, "num_timeslots"));
    config.addConfig("max_lecture_capacity",
                     toml::find<const unsigned int>(configData,
                                                    "max_lecture_capacity"));
    config.addConfig("max_lab_capacity",
                     toml::find<const unsigned int>(configData,
                                                    "max_lab_capacity"));
    config.addConfig("max_annual_teacher_load",
                     toml::find<const unsigned float>(
                       configData, "max_annual_teacher_load"));
    config.addConfig("max_semestral_teacher_load",
                     toml::find<const unsigned float>(
                       configData, "max_semestral_teacher_load"));
    config.addConfig("num_generations",
                     toml::find<const unsigned int>(configData,
                                                    "num_generations"));
    config.addConfig("num_offsprings_per_generation",
                     toml::find<const unsigned int>(
                       configData, "num_offsprings_per_generation"));
  }

  const std::unordered_set<ds::Course*>
  DataManager::getCoursesFromJSONArray(const json coursesJSON,
                                       const char* errorMsg)
  {
    return this->getDataFromJSONArray<ds::Course>(
      coursesJSON,
      errorMsg,
      [&] (const std::string courseName, const char* errorMsg) -> ds::Course*
      {
        return this->getCourseNameObject(courseName, errorMsg);
      });
  }

  const std::unordered_set<ds::RoomFeature*>
  DataManager::getRoomFeaturesFromJSONArray(const json roomFeaturesJSON,
                                            const char* errorMsg)
  {
    return this->getDataFromJSONArray<ds::RoomFeature>(
      roomFeaturesJSON,
      errorMsg,
      [&] (const std::string roomFeatureName, const char* errorMsg)
        -> ds::RoomFeature*
      {
        return this->getRoomFeatureObject(roomFeatureName, errorMsg);
      });
  }

  const std::unordered_set<ds::Teacher*>
  DataManager::getTeachersFromJSONArray(const json teachersJSON,
                                        const char* errorMsg)
  {
    return this->getDataFromJSONArray<ds::Teacher>(
      teachersJSON,
      errorMsg,
      [&] (const std::string teacherName, const char* _)
        -> ds::Teacher*
      {
        return this->getTeacherNameObject(teacherName);
      });
  }

  void DataManager::parseCoursesJSON()
  {
    const std::string coursesFileName = "courses.json";
    const std::string coursesFilePath = this->getDataFolderPath()
                                        + coursesFileName;
    std::ifstream coursesFile(coursesFilePath, std::ifstream::in);
    if (!coursesFile) {
      throw utils::FileNotFoundError("The Courses JSON file cannot be found.");
    }

    json courses;
    coursesFile >> courses;

    std::unordered_map<std::string, std::unordered_set<ds::Course*>>
      divisionCourses;
    std::unordered_map<std::string, std::unordered_set<ds::Course*>>
      teacherToPotentialCourseMap;

    for (const auto& [_, course] : courses.items()) {
      auto* const lecCoursePtr = this->createCourseObject(
        course, false, teacherToPotentialCourseMap);
      const bool hasLab = course["has_lab"].get<bool>();
      if (hasLab) {
        auto* const labCoursePtr = this->createCourseObject(
          course, true, teacherToPotentialCourseMap);
        this->courseToLabObject.insert({lecCoursePtr, labCoursePtr});
      }

      const std::string divisionName = course["division"];
      auto item = divisionCourses.find(divisionName);
      if (item == divisionCourses.end()) {
        divisionCourses.insert({ divisionName, {}});
      }

      divisionCourses[divisionName].insert(lecCoursePtr);
    }

    // Add the set of course pointers to the appropriate teachers.
    for (const auto [teacherName, courses] : teacherToPotentialCourseMap) {
      auto* const teacherPtr = this->getTeacherNameObject(teacherName);
      teacherPtr->setPotentialCourses(courses);
    }

    // Add the set of course pointers to the appropriate divisions.
    for (const auto [divisionName, courses] : divisionCourses) {
      auto* const divisionPtr = this->getDivisionNameObject(divisionName);
      divisionPtr->setCourses(courses);
    }
  }

  void DataManager::parseDivisionsJSON()
  {
    const std::string divisionsFileName = "divisions.json";
    const std::string divisionsFilePath = this->getDataFolderPath()
                                          + divisionsFileName;
    std::ifstream divisionsFile(divisionsFilePath, std::ifstream::in);
    if (!divisionsFile) {
      throw utils::FileNotFoundError("The Divisions JSON file cannot be "
                                     "found.");
    }

    json divisions;
    divisionsFile >> divisions;

    for (const auto& [_, division] : divisions.items()) {
      auto divisionPtr(std::make_unique<ds::Division>(division));
      this->divisionNameToObject.insert({division, divisionPtr.get()});
      this->divisions.insert(std::move(divisionPtr));
    }
  }

  void DataManager::parseRoomsJSON()
  {
    const std::string roomsFileName = "rooms.json";
    const std::string roomsFilePath = this->getDataFolderPath()
                                      + roomsFileName;

    std::ifstream roomsFile(roomsFilePath, std::ifstream::in);
    if (!roomsFile) {
      throw utils::FileNotFoundError("The Rooms JSON file cannot be found.");
    }

    json rooms;
    roomsFile >> rooms;

    std::unordered_map<std::string, std::unordered_set<ds::Room*>>
      divisionRooms;

    for (const auto& [_, room] : rooms.items()) {
      const std::string name = room["name"];
      const unsigned int capacity = room["capacity"].get<int>();
      const std::string divisionName = room["division"];
      auto* const divisionPtr = this->getDivisionNameObject(divisionName);

      std::unordered_set<ds::RoomFeature*> roomFeatures;
      for (const auto& featureName : room["features"]) {
        auto* const roomFeature = this->getRoomFeatureObject(featureName);
        roomFeatures.insert(roomFeature);
      }

      auto roomPtr(std::make_unique<ds::Room>(name, capacity, roomFeatures));
      this->roomNameToObject.insert({name, roomPtr.get()});

      auto item = divisionRooms.find(divisionName);
      if (item == divisionRooms.end()) {
        divisionRooms.insert({ divisionName, {}});
      }

      divisionRooms[divisionName].insert(roomPtr.get());

      this->rooms.insert(std::move(roomPtr));
    }

    // Add the set of room pointers to the appropriate divisions.
    for (const auto [divisionName, rooms] : divisionRooms) {
      auto* const divisionPtr = this->getDivisionNameObject(divisionName);
      divisionPtr->setRooms(rooms);
    }
  }

  void DataManager::parseRoomFeaturesJSON()
  {
    const std::string roomFeaturesFileName = "room_features.json";
    const std::string roomFeaturesFilePath = this->getDataFolderPath()
                                             + roomFeaturesFileName;
    std::ifstream roomFeaturesFile(roomFeaturesFilePath, std::ifstream::in);
    if (!roomFeaturesFile) {
      throw utils::FileNotFoundError("The Room Features JSON file cannot be "
                                     "found.");
    }

    json roomFeatures;
    roomFeaturesFile >> roomFeatures;

    for (const auto& [_, roomFeatureName] : roomFeatures.items()) {
      auto roomFeaturePtr(std::make_unique<ds::RoomFeature>(roomFeatureName));
      this->roomFeatureToObject.insert({roomFeatureName, roomFeaturePtr.get()});
      this->roomFeatures.insert(std::move(roomFeaturePtr));
    }
  }

  void DataManager::parseTeachersJSON()
  {
    const std::string teachersFileName = "teachers.json";
    const std::string teachersFilePath = this->getDataFolderPath()
                                         + teachersFileName;
    std::ifstream teachersFile(teachersFilePath, std::ifstream::in);
    if (!teachersFile) {
      throw utils::FileNotFoundError("The Teachers JSON file cannot be found.");
    }

    json teachers;
    teachersFile >> teachers;

    for (const auto& [_, teacher] : teachers.items()) {
      const std::string teacherName = teacher["name"];
      const unsigned int previousLoad = teacher["previous_load"].get<int>();

      std::unordered_set<
        ds::UnpreferredTimeslot, ds::UnpreferredTimeslotHashFunction>
          unpreferredTimeslots;

      const auto& unpreferredTimeslotsJSON = teacher["unpreferred_timeslots"];
      for (const auto& [_, ut] : unpreferredTimeslotsJSON.items()) {
        const unsigned int day = ut["day"].get<int>();
        const unsigned int timeslot = ut["timeslot"].get<int>();

        ds::UnpreferredTimeslot utObj{ day, timeslot };
        unpreferredTimeslots.insert(utObj);
      }

      auto teacherPtr{std::make_unique<ds::Teacher>(
        teacherName, previousLoad, unpreferredTimeslots)};
      this->teacherNameToObject.insert({teacherName, teacherPtr.get()});
      this->teachers.insert(std::move(teacherPtr));
    }
  }

  void DataManager::parseStudyPlansJSON(
      const unsigned int semester,
      std::unordered_map<std::pair<std::string, unsigned int>,
                         ds::StudentGroup*,
                         PairHash>& generatedStudentGroups,
      std::unordered_map<std::pair<std::string, unsigned int>,
                         std::unordered_set<ds::Course*>,
                         PairHash>& studyPlans)
  {
    const std::string studyPlanFileName = "study_plans.json";
    const std::string studyPlanFilePath = this->getDataFolderPath()
                                          + studyPlanFileName;
    
    std::ifstream studyPlanFile(studyPlanFilePath, std::ifstream::in);
    if (!studyPlanFile) {
      throw utils::FileNotFoundError("The Study Plans JSON file cannot be "
                                     "found.");
    }

    json studyPlan;
    studyPlanFile >> studyPlan;

    // Go through divisions first.
    for (const auto& [_, studyPlanItem] : studyPlan.items()) {
      const std::string divisionName = studyPlanItem["division_name"];
      std::unordered_set<ds::Course*> divisionCourses;
      std::unordered_set<ds::Degree*> divisionDegrees;

      // Now go through each degree offered by the division.
      const auto& divisionItemDegrees = studyPlanItem["degrees"];
      for (const auto& [_, degreeItem] : divisionItemDegrees.items()) {
        const std::string degreeName = degreeItem["degree_name"];
        std::unique_ptr<ds::Degree> degreePtr(
          std::make_unique<ds::Degree>(degreeName));
        divisionDegrees.insert(degreePtr.get());

        // Now go through each year level in the study plan of a degree.
        const auto& degreePlans = degreeItem["plans"];
        for (const auto& [_, planItem] : degreePlans.items()) {
          this->parsePlanFromStudyPlansJSON(planItem,
                                            semester,
                                            degreePtr.get(),
                                            divisionCourses,
                                            generatedStudentGroups,
                                            studyPlans);
        }

        this->degreeNameToObject.insert({degreeName, degreePtr.get()});
        this->degrees.insert(std::move(degreePtr));

        auto* const divisionPtr = this->getDivisionNameObject(divisionName);
        divisionPtr->setDegrees(divisionDegrees);
      }
    }
  }

  void DataManager::parseIrregularStudentGroupsJSON(
      const std::unordered_map<std::pair<std::string, unsigned int>,
                               std::unordered_set<ds::Course*>,
                               PairHash>& studyPlans)
  {
    const std::string jsonFileName = "irregular_student_groups.json";
    const std::string irregStudentGroupFilePath = this->getDataFolderPath()
                                                  + jsonFileName;
    std::ifstream irregStudentGroupFile(irregStudentGroupFilePath,
                                        std::ifstream::in);
    if (!irregStudentGroupFile) {
      throw utils::FileNotFoundError("The Irregular Student Groups JSON file "
                                     "cannot be found.");
    }

    json irregStudentGroups;
    irregStudentGroupFile >> irregStudentGroups;

    for (const auto& [_, group] : irregStudentGroups.items()) {
      const std::string degreeName = group["degree_name"];
      const unsigned int yearLevel = group["year_level"].get<int>();

      const auto studyPlanKey = std::make_pair(degreeName, yearLevel);
      const auto& studyPlanItem = studyPlans.find(studyPlanKey);
      if (studyPlanItem == studyPlans.end()) {
        std::cout << "Study plan with the following details:"
                  << "  Degree:\t" << degreeName
                  << "  Year Level:\t" << yearLevel
                  << "Skipping…"
                  << std::endl;
      } else {
        auto assignedCourses = studyPlanItem->second;
        const auto additionalCourses = group["additional_courses"];
        for (std::string additionalCourse : additionalCourses) {
          std::stringstream errorMsgStream;
          errorMsgStream << "Object for course, "
                         << additionalCourse
                         << ", cannot be found.";
          const char* errorMsg = (errorMsgStream.str()).c_str();
          ds::Course* course = this->getCourseNameObject(additionalCourse,
                                                         errorMsg);
          assignedCourses.insert(course);
        }

        std::unordered_set<ds::Course*> disallowedCourses{};
        const auto uncompletedCoursesJSON = group["uncompleted_courses"];
        auto uncompletedCourses = this->getCoursesFromJSONArray(
          uncompletedCoursesJSON);
        for (auto* const course : assignedCourses) {
          if (this->courseSetsHaveIntersections(&uncompletedCourses,
                                                &(course->prerequisites))) {
            // The current course cannot be enrolled by the irregular student
            // group since they do not have finished all of the required
            // prerequisites.
            disallowedCourses.insert(course);
          }
        }

        // Remove all courses that cannot be enrolled by the student group from
        // the list of assigned courses for the group.
        for (auto* const course : disallowedCourses) {
          assignedCourses.erase(course);
        }

        ds::Degree* degree = this->getDegreeNameObject(degreeName);
        auto irregSgPtr(std::make_unique<ds::StudentGroup>(
          this->getNewStudentGroupID(), degree, yearLevel, assignedCourses));

        const unsigned int numMembers = group["num_members"].get<int>();
        irregSgPtr->setNumMembers(numMembers);

        this->studentGroups.insert(std::move(irregSgPtr));
      }
    }
  }

  void DataManager::parseRegularStudentGroupsGEsElectivesJSON(
      const std::unordered_map<std::pair<std::string, unsigned int>,
                               ds::StudentGroup*,
                               PairHash>& generatedStudentGroups)
  {
    const std::string jsonFileName = "regular_student_ges_electives.json";
    const std::string regGEsElectivesFilePath = this->getDataFolderPath()
                                                + jsonFileName;
    std::ifstream regGEsElectivesFile(regGEsElectivesFilePath,
                                      std::ifstream::in);
    if (!regGEsElectivesFile) {
      throw utils::FileNotFoundError("The GEs and Electives (Regular) JSON "
                                     "file cannot be found.");
    }

    json regGEsElectives;
    regGEsElectivesFile >> regGEsElectives;

    for (const auto& [_, group] : regGEsElectives.items()) {
      const std::string parentDegreeName = group["degree_name"];
      const unsigned int parentYearLevel = group["year_level"].get<int>();
      const unsigned int numMembers = group["num_members"].get<int>();

      const auto& sgItem = generatedStudentGroups.find(
        std::make_pair(parentDegreeName, parentYearLevel));
      if (sgItem == generatedStudentGroups.end()) {
        std::cout << "While processing GEs and electives, encountered a "
                  << "non-existing student group, with the following "
                  << "details:"
                  << "  Degree:\t" << parentDegreeName
                  << "  Year Level:\t" << parentYearLevel
                  << "Skipping…"
                  << std::endl;
      } else {
        const auto& courses = group["courses"];
        const auto assignedCourses = this->getCoursesFromJSONArray(courses);

        ds::StudentGroup* parentGroup = sgItem->second;
        parentGroup->addSubGroup(assignedCourses, numMembers);
      }
    }
  }

  void DataManager::parseStudentGroupsJSON(
      const std::unordered_map<std::pair<std::string, unsigned int>,
                               ds::StudentGroup*,
                               utils::PairHash>& generatedStudentGroups)
  {
    const std::string studentGroupsFileName = "student_groups.json";
    const std::string studentGroupsFilePath = this->getDataFolderPath()
                                              + studentGroupsFileName;
    std::ifstream studentGroupsFile(studentGroupsFilePath, std::ifstream::in);
    if (!studentGroupsFile) {
      throw utils::FileNotFoundError("The Student Groups JSON file cannot "
                                     "be found.");
    }

    json studentGroups;
    studentGroupsFile >> studentGroups;

    for (const auto& [_, studentGroup] : studentGroups.items()) {
      const std::string degreeName = studentGroup["degree_name"];
      const unsigned int yearLevel = studentGroup["year_level"].get<int>();
      const unsigned int numMembers = studentGroup["num_members"].get<int>();

      const auto& sgItem = generatedStudentGroups.find(
        std::make_pair(degreeName, yearLevel));
      if (sgItem == generatedStudentGroups.end()) {
        std::cout << "Encountered a non-existing student group, with the"
                  << "following details:"
                  << "  Degree:\t" << degreeName
                  << "  Year Level:\t" << yearLevel
                  << "Skipping…"
                  << std::endl;
      } else {
        ds::StudentGroup* sg = sgItem->second;
        sg->setNumMembers(numMembers);
      }
    }
  }

  void DataManager::parsePlanFromStudyPlansJSON(
      const json planItemJSON,
      const unsigned int semester,
      ds::Degree* const degree,
      std::unordered_set<ds::Course*>& divisionCourses,
      std::unordered_map<std::pair<std::string, unsigned int>,
                         ds::StudentGroup*,
                         PairHash>& generatedStudentGroups,
      std::unordered_map<std::pair<std::string, unsigned int>,
                         std::unordered_set<ds::Course*>,
                         PairHash>& studyPlans)
  {
    // NOTE: Plans must be sorted ascendingly by year level, then by
    //       semester.
    const int yearLevel = planItemJSON["year_level"].get<int>();

    auto key = std::make_pair(degree->name, yearLevel);
    studyPlans.insert({key, {}});

    const int planSemester = planItemJSON["semester"].get<int>();

    if (planSemester != semester) {
      // If the plan is assigned to a semester that we are not currently
      // scheduling, then we should just skip.
      return;
    }

    const auto& degreeItemCourses = planItemJSON["courses"];
    std::unordered_set<ds::Course*> planCourses;
    for (const auto& [_, courseName] : degreeItemCourses.items()) {
      const char* errorMsg = "Referenced a non-existent course.";
      ds::Course* course = this->getCourseNameObject(courseName, errorMsg);
      planCourses.insert(course);
      divisionCourses.insert(course);
      studyPlans[key].insert(course);
    }

    // Create a StudentGroup. Similar to the course object, we have to
    // make sure that there is only one copy for each StudentGroup object.
    auto sgPtr(std::make_unique<ds::StudentGroup>(this->getNewStudentGroupID(),
                                                  degree,
                                                  yearLevel,
                                                  planCourses));
    generatedStudentGroups.insert({
      std::make_pair(degree->name, yearLevel), sgPtr.get()
    });
    this->studentGroups.insert(std::move(sgPtr));
  }

  ds::Course* const DataManager::createCourseObject(
      const json courseJSON,
      const bool isLab,
      std::unordered_map<std::string, std::unordered_set<ds::Course*>>&
        teacherToPotentialCourseMap)
  {
    const std::string courseName = courseJSON["course_name"];

    const std::string divisionName = courseJSON["division"];
    ds::Division* division = this->getDivisionNameObject(divisionName);

    const bool hasLab = (isLab) ? false : courseJSON["has_lab"].get<bool>();

    const char* timeslotsKey = (isLab) ? "num_lab_timeslots" : "num_timeslots";
    const unsigned int numTimeslots = courseJSON[timeslotsKey].get<int>();

    const char* numUnitsKey = (isLab) ? "num_lab_units" : "num_units";
    const unsigned float numUnits = courseJSON[numUnitsKey].get<float>();

    std::unordered_set<ds::Course*> coursePrereqs;
    if (!isLab) {
      const auto& prereqsJSON = courseJSON["prerequisites"];
      coursePrereqs = this->getCoursesFromJSONArray(prereqsJSON);
    }

    const auto& candidateTeachersJSON = courseJSON["candidate_teachers"];
    const auto candidateTeachers = this->getTeachersFromJSONArray(
      candidateTeachersJSON);
    
    const char* roomReqsKey = (isLab) ? "lab_requirements"
                                      : "room_requirements";
    const auto& roomReqsJSON = courseJSON[roomReqsKey];
    const auto roomReqs = this->getRoomFeaturesFromJSONArray(roomReqsJSON);

    // Create a course object. Make sure that we only have one copy of
    // the newly generated course object throughout the program.
    auto coursePtr(std::make_unique<ds::Course>(courseName,
                                                division,
                                                hasLab,
                                                isLab,
                                                numTimeslots,
                                                numUnits,
                                                coursePrereqs,
                                                candidateTeachers,
                                                roomReqs));
    ds::Course* const outputCoursePtr = coursePtr.get();
    this->courseNameToObject.insert({courseName, coursePtr.get()});
    this->courses.insert(std::move(coursePtr)); 

    for (const auto& [_, teacherName] : candidateTeachersJSON.items()) {
      auto item = teacherToPotentialCourseMap.find(teacherName);
      if (item == teacherToPotentialCourseMap.end()) {
        teacherToPotentialCourseMap[teacherName] = {};
      }

      teacherToPotentialCourseMap[teacherName].insert(outputCoursePtr);
    }

    return outputCoursePtr;
  }

  bool DataManager::courseSetsHaveIntersections(
      const std::unordered_set<ds::Course*>* set0,
      const std::unordered_set<ds::Course*>* set1)
  {
    const std::unordered_set<ds::Course*>* tempSet;
    if (set0->size() > set1->size()) {
      tempSet = set0;
      set0 = set1;
      set1 = tempSet;
    }

    for (auto i = set0->begin(); i != set0->end(); i++) {
      if (set1->find(*i) != set1->end()) {
        return true;
      }
    }

    return false;
  }
}