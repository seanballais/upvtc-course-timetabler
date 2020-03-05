#ifndef UPVTC_CT_UTILS_HASH_SPECIALIZATIONS_HPP
#define UPVTC_CT_UTILS_HASH_SPECIALIZATIONS_HPP

#include <functional>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

namespace upvtc_ct::utils
{
  class PairHash
  {
  public:
    template <class T0, class T1>
    size_t operator()(const std::pair<T0, T1>& pair) const
    {
      std::stringstream objIdentifier;
      objIdentifier << "(";

      if constexpr (std::is_same_v<T0, std::string>) {
        objIdentifier << pair.first;
      } else {
        objIdentifier << std::to_string(pair.first);
      }

      objIdentifier << ", ";

      if constexpr (std::is_same_v<T1, std::string>) {
        objIdentifier << pair.second;
      } else {
        objIdentifier << std::to_string(pair.second);
      }

      objIdentifier << ")";

      return std::hash<std::string>()(objIdentifier.str());
    }
  };
}

#endif
