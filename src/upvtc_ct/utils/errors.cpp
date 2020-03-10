#include <stdexcept>

#include <upvtc_ct/utils/errors.hpp>

namespace upvtc_ct::utils
{
  DisallowedFunctionError::DisallowedFunctionError()
    : std::runtime_error("Called function should not be used.") {}

  FileNotFoundError::FileNotFoundError(const char* what_arg)
    : std::runtime_error(what_arg) {}

  InvalidContentsError::InvalidContentsError(const char* what_arg)
    : std::runtime_error(what_arg) {}

  MaxClassCapacityError::MaxClassCapacityError(const char* what_arg)
    : std::runtime_error(what_arg) {}
}
