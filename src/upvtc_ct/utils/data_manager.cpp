#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>

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

  DataManager::DataManager()
    : config(this->getConfigData())
  {
    ds::Config config = this->getConfig();
    const unsigned int semester = config.get<int>("semester");

    this->parseRoomFeaturesJSON();
    this->parseCoursesJSON();

    std::unordered_map<std::pair<std::string, unsigned int>,
                       ds::StudentGroup*,
                       PairHash> generatedStudentGroups;
    this->parseStudyPlansJSON(semester, generatedStudentGroups);
    this->parseStudentGroupsJSON(generatedStudentGroups);
    this->parseRegularStudentGroupsGEsElectivesJSON(generatedStudentGroups);
  }

  const ds::Config& DataManager::getConfig()
  {
    return this->config;
  }

  const std::unordered_set<std::unique_ptr<ds::Course>>&
  DataManager::getCourses()
  {
    return this->courses;
  }

  const std::unordered_set<std::unique_ptr<ds::Class>>&
  DataManager::getClasses()
  {
    return this->classes;
  }

  const std::unordered_set<std::unique_ptr<ds::Degree>>&
  DataManager::getDegrees()
  {
    return this->degrees;
  }

  const std::unordered_set<std::unique_ptr<ds::Division>>&
  DataManager::getDivisions()
  {
    return this->divisions;
  }

  const std::unordered_set<std::unique_ptr<ds::StudentGroup>>&
  DataManager::getStudentGroups()
  {
    return this->studentGroups;
  }

  ds::Course* const DataManager::getCourseNameObject(
      const std::string courseName,
      const char* errorMsg)
  {
    auto courseItem = this->courseNameToObject.find(courseName);
    if (courseItem == this->courseNameToObject.end()) {
      throw utils::InvalidContentsError(errorMsg);
    }

    return courseItem->second;
  }

  const std::unordered_set<std::unique_ptr<ds::RoomFeature>>&
  DataManager::getRoomFeatures()
  {
    return this->roomFeatures;
  }

  ds::RoomFeature* const DataManager::getRoomFeatureObject(
        const std::string roomFeatureName,
        const char* errorMsg)
  {
    auto roomFeatureItem = this->roomFeatureToObject.find(roomFeatureName);
    if (roomFeatureItem == this->roomFeatureToObject.end()) {
      throw utils::InvalidContentsError(errorMsg);
    }

    return roomFeatureItem->second;
  }

  const std::unordered_map<size_t, std::unordered_set<ds::Class*>>&
  DataManager::getClassGroups()
  {
    return this->classGroups;
  }

  const std::unordered_map<size_t, std::unordered_set<size_t>>&
  DataManager::getClassConflicts()
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

  const std::unordered_map<std::string, std::string>
  DataManager::getConfigData()
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

    return std::unordered_map<std::string, std::string>({
      {
        "semester",
        std::to_string(toml::find<int>(configData, "semester"))
      },
      {
        "max_lecture_capacity",
        std::to_string(toml::find<int>(configData, "max_lecture_capacity"))
      },
      {
        "max_lab_capacity",
        std::to_string(toml::find<int>(configData, "max_lab_capacity"))
      },
      {
        "max_annual_teacher_load",
        std::to_string(toml::find<int>(configData, "max_annual_teacher_load"))
      },
      {
        "max_semestral_teacher_load",
        std::to_string(
          toml::find<int>(configData, "max_semestral_teacher_load"))
      }
    });
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

    for (const auto& [_, course] : courses.items()) {
      const std::string courseName = course["course_name"];
      const bool hasLab = course["has_lab"].get<bool>();
      const unsigned int numTimeslots = course["num_timeslots"].get<int>();

      const auto& prereqsJSON = course["prerequisites"];
      const auto coursePrereqs = this->getCoursesFromJSONArray(prereqsJSON);
      
      const auto& roomReqsJSON = course["room_requirements"];
      const auto roomReqs = this->getRoomFeaturesFromJSONArray(roomReqsJSON);

      // Create a course object. Make sure that we only have one copy of
      // the newly generated course object throughout the program.
      auto coursePtr(std::make_unique<ds::Course>(courseName,
                                                  hasLab,
                                                  numTimeslots,
                                                  coursePrereqs,
                                                  roomReqs));
      this->courseNameToObject.insert({courseName, coursePtr.get()});
      this->courses.insert(std::move(coursePtr));
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

  void DataManager::parseStudyPlansJSON(
      const unsigned int semester,
      std::unordered_map<std::pair<std::string, unsigned int>,
                         ds::StudentGroup*,
                         PairHash>& generatedStudentGroups)
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
                                            generatedStudentGroups);
        }

        this->degrees.insert(std::move(degreePtr));
      }

      std::unique_ptr<ds::Division> divisionPtr(
        new ds::Division(divisionName, divisionCourses, divisionDegrees, {}));
      this->divisions.insert(std::move(divisionPtr));
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
                         PairHash>& generatedStudentGroups)
  {
    // NOTE: Plans must be sorted ascendingly by year level, then by
    //       semester.
    const int yearLevel = planItemJSON["year_level"].get<int>();
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
    }

    // Create a StudentGroup. Similar to the course object, we have to
    // make sure that there is only one copy for each StudentGroup object.
    auto sgPtr(std::make_unique<ds::StudentGroup>(degree,
                                                  yearLevel,
                                                  planCourses));
    generatedStudentGroups.insert({
      std::make_pair(degree->name, yearLevel), sgPtr.get()
    });
    this->studentGroups.insert(std::move(sgPtr));
  }
}