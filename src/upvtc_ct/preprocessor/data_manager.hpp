#ifndef UPVTC_CT_PREPROCESSOR_HPP_
#define UPVTC_CT_PREPROCESSOR_HPP_

#include <string>
#include <unordered_set>

#include <upvtc_ct/ds/models.hpp>

namespace upvtc_ct::preprocessor
{
  namespace ds = upvtc_ct::ds;

  class DataManager
  {
  public:
    DataManager();

    // The member variables, courses and studentGroups, can become huge.
    // Returning them by value will be expensive. As such, we will be returning
    // them by references instead. We are returning const references of them
    // to prevent unwanted manipulations.
    const std::unordered_set<ds::Course, ds::CourseHashFunction>& getCourses();
    const std::unordered_set<ds::StudentGroup, ds::StudentGroupHashFunction>&
      getStudentGroups();

  private:
    std::string getBinFolderPath() const;

    std::unordered_set<ds::Course, ds::CourseHashFunction> courses;
    std::unordered_set<ds::StudentGroup, ds::StudentGroupHashFunction>
      studentGroups;
  };
}

#endif
