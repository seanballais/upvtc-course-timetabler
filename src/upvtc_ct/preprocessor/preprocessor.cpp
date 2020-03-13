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
    // Generate the classes.
    ds::Config config = dataManager->getConfig();
    const unsigned int maxLecCapacity = config.get<int>("max_lecture_capacity");
    const auto numCourseEnrolleesMap = this->getNumEnrolleesPerCourse();

    size_t numClassesGenerated = 0;
    for (const auto item : numCourseEnrolleesMap) {
      int numCourseEnrollees = item.second;
      int numClasses = ceil(static_cast<float>(numCourseEnrollees)
                            / static_cast<float>(maxLecCapacity));      
      const char* errorMsg = "Referenced a non-existent course.";
      ds::Course* course = this->dataManager
                               ->getCourseNameObject(item.first, errorMsg);
      for (int i = 0; i < numClasses; i++) {
        size_t classID = std::hash<std::string>()(
          item.first + std::to_string(i));
        this->generateClassGroup(numClassesGenerated, classID, course);        
        numClassesGenerated += 3; // Assume a course needs 3 timeslots, for now.
      }
    }
  }

  void Preprocessor::identifyClassConflicts()
  {
    for (const auto& group : this->dataManager->getStudentGroups()) {
      // TODO: Throw an exception if the total number of members in the
      //       subgroups are less than or equal to the number of number of
      //       members in the group.
      unsigned int numSubGroupMembers = 0;
      for (const auto& subGroup : group->getSubGroups()) {
        numSubGroupMembers += subGroup->getNumMembers();
      }

      // Let's deal with the regular members first. Regular members are members
      // of the group that do not belong to any sub-student groups, i.e. those
      // that follow only the study plan.
      const
      unsigned int numRegMembers = group->getNumMembers() - numSubGroupMembers;
      for (int i = 0; i < numRegMembers; i++) {
        this->identifyGroupMemberClassConflicts(*group);
      }

      // Now, let's deal with the members that are part of sub-student groups.
      for (const auto& subgroup : group->getSubGroups()) {
        for (int i = 0; i < subgroup->getNumMembers(); i++) {
          this->identifyGroupMemberClassConflicts(*subgroup,
                                                  group->assignedCourses);
        }
      }
    }
  }

  std::unordered_map<std::string, int> Preprocessor::getNumEnrolleesPerCourse()
  {
    std::unordered_map<std::string, int> numCourseEnrolleesMap;
    for (const auto& group : this->dataManager->getStudentGroups()) {
      // Get all courses that are going to be attended by members of the group.
      std::vector<ds::Course*> assignedCourses;
      
      // Go through the courses in the study plan first.
      for (ds::Course* coursePtr : group->assignedCourses) {
        assignedCourses.push_back(coursePtr);
      }

      // And then the assigned courses of each sub-student groups.
      for (const auto& subGroup : group->getSubGroups()) {
        for (ds::Course* coursePtr : subGroup->assignedCourses) {
          assignedCourses.push_back(coursePtr);
        }
      }

      for (const ds::Course* course : assignedCourses) {
        auto numCourseEnrolleesItem = numCourseEnrolleesMap.find(course->name);
        if (numCourseEnrolleesItem != numCourseEnrolleesMap.end()) {
          numCourseEnrolleesMap[course->name] += group->getNumMembers();
        } else {
          numCourseEnrolleesMap[course->name] = group->getNumMembers();
        }
      }
    }

    return numCourseEnrolleesMap;
  }

  void Preprocessor::generateClassGroup(const unsigned int idStart,
                                        size_t classID,
                                        ds::Course* course)
  {
    const auto item = this->courseNameToClassGroupsMap.find(course->name);
    if (item == this->courseNameToClassGroupsMap.end()) {
      this->courseNameToClassGroupsMap[course->name] = {};
    }

    this->courseNameToClassGroupsMap[course->name].insert(classID);

    // Assume for now that a course requires three timeslots.
    const unsigned int numTimeslots = 3;
    for (int ctr = 0; ctr < numTimeslots; ctr++) {
      std::unique_ptr<ds::Class> clsPtr(
      std::make_unique<ds::Class>(idStart + ctr, classID, course, nullptr,
                                  0, nullptr, 0));
      this->dataManager->addClass(std::move(clsPtr));
    }
  }

  void Preprocessor::identifyGroupMemberClassConflicts(
      const ds::BaseStudentGroup& group)
  {
    this->identifyGroupMemberClassConflicts(group, {});
  }

  void Preprocessor::identifyGroupMemberClassConflicts(
      const ds::BaseStudentGroup& group,
      const std::unordered_set<ds::Course*>& additionalCourses)
  {
    // Get all the courses that a student of a group will be enrolled in.
    std::unordered_set<ds::Course*> assignedCourses;
    for (const auto& course : group.assignedCourses) {
      assignedCourses.insert(course);
    }

    for (const auto& course : additionalCourses) {
      assignedCourses.insert(course);
    }

    // Identify the class conflicts.
    std::vector<size_t> studentClassGroups;
    for (const auto& course : assignedCourses) {
      // Pick a class group of a course to assign the student to.
      size_t studentClassGroup = this->selectClassGroup(course->name);
      studentClassGroups.push_back(studentClassGroup);
    }

    // All class groups inside studentClassGroupsare are now known to be
    // in conflict with one another. Save that information.
    for (const size_t targetClassGroup : studentClassGroups) {
      for (const size_t conflictedGroup : studentClassGroups) {
        // Skip if ever we are about to set a class group to be in
        // conflict with itself.
        if (targetClassGroup != conflictedGroup) {
          this->dataManager->addClassConflict(targetClassGroup,
                                              conflictedGroup);
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
