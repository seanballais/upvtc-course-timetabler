#include <sstream>
#include <string>

#include <upvtc_ct/ds/models.hpp>

namespace upvtc_ct::ds
{
  // Model Definitions
  Course::Course(const std::string name)
    : name(name) {}

  bool Course::operator==(const Course& c) const
  {
    return this->name == c.name;
  }

  CourseDependencyList::CourseDependencyList()
    : dependencies({}) {}

  void CourseDependencyList::addDependency(Course c, Course dependency)
  {
    if (this->isCoursePresent(c)) {
      this->dependencies[c].insert(dependency);
    }
  }

  CourseDependencyList::Dependencies
  CourseDependencyList::getDependency(Course c)
  {
    return (this->isCoursePresent(c)) ? this->dependencies[c]
                                      : CourseDependencyList::Dependencies({});
  }

  bool CourseDependencyList::isCoursePresent(Course c)
  {
    auto item = this->dependencies.find(c);
    return item != this->dependencies.end();
  }

  Degree::Degree(const std::string name)
    : name(name) {}

  bool Degree::operator==(const Degree& d) const
  {
    return this->name == d.name;
  }

  RoomFeature::RoomFeature(const std::string name)
    : name(name) {}

  bool RoomFeature::operator==(const RoomFeature& r) const
  {
    return this->name == r.name;
  }

  Room::Room(const std::string name, const unsigned int capacity)
    : name(name),
      capacity(capacity),
      roomFeatures({}) {}

  bool Room::operator==(const Room& r) const
  {
    return this->name == r.name;
  }

  BaseStudentGroup::BaseStudentGroup(
        const unsigned int numMembers,
        const std::unordered_set<Course, CourseHashFunction> assignedCourses)
    : numMembers(numMembers),
      assignedCourses(assignedCourses) {}

  StudentGroup::StudentGroup(
        const Degree degree,
        const unsigned int yearLevel,
        const unsigned int numMembers,
        const std::unordered_set<Course, CourseHashFunction> assignedCourses)
    : BaseStudentGroup(numMembers, assignedCourses),
      degree(degree),
      yearLevel(yearLevel) {}

  bool StudentGroup::operator==(const StudentGroup& sg) const
  {
    return (this->degree == sg.degree
            && this->yearLevel == sg.yearLevel
            && this->assignedCourses == sg.assignedCourses);
  }

  SubStudentGroup::SubStudentGroup(
        const StudentGroup parentGroup,
        const unsigned int numMembers,
        const std::unordered_set<Course, CourseHashFunction> assignedCourses)
    : BaseStudentGroup(numMembers, assignedCourses),
      parentGroup(parentGroup) {}
  
  bool SubStudentGroup::operator==(const SubStudentGroup& ssg) const
  {
    return (this->parentGroup == ssg.parentGroup
            && this->numMembers == ssg.numMembers
            && this->assignedCourses == ssg.assignedCourses);
  }

  Teacher::Teacher(const std::string name)
    : name(name),
      potentialCourses({}) {}

  bool Teacher::operator==(const Teacher& t) const
  {
    return this->name == t.name;
  }

  // Model Hash Function Definitions
  size_t CourseHashFunction::operator()(const Course& c) const
  {
    return std::hash<std::string>()(c.name);
  }

  size_t DegreeHashFunction::operator()(const Degree& d) const
  {
    return std::hash<std::string>()(d.name);
  }

  size_t RoomHashFunction::operator()(const Room& r) const
  {
    return std::hash<std::string>()(r.name);
  }

  size_t StudentGroupHashFunction::operator()(const StudentGroup& sg) const
  {
    std::stringstream objIdentifier;
    objIdentifier << sg.degree.name
                  << "-"
                  << std::to_string(sg.yearLevel)
                  << "-"
                  << std::to_string(sg.numMembers);

    for (const auto& course : sg.assignedCourses) {
      objIdentifier << "-";
      objIdentifier << course.name;
    }

    return std::hash<std::string>()(objIdentifier.str());
  }

  size_t SubStudentGroupHashFunction::operator()(
      const SubStudentGroup& ssg) const
  {
    size_t parentGroupHash = StudentGroupHashFunction()(ssg.parentGroup);

    std::stringstream objIdentifier;
    objIdentifier << std::to_string(parentGroupHash)
                  << "-"
                  << std::to_string(ssg.numMembers);

    for (const auto& course : ssg.assignedCourses) {
      objIdentifier << "-";
      objIdentifier << course.name;
    }

    return std::hash<std::string>()(objIdentifier.str());
  }

  size_t TeacherHashFunction::operator()(const Teacher& t) const
  {
    return std::hash<std::string>()(t.name);
  }
}
