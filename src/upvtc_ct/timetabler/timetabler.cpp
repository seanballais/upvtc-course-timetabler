#include <algorithm>
#include <chrono>
#include <iostream>
#include <memory>
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

  Solution Timetabler::findBestSolutionWithSimpleGA()
  {
    std::cout << "Finding best timetable with a simple genetic algorithm..."
              << std::endl;
    this->assignTeachersToClasses();
    auto generation = this->generateInitialGeneration();

    const utils::Config& config = this->dataManager.getConfig();
    const unsigned int numGenerations = config.get<const unsigned int>(
                                          "num_generations");

    for (int i = 0; i < numGenerations; i++) {
      std::cout << "|::| Generation #" << i + 1 << " Costs" << std::endl;
      std::cout << "  ";
      for (auto& solution : generation) {
        std::cout << solution.getCost() << " ";
      }
      std::cout << std::endl << std::endl;

      std::cout << ":: Generation #" << i + 2 <<  std::endl;
      Solution* solutionA = &(this->tournamentSelection(generation, 2));
      Solution* solutionB = nullptr;
      do {
        solutionB = &(this->tournamentSelection(generation, 2));
      } while (solutionB == solutionA);

      std::cout << "\tParent A Cost: " << solutionA->getCost() << std::endl;
      std::cout << "\tParent B Cost: " << solutionB->getCost() << std::endl;

      const float crossoverRate = config.get<const float>("crossover_rate");

      std::default_random_engine randGen;
      std::uniform_real_distribution<float> realDistrib{0.f, 1.f};

      Solution child{this->generateEmptySolution()};

      float crossoverChance = realDistrib(randGen);
      if (crossoverChance <= crossoverRate) {
        child = this->crossOverSolutions(*solutionA, *solutionB);
      } else {
        // No crossover, so let's just clone one of the parents.
        std::uniform_int_distribution<int> intDistrib{0, 1};
        Solution* baseParent = nullptr;
        int parentIndex = intDistrib(randGen);
        if (parentIndex == 0) {
          baseParent = solutionA;
        } else {
          baseParent = solutionB;
        }

        child = *baseParent;
      }

      const float mutationRate = config.get<const float>("mutation_rate");

      float mutationChance = realDistrib(randGen);
      if (mutationChance <= mutationRate) {
        this->mutateSolution(child);
      }

      this->computeSolutionCost(child);

      int worstSolutionIndex = 0;
      long worstCost = 0;
      for (size_t i = 0; i < generation.size(); i++) {
        if (generation[i].getCost() > worstCost) {
          worstSolutionIndex = i;
          worstCost = generation[i].getCost();
        }
      }

      if (generation[worstSolutionIndex].getCost() > child.getCost()) {
        generation[worstSolutionIndex] = child;
      }

      std::cout << "|| Child Cost: " << child.getCost() << std::endl;

      long total = 0;
      for (Solution& solution : generation) {
        total += solution.getCost();
      }

      double averageCost = static_cast<double>(total)
                           / static_cast<double>(generation.size());
      std::cout << "|| Average Generation Cost: " << averageCost << std::endl;
    }

    Solution* bestSolution = nullptr;
    for (Solution& solution : generation) {
      if (bestSolution == nullptr
          || solution.getCost() < bestSolution->getCost()) {
        bestSolution = &solution;
      }
    }

    std::cout << "Best solution cost: " << bestSolution->getCost() << std::endl;

    return *bestSolution;
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

    std::unordered_map<ds::Teacher*, float> currTeacherLoads;
    for (const auto clsGroup : classGroups) {
      auto& classes = this->dataManager.getClasses(clsGroup);
      auto* sampleClass = *(classes.begin());

      std::vector<ds::Teacher*> candidateTeachers;
      for (auto* const teacher : sampleClass->course->candidateTeachers) {
        candidateTeachers.push_back(teacher);
      }

      std::shuffle(candidateTeachers.begin(), candidateTeachers.end(),
                   std::default_random_engine{seed});

      std::stable_sort(
        candidateTeachers.begin(), candidateTeachers.end(),
        [&currTeacherLoads] (ds::Teacher* tA, ds::Teacher* tB) {
          auto itemA = currTeacherLoads.find(tA);
          if (itemA == currTeacherLoads.end()) {
            currTeacherLoads.insert({tA, 0.f});
          }

          auto itemB = currTeacherLoads.find(tB);
          if (itemB == currTeacherLoads.end()) {
            currTeacherLoads.insert({tB, 0.f});
          }

          const float tALoad = currTeacherLoads[tA];
          const float tBLoad = currTeacherLoads[tB];

          return tALoad < tBLoad;
      });

      // Get all teachers with the smallest loads, and who will not exceed
      // load limits for the semester and the year.
      const utils::Config& config = this->dataManager.getConfig();
      const float maxTeacherSemLoad = config.get<const float>(
                                        "max_semestral_teacher_load");
      const float maxTeacherYearLoad = config.get<const float>(
                                         "max_annual_teacher_load");
      auto* const teacherKey = candidateTeachers[0];
      const float smallestLoad = currTeacherLoads[teacherKey];
      const float courseLoad = sampleClass->course->numUnits;

      std::vector<ds::Teacher*> possibleTeachers;
      for (auto* const teacher : candidateTeachers) {
        const float currTeacherLoad = currTeacherLoads[teacher];
        const float newTeacherLoad = courseLoad + currTeacherLoad;
        const float prevTeacherLoad = teacher->previousLoad;
        const float totalLoad = newTeacherLoad + prevTeacherLoad;
        if (currTeacherLoad == smallestLoad
            && ((newTeacherLoad <= maxTeacherSemLoad)
                 && (totalLoad <= maxTeacherYearLoad))) {
          // Okay, so the teacher may handle the course, since his/her would-be
          // load is not existing her semestral load and annual load limit.
          possibleTeachers.push_back(teacher);
        } else if (currTeacherLoad > smallestLoad) {
          break;
        }
      }

      if (possibleTeachers.empty()) {
        // TODO: Turn this into an exception.
        const std::string courseName = sampleClass->course->name;
        std::cout << "ERROR! Too few teachers available for " << courseName
                  << "." << std::endl;
      }

      // Now select a first teacher in the list.
      auto* const selectedTeacher = possibleTeachers[0];
      currTeacherLoads[selectedTeacher] += courseLoad;
      for (auto* const cls : this->dataManager.getClasses(clsGroup)) {
        cls->teacher = selectedTeacher;
      }
    }
  }

  Solution& Timetabler::tournamentSelection(std::vector<Solution>& pop,
                                            const int k)
  {
    Solution* best = nullptr;

    std::random_device randDevice;
    std::mt19937 mt{randDevice()};

    std::uniform_int_distribution<size_t> popDistrib{0, pop.size() - 1};

    std::unordered_set<int> selectedIndexes;
    for (int i = 0; i < k ; i++) {
      size_t selectedIndex;
      do {
        selectedIndex = popDistrib(mt);
      } while (selectedIndexes.find(selectedIndex) != selectedIndexes.end());

      selectedIndexes.insert(selectedIndex);

      Solution& selectedSolution = pop[selectedIndex];
      if (best == nullptr || selectedSolution.getCost() > best->getCost()) {
        best = &selectedSolution;
      }
    }

    return *best;
  }

  Solution Timetabler::crossOverSolutions(Solution& parentA, Solution& parentB)
  {
    std::random_device randDevice;
    std::mt19937 mt{randDevice()};

    std::uniform_int_distribution<int> parentDistrib{0, 1};
    Solution child = this->generateEmptySolution();
    for (const auto& [classGroup, _] : this->dataManager.getClassGroups()) {
      Solution* inheriterParent;
      const int parentSelection = parentDistrib(mt);
      if (parentSelection == 0) {
        inheriterParent = &parentA;
      } else {
        inheriterParent = &parentB;
      }

      auto day = inheriterParent->getClassDay(classGroup);
      auto timeslot = inheriterParent->getClassTimeslot(classGroup);
      child.changeClassDay(classGroup, day);
      child.changeClassTimeslot(classGroup, timeslot);
    }

    return child;
  }

  void Timetabler::mutateSolution(Solution& solution)
  {
    std::random_device randDevice;
    std::mt19937 mt{randDevice()};

    typedef std::function<void(Solution&)> mutatorFunc;
    std::vector<mutatorFunc> mutators{
      [this] (Solution& solution) { this->applySimpleMove(solution); },
      [this] (Solution& solution) { this->applySimpleSwap(solution); }
    };
    const size_t numMutators = mutators.size();
    std::uniform_int_distribution<size_t> mutatorDistrib{0, numMutators - 1};
    const size_t mutatorIndex = mutatorDistrib(mt);

    auto mutator = mutators[mutatorIndex];
    mutator(solution);
  }

  std::vector<Solution> Timetabler::generateInitialGeneration()
  {
    const utils::Config& config = this->dataManager.getConfig();
    const unsigned int numOffsprings = config.get<const unsigned int>(
                                         "num_offsprings_per_generation");
    std::vector<Solution> generation;
    for (size_t i = 0; i < numOffsprings; i++) {
      generation.push_back(std::move(this->generateRandomSolution()));
    }

    for (auto& solution : generation) {
      std::cout << "####################" << std::endl;
      std::cout << "Cost: " << solution.getCost() << std::endl;

      for (size_t classGrp : solution.getClassGroups()) {
        auto* sampleClass = *(solution.getClasses(classGrp).begin());
        const std::string courseName = sampleClass->course->name;
        const std::string teacherName = sampleClass->teacher->name;
        const unsigned int day = sampleClass->day;
        const unsigned int timeslot = sampleClass->timeslot;

        std::cout << "Class Group Details" << std::endl;
        std::cout << "  Course: " << courseName << std::endl;
        std::cout << "  Teacher: " << teacherName << std::endl;
        std::cout << "  Day: " << day << std::endl;
        std::cout << "  Timeslot: " << timeslot << std::endl;
      }

      std::cout << "####################" << std::endl;
    }

    std::sort(generation.begin(), generation.end(),
              [] (Solution& sA, Solution& sB) {
                return sA.getCost() < sB.getCost();
              });

    return generation;
  }

  Solution Timetabler::generateRandomSolution()
  {
    std::random_device randDevice;
    std::mt19937 mt{randDevice()};

    const utils::Config& config = this->dataManager.getConfig();
    const unsigned int numUniqueDays = config.get<const unsigned int>(
                                          "num_unique_days");
    const unsigned int numTimeslots = config.get<const unsigned int>(
                                          "num_timeslots");
    std::uniform_int_distribution<int> daysDistrib(0, numUniqueDays - 1);
    std::uniform_int_distribution<int> timeslotsDistrib(0, numTimeslots - 1);

    Solution solution = this->generateEmptySolution();
    for (const size_t classGroup : solution.getClassGroups()) {
      const unsigned int newDay = daysDistrib(mt);
      const unsigned int newTimeslot = timeslotsDistrib(mt);

      solution.changeClassDay(classGroup, newDay);
      solution.changeClassTimeslot(classGroup, newTimeslot);
    }

    this->computeSolutionCost(solution);

    return solution;
  }

  Solution Timetabler::generateEmptySolution()
  {
    return {this->dataManager.getClassGroups()};
  }

  void Timetabler::computeSolutionCost(Solution& solution)
  {
    int hc0Cost = this->getHC0Cost(solution) * 100;
    int hc1Cost = this->getHC1Cost(solution) * 100;
    int hc2Cost = this->getHC2Cost(solution) * 100;
    int sc0Cost = this->getSC0Cost(solution);
    int sc1Cost = this->getSC1Cost(solution);
    int cost = hc0Cost + hc1Cost + hc2Cost + sc0Cost + sc1Cost;

    std::cout << "Cost:" << std::endl;
    std::cout << "-----" << std::endl;
    std::cout << "HC 0: " << hc0Cost << std::endl;
    std::cout << "HC 1: " << hc1Cost << std::endl;
    std::cout << "HC 2: " << hc2Cost << std::endl;
    std::cout << "SC 0: " << sc0Cost << std::endl;
    std::cout << "SC 1: " << sc1Cost << std::endl;

    solution.setCost(cost);
  }

  void Timetabler::applySimpleMove(Solution& solution)
  {
    std::vector<size_t>& classGroups = solution.getClassGroups();

    const size_t numClassGroups = classGroups.size();

    std::random_device randDevice;
    std::mt19937 mt{randDevice()};

    std::uniform_int_distribution<int> classGrpDistrib{
      0, static_cast<int>(numClassGroups - 1)};

    size_t classGroupIndex = classGrpDistrib(mt);
    const size_t classGroup = classGroups[classGroupIndex];

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

    size_t classGroupAIndex = distribution(mt);
    size_t classGroupA = classGroups[classGroupAIndex];

    // Make sure solutionBIndex is not the same as solutionAIndex.
    size_t classGroupBIndex;
    do {
      classGroupBIndex = distribution(mt);
    } while (classGroupBIndex == classGroupAIndex);

    size_t classGroupB = classGroups[classGroupBIndex];

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
              [] (const ds::Class* const clsA,
                  const ds::Class* const clsB) -> bool {
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
    std::sort(classes.begin(), classes.end(),
              [] (const ds::Class* const clsA,
                  const ds::Class* const clsB) -> bool {
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
              [] (const ds::Class* const clsA,
                  const ds::Class* const clsB) -> bool {
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
      const unsigned int classDay = cls->day;
      const unsigned int classTimeslot = cls->timeslot;
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
    std::unordered_map<ds::Teacher*, float> currTeacherLoads;
    for (auto classGroup : solution.getClassGroups()) {
      auto* sampleClass = *(solution.getClasses(classGroup).begin());
      auto* teacher = sampleClass->teacher;
      float courseUnits = sampleClass->course->numUnits;

      auto item = currTeacherLoads.find(teacher);
      if (item == currTeacherLoads.end()) {
        currTeacherLoads[teacher] = 0;
      }

      currTeacherLoads[teacher] += courseUnits;
    }

    // Now check if there is a violation.
    const utils::Config& config = this->dataManager.getConfig();
    const float maxTeacherSemLoad = config.get<const float>(
                                      "max_semestral_teacher_load");
    const float maxTeacherYearLoad = config.get<const float>(
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
      const std::unordered_map<size_t, std::unordered_set<ds::Class*>>
        classGroupsToClassesMap)
      : cost(0)
  {
    for (const auto& [classGroup, classes] : classGroupsToClassesMap) {
      this->classGroups.push_back(classGroup);
      for (auto* const cls : classes) {
        auto clsObj{std::make_unique<ds::Class>(cls->id, cls->classID,
                                                cls->course, cls->teacher,
                                                0, cls->room, 0)};
        auto* clsObjPtr = clsObj.get();
        
        auto item = this->classGroupsToClassesMap.find(classGroup);
        if (item == this->classGroupsToClassesMap.end()) {
          this->classGroupsToClassesMap.insert({classGroup, {}});
        }

        this->classGroupsToClassesMap[classGroup].insert(clsObjPtr);

        this->classPtrs.push_back(clsObjPtr);
        this->classes.push_back(std::move(clsObj));
      }
    }
  }

  Solution::Solution(const Solution& rhs)
      : classGroups(rhs.classGroups)
      , cost(rhs.cost)
  {
    for (const auto& [classGroup, classes] : rhs.classGroupsToClassesMap) {
      for (auto* const cls : classes) {
        auto clsObj{std::make_unique<ds::Class>(cls->id, cls->classID,
                                                cls->course, cls->teacher,
                                                cls->day, cls->room,
                                                cls->timeslot)};
        auto* clsObjPtr = clsObj.get();
        
        auto item = this->classGroupsToClassesMap.find(classGroup);
        if (item == this->classGroupsToClassesMap.end()) {
          this->classGroupsToClassesMap.insert({classGroup, {}});
        }

        this->classGroupsToClassesMap[classGroup].insert(clsObjPtr);

        this->classPtrs.push_back(clsObjPtr);
        this->classes.push_back(std::move(clsObj));
      }
    }
  }

  Solution::Solution(Solution&& rhs)
      : classGroups(rhs.classGroups)
      , classPtrs(rhs.classPtrs)
      , classGroupsToClassesMap(rhs.classGroupsToClassesMap)
      , classes(std::move(rhs.classes))
      , cost(rhs.cost)
  {
    rhs.classGroups.clear();
    rhs.classPtrs.clear();
    rhs.classGroupsToClassesMap.clear();
    rhs.classes.clear();
    rhs.cost = 0;
  }

  Solution& Solution::operator=(const Solution& rhs)
  {
    this->cost = rhs.cost;
    for (const auto& [classGroup, classes] : rhs.classGroupsToClassesMap) {
      this->classGroups.push_back(classGroup);
      for (auto* const cls : classes) {
        auto clsObj{std::make_unique<ds::Class>(cls->id, cls->classID,
                                                cls->course, cls->teacher,
                                                cls->day, cls->room,
                                                cls->timeslot)};
        auto* clsObjPtr = clsObj.get();
        
        auto item = this->classGroupsToClassesMap.find(classGroup);
        if (item == this->classGroupsToClassesMap.end()) {
          this->classGroupsToClassesMap.insert({classGroup, {}});
        }

        this->classGroupsToClassesMap[classGroup].insert(clsObjPtr);

        this->classPtrs.push_back(clsObjPtr);
        this->classes.push_back(std::move(clsObj));
      }
    }

    return *this;
  }

  std::vector<size_t>& Solution::getClassGroups()
  {
    return this->classGroups;
  }

  std::unordered_set<ds::Class*>&
  Solution::getClasses(const size_t classGroup)
  {
    std::stringstream errorMsgStream;
    errorMsgStream << "Attempted to obtain classes using an unknown class "
                   << "group, " << classGroup << ".";
    const char* errorMsg = (errorMsgStream.str()).c_str();
    return utils::getValueRefFromMap<size_t, std::unordered_set<ds::Class*>,
                                     utils::UnknownClassGroupError>(
        classGroup, this->classGroupsToClassesMap, errorMsg);
  }

  std::vector<ds::Class*>& Solution::getAllClasses()
  {
    return this->classPtrs;
  }

  const unsigned int Solution::getClassDay(const size_t classGroup)
  {
    const auto& classes = this->getClasses(classGroup);
    ds::Class* cls = *(classes.begin());
    return cls->day;
  }

  const unsigned int Solution::getClassTimeslot(const size_t classGroup)
  {
    const auto& classes = this->getClasses(classGroup);
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

  long Solution::getCost() const
  {
    return this->cost;
  }

  void Solution::setCost(const int cost)
  {
    this->cost = cost;
  }
}
