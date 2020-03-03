#include <filesystem>

#include <limits.h>
#include <unistd.h>

#include <upvtc_ct/preprocessor/data_manager.hpp>
#include <upvtc_ct/ds/models.hpp>

namespace upvtc_ct::preprocessor
{
  DataManager::DataManager() {}

  const std::unordered_set<ds::Course, ds::CourseHashFunction>&
  DataManager::getCourses()
  {
    return this->courses;
  }

  const std::unordered_set<ds::StudentGroup, ds::StudentGroupHashFunction>&
  DataManager::getStudentGroups()
  {
    return this->studentGroups;
  }

  std::string DataManager::getBinFolderPath() const
  {
    // Note that this function is only guaranteed to work in Linux. Please refer
    // to the linkL
    //   https://stackoverflow.com/a/55579815/1116098
    // if you would like to add support for Windows.
    //
    // This function implementation is copied (with some minor changes) from
    // the function of the same nature from the sands-sim-2d project of Sean
    // Ballais, the original author of this project. You may access the
    // sands-sim-2d project via:
    //   https://github.com/seanballais/sands-sim-2d
    char path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
    std::string binPath = std::string(path, (count > 0) ? count : 0);

    return std::filesystem::path(binPath).parent_path();
  }
}