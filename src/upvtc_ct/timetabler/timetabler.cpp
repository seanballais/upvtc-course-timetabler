#include <algorithm>
#include <random>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <upvtc_ct/ds/models.hpp>
#include <upvtc_ct/timetabler/timetabler.hpp>
#include <upvtc_ct/utils/data_manager.hpp>

namespace upvtc_ct::timetabler
{
  namespace ds = upvtc_ct::ds;
  namespace utils = upvtc_ct::utils;

  Timetabler::Timetabler(utils::DataManager& dataManager)
    : dataManager(dataManager) {}

  std::vector<Solution> Timetabler::generateInitialGeneration()
  {
    const utils::Config& config = this->dataManager.getConfig();
    const unsigned int numOffsprings = config.get<const unsigned int>(
                                         "num_offsprings_per_generation");
    std::vector<Solution> generation;
    for (size_t i = 0; i < numOffsprings; i++) {
      generation.push_back(this->generateRandomSolution());
    }

    return generation;
  }

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

    Solution solution{classGroups, classGroupsToClassesMap};
    this->applySimpleMove(solution);

    return solution;
  }

  void computeSolutionCost(Solution& solution)
  {
    int cost = this->getHC0Cost(solution)
               + this->getHC1Cost(solution)
               + this->getHC2Cost(solution)
               + this->getSC0Cost(solution)
               + this->getSC1Cost(solution)
               + this->getSC2Cost(solution);
    solution.setCost(cost);
  }

  void Timetabler::applySimpleMove(Solution& solution)
  {
    std::vector<size_t>& classGroups = solution.getClassGroups();

    const size_t numClassGroups = classGroups.size();

    std::random_device randDevice;
    std::mt19937 mt{randDevice()};

    std::uniform_int_distribution<int> classGrpDistrib(0, numClassGroups - 1);

    size_t classGroup = classGrpDistrib(mt);

    const utils::Config& config = this->dataManager.getConfig();
    const unsigned int numUniqueDays = config.get<const unsigned int>(
                                         "num_unique_days");
    const unsigned int numTimeslots = config.get<const unsigned int>(
                                         "num_timeslots");
    std::uniform_int_distribution<int> daysDistrib(0, numUniqueDays - 1);
    std::uniform_int_distribution<int> timeslotsDistrib(0, numTimeslots - 1);

    const unsigned int prevDay = solution.getClassDay(classGroup);
    const unsigned int prevTimeslot = solution.getClassTimeslot(classGroup);
    unsigned int newDay;
    unsigned int newTimeslot;
    do {
      newDay = daysDistrib(mt);
      newTimeslot = timeslotsDistrib(mt);
    } while (newDay == prevDay || newTimeslot == prevTimeslot);

    solution.changeClassDay(classGroup, newDay);
    solution.changeClassTimeslot(classGroup, newTimeslot);
  }

  void Timetabler::applySimpleSwap(Solution& solution)
  {
    std::vector<size_t>& classGroups = solution.getClassGroups();

    const size_t numClassGroups = classGroups.size();
    std::uniform_int_distribution<int> distribution(0, numClassGroups - 1);

    std::random_device randDevice;
    std::mt19937 mt{randDevice()};

    size_t classGroupA = distribution(mt);

    // Make sure solutionBIndex is not the same as solutionAIndex.
    size_t classGroupB;
    do {
      classGroupB = distribution(mt);
    } while (classGroupB == classGroupA);

    unsigned int tempDay = solution.getClassDay(classGroupA);
    unsigned int tempTimeslot = solution.getClassTimeslot(classGroupA);
    solution.changeClassDay(classGroupA, solution.getClassDay(classGroupB));
    solution.changeClassTimeslot(classGroupA,
                                 solution.getClassTimeslot(classGroupB));
    solution.changeClassDay(classGroupB, tempDay);
    solution.changeClassTimeslot(classGroupB, tempTimeslot);
  }

  int Timetabler::getHC0Cost(Solution& solution)
  {
    // Hard Constraint 0
    // No two classes must have share the same timeslots.

    // Using a reference since the order in the classGroups doesn't really
    // matter in typical usage of Solution objects. Doing so will also benefit
    // performance.
    std::vector<ds::Class* const> classes{};
    for (const size_t classGroup : solution.getClassGroups()) {
      for (const auto& classes : solution.getClasses()) {
        for (Class* const cls : classes) {
          classes.push_back(cls);
        }
      }
    }

    std::sort(classes.begin(), classes.end(),
              [] (const Class* const clsA, const Class* const clsB) -> bool {
                return (clsA->day < clsB->day)
                       || ((clsA->day == clsB->day)
                           && (clsA->timeslot < clsB->timeslot));
              });

    int cost = 0;
    for (size_t i = 0; i < classes.size() - 1; i++) {
      if ((classes[i]->day == classes[i + 1]->day)
          && (classes[i]->timeslot == classes[i + 1]->timeslot)
          && (classes[i]->classID != classes[i + 1]->classID)) {
        cost++;
      }
    }

    return cost;
  }

  int Timetabler::getHC1Cost(Solution& solution)
  {

  }

  int Timetabler::getHC2Cost(Solution& solution)
  {

  }

  int Timetabler::getSC0Cost(Solution& solution)
  {

  }

  int Timetabler::getSC1Cost(Solution& solution)
  {

  }

  int Timetabler::getSC2Cost(Solution& solution)
  {

  }

  Solution::Solution(
      const std::vector<size_t> classGroups,
      const std::unordered_map<size_t, std::unordered_set<ds::Class*>>
        classGroupsToClassesMap)
    : classGroups(classGroups)
    , classGroupsToClassesMap(classGroupsToClassesMap)
    , cost(0) {}

  std::vector<size_t>& Solution::getClassGroups()
  {
    return this->classGroups;
  }

  std::unordered_set<ds::Class*>& Solution::getClasses(const size_t classGroup)
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

  const unsigned int Solution::getClassDay(const size_t classGroup)
  {
    auto& classes = this->getClasses(classGroup);
    ds::Class* cls = *(classes.begin());
    return cls->day;
  }

  const unsigned int Solution::getClassTimeslot(const size_t classGroup)
  {
    auto& classes = this->getClasses(classGroup);
    ds::Class* cls = *(classes.begin());
    return cls->timeslot;
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

  int Solution::getCost() const
  {
    return this->cost;
  }

  void Solution::setCost(const int cost)
  {
    this->cost = cost;
  }

  UnknownClassGroupError::UnknownClassGroupError(const char* what_arg)
    : std::runtime_error(what_arg) {}
}
