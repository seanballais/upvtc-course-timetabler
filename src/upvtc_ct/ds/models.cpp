#include <functional>
#include <memory>
#include <sstream>
#include <string>

#include <upvtc_ct/ds/models.hpp>

namespace upvtc_ct::ds
{
  // Model Definitions
  Course::Course(const std::string name, const bool hasLab, const bool isLab,
                 const unsigned int numTimeslots,
                 const std::unordered_set<Course*> prerequisites,
                 const std::unordered_set<RoomFeature*> roomRequirements)
    : name(name),
      hasLab(hasLab),
      isLab(isLab),
      numTimeslots(numTimeslots),
      prerequisites(prerequisites),
      roomRequirements(roomRequirements) {}

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

  StudentGroup::StudentGroup(const unsigned int id, Degree* const degree,
                             const unsigned int yearLevel,
                             const std::unordered_set<Course*> assignedCourses)
    : BaseStudentGroup(assignedCourses),
      id(id),
      degree(degree),
      yearLevel(yearLevel) {}

  void StudentGroup::addSubGroup(
      const std::unordered_set<Course*> assignedCourses,
      const unsigned int numMembers)
  {
    std::unique_ptr<SubStudentGroup> ssg(
      std::make_unique<SubStudentGroup>(this, assignedCourses));
    ssg->setNumMembers(numMembers);
    this->subGroups.insert(std::move(ssg));
  }

  std::unordered_set<std::unique_ptr<SubStudentGroup>>&
  StudentGroup::getSubGroups()
  {
    return this->subGroups;
  }

  bool StudentGroup::operator==(const StudentGroup& sg) const
  {
    return this->id == sg.id;
  }

  SubStudentGroup::SubStudentGroup(
        StudentGroup* const parentGroup,
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

  Class::Class(const size_t id, const size_t classID, Course* const course,
               Teacher* const teacher, const unsigned int day, Room* const room,
               const unsigned int timeslot)
    : id(id),
      classID(classID),
      course(course),
      teacher(teacher),
      day(day),
      room(room),
      timeslot(timeslot) {}

  bool Class::operator==(const Class& c) const
  {
    return this->id == c.id;
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
    size_t parentGroupHash = StudentGroupHashFunction()(*(ssg.parentGroup));

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
    return c.id;
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
