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

namespace upvtc_ct::preprocessor
{
  using json = nlohmann::json;
  namespace ds = upvtc_ct::ds;

  DataManager::DataManager(const unsigned int semester)
  {
    const std::string dataFolder = this->getBinFolderPath()
                                   + std::string("/data/");
    const std::string studyPlanFilePath = dataFolder
                                          + std::string("study_plan.json");
    
    std::ifstream studyPlanFile(studyPlanFilePath, std::ifstream::in);
    json studyPlan;
    studyPlanFile >> studyPlan;

    // Go through divisions first.
    for (const auto& divisionItem : studyPlan.items()) {
      const std::string divisionName = divisionItem.key();
      //ds::Division division = ds::Division(divisionName);

      // Now go through each degree offered by the division.
      for (const auto& degreeItem : divisionItem.value()["degrees"].items()) {
        const std::string degreeName = degreeItem.key();
        //ds::Degree degree = ds::Degree(degreeName);

        // Now go through each year level in the study plan of a degree.
        for (const auto& yearLevelItem : degreeItem.value().items()) {
          const int yearLevel = std::stoi(yearLevelItem.key());

          // Now go through each semester of a year level.
          for (const auto& semesterItem : yearLevelItem.value().items()) {
            const int semester = std::stoi(semesterItem.key());

            for (const auto& courseItem
                  : semesterItem.value()["courses"].items()) {
              const std::string courseName = courseItem.key();
              std::unordered_set<ds::Course, ds::CourseHashFunction> prereqs;

              for (const auto& prereq : courseItem.value()["prerequisites"]) {
                //ds::Course
              }
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