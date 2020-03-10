#include <cmath>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <upvtc_ct/ds/models.hpp>
#include <upvtc_ct/preprocessor/preprocessor.hpp>
#include <upvtc_ct/utils/data_manager.hpp>
#include <upvtc_ct/utils/errors.hpp>

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
    this->identifyClassConflicts();
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
    const unsigned int maxLecCapacity = config.get<int>("max_lecture_capacity");

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

        const auto item = this->courseNameToClassGroupsMap.find(course->name);
        if (item == this->courseNameToClassGroupsMap.end()) {
          this->courseNameToClassGroupsMap[course->name] = {};
        }

        this->courseNameToClassGroupsMap[course->name].insert(classID);

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

  void Preprocessor::identifyClassConflicts()
  {
    for (const auto& group : this->dataManager->getStudentGroups()) {
      for (int i = 0; i < group->getNumMembers(); i++) {
        std::vector<size_t> studentClassGroups;
        for (const auto& course : group->assignedCourses) {
          // Pick a class group of a course to assign the student to.
          size_t studentClassGroup = this->selectClassGroup(course->name);
          studentClassGroups.push_back(studentClassGroup);
        }

        // All class groups inside studentClassGroupsare are now known to be in
        // conflict with one another. Save that information.
        for (const size_t targetGroup : studentClassGroups) {
          for (const size_t conflictedGroup : studentClassGroups) {
            // Skip if ever we are about to set a class group to be in conflict
            // with itself.
            if (targetGroup != conflictedGroup) {
              this->dataManager->addClassConflict(targetGroup, conflictedGroup);
            }
          }
        }
      }
    }
  }

  size_t Preprocessor::selectClassGroup(std::string courseName)
  {
    ds::Config config = this->dataManager->getConfig();
    const unsigned int maxLecCapacity = config.get<int>("max_lecture_capacity");

    std::unordered_set<size_t>
      candidates = this->courseNameToClassGroupsMap[courseName];
    for (const size_t candidate : candidates) {
      const auto item = this->classGroupSizes.find(candidate);
      if (item == this->classGroupSizes.end()) {
        this->classGroupSizes[candidate] = 0;
      }

      unsigned int classGroupSize = this->classGroupSizes[candidate];
      if (classGroupSize < maxLecCapacity) {
        // We can still fit another student to the class group.
        this->classGroupSizes[candidate]++;
        return candidate;
      }
    }

    // Huh. We are somehow unable to fit all students into the classes. Weird.
    throw utils::MaxClassCapacityError(
      (std::string("Not enough classes for ") + courseName).c_str());
  }
}
