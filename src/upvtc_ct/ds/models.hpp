#ifndef UPVTC_CT_DS_MODELS_HPP_
#define UPVTC_CT_DS_MODELS_HPP_

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <upvtc_ct/utils/errors.hpp>

namespace upvtc_ct::ds
{
  namespace utils = upvtc_ct::utils;

  // Note that these models, except Class, are and should be read-only. There is
  // no reason at the moment that most of these models should be mutable.
  class RoomFeature;

  struct Course
  {
    Course(const std::string name, const bool hasLab,
           const std::unordered_set<Course*> prerequisites,
           const std::unordered_set<RoomFeature*> roomRequirements);
    bool operator==(const Course& c) const;

    const std::string name;
    const bool hasLab;
    const std::unordered_set<Course*> prerequisites;
    const std::unordered_set<RoomFeature*> roomRequirements;
  };

  class CourseHashFunction
  {
  public:
    size_t operator()(const Course& c) const;
  };

  class CourseDependencyList
  {
    using Dependencies = std::unordered_set<Course, CourseHashFunction>;

  public:
    CourseDependencyList();
    void addDependency(Course c, Course dependency);
    Dependencies getDependency(Course c);
  
  private:    
    bool isCoursePresent(Course c);

    std::unordered_map<Course, Dependencies, CourseHashFunction>
      dependencies;
  };

  struct Degree
  {
    Degree(const std::string name);
    bool operator==(const Degree& d) const;

    const std::string name;
  };

  class DegreeHashFunction
  {
  public:
    size_t operator()(const Degree& d) const;
  };

  struct RoomFeature
  {
    RoomFeature(const std::string name);
    bool operator==(const RoomFeature& r) const;

    const std::string name;
  };

  class RoomFeatureHashFunction
  {
  public:
    size_t operator()(const RoomFeature& r) const;
  };

  struct Room
  {
    Room(const std::string name,
         const unsigned int capacity,
         const std::unordered_set<RoomFeature*> roomFeatures);
    bool operator==(const Room& r) const;

    const std::string name;
    const unsigned int capacity;
    const std::unordered_set<RoomFeature*> roomFeatures;
  };

  class RoomHashFunction
  {
  public:
    size_t operator()(const Room& r) const;
  };

  struct BaseStudentGroup
  {
    BaseStudentGroup(const std::unordered_set<Course*> assignedCourses);
    const unsigned int getNumMembers() const;
    void setNumMembers(const unsigned int numMembers);

    const std::unordered_set<Course*> assignedCourses;
  private:
    unsigned int numMembers;
    bool isNumMembersAssigned;
  };

  struct StudentGroup : public BaseStudentGroup
  {
    StudentGroup(Degree* const degree, const unsigned int yearLevel,
                 const std::unordered_set<Course*> assignedCourses);
    bool operator==(const StudentGroup& sg) const;

    Degree* const degree;
    const unsigned int yearLevel;
  };

  class StudentGroupHashFunction
  {
  public:
    size_t operator()(const StudentGroup& sg) const;
  };

  struct SubStudentGroup : public BaseStudentGroup
  {
    SubStudentGroup(StudentGroup* const parentGroup,
                    const std::unordered_set<Course*> assignedCourses);
    bool operator==(const SubStudentGroup& ssg) const;

    StudentGroup* const parentGroup;
  };

  class SubStudentGroupHashFunction
  {
  public:
    size_t operator()(const SubStudentGroup& ssg) const;
  };

  struct Teacher
  {
    Teacher(const std::string name);
    bool operator==(const Teacher& t) const;

    const std::string name;
    const std::unordered_set<Course*> potentialCourses;
  };

  class TeacherHashFunction
  {
  public:
    size_t operator()(const Teacher& t) const;
  };

  struct Division
  {
    Division(const std::string name,
             const std::unordered_set<Course*> courses,
             const std::unordered_set<Degree*> degrees,
             const std::unordered_set<Room*> rooms);
    bool operator==(const Division& d) const;

    const std::string name;
    const std::unordered_set<Course*> courses;
    const std::unordered_set<Degree*> degrees;
    const std::unordered_set<Room*> rooms;
  };

  class DivisionHashFunction
  {
  public:
    size_t operator()(const Division& d) const;
  };

  struct Class
  {
    Class(const size_t id, const size_t classID, Course* const course,
          Teacher* const teacher, const unsigned int day, Room* const room,
          const unsigned int timeslot);
    bool operator==(const Class& c) const;

    const size_t id; // Used to identify different classes from one another.
    const size_t classID; // Used to identify which Class objects describe the
                          // same class.
    Course* const course;
    Teacher* teacher;
    unsigned int day;
    Room* room;
    unsigned int timeslot;
  };

  class ClassHashFunction
  {
  public:
    size_t operator()(const Class& c) const;
  };

  class Config
  {
  public:
    Config(const std::unordered_map<std::string, std::string> configData);
    bool empty() const;

    template<typename T>
    T get(const std::string key)
    {
      throw utils::DisallowedFunctionError();
    }

  private:
    const std::string getValue(const std::string key) const;

    const std::unordered_map<std::string, std::string> configData;
  };

  class ConfigError : public std::runtime_error
  {
  public:
    ConfigError(const char* what_arg);
  };
}

#endif
