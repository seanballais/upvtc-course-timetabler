#include <iostream>

#include <upvtc_ct/ds/models.hpp>
#include <upvtc_ct/preprocessor/preprocessor.hpp>
#include <upvtc_ct/utils/data_manager.hpp>

int main(int argc, char* argv[])
{
  upvtc_ct::utils::DataManager dm;

  std::cout << "DIVISIONS" << std::endl;
  for (const auto& division : dm.getDivisions()) {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "\t" << division->name << std::endl;
    std::cout << "\t  Courses:" << std::endl;
    for (const auto course : division->getCourses()) {
      std::cout << "\t\t    " << course->name << std::endl;
    }

    std::cout << "\t  Degrees:" << std::endl;
    for (const auto degree : division->getDegrees()) {
      std::cout << "\t\t    " << degree->name << std::endl;
    }

    std::cout << "\t  Rooms:" << std::endl;
    for (const auto room : division->getRooms()) {
      std::cout << "\t\t    " << room->name << std::endl;
    }
  }
  std::cout << "-------------------------------------------" << std::endl;

  std::cout << "DEGREES" << std::endl;
  for (const auto& degree : dm.getDegrees()) {
    std::cout << "\t" << degree->name << std::endl;
  }

  std::cout << "COURSES" << std::endl;
  for (const auto& course : dm.getCourses()) {
    std::cout << "\t"
              << course->name
              << ((course->isLab) ? " (Lab)" : "")
              << std::endl;
    std::cout << "\t "
              << "Division: " << course->division->name
              << std::endl;
    std::cout << "\t "
              << "Number of Timeslots: " << course->numTimeslots
              << std::endl;
    std::cout << "\t "
              << "Number of Units: " << course->numUnits
              << std::endl;
    std::cout << "\t " << "Room Requirements:" << std::endl;
    for (const auto& roomReq : course->roomRequirements) {
      std::cout << "\t\t" << roomReq->name << std::endl;
    }
    std::cout << "\t " << "Candidate Teachers:" << std::endl;
    for (const auto& teacher : course->candidateTeachers) {
      std::cout << "\t\t" << teacher->name << std::endl;
    }
  }

  std::cout << "ROOMS" << std::endl;
  for (const auto& room  : dm.getRooms()) {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "\tName: " << room->name << std::endl;
    std::cout << "\tCapacity: " << room->capacity << std::endl;
    std::cout << "\tFeatures: " << std::endl;

    for (const auto& feature : room->roomFeatures) {
      std::cout << "\t  " << feature->name << std::endl;
    }
  }
  std::cout << "-------------------------------------------" << std::endl;

  std::cout << "ROOM FEATURES" << std::endl;
  for (const auto& roomFeature : dm.getRoomFeatures()) {
    std::cout << "\t"
              << roomFeature->name
              << std::endl;
  }
  std::cout << "-------------------------------------------" << std::endl;

  std::cout << "TEACHERS" << std::endl;
  for (const auto& teacher : dm.getTeachers()) {
    std::cout << "\t" << teacher->name << std::endl;
    std::cout << "\t" << "Previous Load: "
              << teacher->previousLoad << std::endl;

    std::cout << "\tUnpreferred Timeslots:" << std::endl;
    for (const auto& ut : teacher->unpreferredTimeslots) {
      std::cout << "\t  (" << ut.day << ", " << ut.timeslot << ")" << std::endl;
    }

    std::cout << "\tPotential Courses:" << std::endl;
    for (const auto& course : teacher->getPotentialCourses()) {
      std::cout << "\t  " << course->name << std::endl;
    }
  }
  std::cout << "-------------------------------------------" << std::endl;

  std::cout << "STUDENT GROUPS" << std::endl;
  for (const auto& group : dm.getStudentGroups()) {
    std::cout << "----------------------------" << std::endl;
    std::cout << "\tDegree:\t" << group->degree->name << std::endl;
    std::cout << "\tYear Level:\t" << group->yearLevel << std::endl;
    std::cout << "\tNo. of Members:\t" << group->getNumMembers() << std::endl;
    std::cout << "\tCourses:" << std::endl;
    for (const auto& course : group->assignedCourses) {
        std::cout << "\t\t" << course->name << std::endl;
    }

    std::cout << "\tSUB STUDENT GROUPS" << std::endl;
    for (const auto& group : group->getSubGroups()) {
      std::cout << "----------------------------" << std::endl;
      std::cout << "\t\tParent Degree:\t"
                << group->parentGroup->degree->name << std::endl;
      std::cout << "\t\tParent Year Level:\t"
                << group->parentGroup->yearLevel << std::endl;
      std::cout << "\t\tNum Members:\t"
                << group->getNumMembers() << std::endl;
      std::cout << "\t\tCourses:" << std::endl;
      for (const auto& course : group->assignedCourses) {
        std::cout << "\t\t\t" << course->name << std::endl;
      }
    }
    std::cout << "----------------------------" << std::endl;
  }
  std::cout << "----------------------------" << std::endl;

  const upvtc_ct::utils::Config& config = dm.getConfig();

  std::cout << "CONFIG" << std::endl;

  const unsigned int& semester = config.get<const unsigned int>("semester");
  std::cout << "\tSemester: " << semester << std::endl;

  const unsigned int& numUniqueDays = config.get<const unsigned int>(
                                        "num_unique_days");
  std::cout << "\tNo. of Unique Days: "
            << numUniqueDays << std::endl;

  std::cout << "\tDays with Double Timeslots: " << std::endl;
  auto& daysWithDoubleTimeslots = config.get<const std::vector<unsigned int>>(
                                    "days_with_double_timeslots");
  for (auto day : daysWithDoubleTimeslots) {
    std::cout << "\t  " << day << std::endl;
  }

  const unsigned int& numTimeslots = config.get<const unsigned int>(
                                       "num_timeslots");
  std::cout << "\tNo. of Timeslots: "
            << numTimeslots << std::endl;

  const unsigned int& maxLecCapacity = config.get<const unsigned int>(
                                         "max_lecture_capacity");
  std::cout << "\tMax Lecture Capacity: " << maxLecCapacity << std::endl;

  const unsigned int& maxLabCapacity = config.get<const unsigned int>(
                                         "max_lab_capacity");
  std::cout << "\tMax Lab Capacity: " << maxLabCapacity << std::endl;

  const float& maxAnnualTeacherLoad = config.get<const float>(
                                               "max_annual_teacher_load");
  std::cout << "\tMax Annual Teacher Load: "
            << maxAnnualTeacherLoad
            << std::endl;

  const float& maxSemTeacherLoad = config.get<const float>(
                                            "max_semestral_teacher_load");
  std::cout << "\tMax Semestral Teacher Load: "
            << maxSemTeacherLoad
            << std::endl;

  const unsigned int& numGenerations = config.get<const unsigned int>(
                                         "num_generations");
  std::cout << "\tNo. of Generations: " << numGenerations << std::endl;

  const unsigned int& numOffspringsPerGen = config.get<const unsigned int>(
                                              "num_offsprings_per_generation");
  std::cout << "\tNo. of Offsprings Per Generation: "
            << numOffspringsPerGen
            << std::endl;

  std::cout << "-----------------------------------" << std::endl;

  std::cout << "CLASSES" << std::endl;
  auto pp = upvtc_ct::preprocessor::Preprocessor(&dm);
  pp.preprocess();

  unsigned int numClassesGenerated = 0;
  for (const auto& classGroup : dm.getClassGroups()) {
    numClassesGenerated++;

    // The class instance you select doesn't matter as long as it is in the same
    // class group.
    const upvtc_ct::ds::Class* cls = *(classGroup.second.begin());
    std::cout << "---------------------------------" << std::endl;
    std::cout << "\tClass ID: " << cls->classID << std::endl;

    std::cout << "\tCourse: ";
    if (cls->course != nullptr) {
      if (cls->course->isLab) {
        std::cout << cls->course->name << " (Lab)" << std::endl;
      } else {
        std::cout << cls->course->name << std::endl;
      }      
    } else {
      std::cout << "None" << std::endl;
    }

    std::cout << "\tTeacher: ";
    if (cls->teacher != nullptr) {
      std::cout << cls->teacher->name << std::endl;
    } else {
      std::cout << "None" << std::endl;
    }

    std::cout << "\tDay: " << cls->day << std::endl;

    std::cout << "\tRoom: ";
    if (cls->room != nullptr) {
      std::cout << cls->room->name << std::endl;
    } else {
      std::cout << "None" << std::endl;
    }

    std::cout << "\tTimeslot: " << cls->timeslot << std::endl;
  }

  std::cout << "Number of Classes Generated: " << numClassesGenerated
            << std::endl;
  std::cout << "-----------------------------------" << std::endl;

  std::cout << "CLASS CONFLICTS" << std::endl;
  
  unsigned int numClassesWithConflicts = 0;
  const auto& classGroups = dm.getClassGroups();
  for (const auto item : dm.getClassConflicts()) {
    numClassesWithConflicts++;
    // We don't care which class we get, as long as it's in the correct
    // class group. We only need the class group information, like the class ID
    // and the course name.
    const upvtc_ct::ds::Class* cls = *(classGroups.at(item.first).begin());
    std::cout << cls->course->name
              << " ("
              << ((cls->course->isLab) ? "Lab, " : "") << item.first
              << ")"
              << " is in conflict with:" << std::endl;
    for (const auto classGrpID : item.second) {
      // We don't care which class we get, as long as it's in the correct
      // class group. We only need the class group information, like the class
      // ID and the course name.
      const upvtc_ct::ds::Class* conflictingClass = *(classGroups.at(classGrpID)
                                                                 .begin());
      std::cout << "\t"
                << conflictingClass->course->name
                << " ("
                << ((conflictingClass->course->isLab) ? "Lab, " : "")
                << classGrpID << ")"
                << std::endl;
    }
  }
  std::cout << "Number of Classes with Conflicts: "
            << numClassesWithConflicts << std::endl;
  std::cout << "-----------------------------------" << std::endl;

  return 0;
}
