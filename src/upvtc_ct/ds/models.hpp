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
  class CourseHashFunction;

  struct Course
  {
    Course(const std::string name,
           const std::unordered_set<Course*> prerequisites);
    bool operator==(const Course& c) const;

    const std::string name;
    const std::unordered_set<Course*> prerequisites;
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
    StudentGroup(const Degree* degree, const unsigned int yearLevel,
                 const std::unordered_set<Course*> assignedCourses);
    bool operator==(const StudentGroup& sg) const;

    const Degree* degree;
    const unsigned int yearLevel;
  };

  class StudentGroupHashFunction
  {
  public:
    size_t operator()(const StudentGroup& sg) const;
  };

  struct SubStudentGroup : public BaseStudentGroup
  {
    SubStudentGroup(const StudentGroup parentGroup,
                    const std::unordered_set<Course*> assignedCourses);
    bool operator==(const SubStudentGroup& ssg) const;

    const StudentGroup parentGroup;
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
    bool operator==(const Class& c) const;

    size_t id; // Used to identify different classes from one another.
    size_t classID; // Used to identify which Class objects describe the
                    // same class.
    const Course* course;
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
