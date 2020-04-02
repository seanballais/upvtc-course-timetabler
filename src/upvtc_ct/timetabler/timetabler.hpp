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

    Solution generateRandomSolution();

  private:
    void applySimpleMove(Solution& solution);
    void applySimpleSwap(Solution& solution);

    const utils::DataManager& dataManager;
  };

  class Solution
  {
  public:
    Solution(const std::vector<size_t> classGroups,
             const std::unordered_map<size_t, std::unordered_set<ds::Class*>>
               classGroupsToClassesMap);
    std::vector<size_t>& getClassGroups();
    std::unordered_set<ds::Class*>& getClasses(size_t classGroup);

    void changeClassTeacher(const size_t classGroup,
                            ds::Teacher* const teacher);
    void changeClassDay(const size_t classGroup, const unsigned int day);
    void changeClassRoom(const size_t classGroup, ds::Room* const room);
    void changeClassTimeslot(const size_t classGroup,
                             const unsigned int timeslot);

  private:
    std::vector<size_t> classGroups;
    std::unordered_map<size_t, std::unordered_set<ds::Class*>>
      classGroupsToClassesMap;
  };

  class UnknownClassGroupError : public std::runtime_error
  {
  public:
    UnknownClassGroupError(const char* what_arg);
  };
}
