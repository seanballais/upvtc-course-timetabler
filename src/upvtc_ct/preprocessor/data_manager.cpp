#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>

#include <limits.h>
#include <unistd.h>

#include <nlohmann/json.hpp>

#include <upvtc_ct/preprocessor/data_manager.hpp>
#include <upvtc_ct/ds/models.hpp>
#include <upvtc_ct/utils/errors.hpp>
#include <upvtc_ct/utils/hash_specializations.hpp>

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

    std::unordered_map<std::string, ds::Course*> courseNameToObject;
    std::unordered_map<std::pair<std::string, unsigned int>,
                       ds::StudentGroup*,
                       utils::PairHash>
      generatedStudentGroups;

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
          // NOTE: Plans must be sorted ascendingly by year level, then by
          //       semester.
          const int yearLevel = planItem["year_level"].get<int>();
          const int planSemester = planItem["semester"].get<int>();

          if (planSemester != semester) {
            // If the plan is assigned to a semester that we are not curerntly
            // scheduling, then we should just skip.
            continue;
          }

          const auto& degreeItemCourses = planItem["courses"];
          std::unordered_set<ds::Course*> planCourses;
          for (const auto& [_, courseItem] : degreeItemCourses.items()) {
            const std::string courseName = courseItem["course_name"];
            std::unordered_set<ds::Course*> coursePrereqs;

            const auto& prerequisites = courseItem["prerequisites"];
            for (const auto& [_, prereq] : prerequisites.items()) {
              auto prereqCourseItem = courseNameToObject.find(prereq);
              if (prereqCourseItem == courseNameToObject.end()) {
                throw utils::InvalidContentsError(
                  "Referenced another course that was not yet generated. "
                  "Please check your Study Plans JSON file.");
              }

              ds::Course* prereqCourse = prereqCourseItem->second;
              coursePrereqs.insert(prereqCourse);
            }

            // Create a course object. Make sure that we only have one copy of
            // the newly generated course object throughout the program.
            std::unique_ptr<ds::Course> coursePtr(
              std::make_unique<ds::Course>(courseName, coursePrereqs));
            planCourses.insert(coursePtr.get());
            divisionCourses.insert(coursePtr.get());
            courseNameToObject.insert({courseName, coursePtr.get()});
            this->courses.insert(std::move(coursePtr));
          }

          // Create a StudentGroup. Similar to the course object, we have to
          // make sure that there is only one copy for each StudentGroup object.
          auto sgPtr(std::make_unique<ds::StudentGroup>(degreePtr.get(),
                                                        yearLevel,
                                                        planCourses));
          generatedStudentGroups.insert({
            std::make_pair(degreeName, yearLevel), std::move(sgPtr.get())
          });
          this->studentGroups.insert(std::move(sgPtr));
        }

        this->degrees.insert(std::move(degreePtr));
      }

      std::unique_ptr<ds::Division> divisionPtr(
        new ds::Division(divisionName, divisionCourses, divisionDegrees, {}));
      this->divisions.insert(std::move(divisionPtr));
    }

    // Parse the student_groups.json file.
    const std::string studentGroupsFilePath = 
      dataFolder + std::string("student_groups.json");
    std::ifstream studentGroupsFile(studentGroupsFilePath, std::ifstream::in);
    if (!studentGroupsFile) {
      throw utils::FileNotFoundError(
        "The Student Groups JSON file cannot be found.");
    }

    json studentGroups;
    studentGroupsFile >> studentGroups;

    for (const auto& [_, studentGroup] : studentGroups.items()) {
      std::string degreeName = studentGroup["degree_name"];
      unsigned int yearLevel = studentGroup["year_level"].get<int>();
      unsigned int numMembers = studentGroup["num_members"].get<int>();

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

  const std::unordered_set<std::unique_ptr<ds::Course>>&
  DataManager::getCourses()
  {
    return this->courses;
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