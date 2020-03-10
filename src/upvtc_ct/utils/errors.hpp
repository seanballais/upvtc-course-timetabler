#ifndef UPVTC_CT_UTILS_ERRORS_HPP_
#define UPVTC_CT_UTILS_ERRORS_HPP_

#include <stdexcept>

namespace upvtc_ct::utils
{
  class DisallowedFunctionError : public std::runtime_error
  {
  public:
    DisallowedFunctionError();
  };

  class FileNotFoundError : public std::runtime_error
  {
  public:
    FileNotFoundError(const char* what_arg);
  };

  class InvalidContentsError : public std::runtime_error
  {
  public:
    InvalidContentsError(const char* what_arg);
  };

  class MaxClassCapacityError : public std::runtime_error
  {
  public:
    MaxClassCapacityError(const char* what_arg);
  };
}

#endif
