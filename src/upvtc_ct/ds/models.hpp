#ifndef UPVTC_CT_DS_MODELS_HPP_
#define UPVTC_CT_DS_MODELS_HPP_

#include <string>
#include <unordered_map>
#include <unordered_set>

namespace upvtc_ct::ds
{
  // Note that these models, except Class, are and should be read-only. There is
  // no reason at the moment that most of these models should be mutable.
  struct Course
  {
    Course(const std::string name);
    bool operator==(const Course& c) const;

    const std::string name;
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
    Room(const std::string name, const unsigned int capacity);
    bool operator==(const Room& r) const;

    const std::string name;
    const unsigned int capacity;
    const std::unordered_set<RoomFeature, RoomFeatureHashFunction> roomFeatures;
  };

  class RoomHashFunction
  {
  public:
    size_t operator()(const Room& r) const;
  };

  struct StudentGroup
  {
    StudentGroup(const Degree degree, const unsigned int yearLevel,
                 const unsigned int numMembers);
    bool operator==(const StudentGroup& sg) const;

    const Degree degree;
    const unsigned int yearLevel;
    const unsigned int numMembers;
    const std::unordered_set<Course, CourseHashFunction> assignedCourses;
  };

  class StudentGroupHashFunction
  {
  public:
    size_t operator()(const StudentGroup& sg) const;
  };

  struct Teacher
  {
    Teacher(const std::string name);
    bool operator==(const Teacher& t) const;

    const std::string name;
    const std::unordered_set<Course, CourseHashFunction> potentialCourses;
  };

  class TeacherHashFunction
  {
  public:
    size_t operator()(const Teacher& t) const;
  };

  struct Division
  {
    std::unordered_set<Course, CourseHashFunction> courses;
    std::unordered_set<Degree, DegreeHashFunction> degrees;
    std::unordered_set<Room, RoomHashFunction> rooms;
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
