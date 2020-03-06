#ifndef UPVTC_CT_PREPROCESSOR_PREPROCESSOR_HPP_
#define UPVTC_CT_PREPROCESSOR_PREPROCESSOR_HPP_

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
    const std::unordered_set<ds::Class, ds::ClassHashFunction>
      getClasses() const;

  private:
    utils::DataManager* const dataManager;
    std::unordered_set<ds::Class, ds::ClassHashFunction> classes;
  };
}

#endif
