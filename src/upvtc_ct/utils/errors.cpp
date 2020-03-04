#include <stdexcept>

#include <upvtc_ct/utils/errors.hpp>

namespace upvtc_ct::utils
{
  FileNotFoundError::FileNotFoundError(const char* what_arg)
    : std::runtime_error(what_arg) {}
}
