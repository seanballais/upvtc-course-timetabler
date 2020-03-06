#include <functional>
#include <sstream>
#include <string>

#include <upvtc_ct/ds/models.hpp>

namespace upvtc_ct::ds
{
  // Model Definitions
  Course::Course(const std::string name,
                 const std::unordered_set<Course*> prerequisites)
    : name(name),
      prerequisites(prerequisites) {}

  bool Course::operator==(const Course& c) const
  {
    return this->name == c.name;
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

  Room::Room(const std::string name,
             const unsigned int capacity,
             const std::unordered_set<RoomFeature*> roomFeatures)
    : name(name),
      capacity(capacity),
      roomFeatures(roomFeatures) {}

  bool Room::operator==(const Room& r) const
  {
    return this->name == r.name;
  }

  BaseStudentGroup::BaseStudentGroup(
        const std::unordered_set<Course*> assignedCourses)
    : assignedCourses(assignedCourses),
      isNumMembersAssigned(false) {}

  const unsigned int BaseStudentGroup::getNumMembers() const
  {
    return this->numMembers;
  }

  void BaseStudentGroup::setNumMembers(const unsigned int numMembers)
  {
    // numMembers should only be assigned once to preserve its const-ness in
    // principle.
    if (!this->isNumMembersAssigned) {
      this->numMembers = numMembers;
      this->isNumMembersAssigned = true;
    }
  }

  StudentGroup::StudentGroup(const Degree* degree, const unsigned int yearLevel,
                             const std::unordered_set<Course*> assignedCourses)
    : BaseStudentGroup(assignedCourses),
      degree(degree),
      yearLevel(yearLevel) {}

  bool StudentGroup::operator==(const StudentGroup& sg) const
  {
    return this->degree == sg.degree
           && this->yearLevel == sg.yearLevel
           && this->assignedCourses == sg.assignedCourses;
  }

  SubStudentGroup::SubStudentGroup(
        const StudentGroup parentGroup,
        const std::unordered_set<Course*> assignedCourses)
    : BaseStudentGroup(assignedCourses),
      parentGroup(parentGroup) {}
  
  bool SubStudentGroup::operator==(const SubStudentGroup& ssg) const
  {
    return this->parentGroup == ssg.parentGroup
           && this->assignedCourses == ssg.assignedCourses
           && this->getNumMembers() == ssg.getNumMembers();
  }

  Teacher::Teacher(const std::string name)
    : name(name),
      potentialCourses({}) {}

  bool Teacher::operator==(const Teacher& t) const
  {
    return this->name == t.name;
  }

  Division::Division(const std::string name,
                     const std::unordered_set<Course*> courses,
                     const std::unordered_set<Degree*> degrees,
                     const std::unordered_set<Room*> rooms)
    : name(name),
      courses(courses),
      degrees(degrees),
      rooms(rooms) {}
  
  bool Division::operator==(const Division& d) const
  {
    return this->name == d.name;
  }

  bool Class::operator==(const Class& c) const
  {
    const std::string thisCourseName = (this->course != nullptr)
      ? this->course->name
      : "nullptr";
    const std::string thisTeacherName = (this->teacher != nullptr)
      ? this->teacher->name
      : "nullptr";
    const std::string thisRoomName = (this->room != nullptr)
      ? this->room->name
      : "nullptr";
    const std::string otherCourseName = (c.course != nullptr)
      ? c.course->name
      : "nullptr";
    const std::string otherTeacherName = (c.teacher != nullptr)
      ? c.teacher->name
      : "nullptr";
    const std::string otherRoomName = (c.room != nullptr)
      ? c.room->name
      : "nullptr";

    return this->id == c.id
           && thisCourseName == otherCourseName
           && thisTeacherName == otherTeacherName
           && this->day == c.day
           && thisRoomName == otherRoomName
           && this->timeslot == c.timeslot;
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
    objIdentifier << sg.degree->name
                  << "-"
                  << std::to_string(sg.yearLevel)
                  << "-"
                  << std::to_string(sg.getNumMembers());

    return std::hash<std::string>()(objIdentifier.str());
  }

  size_t SubStudentGroupHashFunction::operator()(
      const SubStudentGroup& ssg) const
  {
    size_t parentGroupHash = StudentGroupHashFunction()(ssg.parentGroup);

    std::stringstream objIdentifier;
    objIdentifier << std::to_string(parentGroupHash)
                  << "-"
                  << std::to_string(ssg.getNumMembers());

    return std::hash<std::string>()(objIdentifier.str());
  }

  size_t TeacherHashFunction::operator()(const Teacher& t) const
  {
    return std::hash<std::string>()(t.name);
  }

  size_t DivisionHashFunction::operator()(const Division& d) const
  {
    return std::hash<std::string>()(d.name);
  }

  size_t ClassHashFunction::operator()(const Class& c) const
  {
    std::stringstream objIdentifier;
    objIdentifier << "("
                  << c.id << ", ";

    if (c.course == nullptr) {
      objIdentifier << "nullptr";
    } else {
      objIdentifier << c.course->name;
    }
    
    objIdentifier << ", ";

    if (c.teacher == nullptr) {
      objIdentifier << "nullptr";
    } else {
      objIdentifier << c.teacher->name;
    }

    objIdentifier << ", "
                  << c.day << ", ";

    if (c.room == nullptr) {
      objIdentifier << "nullptr";
    } else {
      objIdentifier << c.room->name;
    }
    
    objIdentifier << ", "
                  << c.timeslot
                  << ")"
                  << std::endl;

    return std::hash<std::string>()(objIdentifier.str());
  }

  Config::Config(const std::unordered_map<std::string, std::string> configData)
    : configData(configData) {}

  bool Config::empty() const
  {
    return this->configData.empty();
  }

  template<>
  int Config::get<int>(const std::string key)
  {
    return std::stoi(this->getValue(key));
  }

  const std::string Config::getValue(const std::string key) const
  {
    auto configPair = this->configData.find(key);
    if (configPair == this->configData.end()) {
      throw ConfigError("Attempted to access a non-existent key.");
    }

    return configPair->second;
  }

  ConfigError::ConfigError(const char* what_arg)
    : std::runtime_error(what_arg) {}
}
