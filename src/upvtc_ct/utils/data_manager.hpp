#ifndef UPVTC_CT_UTILS_DATA_MANAGER_HPP_
#define UPVTC_CT_UTILS_DATA_MANAGER_HPP_

#include <string>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include <upvtc_ct/ds/models.hpp>

namespace upvtc_ct::utils
{
  namespace ds = upvtc_ct::ds;

  class DataManager
  {
  public:
    DataManager();

    // The member variables can become huge.
    // Returning them by value will be expensive. As such, we will be returning
    // them by references instead. We are returning const references of them
    // to prevent unwanted manipulations.
    const ds::Config& getConfig();
    const std::unordered_set<std::unique_ptr<ds::Course>>& getCourses();
    const std::unordered_set<std::unique_ptr<ds::Degree>>& getDegrees();
    const std::unordered_set<std::unique_ptr<ds::Division>>& getDivisions();
    const std::unordered_set<std::unique_ptr<ds::StudentGroup>>&
      getStudentGroups();
    ds::Course* const getCourseNameObject(const std::string courseName,
                                          const char* errorMsg);

  private:
    std::string getBinFolderPath() const;
    const std::unordered_map<std::string, std::string> getConfigData();

    ds::Config config;
    std::unordered_set<std::unique_ptr<ds::Course>> courses;
    std::unordered_set<std::unique_ptr<ds::Degree>> degrees;
    std::unordered_set<std::unique_ptr<ds::Division>> divisions;
    std::unordered_set<std::unique_ptr<ds::StudentGroup>> studentGroups;
    std::unordered_map<std::string, ds::Course*> courseNameToObject;
  };
}

#endif
