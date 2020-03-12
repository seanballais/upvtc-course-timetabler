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
    const std::unordered_set<std::unique_ptr<ds::Class>>& getClasses();
    const std::unordered_set<std::unique_ptr<ds::Degree>>& getDegrees();
    const std::unordered_set<std::unique_ptr<ds::Division>>& getDivisions();
    const std::unordered_set<std::unique_ptr<ds::StudentGroup>>&
      getStudentGroups();
    const std::unordered_set<std::unique_ptr<ds::SubStudentGroup>>&
      getSubStudentGroups();
    ds::Course* const getCourseNameObject(const std::string courseName,
                                          const char* errorMsg);
    const std::unordered_map<size_t, std::unordered_set<ds::Class*>>&
      getClassGroups();
    const std::unordered_map<size_t, std::unordered_set<size_t>>&
      getClassConflicts();

    void addClass(std::unique_ptr<ds::Class>&& cls);
    void addClassConflict(const size_t classGroup,
                          const size_t conflictedGroup);

  private:
    std::string getBinFolderPath() const;
    const std::unordered_map<std::string, std::string> getConfigData();

    ds::Config config;
    std::unordered_set<std::unique_ptr<ds::Course>> courses;
    std::unordered_set<std::unique_ptr<ds::Class>> classes;
    std::unordered_set<std::unique_ptr<ds::Degree>> degrees;
    std::unordered_set<std::unique_ptr<ds::Division>> divisions;
    std::unordered_set<std::unique_ptr<ds::StudentGroup>> studentGroups;
    std::unordered_set<std::unique_ptr<ds::SubStudentGroup>> subStudentGroups;
    std::unordered_map<std::string, ds::Course*> courseNameToObject;

    // NOte that the key is the class group ID.
    std::unordered_map<size_t, std::unordered_set<ds::Class*>> classGroups;

    // Note that the key is the class group ID, and the values are the class
    // groups that are in conflict with the class group referred to by the key.
    std::unordered_map<size_t, std::unordered_set<size_t>> classConflicts;
  };
}

#endif
