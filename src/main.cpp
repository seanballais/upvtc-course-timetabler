#include <iostream>

#include <upvtc_ct/preprocessor/data_manager.hpp>

int main(int argc, char* argv[])
{
  std::cout << "I'm working!" << std::endl;

  upvtc_ct::preprocessor::DataManager dm(0);

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

  return 0;
}
