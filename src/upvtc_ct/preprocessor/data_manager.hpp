#ifndef UPVTC_CT_PREPROCESSOR_HPP_
#define UPVTC_CT_PREPROCESSOR_HPP_

#include <string>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include <upvtc_ct/ds/models.hpp>

namespace upvtc_ct::preprocessor
{
  namespace ds = upvtc_ct::ds;

  class DataManager
  {
  public:
    DataManager(const unsigned int semester);

    // The member variables, courses and studentGroups, can become huge.
    // Returning them by value will be expensive. As such, we will be returning
    // them by references instead. We are returning const references of them
    // to prevent unwanted manipulations.
    const std::unordered_set<std::unique_ptr<ds::Course>>& getCourses();
    const std::unordered_set<std::unique_ptr<ds::Degree>>& getDegrees();
    const std::unordered_set<std::unique_ptr<ds::Division>>& getDivisions();
    const std::unordered_set<std::unique_ptr<ds::StudentGroup>>&
      getStudentGroups();

  private:
    std::string getBinFolderPath() const;

    std::unordered_set<std::unique_ptr<ds::Course>> courses;
    std::unordered_set<std::unique_ptr<ds::Degree>> degrees;
    std::unordered_set<std::unique_ptr<ds::Division>> divisions;
    std::unordered_set<std::unique_ptr<ds::StudentGroup>> studentGroups;

    std::unordered_map<std::string, ds::Course*> courseNameToObject;
  };
}

#endif
