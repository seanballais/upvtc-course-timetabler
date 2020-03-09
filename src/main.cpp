#include <iostream>

#include <upvtc_ct/ds/models.hpp>
#include <upvtc_ct/preprocessor/preprocessor.hpp>
#include <upvtc_ct/utils/data_manager.hpp>

int main(int argc, char* argv[])
{
  std::cout << "I'm working!" << std::endl;

  upvtc_ct::utils::DataManager dm;

  std::cout << "DIVISIONS" << std::endl;
  for (const auto& division : dm.getDivisions()) {
    std::cout << "\t" << division->name << std::endl;
  }

  std::cout << "DEGREES" << std::endl;
  for (const auto& degree : dm.getDegrees()) {
    std::cout << "\t" << degree->name << std::endl;
  }

  std::cout << "COURSES" << std::endl;
  for (const auto& course : dm.getCourses()) {
    std::cout << "\t" << course->name << std::endl;
  }

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
  }
  std::cout << "----------------------------" << std::endl;

  upvtc_ct::ds::Config config = dm.getConfig();

  std::cout << "CONFIG" << std::endl;
  std::cout << "\tSemester: " << config.get<int>("semester") << std::endl;
  std::cout << "\tMax Lecture Capacity: "
            << config.get<int>("max_lecture_capacity")
            << std::endl;
  std::cout << "\tMax Lab Capacity: "
            << config.get<int>("max_lab_capacity")
            << std::endl;
  std::cout << "\tMax Annual Teacher Load: "
            << config.get<int>("max_annual_teacher_load")
            << std::endl;
  std::cout << "\tMax Semestral Teacher Load: "
            << config.get<int>("max_semestral_teacher_load")
            << std::endl;
  std::cout << "-----------------------------------" << std::endl;

  std::cout << "CLASSES" << std::endl;
  auto pp = upvtc_ct::preprocessor::Preprocessor(&dm);
  for (auto cls : pp.getClasses()) {
    std::cout << "---------------------------------" << std::endl;
    std::cout << "\tID: " << cls.id << std::endl;
    std::cout << "\tClass ID: " << cls.classID << std::endl;

    std::cout << "\tCourse: ";
    if (cls.course != nullptr) {
      std::cout << cls.course->name << std::endl;
    } else {
      std::cout << "None" << std::endl;
    }

    std::cout << "\tTeacher: ";
    if (cls.teacher != nullptr) {
      std::cout << cls.teacher->name << std::endl;
    } else {
      std::cout << "None" << std::endl;
    }

    std::cout << "\tDay: " << cls.day << std::endl;

    std::cout << "\tRoom: ";
    if (cls.room != nullptr) {
      std::cout << cls.room->name << std::endl;
    } else {
      std::cout << "None" << std::endl;
    }

    std::cout << "\tTimeslot: " << cls.timeslot << std::endl;
  }
  std::cout << "-----------------------------------" << std::endl;

  return 0;
}
