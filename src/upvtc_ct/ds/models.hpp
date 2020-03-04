#ifndef UPVTC_CT_DS_MODELS_HPP_
#define UPVTC_CT_DS_MODELS_HPP_

#include <string>
#include <unordered_map>
#include <unordered_set>

namespace upvtc_ct::ds
{
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
    BaseStudentGroup(
      const unsigned int numMembers,
      const std::unordered_set<Course*> assignedCourses);

    const unsigned int numMembers;
    const std::unordered_set<Course*> assignedCourses;
  };

  struct StudentGroup : public BaseStudentGroup
  {
    StudentGroup(
      const Degree degree, const unsigned int yearLevel,
      const unsigned int numMembers,
      const std::unordered_set<Course*> assignedCourses);
    bool operator==(const StudentGroup& sg) const;

    const Degree degree;
    const unsigned int yearLevel;
  };

  class StudentGroupHashFunction
  {
  public:
    size_t operator()(const StudentGroup& sg) const;
  };

  struct SubStudentGroup : public BaseStudentGroup
  {
    SubStudentGroup(
      const StudentGroup parentGroup,
      const unsigned int numMembers,
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
    const Course course;
    Teacher teacher;
    unsigned int day;
    Room room;
    unsigned int timeslot;
  };
}

#endif
