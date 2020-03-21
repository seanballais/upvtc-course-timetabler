#ifndef UPVTC_CT_UTILS_DATA_MANAGER_HPP_
#define UPVTC_CT_UTILS_DATA_MANAGER_HPP_

#include <string>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include <nlohmann/json.hpp>

#include <upvtc_ct/ds/models.hpp>
#include <upvtc_ct/utils/hash_specializations.hpp>

namespace upvtc_ct::utils
{
  using json = nlohmann::json;
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
    const std::unordered_set<std::unique_ptr<ds::RoomFeature>>&
      getRoomFeatures();
    ds::Course* const getCourseNameObject(const std::string courseName,
                                          const char* errorMsg);
    ds::Degree* const getDegreeNameObject(const std::string degreeName);
    ds::RoomFeature* const getRoomFeatureObject(
      const std::string roomFeatureName,
      const char* errorMsg);
    ds::Course* const getCourseLab(ds::Course* const course);
    const std::unordered_map<size_t, std::unordered_set<ds::Class*>>&
      getClassGroups();
    const std::unordered_map<size_t, std::unordered_set<size_t>>&
      getClassConflicts();

    void addClass(std::unique_ptr<ds::Class>&& cls);
    void addClassConflict(const size_t classGroup,
                          const size_t conflictedGroup);

  private:
    const std::string getBinFolderPath() const;
    const std::string getDataFolderPath() const;

    const std::unordered_map<std::string, std::string> getConfigData();

    const std::unordered_set<ds::Course*> getCoursesFromJSONArray(
      const json coursesJSON,
      const char* errorMsg = "Referenced another course that was not "
                             "yet generated. Please check your Study "
                             "Plans JSON file.");
    const std::unordered_set<ds::RoomFeature*> getRoomFeaturesFromJSONArray(
      const json roomFeaturesJSON,
      const char* errorMsg = "Referenced a room feature that was not "
                             "yet generated. Please check your Room Features "
                             "JSON file.");

    void parseCoursesJSON();
    void parseRoomFeaturesJSON();
    void parseStudyPlansJSON(
      const unsigned int semester,
      std::unordered_map<std::pair<std::string, unsigned int>,
                         ds::StudentGroup*,
                         PairHash>& generatedStudentGroups,
      std::unordered_map<std::pair<std::string, unsigned int>,
                         std::unordered_set<ds::Course*>,
                         PairHash>& studyPlans);
    void parseIrregularStudentGroupsJSON(
      const std::unordered_map<std::pair<std::string, unsigned int>,
                               std::unordered_set<ds::Course*>,
                               PairHash>& studyPlans);
    void parseRegularStudentGroupsGEsElectivesJSON(
      const std::unordered_map<std::pair<std::string, unsigned int>,
                               ds::StudentGroup*,
                               PairHash>& generatedStudentGroups);
    void parseStudentGroupsJSON(
      const std::unordered_map<std::pair<std::string, unsigned int>,
                               ds::StudentGroup*,
                               PairHash>& generatedStudentGroups);

    void parsePlanFromStudyPlansJSON(
      const json planItemJSON,
      const unsigned int semester,
      ds::Degree* const degree,
      std::unordered_set<ds::Course*>& divisionCourses,
      std::unordered_map<std::pair<std::string, unsigned int>,
                         ds::StudentGroup*,
                         PairHash>& generatedStudentGroups,
      std::unordered_map<std::pair<std::string, unsigned int>,
                         std::unordered_set<ds::Course*>,
                         PairHash>& studyPlans);

    ds::Course* const createCourseObject(const json courseJSON,
                                         const bool isLab);

    bool courseSetsHaveIntersections(
      const std::unordered_set<ds::Course*>* set0,
      const std::unordered_set<ds::Course*>* set1);

    unsigned int getNewStudentGroupID();

    template <typename T>
    const std::unordered_set<T*> getDataFromJSONArray(
      const json jsonArray,
      const char* errorMsg,
      std::function<T*(const std::string, const char*)> mapperFunc)
    {
      std::unordered_set<T*> collection;
      for (const auto& [_, dataName] : jsonArray.items()) {
        T* data = mapperFunc(dataName, errorMsg);
        collection.insert(data);
      }

      return collection;
    }

    ds::Config config;
    std::unordered_set<std::unique_ptr<ds::Course>> courses;
    std::unordered_set<std::unique_ptr<ds::Class>> classes;
    std::unordered_set<std::unique_ptr<ds::Degree>> degrees;
    std::unordered_set<std::unique_ptr<ds::Division>> divisions;
    std::unordered_set<std::unique_ptr<ds::RoomFeature>> roomFeatures;
    std::unordered_set<std::unique_ptr<ds::StudentGroup>> studentGroups;

    std::unordered_map<std::string, ds::Course*> courseNameToObject;
    std::unordered_map<std::string, ds::Degree*> degreeNameToObject;
    std::unordered_map<std::string, ds::RoomFeature*> roomFeatureToObject;
    std::unordered_map<ds::Course*, ds::Course*> courseToLabObject;

    // NOte that the key is the class group ID.
    std::unordered_map<size_t, std::unordered_set<ds::Class*>> classGroups;

    // Note that the key is the class group ID, and the values are the class
    // groups that are in conflict with the class group referred to by the key.
    std::unordered_map<size_t, std::unordered_set<size_t>> classConflicts;

    unsigned int currStudentGroupID;
  };
}

#endif
