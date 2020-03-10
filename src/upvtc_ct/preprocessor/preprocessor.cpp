#include <cmath>
#include <functional>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include <upvtc_ct/ds/models.hpp>
#include <upvtc_ct/preprocessor/preprocessor.hpp>
#include <upvtc_ct/utils/data_manager.hpp>

namespace upvtc_ct::preprocessor
{
  namespace ds = upvtc_ct::ds;
  namespace utils = upvtc_ct::utils;

  // We do not want to copy a DataManager instance, because there will be
  // redundancies. We can just use a pointer to an instance of it instead.
  Preprocessor::Preprocessor(utils::DataManager* const dataManager)
    : dataManager(dataManager) {}

  void Preprocessor::preprocess()
  {
    this->generateClasses();
  }

  void Preprocessor::generateClasses()
  {
    std::unordered_map<std::string, int> numCourseEnrolleesMap;
    for (const auto& group : this->dataManager->getStudentGroups()) {
      for (const ds::Course* course : group->assignedCourses) {
        auto numCourseEnrolleesItem = numCourseEnrolleesMap.find(course->name);
        if (numCourseEnrolleesItem != numCourseEnrolleesMap.end()) {
          numCourseEnrolleesMap[course->name] += group->getNumMembers();
        } else {
          numCourseEnrolleesMap[course->name] = group->getNumMembers();
        }
      }
    }

    // Generate the classes.
    ds::Config config = dataManager->getConfig();
    unsigned int maxLecCapacity = config.get<int>("max_lecture_capacity");

    size_t numClassesGenerated = 0;
    for (const auto item : numCourseEnrolleesMap) {
      int numCourseEnrollees = item.second;
      int numClasses = static_cast<int>(
        ceil(
          static_cast<float>(numCourseEnrollees)
          / static_cast<float>(maxLecCapacity)));
      
      const char* errorMsg = "Referenced a non-existent course.";
      ds::Course* course = this->dataManager
                               ->getCourseNameObject(item.first, errorMsg);
      for (int i = 0; i < numClasses; i++) {
        size_t classID = std::hash<std::string>()(
          item.first + std::to_string(i));

        // Assume for now that a course requires three timeslots.
        for (int ctr = 0; ctr < 3; ctr++) {
          std::unique_ptr<ds::Class> clsPtr(
            std::make_unique<ds::Class>(
              numClassesGenerated, classID, course, nullptr, 0, nullptr, 0));
          this->dataManager->addClass(std::move(clsPtr));

          numClassesGenerated++;
        }
      }
    }


  }
}
