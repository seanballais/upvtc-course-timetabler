#ifndef UPVTC_CT_TIMETABLER_TIMETABLER
#define UPVTC_CT_TIMETABLER_TIMETABLER

#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <upvtc_ct/ds/models.hpp>
#include <upvtc_ct/utils/data_manager.hpp>

namespace upvtc_ct::timetabler
{
  namespace ds = upvtc_ct::ds;
  namespace utils = upvtc_ct::utils;

  class Solution;

  class Timetabler
  {
  public:
    Timetabler(utils::DataManager& dataManager);

  private:
    Solution findBestSolutionWithSimpleGA();
    std::vector<Solution> generateInitialGeneration();
    Solution generateRandomSolution();

    void assignTeachersToClasses();

    void computeSolutionCost(Solution& solution);
    void applySimpleMove(Solution& solution);
    void applySimpleSwap(Solution& solution);

    int getHC0Cost(Solution& solution);
    int getHC1Cost(Solution& solution);
    int getHC2Cost(Solution& solution);
    int getSC0Cost(Solution& solution);
    int getSC1Cost(Solution& solution);
    int getSC2Cost(Solution& solution);

    utils::DataManager& dataManager;
    const std::unordered_set<unsigned int> discouragedTimeslots;
  };

  class Solution
  {
    // TODO: Add solution cost member.
  public:
    Solution(const std::vector<size_t> classGroups,
             const std::unordered_map<size_t, std::unordered_set<ds::Class*>>
               classGroupsToClassesMap);
    std::vector<size_t>& getClassGroups();
    std::unordered_set<ds::Class*>& getClasses(const size_t classGroup);
    std::vector<ds::Class*>& getAllClasses();

    const unsigned int getClassDay(const size_t classGroup);
    const unsigned int getClassTimeslot(const size_t classGroup);

    int getCost() const;

    void setCost(const int cost);
    void changeClassTeacher(const size_t classGroup,
                            ds::Teacher* const teacher);
    void changeClassDay(const size_t classGroup, const unsigned int day);
    void changeClassRoom(const size_t classGroup, ds::Room* const room);
    void changeClassTimeslot(const size_t classGroup,
                             const unsigned int timeslot);

  private:
    int cost;
    std::vector<size_t> classGroups;
    std::vector<ds::Class*> classes;
    std::unordered_map<size_t, std::unordered_set<ds::Class*>>
      classGroupsToClassesMap;
  };
}
#endif
