#ifndef UPVTC_CT_PREPROCESSOR_PREPROCESSOR_HPP_
#define UPVTC_CT_PREPROCESSOR_PREPROCESSOR_HPP_

#include <memory>
#include <string>
#include <unordered_set>

#include <upvtc_ct/ds/models.hpp>
#include <upvtc_ct/utils/data_manager.hpp>

namespace upvtc_ct::preprocessor
{
  namespace ds = upvtc_ct::ds;
  namespace utils = upvtc_ct::utils;

  class Preprocessor
  {
  public:
    Preprocessor(utils::DataManager* const dataManager);
    void preprocess();

  private:
    void generateClasses();
    void identifyClassConflicts();

    size_t selectClassGroup(std::string courseName);

    utils::DataManager* const dataManager;
    std::unordered_map<std::string, std::unordered_set<size_t>>
      courseNameToClassGroupsMap;
    std::unordered_map<size_t, unsigned int> classGroupSizes;
  };
}

#endif
