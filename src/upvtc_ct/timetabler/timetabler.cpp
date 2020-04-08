#include <algorithm>
#include <chrono>
#include <random>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <upvtc_ct/ds/models.hpp>
#include <upvtc_ct/timetabler/timetabler.hpp>
#include <upvtc_ct/utils/data_manager.hpp>
#include <upvtc_ct/utils/errors.hpp>
#include <upvtc_ct/utils/utils.hpp>

namespace upvtc_ct::timetabler
{
  namespace ds = upvtc_ct::ds;
  namespace utils = upvtc_ct::utils;

  Timetabler::Timetabler(utils::DataManager& dataManager)
    : dataManager(dataManager)
    , discouragedTimeslots({0, 1, 9, 10, 11, 21, 22, 23}) {}

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
      for (auto& cls : classes) {
        cls.day = 0;
        cls.timeslot = 0;
      }

      classGroups.push_back(clsGroup);
      classGroupsToClassesMap.insert({clsGroup, classes});
    }

    Solution solution{classGroups, classGroupsToClassesMap};
    for (size_t i = 0; i < classGroups.size(); i++) {
      this->applySimpleMove(solution);
    }

    return solution;
  }

  void Timetabler::assignTeachersToClasses()
  {
    std::vector<size_t> classGroups;
    for (const auto item : this->dataManager.getClassGroups()) {
      auto clsGroup = item.first;
      classGroups.push_back(clsGroup);
    }

    const unsigned seed = std::chrono::system_clock::now().time_since_epoch()
                                                          .count();
    std::shuffle(classGroups.begin(), classGroups.end(),
                 std::default_random_engine{seed});

    std::unordered_map<Teacher* const, unsigned float> currTeacherLoads;
    for (const auto clsGroup : classGroups) {
      auto& 
    }
  }

  void Timetabler::computeSolutionCost(Solution& solution)
  {
    int cost = (this->getHC0Cost(solution) * 100)
               + (this->getHC1Cost(solution) * 100)
               + (this->getHC2Cost(solution) * 100)
               + this->getSC0Cost(solution)
               + this->getSC1Cost(solution);
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
    auto classes = solution.getAllClasses();
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
    // Hard Constraint 0
    // A teacher must not have classes that share timeslots.
    auto classes = solution.getAllClasses();
    std:::sort(classes.begin(), classes.end(),
               [] (const Class* const clsA, const Class* const clsB) -> bool {
                 // At this point, it is assumed that each class has been
                 // assigned a teacher.
                 return (clsA->teacher->name < clsB->teacher->name)
                        || ((clsA->teacher->name == clsB->teacher->name)
                            && ((clsA->day < clsB->day)
                                || ((clsA->day == clsB->day)
                                    && (clsA->timeslot < clsB->timeslot))));
               });

    int cost = 0;
    for (size_t i = 0; i < classes.size() - 1; i++) {
      if ((classes[i]->teacher->name == classes[i + 1]->teacher->name)
          && (classes[i]->day == classes[i + 1]->day)
          && (classes[i]->timeslot == classes[i + 1]->timeslot)
          && (classes[i]->classID != classes[i + 1]->classID)) {
        cost++;
      }
    }

    return cost;
  }

  int Timetabler::getHC2Cost(Solution& solution)
  {
    // Hard Constraint 2
    // A student must not have classes that share timeslots.
    auto classes = solution.getAllClasses();
    std::sort(classes.begin(), classes.end(),
              [] (const Class* const clsA, const Class* const clsB) -> bool {
                return (clsA->day < clsB->day)
                       || ((clsA->day == clsB->day)
                           && (clsA->timeslot < clsB->timeslot));
              });

    const auto& classConflicts = this->dataManager.getClassConflicts();

    int cost = 0;
    for (size_t i = 0; i < classes.size() - 1; i++) {
      // TODO: Create all possible pairs of the group!
      auto* clsA = classes[i];
      auto* clsB = classes[i + 1];
      if (clsA->day == clsB->day && clsA->timeslot == clsB->timeslot) {
        // Okay, so both classes are in the same partition, as per the
        // mathematical function of this constraint.
        auto clsConflictsItem = classConflicts.find(clsA->classID);
        auto& clsAConflicts = clsConflictsItem->second;

        auto item = clsAConflicts.find(clsB->classID);
        if (item != clsAConflicts.end()) {
          // Oops. clsA and clsB share students.
          cost++;
        }
      }
    }

    return cost;
  }

  int Timetabler::getSC0Cost(Solution& solution)
  {
    // Soft Constraint 0
    // No classes must be scheduled in the timeslots that are unpreferrable
    // to the teacher assigned to the class.

    // Note that we are assuming that all classes have been assigned a teacher
    // already.
    auto classes = solution.getAllClasses();
    int cost = 0;
    for (ds::Class* const cls : classes) {
      int classDay = cls->day;
      int classTimeslot = cls->timeslot;
      const auto& unpreferredTimeslots = cls->teacher->unpreferredTimeslots;

      auto item = unpreferredTimeslots.find({classDay, classTimeslot});
      if (item != unpreferredTimeslots.end()) {
        cost++;
      }
    }

    return cost;
  }

  int Timetabler::getSC1Cost(Solution& solution)
  {
    // Soft Constraint 1
    // No classes must be scheduled in between 7AM - 8AM, 11:30AM - 1PM, and
    // 5:30PM - 7PM.
    auto classes = solution.getAllClasses();
    
    int cost = 0;
    for (ds::Class* const cls : classes) {
      auto item = this->discouragedTimeslots.find(cls->timeslot);
      if (item != this->discouragedTimeslots.end()) {
        // Class is given timeslots of which some or all are discouraged
        // timeslots.
        cost++;
      }
    }

    return cost;
  }

  int Timetabler::getSC2Cost(Solution& solution)
  {
    // Soft Constraint 2
    // Teacher load must exceed the maximum load for a semester, or for a year.
    std::unordered_map<Teacher* const, unsigned float> currTeacherLoads;
    for (auto classGroup : solution.getClassGroups()) {
      auto* sampleClass = *(solution.getClasses(classGroup).begin());
      auto* teacher = sampleClass->teacher;
      unsigned float courseUnits = sampleClass->course->numUnits;

      auto item = currTeacherLoads.find(teacher);
      if (item == currTeacherLoads.end()) {
        currTeacherLoads[teacher] = 0;
      }

      currTeacherLoads[teacher] += courseUnits;
    }

    // Now check if there is a violation.
    utils::Config& config = this->dataManager.getConfig();
    const unsigned float maxTeacherSemLoad = config.get<float>(
                                               "max_semestral_teacher_load");
    const unsigned float maxTeacherYearLoad = config.get<float>(
                                                "max_annual_teacher_load");
    int cost = 0;
    for (const auto& [teacher, currLoad] : currTeacherLoads) {
      if ((currLoad > maxTeacherSemLoad)
          || ((currLoad + teacher->previousLoad) > maxTeacherYearLoad)) {
        cost++;
      }
    }

    return cost;
  }

  Solution::Solution(
      const std::vector<size_t> classGroups,
      const std::unordered_map<size_t, std::unordered_set<ds::Class*>>
        classGroupsToClassesMap)
      : classGroups(classGroups)
      , classGroupsToClassesMap(classGroupsToClassesMap)
      , cost(0)
  {
    for (const size_t classGroup : solution.getClassGroups()) {
      for (const auto& classes : solution.getClasses()) {
        for (Class* const cls : classes) {
          this->classes.push_back(cls);
        }
      }
    }
  }

  std::vector<size_t>& Solution::getClassGroups() const
  {
    return this->classGroups;
  }

  std::unordered_set<ds::Class*>&
  Solution::getClasses(const size_t classGroup) const
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

  std::vector<ds::Class* const>& getAllClasses() const
  {
    return this->classes;
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
}
