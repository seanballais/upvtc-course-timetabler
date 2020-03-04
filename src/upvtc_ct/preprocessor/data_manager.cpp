#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_set>

#include <limits.h>
#include <unistd.h>

#include <nlohmann/json.hpp>

#include <upvtc_ct/preprocessor/data_manager.hpp>
#include <upvtc_ct/ds/models.hpp>
#include <upvtc_ct/utils/errors.hpp>

namespace upvtc_ct::preprocessor
{
  using json = nlohmann::json;
  namespace ds = upvtc_ct::ds;
  namespace utils = upvtc_ct::utils;

  DataManager::DataManager(const unsigned int semester)
  {
    const std::string dataFolder = this->getBinFolderPath()
                                   + std::string("/data/");
    const std::string studyPlanFilePath = dataFolder
                                          + std::string("study_plans.json");
    
    std::ifstream studyPlanFile(studyPlanFilePath, std::ifstream::in);
    if (!studyPlanFile) {
      throw utils::FileNotFoundError(
        "The Study Plans JSON file cannot be found.");
    }

    json studyPlan;
    studyPlanFile >> studyPlan;

    // Go through divisions first.
    for (const auto& [_, studyPlanItem] : studyPlan.items()) {
      const std::string divisionName = studyPlanItem["division_name"];

      // Now go through each degree offered by the division.
      const auto& divisionDegrees = studyPlanItem["degrees"];
      for (const auto& [_, degreeItem] : divisionDegrees.items()) {
        const std::string degreeName = degreeItem["degree_name"];

        // Now go through each year level in the study plan of a degree.
        const auto& degreePlans = degreeItem["plans"];
        for (const auto& [_, planItem] : degreePlans.items()) {
          const int yearLevel = planItem["year_level"].get<int>();
          const int semester = planItem["semester"].get<int>();

          const auto& degreeCourses = planItem["courses"];
          for (const auto& [_, courseItem] : degreeCourses.items()) {
            const std::string courseName = courseItem["course_name"];

            const auto& prerequisites = courseItem["prerequisites"];
            for (const auto& [_, prereq] : prerequisites.items()) {
              std::cout << prereq << std::endl;
            }
          }
        }
      }
    }
  }

  const std::unordered_set<ds::Course, ds::CourseHashFunction>&
  DataManager::getCourses()
  {
    return this->courses;
  }

  const std::unordered_set<ds::Division, ds::DivisionHashFunction>&
  DataManager::getDivisions()
  {
    return this->divisions;
  }

  const std::unordered_set<ds::StudentGroup, ds::StudentGroupHashFunction>&
  DataManager::getStudentGroups()
  {
    return this->studentGroups;
  }

  std::string DataManager::getBinFolderPath() const
  {
    // Note that this function is only guaranteed to work in Linux. Please refer
    // to the linkL
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
}