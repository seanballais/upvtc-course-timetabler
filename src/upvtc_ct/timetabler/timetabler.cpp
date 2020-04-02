#include <random>
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
    std::unordered_map<size_t, std::unordered_set<ds::Class*>>
      classGroupsToClassesMap;
    for (const auto item : this->dataManager.getClassGroups()) {
      auto clsGroup = item.first;
      auto classes = item.second;
      classGroups.push_back(clsGroup);
      classGroupsToClassesMap.insert({clsGroup, classes});
    }

    return Solution{classGroups, classGroupsToClassesMap};
  }

  void Timetabler::applySimpleMove(Solution& solution)
  {

  }

  void Timetabler::applySimpleSwap(Solution& solution)
  {
    std::vector<size_t>& classGroups = solution.getClassGroups();

    size_t numClassGroups = classGroups.size();
    std::uniform_real_distribution<size_t> distribution(0, numClassGroups - 1);

    std::random_device randDevice;
    std::mt19937 mt{randDevice{}};

    size_t solutionAIndex = distribution(mt);

    // Make sure solutionBIndex is not the same as solutionAIndex.
    size_t solutionBIndex;
    do {
      solutionBIndex = distribution(mt);
    } while (solutionBIndex == solutionAIndex);

    Solution& solutionA = classGroups[solutionAIndex];
    Solution& solutionB = classGroups[solutionBIndex];

    unsigned int tempDay = solutionA.day;
    unsigned int tempTimeslot = solutionA.timeslot;
    solutionA.day = solutionB.day;
    solutionA.timeslot = solutionB.timeslot;
    solutionB.day = tempDay;
    solutionB.timeslot = tempTimeslot;
  }

  Solution::Solution(
      const std::vector<size_t> classGroups,
      const std::unordered_map<size_t, std::unordered_set<ds::Class*>>
        classGroupsToClassesMap)
    : classGroups(classGroups)
    , classGroupsToClassesMap(classGroupsToClassesMap) {}

  std::vector<size_t>& Solution::getClassGroups()
  {
    return this->classGroups;
  }

  std::unordered_set<ds::Class*>& Solution::getClasses(size_t classGroup)
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
    for (ds::Class* cls : classes) {
      cls->teacher = teacher;
    }
  }

  void Solution::changeClassDay(const size_t classGroup, const unsigned int day)
  {
    auto& classes = this->getClasses(classGroup);
    for (ds::Class* cls : classes) {
      cls->day = day;
    }
  }

  void Solution::changeClassRoom(const size_t classGroup, ds::Room* const room)
  {
    auto& classes = this->getClasses(classGroup);
    for (ds::Class* cls : classes) {
      cls->room = room;
    }
  }

  void Solution::changeClassTimeslot(const size_t classGroup,
                                     const unsigned int timeslot)
  {
    auto& classes = this->getClasses(classGroup);
    for (ds::Class* cls : classes) {
      cls->timeslot = timeslot;
    }
  }

  UnknownClassGroupError::UnknownClassGroupError(const char* what_arg)
    : std::runtime_error(what_arg) {}
}
