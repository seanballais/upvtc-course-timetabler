#ifndef UPVTC_CT_PREPROCESSOR_HPP_
#define UPVTC_CT_PREPROCESSOR_HPP_

#include <string>
#include <unordered_set>

#include <upvtc_ct/ds/models.hpp>

namespace upvtc_ct::preprocessor
{
  using upvtc_ct;

  class DataManager
  {
  private:
    std::string getBinFullPath() const;

    const std::unordered_set<ds::Course, ds::CourseHashFunction> courses;
    const std::unordered_set<ds::StudentGroup, StudentGroupHashFunction>
      studentGroups;
    ds::CourseDependencyList courseDependencyList;
  }
}


#endif
