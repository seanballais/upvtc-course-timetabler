#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <upvtc_ct/timetabler/timetabler.hpp>

namespace upvtc_ct::timetabler
{
  Timetabler::Timetabler(utils::DataManager& dataManager)
    : dataManager(dataManager) {}

  Solution Timetabler::generateRandomSolution()
  {
    std::vector<size_t> classGroups;
    std::unordered_map<size_t, std::unordered_set<ds::Class>>
      classGroupsToClassesMap
    for (const auto [clsGroup, classes] : this->dataManager.getClassGroups()) {
      classGroups.insert(clsGroup);
      classGroupsToClassesMap.insert({clsGroup, classes});
    }

    return Solution{classGroups, classGroupsToClassesMap};
  }

  Solution::Solution(
      const std::vector<size_t> classGroups,
      const std::unordered_map<size_t, std::unordered_set<ds::Class>>
        classGroupsToClassesMap)
    : classGroups(classGroups)
    , classGroupsToClassesMap(classGroupsToClassesMap) {}

  std::vector<size_t>& Solution::getClassGroups() const
  {
    return this->classGroups;
  }

  std::unordered_set<ds::Class>& Solution::getClasses(size_t classGroup) const
  {
    auto item = this->classGroupsToClassesMap.find(classGroup);
    if (item == this->classGroupsToClassesMap.end()) {
      std::stringstream errorMsgStream;
      errorMsgStream << "Attempted to obtain classes using an unknown class "
                     << "group, " << classGroup << ".";
      const char* errorMsg = (errorMsgStream.str()).c_str();
      throw UnknownClassGroupError(errorMsg);
    }

    return item->second;
  }

  void Solution::changeClassTeacher(const size_t classGroup,
                                    ds::Teacher* const teacher)
  {
    auto& classes = this->getClasses(classGroup);
    for (ds::Class& cls : classes) {
      cls.teacher = teacher;
    }
  }

  void Solution::changeClassDay(const size_t classGroup, const unsigned int day)
  {
    auto& classes = this->getClasses(classGroup);
    for (ds::Class& cls : classes) {
      cls.day = day;
    }
  }

  void Solution::changeClassRoom(const size_t classGroup, ds::Room* const room)
  {
    auto& classes = this->getClasses(classGroup);
    for (ds::Class& cls : classes) {
      cls.room = room;
    }
  }

  void Solution::changeClassTimeslot(const size_t classGroup,
                                     const unsigned int timeslot)
  {
    auto& classes = this->getClasses(classGroup);
    for (ds::Class& cls : classes) {
      cls.timeslot = timeslot;
    }
  }

  UnknownClassGroupError::UnknownClassGroupError(const char* what_arg)
    : std::runtime_error(what_arg) {}
}
