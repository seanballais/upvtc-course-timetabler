#ifndef UPVTC_CT_PREPROCESSOR_PREPROCESSOR_HPP_
#define UPVTC_CT_PREPROCESSOR_PREPROCESSOR_HPP_

#include <memory>
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

    utils::DataManager* const dataManager;
  };
}

#endif
